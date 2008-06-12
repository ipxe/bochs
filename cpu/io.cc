/////////////////////////////////////////////////////////////////////////
// $Id: io.cc,v 1.60 2008/06/12 19:14:39 sshwarts Exp $
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
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#include "iodev/iodev.h"

#if BX_SUPPORT_X86_64==0
// Make life easier for merging cpu64 and cpu32 code.
#define RDI EDI
#define RSI ESI
#define RAX EAX
#define RCX ECX
#endif

//
// Repeat Speedups methods
//

#if BX_SupportRepeatSpeedups
Bit32u BX_CPU_C::FastRepINSW(bxInstruction_c *i, bx_address dstOff, Bit16u port, Bit32u wordCount)
{
  Bit32u wordsFitDst;
  signed int pointerDelta;
  Bit8u *hostAddrDst;
  unsigned count;

  bx_segment_reg_t *dstSegPtr = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES];
  if (!(dstSegPtr->cache.valid & SegAccessWOK4G))
    return 0;

  bx_address laddrDst = BX_CPU_THIS_PTR get_laddr(BX_SEG_REG_ES, dstOff);
  // check that the address is word aligned
  if (laddrDst & 1) return 0;

#if BX_SupportGuest2HostTLB
  hostAddrDst = v2h_write_byte(laddrDst, CPL);
#else
  bx_phy_address paddrDst;

  if (BX_CPU_THIS_PTR cr0.get_PG())
    paddrDst = dtranslate_linear(laddrDst, CPL, BX_WRITE);
  else
    paddrDst = laddrDst;

  // If we want to write directly into the physical memory array,
  // we need the A20 address.
  hostAddrDst = BX_MEM(0)->getHostMemAddr(BX_CPU_THIS,
            A20ADDR(paddrDst), BX_WRITE, DATA_ACCESS);
#endif

  // Check that native host access was not vetoed for that page
  if (!hostAddrDst) return 0;

  // See how many words can fit in the rest of this page.
  if (BX_CPU_THIS_PTR get_DF()) {
    // Counting downward
    // 1st word must cannot cross page boundary because it is word aligned
    wordsFitDst = (2 + (PAGE_OFFSET(laddrDst))) >> 1;
    pointerDelta = -2;
  }
  else {
    // Counting upward
    wordsFitDst = (0x1000 - PAGE_OFFSET(laddrDst)) >> 1;
    pointerDelta =  2;
  }

  // Restrict word count to the number that will fit in this page.
  if (wordCount > wordsFitDst)
      wordCount = wordsFitDst;

  // If after all the restrictions, there is anything left to do...
  if (wordCount) {
    for (count=0; count<wordCount; ) {
      bx_devices.bulkIOQuantumsTransferred = 0;
      if (BX_CPU_THIS_PTR get_DF()==0) { // Only do accel for DF=0
        bx_devices.bulkIOHostAddr = hostAddrDst;
        bx_devices.bulkIOQuantumsRequested = (wordCount - count);
      }
      else
        bx_devices.bulkIOQuantumsRequested = 0;
      Bit16u temp16 = BX_INP(port, 2);
      if (bx_devices.bulkIOQuantumsTransferred) {
        hostAddrDst = bx_devices.bulkIOHostAddr;
        count += bx_devices.bulkIOQuantumsTransferred;
      }
      else {
        WriteHostWordToLittleEndian(hostAddrDst, temp16);
        hostAddrDst += pointerDelta;
        count++;
      }
      // Terminate early if there was an event.
      if (BX_CPU_THIS_PTR async_event) break;
    }

    // Reset for next non-bulk IO
    bx_devices.bulkIOQuantumsRequested = 0;

    return count;
  }

  return 0;
}

Bit32u BX_CPU_C::FastRepOUTSW(bxInstruction_c *i, unsigned srcSeg, bx_address srcOff, Bit16u port, Bit32u wordCount)
{
  Bit32u wordsFitSrc;
  signed int pointerDelta;
  Bit8u *hostAddrSrc;
  unsigned count;

  bx_segment_reg_t *srcSegPtr = &BX_CPU_THIS_PTR sregs[srcSeg];
  if (!(srcSegPtr->cache.valid & SegAccessROK4G))
    return 0;

  bx_address laddrSrc = BX_CPU_THIS_PTR get_laddr(srcSeg, srcOff);
  // check that the address is word aligned
  if (laddrSrc & 1) return 0;

#if BX_SupportGuest2HostTLB
  hostAddrSrc = v2h_read_byte(laddrSrc, CPL);
#else
  bx_phy_address paddrSrc;

  if (BX_CPU_THIS_PTR cr0.get_PG())
    paddrSrc = dtranslate_linear(laddrSrc, CPL, BX_READ);
  else
    paddrSrc = laddrSrc;

  // If we want to write directly into the physical memory array,
  // we need the A20 address.
  hostAddrSrc = BX_MEM(0)->getHostMemAddr(BX_CPU_THIS,
          A20ADDR(paddrSrc), BX_READ, DATA_ACCESS);
#endif

  // Check that native host access was not vetoed for that page
  if (!hostAddrSrc) return 0;

  // See how many words can fit in the rest of this page.
  if (BX_CPU_THIS_PTR get_DF()) {
    // Counting downward
    // 1st word must cannot cross page boundary because it is word aligned
    wordsFitSrc = (2 + (PAGE_OFFSET(laddrSrc))) >> 1;
    pointerDelta = (unsigned) -2;
  }
  else {
    // Counting upward
    wordsFitSrc = (0x1000 - PAGE_OFFSET(laddrSrc)) >> 1;
    pointerDelta =  2;
  }

  // Restrict word count to the number that will fit in this page.
  if (wordCount > wordsFitSrc)
      wordCount = wordsFitSrc;

  // If after all the restrictions, there is anything left to do...
  if (wordCount) {
    for (count=0; count<wordCount; ) {
      bx_devices.bulkIOQuantumsTransferred = 0;
      if (BX_CPU_THIS_PTR get_DF()==0) { // Only do accel for DF=0
        bx_devices.bulkIOHostAddr = hostAddrSrc;
        bx_devices.bulkIOQuantumsRequested = (wordCount - count);
      }
      else
        bx_devices.bulkIOQuantumsRequested = 0;
      Bit16u temp16;
      ReadHostWordFromLittleEndian(hostAddrSrc, temp16);
      BX_OUTP(port, temp16, 2);
      if (bx_devices.bulkIOQuantumsTransferred) {
        hostAddrSrc = bx_devices.bulkIOHostAddr;
        count += bx_devices.bulkIOQuantumsTransferred;
      }
      else {
        hostAddrSrc += pointerDelta;
        count++;
      }
      // Terminate early if there was an event.
      if (BX_CPU_THIS_PTR async_event) break;
    }

    // Reset for next non-bulk IO
    bx_devices.bulkIOQuantumsRequested = 0;

    return count;
  }

  return 0;
}

#endif

//
// REP INS methods
//

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_INSB_YbDX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSB_YbDX);
#if BX_SUPPORT_X86_64
  if (i->as32L()) {
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
#endif
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_INSW_YwDX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSW_YwDX);
#if BX_SUPPORT_X86_64
  if (i->as32L()) {
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
#endif
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_INSD_YdDX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::INSD_YdDX);
#if BX_SUPPORT_X86_64
  if (i->as32L()) {
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RDI); // always clear upper part of RDI
  }
#endif
}

//
// INSB/INSW/INSD methods
//

void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSB_YbDX(bxInstruction_c *i)
{
  Bit8u value8=0;

  if (! BX_CPU_THIS_PTR allow_io(DX, 1)) {
    BX_DEBUG(("INSB_YbDX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_byte_64(BX_SEG_REG_ES, RDI, value8);

    value8 = BX_INP(DX, 1);

    /* no seg override possible */
    write_virtual_byte_64(BX_SEG_REG_ES, RDI, value8);

    if (BX_CPU_THIS_PTR get_DF())
      RDI--;
    else
      RDI++;
  }
  else
#endif
   if (i->as32L()) {
    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_byte(BX_SEG_REG_ES, EDI, value8);

    value8 = BX_INP(DX, 1);

    /* no seg override possible */
    write_virtual_byte(BX_SEG_REG_ES, EDI, value8);

    if (BX_CPU_THIS_PTR get_DF()) {
      RDI = EDI - 1;
    }
    else {
      RDI = EDI + 1;
    }
  }
  else { // 16-bit address size
    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_byte_32(BX_SEG_REG_ES, DI, value8);

    value8 = BX_INP(DX, 1);

    /* no seg override possible */
    write_virtual_byte_32(BX_SEG_REG_ES, DI, value8);

    if (BX_CPU_THIS_PTR get_DF())
      DI--;
    else
      DI++;
  }
}

// input word from port to string
void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSW_YwDX(bxInstruction_c *i)
{
  Bit16u value16=0;

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    Bit64u rdi = RDI;

    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_word_64(BX_SEG_REG_ES, rdi, value16);

    value16 = BX_INP(DX, 2);

    /* no seg override allowed */
    write_virtual_word_64(BX_SEG_REG_ES, rdi, value16);

    if (BX_CPU_THIS_PTR get_DF())
      rdi -= 2;
    else
      rdi += 2;

    RDI = rdi;
  }
  else
#endif
  {
    Bit32u edi;
    Bit32u incr = 2;

    if (i->as32L())
      edi = EDI;
    else
      edi = DI;

#if (BX_SupportRepeatSpeedups) && (BX_DEBUGGER == 0)
    /* If conditions are right, we can transfer IO to physical memory
     * in a batch, rather than one instruction at a time.
     */
    if (i->repUsedL() && !BX_CPU_THIS_PTR async_event)
    {
      Bit32u wordCount;

      if (i->as32L())
        wordCount = ECX;
      else
        wordCount =  CX;

      BX_ASSERT(wordCount > 0);

      wordCount = FastRepINSW(i, edi, DX, wordCount);
      if (wordCount)
      {
        // Decrement the ticks count by the number of iterations, minus
        // one, since the main cpu loop will decrement one.  Also,
        // the count is predecremented before examined, so defintely
        // don't roll it under zero.
        BX_TICKN(wordCount-1);

        if (i->as32L())
          RCX = ECX - (wordCount-1);
        else
           CX -= (wordCount-1);

        incr = wordCount << 1; // count * 2.
        goto doIncr;
      }
    }
#endif

    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_word(BX_SEG_REG_ES, edi, value16);

    value16 = BX_INP(DX, 2);

    /* no seg override allowed */
    write_virtual_word(BX_SEG_REG_ES, edi, value16);

#if (BX_SupportRepeatSpeedups) && (BX_DEBUGGER == 0)
doIncr:
#endif

    if (i->as32L()) {
      if (BX_CPU_THIS_PTR get_DF())
        RDI = EDI - incr;
      else
        RDI = EDI + incr;
    }
    else {
      if (BX_CPU_THIS_PTR get_DF())
        DI -= incr;
      else
        DI += incr;
    }
  }
}

// input doubleword from port to string
void BX_CPP_AttrRegparmN(1) BX_CPU_C::INSD_YdDX(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR allow_io(DX, 4)) {
    BX_DEBUG(("INSD_YdDX: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    Bit64u rdi = RDI;

    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_dword_64(BX_SEG_REG_ES, rdi, 0);

    Bit32u value32 = BX_INP(DX, 4);

    /* no seg override allowed */
    write_virtual_dword_64(BX_SEG_REG_ES, rdi, value32);

    if (BX_CPU_THIS_PTR get_DF())
      rdi -= 4;
    else
      rdi += 4;

    RDI = rdi;
  }
  else
#endif
  if (i->as32L()) {
    Bit32u edi = EDI;

    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_dword(BX_SEG_REG_ES, edi, 0);

    Bit32u value32 = BX_INP(DX, 4);

    /* no seg override allowed */
    write_virtual_dword(BX_SEG_REG_ES, edi, value32);

    if (BX_CPU_THIS_PTR get_DF())
      edi -= 4;
    else
      edi += 4;

    RDI = edi;
  }
  else { // 16-bit address size
    Bit16u di = DI;

    // Write a zero to memory, to trigger any segment or page
    // faults before reading from IO port.
    write_virtual_dword_32(BX_SEG_REG_ES, di, 0);

    Bit32u value32 = BX_INP(DX, 4);

    /* no seg override allowed */
    write_virtual_dword_32(BX_SEG_REG_ES, di, value32);

    if (BX_CPU_THIS_PTR get_DF())
      di -= 4;
    else
      di += 4;

    DI = di;
  }
}

//
// REP OUTS methods
//

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_OUTSB_DXXb(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSB_DXXb);
#if BX_SUPPORT_X86_64
  if (i->as32L()) {
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
#endif
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_OUTSW_DXXw(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSW_DXXw);
#if BX_SUPPORT_X86_64
  if (i->as32L()) {
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
#endif
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::REP_OUTSD_DXXd(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR repeat(i, &BX_CPU_C::OUTSD_DXXd);
#if BX_SUPPORT_X86_64
  if (i->as32L()) {
    BX_CLEAR_64BIT_HIGH(BX_64BIT_REG_RSI); // always clear upper part of RSI
  }
#endif
}

//
// OUTSB/OUTSW/OUTSD methods
//

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSB_DXXb(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR allow_io(DX, 1)) {
    BX_DEBUG(("OUTSB_DXXb: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  Bit8u value8;

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    Bit64u rsi = RSI;

    value8 = read_virtual_byte_64(i->seg(), rsi);
    BX_OUTP(DX, value8, 1);

    if (BX_CPU_THIS_PTR get_DF())
      rsi--;
    else
      rsi++;

    RSI = rsi;
  }
  else
#endif
  if (i->as32L()) {
    Bit32u esi = ESI;

    value8 = read_virtual_byte(i->seg(), esi);
    BX_OUTP(DX, value8, 1);

    if (BX_CPU_THIS_PTR get_DF())
      esi--;
    else
      esi++;

    RSI = esi;
  }
  else { // address size 16-bit
    Bit16u si = SI;

    value8 = read_virtual_byte_32(i->seg(), si);
    BX_OUTP(DX, value8, 1);

    if (BX_CPU_THIS_PTR get_DF())
      si--;
    else
      si++;

    SI = si;
  }
}

// output word string to port
void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSW_DXXw(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR allow_io(DX, 2)) {
    BX_DEBUG(("OUTSW_DXXw: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  Bit16u value16;

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    Bit64u rsi = RSI;

    value16 = read_virtual_word_64(i->seg(), rsi);
    BX_OUTP(DX, value16, 2);

    if (BX_CPU_THIS_PTR get_DF())
      rsi -= 2;
    else
      rsi += 2;

    RSI = rsi;
  }
  else
#endif
  {
    Bit32u esi;
    Bit32u incr = 2;

    if (i->as32L())
      esi = ESI;
    else
      esi =  SI;

#if (BX_SupportRepeatSpeedups) && (BX_DEBUGGER == 0)
    /* If conditions are right, we can transfer IO to physical memory
     * in a batch, rather than one instruction at a time.
     */
    if (i->repUsedL() && !BX_CPU_THIS_PTR async_event) {
      Bit32u wordCount;

      if (i->as32L())
        wordCount = ECX;
      else
        wordCount =  CX;

      wordCount = FastRepOUTSW(i, i->seg(), esi, DX, wordCount);
      if (wordCount) {
        // Decrement eCX.  Note, the main loop will decrement 1 also, so
        // decrement by one less than expected, like the case above.
        BX_TICKN(wordCount-1); // Main cpu loop also decrements one more.

        if (i->as32L())
          RCX = ECX - (wordCount-1);
        else
           CX -= (wordCount-1);
        incr = wordCount << 1; // count * 2.
      }
      else {
        value16 = read_virtual_word(i->seg(), esi);
        BX_OUTP(DX, value16, 2);
      }
    }
    else
#endif
    {
      value16 = read_virtual_word(i->seg(), esi);
      BX_OUTP(DX, value16, 2);
    }

    if (i->as32L()) {
      if (BX_CPU_THIS_PTR get_DF())
        RSI = ESI - incr;
      else
        RSI = ESI + incr;
    }
    else {
      if (BX_CPU_THIS_PTR get_DF())
        SI = SI - incr;
      else
        SI = SI + incr;
    }
  }
}

// output doubleword string to port
void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUTSD_DXXd(bxInstruction_c *i)
{
  if (! BX_CPU_THIS_PTR allow_io(DX, 4)) {
    BX_DEBUG(("OUTSD_DXXd: I/O access not allowed !"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  Bit32u value32;

#if BX_SUPPORT_X86_64
  if (i->as64L()) {
    Bit64u rsi = RSI;

    value32 = read_virtual_dword_64(i->seg(), rsi);
    BX_OUTP(DX, value32, 4);

    if (BX_CPU_THIS_PTR get_DF())
      rsi -= 4;
    else
      rsi += 4;

    RSI = rsi;
  }
  else
#endif
  if (i->as32L()) {
    Bit32u esi = ESI;

    value32 = read_virtual_dword(i->seg(), esi);
    BX_OUTP(DX, value32, 4);

    if (BX_CPU_THIS_PTR get_DF())
      esi -= 4;
    else
      esi += 4;

    RSI = esi;
  }
  else { // address size 16-bit
    Bit16u si = SI;

    value32 = read_virtual_dword_32(i->seg(), si);
    BX_OUTP(DX, value32, 4);

    if (BX_CPU_THIS_PTR get_DF())
      si -= 4;
    else
      si += 4;

    SI = si;
  }
}

//
// non repeatable IN/OUT methods
//

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_ALIb(bxInstruction_c *i)
{
  AL = BX_CPU_THIS_PTR inp8(i->Ib());
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_AXIb(bxInstruction_c *i)
{
  AX = BX_CPU_THIS_PTR inp16(i->Ib());
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_EAXIb(bxInstruction_c *i)
{
  RAX = BX_CPU_THIS_PTR inp32(i->Ib());
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_IbAL(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR outp8(i->Ib(), AL);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_IbAX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR outp16(i->Ib(), AX);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_IbEAX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR outp32(i->Ib(), EAX);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_ALDX(bxInstruction_c *i)
{
  AL = BX_CPU_THIS_PTR inp8(DX);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_AXDX(bxInstruction_c *i)
{
  AX = BX_CPU_THIS_PTR inp16(DX);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IN_EAXDX(bxInstruction_c *i)
{
  RAX = BX_CPU_THIS_PTR inp32(DX);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_DXAL(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR outp8(DX, AL);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_DXAX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR outp16(DX, AX);
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::OUT_DXEAX(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR outp32(DX, EAX);
}
