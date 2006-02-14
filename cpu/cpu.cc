/////////////////////////////////////////////////////////////////////////
// $Id: cpu.cc,v 1.131 2006/02/14 20:03:14 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA



#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "iodev/iodev.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_PROVIDE_CPU_MEMORY==1

#if BX_ADDRESS_SPACES==1
BOCHSAPI BX_MEM_C bx_mem;
#else
BOCHSAPI BX_MEM_C bx_mem_array[BX_ADDRESS_SPACES];
#endif

#endif

#if BX_SUPPORT_ICACHE

bxPageWriteStampTable pageWriteStampTable;

void purgeICaches(void)
{
#if BX_SUPPORT_SMP
  for (unsigned i=0; i<BX_SMP_PROCESSORS; i++)
    BX_CPU(i)->iCache.purgeICacheEntries();
#else
  BX_CPU(0)->iCache.purgeICacheEntries();
#endif

  pageWriteStampTable.resetWriteStamps();
}

void flushICaches(void)
{
#if BX_SUPPORT_SMP
  for (unsigned i=0; i<BX_SMP_PROCESSORS; i++)
    BX_CPU(i)->iCache.flushICacheEntries();
#else
  BX_CPU(0)->iCache.flushICacheEntries();
#endif

  pageWriteStampTable.resetWriteStamps();
}

#define InstrumentICACHE 0

#if InstrumentICACHE
static unsigned iCacheLookups=0;
static unsigned iCacheMisses=0;

#define InstrICache_StatsMask 0xffffff

#define InstrICache_Stats() {\
  if ((iCacheLookups & InstrICache_StatsMask) == 0) { \
    BX_INFO(("ICACHE lookups: %u, misses: %u, hit rate = %6.2f%% ", \
          iCacheLookups, \
          iCacheMisses,  \
          (iCacheLookups-iCacheMisses) * 100.0 / iCacheLookups)); \
    iCacheLookups = iCacheMisses = 0; \
  } \
}
#define InstrICache_Increment(v) (v)++

#else
#define InstrICache_Stats()
#define InstrICache_Increment(v)
#endif

#endif

// notes:

// The CHECK_MAX_INSTRUCTIONS macro allows cpu_loop to execute a few
// instructions and then return so that the other processors have a chance to
// run.  This is used only when simulating multiple processors.
// 
// If maximum instructions have been executed, return.  A count less
// than zero means run forever.
#define CHECK_MAX_INSTRUCTIONS(count) \
  if (count >= 0) {                   \
    count--; if (count == 0) return;  \
  }

#if BX_SUPPORT_SMP
#  define BX_TICK1_IF_SINGLE_PROCESSOR()
#else
#  define BX_TICK1_IF_SINGLE_PROCESSOR() BX_TICK1()
#endif

// Make code more tidy with a few macros.
#if BX_SUPPORT_X86_64==0
#define RIP EIP
#define RSP ESP
#endif

void BX_CPU_C::cpu_loop(Bit32s max_instr_count)
{
  unsigned ret;
  bxInstruction_c iStorage BX_CPP_AlignN(32);
  bxInstruction_c *i = &iStorage;

#if BX_DEBUGGER
  BX_CPU_THIS_PTR break_point = 0;
#if BX_MAGIC_BREAKPOINT
  BX_CPU_THIS_PTR magic_break = 0;
#endif
  BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
#endif

  if (setjmp( BX_CPU_THIS_PTR jmp_buf_env )) 
  { 
    // only from exception function can we get here ...
    BX_INSTR_NEW_INSTRUCTION(BX_CPU_ID);

#if BX_GDBSTUB
    if (bx_dbg.gdbstub_enabled) {
      return;
    }
#endif
  }

#if BX_DEBUGGER
  // If the exception() routine has encountered a nasty fault scenario,
  // the debugger may request that control is returned to it so that
  // the situation may be examined.
  if (bx_guard.special_unwind_stack) {
    printf("CPU_LOOP %d\n", bx_guard.special_unwind_stack);
    return;
  }
#endif

  // We get here either by a normal function call, or by a longjmp
  // back from an exception() call.  In either case, commit the
  // new EIP/ESP, and set up other environmental fields.  This code
  // mirrors similar code below, after the interrupt() call.
  BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
  BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP
  BX_CPU_THIS_PTR EXT = 0;
  BX_CPU_THIS_PTR errorno = 0;

  while (1) {

  // First check on events which occurred for previous instructions
  // (traps) and ones which are asynchronous to the CPU
  // (hardware interrupts).
  if (BX_CPU_THIS_PTR async_event) {
    if (handleAsyncEvent()) {
      // If request to return to caller ASAP.
      return;
    }
  }

#if BX_DEBUGGER
  {
  bx_address debug_eip = BX_CPU_THIS_PTR prev_eip;
  if (dbg_is_begin_instr_bpoint(
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value,
         debug_eip,
         BX_CPU_THIS_PTR get_segment_base(BX_SEG_REG_CS) + debug_eip,
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b,
         Is64BitMode()))
    {
      return;
    }
  }
#endif  // #if BX_DEBUGGER

#if BX_EXTERNAL_DEBUGGER
  if (regs.debug_state != debug_run) {
    bx_external_debugger(BX_CPU_THIS);
  }
#endif

  bx_address eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;

  if (eipBiased >= BX_CPU_THIS_PTR eipPageWindowSize) {
    prefetch();
    eipBiased = RIP + BX_CPU_THIS_PTR eipPageBias;
  }

#if BX_SUPPORT_ICACHE
  Bit32u pAddr = BX_CPU_THIS_PTR pAddrA20Page + eipBiased;
  unsigned iCacheHash = BX_CPU_THIS_PTR iCache.hash(pAddr);
  bxICacheEntry_c *cache_entry = &(BX_CPU_THIS_PTR iCache.entry[iCacheHash]);
  i = &(cache_entry->i);

  Bit32u pageWriteStamp = *(BX_CPU_THIS_PTR currPageWriteStampPtr);

#if BX_SUPPORT_ICACHE
  InstrICache_Increment(iCacheLookups);
  InstrICache_Stats();
#endif

  if ((cache_entry->pAddr == pAddr) &&
      (cache_entry->writeStamp == pageWriteStamp))
  {
    // iCache hit. Instruction is already decoded and stored in the
    // instruction cache.

#if BX_INSTRUMENTATION
    // An instruction was found in the iCache.
    BX_INSTR_OPCODE(BX_CPU_ID, BX_CPU_THIS_PTR eipFetchPtr + eipBiased,
       i->ilen(), BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b, Is64BitMode());
#endif
  }
  else
#endif
  {
    // iCache miss. No validated instruction with matching fetch parameters
    // is in the iCache. Or we're not compiling iCache support in, in which
    // case we always have an iCache miss.  :^)
    bx_address remainingInPage;
    remainingInPage = (BX_CPU_THIS_PTR eipPageWindowSize - eipBiased);
    unsigned maxFetch = 15;
    if (remainingInPage < 15) maxFetch = remainingInPage;
    Bit8u *fetchPtr = BX_CPU_THIS_PTR eipFetchPtr + eipBiased;

#if BX_SUPPORT_ICACHE
    // The entry will be marked valid if fetchdecode will succeed
    cache_entry->writeStamp = ICacheWriteStampInvalid;
    InstrICache_Increment(iCacheMisses);
#endif

#if BX_SUPPORT_X86_64
    if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64)
      ret = fetchDecode64(fetchPtr, i, maxFetch);
    else
#endif
      ret = fetchDecode(fetchPtr, i, maxFetch);

    if (ret==0) {
#if BX_SUPPORT_ICACHE
      i = &iStorage;	// Leave entry invalid
#endif
      boundaryFetch(fetchPtr, remainingInPage, i);
    }
    else
    {
#if BX_SUPPORT_ICACHE
      // In the case where the page is marked ICacheWriteStampInvalid, all
      // counter bits will be high, being eqivalent to ICacheWriteStampMax.
      // In the case where the page is marked as possibly having associated
      // iCache entries, we need to leave the counter as-is, unless we're
      // willing to dump all iCache entries which can hash to this page.
      // Therefore, in either case, we can keep the counter as-is and
      // replace the fetch mode bits.
      Bit32u fetchModeMask = BX_CPU_THIS_PTR iCache.fetchModeMask;
      pageWriteStamp &= ICacheWriteStampMask;  // Clear out old fetch mode bits.
      pageWriteStamp |= fetchModeMask;         // Add in new ones.
      pageWriteStampTable.setPageWriteStamp(pAddr, pageWriteStamp);
      cache_entry->pAddr = pAddr;
      cache_entry->writeStamp = pageWriteStamp;
#endif
#if BX_INSTRUMENTATION
      // An instruction was either fetched, or found in the iCache.
      BX_INSTR_OPCODE(BX_CPU_ID, fetchPtr, i->ilen(),
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b, Is64BitMode());
#endif
    }
  }

  bx_address next_RIP = RIP + i->ilen();
  if (! Is64BitMode()) {
    if (((Bit32u) next_RIP - 1) > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
      BX_ERROR(("GP(0): instruction cross segment boundary !"));
      exception(BX_GP_EXCEPTION, 0, 0);
    }
  }

  BxExecutePtr_tR resolveModRM = i->ResolveModrm; // Get as soon as possible for speculation
  BxExecutePtr_t execute = i->execute; // fetch as soon as possible for speculation
  if (resolveModRM)
    BX_CPU_CALL_METHODR(resolveModRM, (i));

  // An instruction will have been fetched using either the normal case,
  // or the boundary fetch (across pages), by this point.
  BX_INSTR_FETCH_DECODE_COMPLETED(BX_CPU_ID, i);

#if BX_DEBUGGER
  if (BX_CPU_THIS_PTR trace) {
    // print the instruction that is about to be executed.
    bx_dbg_disassemble_current (BX_CPU_ID, 1);  // only one cpu, print time stamp
  }
#endif

  // decoding instruction compeleted -> continue with execution
  BX_INSTR_BEFORE_EXECUTION(BX_CPU_ID, i);

  if ( !(i->repUsedL() && i->repeatableL()) ) {
    // non repeating instruction
    RIP = next_RIP;
    BX_CPU_CALL_METHOD(execute, (i));
    BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
    BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP

    BX_INSTR_AFTER_EXECUTION(BX_CPU_ID, i);
    BX_TICK1_IF_SINGLE_PROCESSOR();
  }
  else {

repeat_loop:

    if (i->repeatableZFL()) {
#if BX_SUPPORT_X86_64
      if (i->as64L()) {
        if (RCX != 0) {
          BX_CPU_CALL_METHOD(execute, (i));
          RCX --;
        }
        if ((i->repUsedValue()==3) && (get_ZF()==0)) goto repeat_done;
        if ((i->repUsedValue()==2) && (get_ZF()!=0)) goto repeat_done;
        if (RCX == 0) goto repeat_done;
        goto repeat_not_done;
      }
      else
#endif
      if (i->as32L()) {
        if (ECX != 0) {
          BX_CPU_CALL_METHOD(execute, (i));
          ECX --;
        }
        if ((i->repUsedValue()==3) && (get_ZF()==0)) goto repeat_done;
        if ((i->repUsedValue()==2) && (get_ZF()!=0)) goto repeat_done;
        if (ECX == 0) goto repeat_done;
        goto repeat_not_done;
      }
      else {
        if (CX != 0) {
          BX_CPU_CALL_METHOD(execute, (i));
          CX --;
        }
        if ((i->repUsedValue()==3) && (get_ZF()==0)) goto repeat_done;
        if ((i->repUsedValue()==2) && (get_ZF()!=0)) goto repeat_done;
        if (CX == 0) goto repeat_done;
        goto repeat_not_done;
      }
    }
    else { // normal repeat, no concern for ZF
#if BX_SUPPORT_X86_64
      if (i->as64L()) {
        if (RCX != 0) {
          BX_CPU_CALL_METHOD(execute, (i));
          RCX --;
        }
        if (RCX == 0) goto repeat_done;
        goto repeat_not_done;
      }
      else
#endif
      if (i->as32L()) {
        if (ECX != 0) {
          BX_CPU_CALL_METHOD(execute, (i));
          ECX --;
        }
        if (ECX == 0) goto repeat_done;
        goto repeat_not_done;
      }
      else { // 16bit addrsize
        if (CX != 0) {
          BX_CPU_CALL_METHOD(execute, (i));
          CX --;
        }
        if (CX == 0) goto repeat_done;
        goto repeat_not_done;
      }
    }

    // shouldn't get here from above
repeat_not_done:
    BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
    BX_TICK1_IF_SINGLE_PROCESSOR();

#if BX_DEBUGGER == 0
    if (BX_CPU_THIS_PTR async_event) {
      invalidate_prefetch_q();
      goto debugger_check;
    }
    goto repeat_loop;
#else  /* if BX_DEBUGGER == 1 */
    invalidate_prefetch_q();
    goto debugger_check;
#endif

repeat_done:
    RIP = next_RIP;
    BX_CPU_THIS_PTR prev_eip = RIP; // commit new EIP
    BX_CPU_THIS_PTR prev_esp = RSP; // commit new ESP
    BX_INSTR_REPEAT_ITERATION(BX_CPU_ID, i);
    BX_INSTR_AFTER_EXECUTION(BX_CPU_ID, i);
    BX_TICK1_IF_SINGLE_PROCESSOR();
  }

debugger_check:
  // inform instrumentation about new instruction
  BX_INSTR_NEW_INSTRUCTION(BX_CPU_ID);

#if (BX_SUPPORT_SMP && BX_DEBUGGER==0)
  // The CHECK_MAX_INSTRUCTIONS macro allows cpu_loop to execute a few
  // instructions and then return so that the other processors have a chance
  // to run.  This is used only when simulating multiple processors.  If only
  // one processor, don't waste any cycles on it!  Also, it is not needed
  // with the debugger because its guard mechanism provides the same
  // functionality.
  CHECK_MAX_INSTRUCTIONS(max_instr_count);
#endif

#if BX_DEBUGGER
    // Mode switch support is in dbg_is_begin_instr_bpoint
    // note instr generating exceptions never reach this point.

    // (mch) Read/write, time break point support
    if (BX_CPU_THIS_PTR break_point) {
      switch (BX_CPU_THIS_PTR break_point) {
        case BREAK_POINT_TIME:
          BX_INFO(("[" FMT_LL "d] Caught time breakpoint", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_TIME_BREAK_POINT;
          return;
        case BREAK_POINT_READ:
          BX_INFO(("[" FMT_LL "d] Caught read watch point", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_READ_WATCH_POINT;
          return;
        case BREAK_POINT_WRITE:
          BX_INFO(("[" FMT_LL "d] Caught write watch point", bx_pc_system.time_ticks()));
          BX_CPU_THIS_PTR stop_reason = STOP_WRITE_WATCH_POINT;
          return;
        default:
          BX_PANIC(("Weird break point condition"));
      }
    }
#if BX_MAGIC_BREAKPOINT
    // (mch) Magic break point support
    if (BX_CPU_THIS_PTR magic_break) {
      if (bx_dbg.magic_break_enabled) {
        BX_DEBUG(("Stopped on MAGIC BREAKPOINT"));
        BX_CPU_THIS_PTR stop_reason = STOP_MAGIC_BREAK_POINT;
        return;
      }
      else {
        BX_CPU_THIS_PTR magic_break = 0;
        BX_CPU_THIS_PTR stop_reason = STOP_NO_REASON;
        BX_DEBUG(("Ignoring MAGIC BREAKPOINT"));
      }
    }
#endif

    {
      // check for icount or control-C.  If found, set guard reg and return.
      bx_address debug_eip = BX_CPU_THIS_PTR prev_eip;
      if (dbg_is_end_instr_bpoint(
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value,
           debug_eip,
           BX_CPU_THIS_PTR get_segment_base(BX_SEG_REG_CS) + debug_eip,
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b,
           Is64BitMode()))
      {
        return;
      }
    }
#endif  // #if BX_DEBUGGER

#if BX_GDBSTUB
    if (bx_dbg.gdbstub_enabled) {
      unsigned int reason = bx_gdbstub_check(EIP);
      if (reason != GDBSTUB_STOP_NO_REASON) {
        return;
      }
    }
#endif

  }  // while (1)
}

unsigned BX_CPU_C::handleAsyncEvent(void)
{
  //
  // This area is where we process special conditions and events.
  //

  if (BX_CPU_THIS_PTR debug_trap & 0x80000000) {
    // I made up the bitmask above to mean HALT state.
#if BX_SUPPORT_SMP == 0
    BX_CPU_THIS_PTR debug_trap = 0; // clear traps for after resume
    BX_CPU_THIS_PTR inhibit_mask = 0; // clear inhibits for after resume
    // for one processor, pass the time as quickly as possible until
    // an interrupt wakes up the CPU.
#if BX_DEBUGGER
    while (bx_guard.interrupt_requested != 1)
#else
    while (1)
#endif
    {
      if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF ()) {
        break;
      }
      if (BX_CPU_THIS_PTR async_event == 2) {
        BX_INFO(("decode: reset detected in halt state"));
        break;
      }
      BX_TICK1();
    }
#else   /* BX_SUPPORT_SMP */
    // for multiprocessor simulation, even if this CPU is halted we still
    // must give the others a chance to simulate.  If an interrupt has 
    // arrived, then clear the HALT condition; otherwise just return from
    // the CPU loop with stop_reason STOP_CPU_HALTED.
    if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF ()) {
      // interrupt ends the HALT condition
      BX_CPU_THIS_PTR debug_trap = 0; // clear traps for after resume
      BX_CPU_THIS_PTR inhibit_mask = 0; // clear inhibits for after resume
      //bx_printf ("halt condition has been cleared in %s", name);
    } else {
      // HALT condition remains, return so other CPUs have a chance
#if BX_DEBUGGER
      BX_CPU_THIS_PTR stop_reason = STOP_CPU_HALTED;
#endif
      return 1; // Return to caller of cpu_loop.
    }
#endif
  } else if (BX_CPU_THIS_PTR kill_bochs_request) {
    // setting kill_bochs_request causes the cpu loop to return ASAP.
    return 1; // Return to caller of cpu_loop.
  }


  // Priority 1: Hardware Reset and Machine Checks
  //   RESET
  //   Machine Check
  // (bochs doesn't support these)

  // Priority 2: Trap on Task Switch
  //   T flag in TSS is set
  if (BX_CPU_THIS_PTR debug_trap & 0x00008000) {
    BX_CPU_THIS_PTR dr6 |= BX_CPU_THIS_PTR debug_trap;
    exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
  }

  // Priority 3: External Hardware Interventions
  //   FLUSH
  //   STOPCLK
  //   SMI
  //   INIT
  // (bochs doesn't support these)
  if (BX_CPU_THIS_PTR smi_pending && ! BX_CPU_THIS_PTR smm_mode())
  {
     BX_PANIC(("SMI: system management mode still not implemented !"));
     // clear SMI pending flag if accepting SMM !
  }

  // Priority 4: Traps on Previous Instruction
  //   Breakpoints
  //   Debug Trap Exceptions (TF flag set or data/IO breakpoint)
  if (BX_CPU_THIS_PTR debug_trap &&
       !(BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG))
  {
    // A trap may be inhibited on this boundary due to an instruction
    // which loaded SS.  If so we clear the inhibit_mask below
    // and don't execute this code until the next boundary.
    // Commit debug events to DR6
    BX_CPU_THIS_PTR dr6 |= BX_CPU_THIS_PTR debug_trap;
    exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
  }

  // Priority 5: External Interrupts
  //   NMI Interrupts
  //   Maskable Hardware Interrupts
  if (BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_INTERRUPTS) {
    // Processing external interrupts is inhibited on this
    // boundary because of certain instructions like STI.
    // inhibit_mask is cleared below, in which case we will have
    // an opportunity to check interrupts on the next instruction
    // boundary.
  }
  else if (BX_CPU_INTR && BX_CPU_THIS_PTR get_IF() && BX_DBG_ASYNC_INTR)
  {
    Bit8u vector;

    // NOTE: similar code in ::take_irq()
#if BX_SUPPORT_APIC
    if (BX_CPU_THIS_PTR local_apic.INTR)
      vector = BX_CPU_THIS_PTR local_apic.acknowledge_int ();
    else
      vector = DEV_pic_iac(); // may set INTR with next interrupt
#else
    // if no local APIC, always acknowledge the PIC.
    vector = DEV_pic_iac(); // may set INTR with next interrupt
#endif
    //BX_DEBUG(("decode: interrupt %u", (unsigned) vector));
    BX_CPU_THIS_PTR errorno = 0;
    BX_CPU_THIS_PTR EXT = 1; /* external event */
    interrupt(vector, 0, 0, 0);
    BX_INSTR_HWINTERRUPT(BX_CPU_ID, vector,
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
    // Set up environment, as would be when this main cpu loop gets
    // invoked.  At the end of normal instructions, we always commmit
    // the new EIP/ESP values.  But here, we call interrupt() much like
    // it was a sofware interrupt instruction, and need to effect the
    // commit here.  This code mirrors similar code above.
    BX_CPU_THIS_PTR prev_eip = RIP; // commit new RIP
    BX_CPU_THIS_PTR prev_esp = RSP; // commit new RSP
    BX_CPU_THIS_PTR EXT = 0;
    BX_CPU_THIS_PTR errorno = 0;
  }
  else if (BX_HRQ && BX_DBG_ASYNC_DMA) {
    // NOTE: similar code in ::take_dma()
    // assert Hold Acknowledge (HLDA) and go into a bus hold state
    DEV_dma_raise_hlda();
  }

  // Priority 6: Faults from fetching next instruction
  //   Code breakpoint fault
  //   Code segment limit violation (priority 7 on 486/Pentium)
  //   Code page fault (priority 7 on 486/Pentium)
  // (handled in main decode loop)

  // Priority 7: Faults from decoding next instruction
  //   Instruction length > 15 bytes
  //   Illegal opcode
  //   Coprocessor not available
  // (handled in main decode loop etc)

  // Priority 8: Faults on executing an instruction
  //   Floating point execution
  //   Overflow
  //   Bound error
  //   Invalid TSS
  //   Segment not present
  //   Stack fault
  //   General protection
  //   Data page fault
  //   Alignment check
  // (handled by rest of the code)

  if (BX_CPU_THIS_PTR get_TF ())
  {
    // TF is set before execution of next instruction.  Schedule
    // a debug trap (#DB) after execution.  After completion of
    // next instruction, the code above will invoke the trap.
    BX_CPU_THIS_PTR debug_trap |= 0x00004000; // BS flag in DR6
  }

  // Now we can handle things which are synchronous to instruction
  // execution.
  if (BX_CPU_THIS_PTR get_RF ()) {
    BX_CPU_THIS_PTR clear_RF ();
  }
#if BX_X86_DEBUGGER
  else {
    // only bother comparing if any breakpoints enabled
    if ( BX_CPU_THIS_PTR dr7 & 0x000000ff ) {
      Bit32u iaddr =
        BX_CPU_THIS_PTR get_segment_base(BX_SEG_REG_CS) +
        BX_CPU_THIS_PTR prev_eip;
      Bit32u dr6_bits;
      if ( (dr6_bits = hwdebug_compare(iaddr, 1, BX_HWDebugInstruction,
                                       BX_HWDebugInstruction)) )
      {
        // Add to the list of debug events thus far.
        BX_CPU_THIS_PTR async_event = 1;
        BX_CPU_THIS_PTR debug_trap |= dr6_bits;
        // If debug events are not inhibited on this boundary,
        // fire off a debug fault.  Otherwise handle it on the next
        // boundary. (becomes a trap)
        if ( !(BX_CPU_THIS_PTR inhibit_mask & BX_INHIBIT_DEBUG) ) {
          // Commit debug events to DR6
          BX_CPU_THIS_PTR dr6 = BX_CPU_THIS_PTR debug_trap;
          exception(BX_DB_EXCEPTION, 0, 0); // no error, not interrupt
        }
      }
    }
  }
#endif

  // We have ignored processing of external interrupts and
  // debug events on this boundary.  Reset the mask so they
  // will be processed on the next boundary.
  BX_CPU_THIS_PTR inhibit_mask = 0;

  if ( !(BX_CPU_INTR ||
         BX_CPU_THIS_PTR debug_trap ||
         BX_HRQ ||
         BX_CPU_THIS_PTR get_TF () 
#if BX_X86_DEBUGGER
         || (BX_CPU_THIS_PTR dr7 & 0xff)
#endif
        ))
    BX_CPU_THIS_PTR async_event = 0;

  return 0; // Continue executing cpu_loop.
}


// boundaries of consideration:
//
//  * physical memory boundary: 1024k (1Megabyte) (increments of...)
//  * A20 boundary:             1024k (1Megabyte)
//  * page boundary:            4k
//  * ROM boundary:             2k (dont care since we are only reading)
//  * segment boundary:         any


void BX_CPU_C::prefetch(void)
{
  // prefetch QSIZE byte quantity aligned on corresponding boundary
  Bit32u pAddr;
  bx_address laddrPageOffset0, eipPageOffset0;

  bx_address temp_rip = RIP;
  bx_address laddr = BX_CPU_THIS_PTR get_segment_base(BX_SEG_REG_CS) + temp_rip;

  if (! Is64BitMode()) {
    Bit32u temp_limit = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled;

    if (((Bit32u) temp_rip) > temp_limit) {
      BX_PANIC(("prefetch: RIP > CS.limit"));
      exception(BX_GP_EXCEPTION, 0, 0);
    }
  }

#if BX_SUPPORT_PAGING
  if (BX_CPU_THIS_PTR cr0.pg) {
    // aligned block guaranteed to be all in one page, same A20 address
    pAddr = itranslate_linear(laddr, CPL==3);
    pAddr = A20ADDR(pAddr);
  }
  else
#endif // BX_SUPPORT_PAGING
  {
    pAddr = A20ADDR(laddr);
  }

  // Linear address at the beginning of the page.
#if BX_SUPPORT_X86_64
  laddrPageOffset0 = laddr & BX_CONST64(0xfffffffffffff000);
#else
  laddrPageOffset0 = laddr & 0xfffff000;
#endif
  // Calculate RIP at the beginning of the page.
  eipPageOffset0 = RIP - (laddr - laddrPageOffset0);
  BX_CPU_THIS_PTR eipPageBias = - eipPageOffset0;
  BX_CPU_THIS_PTR eipPageWindowSize = 4096; // FIXME:
  BX_CPU_THIS_PTR pAddrA20Page = pAddr & 0xfffff000;
  BX_CPU_THIS_PTR eipFetchPtr =
       BX_CPU_THIS_PTR mem->getHostMemAddr(BX_CPU_THIS, BX_CPU_THIS_PTR pAddrA20Page, BX_READ);

  // Sanity checks
  if ( !BX_CPU_THIS_PTR eipFetchPtr ) {
    if ( pAddr >= BX_CPU_THIS_PTR mem->len ) {
      BX_PANIC(("prefetch : running in bogus memory, pAddr=0x%08x", pAddr));
    }
    else {
      BX_PANIC(("prefetch: getHostMemAddr vetoed direct read, pAddr=0x%08x", pAddr));
    }
  }

#if BX_SUPPORT_ICACHE
  BX_CPU_THIS_PTR currPageWriteStampPtr = pageWriteStampTable.getPageWriteStampPtr(pAddr);
  Bit32u pageWriteStamp = *(BX_CPU_THIS_PTR currPageWriteStampPtr);
  Bit32u fetchModeMask  = BX_CPU_THIS_PTR iCache.fetchModeMask;
  if ((pageWriteStamp & ICacheFetchModeMask) != fetchModeMask)
  {
    // The current CPU mode does not match iCache entries for this
    // physical page.
    pageWriteStamp &= ICacheWriteStampMask; // Clear out old fetch mode bits.
    pageWriteStamp |= fetchModeMask;        // Add in new ones.
    pageWriteStampTable.setPageWriteStamp(pAddr, pageWriteStamp);
  }
#endif
}

void BX_CPU_C::boundaryFetch(Bit8u *fetchPtr, unsigned remainingInPage, bxInstruction_c *i)
{
  unsigned j;
  Bit8u fetchBuffer[16]; // Really only need 15
  unsigned ret;

  if (remainingInPage >= 15) {
    BX_INFO(("fetchDecode #GP(0): too many instruction prefixes"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  // Read all leftover bytes in current page up to boundary.
  for (j=0; j<remainingInPage; j++) {
    fetchBuffer[j] = *fetchPtr++;
  }

  // The 2nd chunk of the instruction is on the next page.
  // Set RIP to the 0th byte of the 2nd page, and force a
  // prefetch so direct access of that physical page is possible, and
  // all the associated info is updated.
  RIP += remainingInPage;
  prefetch();
  if (BX_CPU_THIS_PTR eipPageWindowSize < 15) {
    BX_PANIC(("fetch_decode: small window size after prefetch"));
  }

  // We can fetch straight from the 0th byte, which is eipFetchPtr;
  fetchPtr = BX_CPU_THIS_PTR eipFetchPtr;

  // read leftover bytes in next page
  for (; j<15; j++) {
    fetchBuffer[j] = *fetchPtr++;
  }
#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    ret = fetchDecode64(fetchBuffer, i, 15);
  }
  else
#endif
  {
    ret = fetchDecode(fetchBuffer, i, 15);
  }

  if (ret==0) {
    BX_INFO(("fetchDecode #GP(0): cross boundary"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  // Restore EIP since we fudged it to start at the 2nd page boundary.
  RIP = BX_CPU_THIS_PTR prev_eip;

// Since we cross an instruction boundary, note that we need a prefetch()
// again on the next instruction.  Perhaps we can optimize this to
// eliminate the extra prefetch() since we do it above, but have to
// think about repeated instructions, etc.
BX_CPU_THIS_PTR eipPageWindowSize = 0; // Fixme

  BX_INSTR_OPCODE(BX_CPU_ID, fetchBuffer, i->ilen(),
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b, Is64BitMode());
}


#if BX_EXTERNAL_DEBUGGER

void BX_CPU_C::ask (int level, const char *prefix, const char *fmt, va_list ap)
{
  char buf1[1024];
  vsprintf (buf1, fmt, ap);
  printf ("%s %s\n", prefix, buf1);
  trap_debugger(1);
}

void BX_CPU_C::trap_debugger (bx_bool callnow)
{
  regs.debug_state = debug_step;
  if (callnow) {
    bx_external_debugger(BX_CPU_THIS);
  }
}

#endif  // #if BX_EXTERNAL_DEBUGGER


#if BX_DEBUGGER
extern unsigned dbg_show_mask;

bx_bool BX_CPU_C::dbg_is_begin_instr_bpoint(Bit16u cs, bx_address eip, bx_address laddr, bx_bool is_32, bx_bool is_64)
{ 
  Bit64u tt = bx_pc_system.time_ticks();

  BX_CPU_THIS_PTR guard_found.cs  = cs;
  BX_CPU_THIS_PTR guard_found.eip = eip;
  BX_CPU_THIS_PTR guard_found.laddr = laddr;
  BX_CPU_THIS_PTR guard_found.is_32bit_code = is_32;
  BX_CPU_THIS_PTR guard_found.is_64bit_code = is_64;

  // mode switch breakpoint
  // instruction which generate exceptions never reach the end of the
  // loop due to a long jump. Thats why we check at start of instr.
  // Downside is that we show the instruction about to be executed
  // (not the one generating the mode switch).
  if (BX_CPU_THIS_PTR mode_break && 
     (BX_CPU_THIS_PTR dbg_cpu_mode != BX_CPU_THIS_PTR get_cpu_mode()))
  {
    BX_INFO(("[" FMT_LL "d] Caught mode switch breakpoint, switching from '%s' to '%s'",
        bx_pc_system.time_ticks(), cpu_mode_string(BX_CPU_THIS_PTR dbg_cpu_mode),
        cpu_mode_string(BX_CPU_THIS_PTR get_cpu_mode())));
    BX_CPU_THIS_PTR dbg_cpu_mode = BX_CPU_THIS_PTR get_cpu_mode();
    BX_CPU_THIS_PTR stop_reason = STOP_MODE_BREAK_POINT;
    return(1);
  }

  // support for 'show' command in debugger
  if(dbg_show_mask) {
    int rv = bx_dbg_show_symbolic();
    if (rv) return(rv);
  }

  // see if debugger is looking for iaddr breakpoint of any type
  if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_ALL) {
#if BX_DBG_SUPPORT_VIR_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_VIR) {
      if ((BX_CPU_THIS_PTR guard_found.icount!=0) ||
          (tt != BX_CPU_THIS_PTR guard_found.time_tick))
      {
        for (unsigned i=0; i<bx_guard.iaddr.num_virtual; i++) {
          if ( bx_guard.iaddr.vir[i].enabled &&
               (bx_guard.iaddr.vir[i].cs  == cs) &&
               (bx_guard.iaddr.vir[i].eip == eip) )
          {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_VIR;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
	    BX_CPU_THIS_PTR guard_found.time_tick = tt;
            return(1); // on a breakpoint
          }
        }
      }
    }
#endif
#if BX_DBG_SUPPORT_LIN_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_LIN) {
      if ((BX_CPU_THIS_PTR guard_found.icount!=0) ||
          (tt != BX_CPU_THIS_PTR guard_found.time_tick))
      {
        for (unsigned i=0; i<bx_guard.iaddr.num_linear; i++) {
          if (bx_guard.iaddr.lin[i].enabled && 
              (bx_guard.iaddr.lin[i].addr == BX_CPU_THIS_PTR guard_found.laddr) )
          {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_LIN;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
	    BX_CPU_THIS_PTR guard_found.time_tick = tt;
            return(1); // on a breakpoint
          }
        }
      }
    }
#endif
#if BX_DBG_SUPPORT_PHY_BPOINT
    if (bx_guard.guard_for & BX_DBG_GUARD_IADDR_PHY) {
      Bit32u phy;
      bx_bool valid;
      dbg_xlate_linear2phy(BX_CPU_THIS_PTR guard_found.laddr, &phy, &valid);
      // The "guard_found.icount!=0" condition allows you to step or
      // continue beyond a breakpoint.  Bryce tried removing it once,
      // and once you get to a breakpoint you are stuck there forever.
      // Not pretty.
      if (valid && ((BX_CPU_THIS_PTR guard_found.icount!=0) ||
          (tt != BX_CPU_THIS_PTR guard_found.time_tick)))
      {
        for (unsigned i=0; i<bx_guard.iaddr.num_physical; i++) {
          if (bx_guard.iaddr.phy[i].enabled && (bx_guard.iaddr.phy[i].addr == phy))
          {
            BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_IADDR_PHY;
            BX_CPU_THIS_PTR guard_found.iaddr_index = i;
	    BX_CPU_THIS_PTR guard_found.time_tick = tt;
            return(1); // on a breakpoint
          }
        }
      }
    }
#endif
  }

  return(0); // not on a breakpoint
}

bx_bool BX_CPU_C::dbg_is_end_instr_bpoint(Bit16u cs, bx_address eip, bx_address laddr, bx_bool is_32, bx_bool is_64)
{
  BX_CPU_THIS_PTR guard_found.icount++;

  // convenient point to see if user typed Ctrl-C
  if (bx_guard.interrupt_requested &&
     (bx_guard.guard_for & BX_DBG_GUARD_CTRL_C))
  {
    BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_CTRL_C;
    return(1);
  }

  // see if debugger requesting icount guard
  if (bx_guard.guard_for & BX_DBG_GUARD_ICOUNT) {
    if (BX_CPU_THIS_PTR guard_found.icount >= bx_guard.icount) {
      BX_CPU_THIS_PTR guard_found.cs  = cs;
      BX_CPU_THIS_PTR guard_found.eip = eip;
      BX_CPU_THIS_PTR guard_found.laddr = laddr;
      BX_CPU_THIS_PTR guard_found.is_32bit_code = is_32;
      BX_CPU_THIS_PTR guard_found.is_64bit_code = is_64;
      BX_CPU_THIS_PTR guard_found.guard_found = BX_DBG_GUARD_ICOUNT;
      return(1);
    }
  }

  return(0); // no breakpoint
}

void BX_CPU_C::dbg_take_irq(void)
{
  unsigned vector;

  // NOTE: similar code in ::cpu_loop()

  if ( BX_CPU_INTR && BX_CPU_THIS_PTR get_IF () ) {
    if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
      // normal return from setjmp setup
      vector = DEV_pic_iac(); // may set INTR with next interrupt
      BX_CPU_THIS_PTR errorno = 0;
      BX_CPU_THIS_PTR EXT   = 1; // external event
      BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
      interrupt(vector, 0, 0, 0);
    }
  }
}

void BX_CPU_C::dbg_force_interrupt(unsigned vector)
{
  // Used to force slave simulator to take an interrupt, without
  // regard to IF

  if ( setjmp(BX_CPU_THIS_PTR jmp_buf_env) == 0 ) {
    // normal return from setjmp setup
    BX_CPU_THIS_PTR errorno = 0;
    BX_CPU_THIS_PTR EXT   = 1; // external event
    BX_CPU_THIS_PTR async_event = 1; // probably don't need this
    interrupt(vector, 0, 0, 0);
  }
}

void BX_CPU_C::dbg_take_dma(void)
{
  // NOTE: similar code in ::cpu_loop()
  if ( BX_HRQ ) {
    BX_CPU_THIS_PTR async_event = 1; // set in case INTR is triggered
    DEV_dma_raise_hlda();
  }
}

#endif  // #if BX_DEBUGGER
