/////////////////////////////////////////////////////////////////////////
// $Id: ctrl_xfer16.cc,v 1.15 2002/09/24 00:44:55 kevinlawton Exp $
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
#define LOG_THIS BX_CPU_THIS_PTR




  void
BX_CPU_C::RETnear16_Iw(bxInstruction_c *i)
{
BailBigRSP("RETnear16_Iw");
  Bit16u imm16;
  Bit16u return_IP;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  imm16 = i->Iw();

  pop_16(&return_IP);
  if (protected_mode()) {
    if ( return_IP >
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled ) {
      BX_PANIC(("retnear_iw: IP > limit"));
      }
    }
  EIP = return_IP;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) /* 32bit stack */
    ESP += imm16; /* ??? should it be 2*imm16 ? */
  else
    SP  += imm16;

  BX_INSTR_UCNEAR_BRANCH(BX_INSTR_IS_RET, EIP);
}

  void
BX_CPU_C::RETnear16(bxInstruction_c *i)
{
BailBigRSP("RETnear16");
  Bit16u return_IP;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  pop_16(&return_IP);
  if (protected_mode()) {
    if ( return_IP >
         BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled ) {
      BX_PANIC(("retnear: IP > limit"));
      }
    }
  EIP = return_IP;

  BX_INSTR_UCNEAR_BRANCH(BX_INSTR_IS_RET, EIP);
}

  void
BX_CPU_C::RETfar16_Iw(bxInstruction_c *i)
{
BailBigRSP("RETfar16_Iw");
  Bit16s imm16;
  Bit16u ip, cs_raw;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  /* ??? is imm16, number of bytes/words depending on operandsize ? */

  imm16 = i->Iw();

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    BX_CPU_THIS_PTR return_protected(i, imm16);
    goto done;
    }
#endif


  pop_16(&ip);
  pop_16(&cs_raw);
  EIP = (Bit32u) ip;
  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
    ESP += imm16;
  else
    SP  += imm16;

done:
  BX_INSTR_FAR_BRANCH(BX_INSTR_IS_RET,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

  void
BX_CPU_C::RETfar16(bxInstruction_c *i)
{
BailBigRSP("RETfar16");
  Bit16u ip, cs_raw;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

#if BX_CPU_LEVEL >= 2
  if ( protected_mode() ) {
    BX_CPU_THIS_PTR return_protected(i, 0);
    goto done;
    }
#endif

  pop_16(&ip);
  pop_16(&cs_raw);
  EIP = (Bit32u) ip;
  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);

done:
  BX_INSTR_FAR_BRANCH(BX_INSTR_IS_RET,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}



  void
BX_CPU_C::CALL_Aw(bxInstruction_c *i)
{
BailBigRSP("CALL_Aw");
  Bit32u new_EIP;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

  new_EIP = EIP + (Bit32s) i->Id();
  new_EIP &= 0x0000ffff;
#if BX_CPU_LEVEL >= 2
  if ( protected_mode() &&
       (new_EIP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) ) {
    BX_PANIC(("call_av: new_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].limit"));
    exception(BX_GP_EXCEPTION, 0, 0);
    }
#endif

  /* push 16 bit EA of next instruction */
  push_16(IP);
  EIP = new_EIP;

  BX_INSTR_UCNEAR_BRANCH(BX_INSTR_IS_CALL, EIP);
}

  void
BX_CPU_C::CALL16_Ap(bxInstruction_c *i)
{
BailBigRSP("CALL16_Ap");
  Bit16u cs_raw;
  Bit16u disp16;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

  disp16 = i->Iw();
  cs_raw = i->Iw2();

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    BX_CPU_THIS_PTR call_protected(i, cs_raw, disp16);
    goto done;
    }
#endif

  push_16(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
  push_16((Bit16u) EIP);
  EIP = (Bit32u) disp16;
  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);

done:
  BX_INSTR_FAR_BRANCH(BX_INSTR_IS_CALL,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

  void
BX_CPU_C::CALL_Ew(bxInstruction_c *i)
{
BailBigRSP("CALL_Ew");
  Bit16u op1_16;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

  if (i->modC0()) {
    op1_16 = BX_READ_16BIT_REG(i->rm());
    }
  else {
    read_virtual_word(i->seg(), RMAddr(i), &op1_16);
    }

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    if (op1_16 >
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
      BX_PANIC(("call_ev: IP out of CS limits!"));
      exception(BX_GP_EXCEPTION, 0, 0);
      }
    }
#endif

  push_16(IP);
  EIP = op1_16;

  BX_INSTR_UCNEAR_BRANCH(BX_INSTR_IS_CALL, EIP);
}

  void
BX_CPU_C::CALL16_Ep(bxInstruction_c *i)
{
BailBigRSP("CALL_16_Ep");
  Bit16u cs_raw;
  Bit16u op1_16;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

  if (i->modC0()) {
    BX_PANIC(("CALL_Ep: op1 is a register"));
    }

  read_virtual_word(i->seg(), RMAddr(i), &op1_16);
  read_virtual_word(i->seg(), RMAddr(i)+2, &cs_raw);

  if ( protected_mode() ) {
    BX_CPU_THIS_PTR call_protected(i, cs_raw, op1_16);
    goto done;
    }

  push_16(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
  push_16(IP);

  EIP = op1_16;
  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);

done:
  BX_INSTR_FAR_BRANCH(BX_INSTR_IS_CALL,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}


  void
BX_CPU_C::JMP_Jw(bxInstruction_c *i)
{
BailBigRSP("JMP_Jw");
  Bit32u new_EIP;

  invalidate_prefetch_q();

  new_EIP = EIP + (Bit32s) i->Id();
  new_EIP &= 0x0000ffff;

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    if ( new_EIP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled ) {
      BX_PANIC(("jmp_jv: offset outside of CS limits"));
      exception(BX_GP_EXCEPTION, 0, 0);
      }
    }
#endif

  EIP = new_EIP;
  BX_INSTR_UCNEAR_BRANCH(BX_INSTR_IS_JMP, new_EIP);
}

  void
BX_CPU_C::JCC_Jw(bxInstruction_c *i)
{
BailBigRSP("JCC_Jw");
  Boolean condition;

  switch (i->b1() & 0x0f) {
    case 0x00: /* JO */ condition = get_OF(); break;
    case 0x01: /* JNO */ condition = !get_OF(); break;
    case 0x02: /* JB */ condition = get_CF(); break;
    case 0x03: /* JNB */ condition = !get_CF(); break;
    case 0x04: /* JZ */ condition = get_ZF(); break;
    case 0x05: /* JNZ */ condition = !get_ZF(); break;
    case 0x06: /* JBE */ condition = get_CF() || get_ZF(); break;
    case 0x07: /* JNBE */ condition = !get_CF() && !get_ZF(); break;
    case 0x08: /* JS */ condition = get_SF(); break;
    case 0x09: /* JNS */ condition = !get_SF(); break;
    case 0x0A: /* JP */ condition = get_PF(); break;
    case 0x0B: /* JNP */ condition = !get_PF(); break;
    case 0x0C: /* JL */ condition = getB_SF() != getB_OF(); break;
    case 0x0D: /* JNL */ condition = getB_SF() == getB_OF(); break;
    case 0x0E: /* JLE */ condition = get_ZF() || (getB_SF() != getB_OF());
      break;
    case 0x0F: /* JNLE */ condition = (getB_SF() == getB_OF()) &&
                            !get_ZF();
      break;
    default:
      condition = 0; // For compiler...all targets should set condition.
      break;
    }

  if (condition) {
    Bit32u new_EIP;

    new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
#if BX_CPU_LEVEL >= 2
    if (protected_mode()) {
      if ( new_EIP >
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled ) {
        BX_PANIC(("jo_routine: offset outside of CS limits"));
        exception(BX_GP_EXCEPTION, 0, 0);
        }
      }
#endif
    EIP = new_EIP;
    BX_INSTR_CNEAR_BRANCH_TAKEN(new_EIP);
    revalidate_prefetch_q();
    }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN();
    }
#endif
}

  void
BX_CPU_C::JZ_Jw(bxInstruction_c *i)
{
BailBigRSP("JZ_Jw");
  if (get_ZF()) {
    Bit32u new_EIP;

    new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
#if BX_CPU_LEVEL >= 2
    if (protected_mode()) {
      if ( new_EIP >
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled ) {
        BX_PANIC(("jo_routine: offset outside of CS limits"));
        exception(BX_GP_EXCEPTION, 0, 0);
        }
      }
#endif
    EIP = new_EIP;
    BX_INSTR_CNEAR_BRANCH_TAKEN(new_EIP);
    revalidate_prefetch_q();
    }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN();
    }
#endif
}

  void
BX_CPU_C::JNZ_Jw(bxInstruction_c *i)
{
BailBigRSP("JNZ_Jw");
  if (!get_ZF()) {
    Bit32u new_EIP;

    new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
#if BX_CPU_LEVEL >= 2
    if (protected_mode()) {
      if ( new_EIP >
           BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled ) {
        BX_PANIC(("jo_routine: offset outside of CS limits"));
        exception(BX_GP_EXCEPTION, 0, 0);
        }
      }
#endif
    EIP = new_EIP;
    BX_INSTR_CNEAR_BRANCH_TAKEN(new_EIP);
    revalidate_prefetch_q();
    }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN();
    }
#endif
}


  void
BX_CPU_C::JMP_Ew(bxInstruction_c *i)
{
BailBigRSP("JMP_Ew");
  Bit32u new_EIP;
  Bit16u op1_16;

  invalidate_prefetch_q();

  if (i->modC0()) {
    op1_16 = BX_READ_16BIT_REG(i->rm());
    }
  else {
    read_virtual_word(i->seg(), RMAddr(i), &op1_16);
    }

  new_EIP = op1_16;

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    if (new_EIP >
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) {
      BX_PANIC(("jmp_ev: IP out of CS limits!"));
      exception(BX_GP_EXCEPTION, 0, 0);
      }
    }
#endif

  EIP = new_EIP;

  BX_INSTR_UCNEAR_BRANCH(BX_INSTR_IS_JMP, new_EIP);
}

  /* Far indirect jump */

  void
BX_CPU_C::JMP16_Ep(bxInstruction_c *i)
{
BailBigRSP("JMP16_Ep");
  Bit16u cs_raw;
  Bit16u op1_16;

  invalidate_prefetch_q();

  if (i->modC0()) {
    /* far indirect must specify a memory address */
    BX_PANIC(("JMP_Ep(): op1 is a register"));
    }

  read_virtual_word(i->seg(), RMAddr(i), &op1_16);
  read_virtual_word(i->seg(), RMAddr(i)+2, &cs_raw);

#if BX_CPU_LEVEL >= 2
  if ( protected_mode() ) {
    BX_CPU_THIS_PTR jump_protected(i, cs_raw, op1_16);
    goto done;
    }
#endif

  EIP = op1_16;
  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);

done:
  BX_INSTR_FAR_BRANCH(BX_INSTR_IS_JMP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

  void
BX_CPU_C::IRET16(bxInstruction_c *i)
{
BailBigRSP("IRET16");
  Bit16u ip, cs_raw, flags;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_iret;
  BX_CPU_THIS_PTR show_eip = EIP;
#endif

  if (v8086_mode()) {
    // IOPL check in stack_return_from_v86()
    stack_return_from_v86(i);
    goto done;
    }

#if BX_CPU_LEVEL >= 2
  if (BX_CPU_THIS_PTR cr0.pe) {
    iret_protected(i);
    goto done;
    }
#endif


  pop_16(&ip);
  pop_16(&cs_raw);
  pop_16(&flags);

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = (Bit32u) ip;
  write_flags(flags, /* change IOPL? */ 1, /* change IF? */ 1);

done:
  BX_INSTR_FAR_BRANCH(BX_INSTR_IS_IRET,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}
