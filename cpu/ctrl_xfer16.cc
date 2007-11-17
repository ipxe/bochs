/////////////////////////////////////////////////////////////////////////
// $Id: ctrl_xfer16.cc,v 1.41 2007/11/17 18:08:46 sshwarts Exp $
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
/////////////////////////////////////////////////////////////////////////


#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR


void BX_CPU_C::RETnear16_Iw(bxInstruction_c *i)
{
  Bit16u return_IP;

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  pop_16(&return_IP);

  if (return_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) 
  {
    BX_ERROR(("retnear_iw: IP > limit"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  EIP = return_IP;

  Bit16u imm16 = i->Iw();

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) /* 32bit stack */
    ESP += imm16;
  else
    SP  += imm16;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET, EIP);
}

void BX_CPU_C::RETnear16(bxInstruction_c *i)
{
  Bit16u return_IP;

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  pop_16(&return_IP);

  if (return_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled) 
  {
    BX_ERROR(("retnear: IP > limit"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  EIP = return_IP;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET, EIP);
}

void BX_CPU_C::RETfar16_Iw(bxInstruction_c *i)
{
  Bit16s imm16;
  Bit16u ip, cs_raw;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_ret;
#endif

  imm16 = i->Iw();

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    BX_CPU_THIS_PTR return_protected(i, imm16);
    goto done;
  }
#endif

  pop_16(&ip);
  pop_16(&cs_raw);

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = (Bit32u) ip;

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
    ESP += imm16;
  else
    SP  += imm16;

done:
  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

void BX_CPU_C::RETfar16(bxInstruction_c *i)
{
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

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = (Bit32u) ip;

done:
  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_RET,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

void BX_CPU_C::CALL_Jw(bxInstruction_c *i)
{
  Bit32u new_EIP;

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

  new_EIP = EIP + (Bit32s) i->Id();
  new_EIP &= 0x0000ffff;

  if (new_EIP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled)
  {
    BX_ERROR(("CALL_Jw: new_IP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].limit"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  /* push 16 bit EA of next instruction */
  push_16(IP);
  EIP = new_EIP;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL, EIP);
}

void BX_CPU_C::CALL16_Ap(bxInstruction_c *i)
{
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

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = (Bit32u) disp16;

done:
  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

void BX_CPU_C::CALL_Ew(bxInstruction_c *i)
{
  Bit16u op1_16;

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
  if (op1_16 > BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled)
  {
    BX_ERROR(("CALL_Ew: IP out of CS limits!"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }
#endif

  push_16(IP);
  EIP = op1_16;

  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL, EIP);
}

void BX_CPU_C::CALL16_Ep(bxInstruction_c *i)
{
  Bit16u cs_raw;
  Bit16u op1_16;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_call;
#endif

  if (i->modC0()) {
    BX_INFO(("CALL_Ep: op1 is a register"));
    exception(BX_UD_EXCEPTION, 0, 0);
  }

  read_virtual_word(i->seg(), RMAddr(i), &op1_16);
  read_virtual_word(i->seg(), RMAddr(i)+2, &cs_raw);

  if (protected_mode()) {
    BX_CPU_THIS_PTR call_protected(i, cs_raw, op1_16);
    goto done;
  }

  push_16(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
  push_16(IP);

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = op1_16;

done:
  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_CALL,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

void BX_CPU_C::JMP_Jw(bxInstruction_c *i)
{
  Bit32u new_EIP = EIP + (Bit32s) i->Id();
  new_EIP &= 0x0000ffff;
  branch_near32(new_EIP);
  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP, new_EIP);
}

void BX_CPU_C::JO_Jw(bxInstruction_c *i)
{
  if (get_OF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNO_Jw(bxInstruction_c *i)
{
  if (! get_OF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JB_Jw(bxInstruction_c *i)
{
  if (get_CF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNB_Jw(bxInstruction_c *i)
{
  if (! get_CF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JZ_Jw(bxInstruction_c *i)
{
  if (get_ZF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNZ_Jw(bxInstruction_c *i)
{
  if (! get_ZF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JBE_Jw(bxInstruction_c *i)
{
  if (get_CF() || get_ZF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNBE_Jw(bxInstruction_c *i)
{
  if (! (get_CF() || get_ZF())) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JS_Jw(bxInstruction_c *i)
{
  if (get_SF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNS_Jw(bxInstruction_c *i)
{
  if (! get_SF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JP_Jw(bxInstruction_c *i)
{
  if (get_PF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNP_Jw(bxInstruction_c *i)
{
  if (! get_PF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JL_Jw(bxInstruction_c *i)
{
  if (getB_SF() != getB_OF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNL_Jw(bxInstruction_c *i)
{
  if (getB_SF() == getB_OF()) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JLE_Jw(bxInstruction_c *i)
{
  if (get_ZF() || (getB_SF() != getB_OF())) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JNLE_Jw(bxInstruction_c *i)
{
  if (! get_ZF() && (getB_SF() == getB_OF())) {
    Bit32u new_EIP = EIP + (Bit32s) i->Id();
    new_EIP &= 0x0000ffff;
    branch_near32(new_EIP);
    BX_INSTR_CNEAR_BRANCH_TAKEN(BX_CPU_ID, new_EIP);
  }
#if BX_INSTRUMENTATION
  else {
    BX_INSTR_CNEAR_BRANCH_NOT_TAKEN(BX_CPU_ID);
  }
#endif
}

void BX_CPU_C::JMP_Ew(bxInstruction_c *i)
{
  Bit16u op1_16;

  if (i->modC0()) {
    op1_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    read_virtual_word(i->seg(), RMAddr(i), &op1_16);
  }

  Bit32u new_EIP = op1_16;
  branch_near32(new_EIP);
  BX_INSTR_UCNEAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP, new_EIP);
}

/* Far indirect jump */
void BX_CPU_C::JMP16_Ep(bxInstruction_c *i)
{
  Bit16u cs_raw;
  Bit16u op1_16;

  invalidate_prefetch_q();

  if (i->modC0()) {
    /* far indirect must specify a memory address */
    BX_INFO(("JMP_Ep(): op1 is a register"));
    exception(BX_UD_EXCEPTION, 0, 0);
  }

  read_virtual_word(i->seg(), RMAddr(i), &op1_16);
  read_virtual_word(i->seg(), RMAddr(i)+2, &cs_raw);

#if BX_CPU_LEVEL >= 2
  if ( protected_mode() ) {
    BX_CPU_THIS_PTR jump_protected(i, cs_raw, op1_16);
    goto done;
  }
#endif

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = op1_16;

done:
  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_JMP,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}

void BX_CPU_C::IRET16(bxInstruction_c *i)
{
  Bit16u ip, cs_raw, flags;

  invalidate_prefetch_q();

#if BX_DEBUGGER
  BX_CPU_THIS_PTR show_flag |= Flag_iret;
#endif

  BX_CPU_THIS_PTR nmi_disable = 0;

  if (v8086_mode()) {
    // IOPL check in stack_return_from_v86()
    iret16_stack_return_from_v86(i);
    goto done;
  }

  if (protected_mode()) {
    iret_protected(i);
    goto done;
  }

  if (! can_pop(6)) {
    BX_ERROR(("IRET: top 6 bytes of stack not within stack limits"));
    exception(BX_SS_EXCEPTION, 0, 0);
  }

  pop_16(&ip);
  pop_16(&cs_raw);
  pop_16(&flags);

  load_seg_reg(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS], cs_raw);
  EIP = (Bit32u) ip;
  write_flags(flags, /* change IOPL? */ 1, /* change IF? */ 1);

done:
  BX_INSTR_FAR_BRANCH(BX_CPU_ID, BX_INSTR_IS_IRET,
                      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
}
