/////////////////////////////////////////////////////////////////////////
// $Id: mult16.cc,v 1.30 2008/05/24 10:26:03 sshwarts Exp $
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

void BX_CPP_AttrRegparmN(1) BX_CPU_C::MUL_AXEw(bxInstruction_c *i)
{
  Bit16u op1_16, op2_16;

  op1_16 = AX;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    /* pointer, segment address pair */
    op2_16 = read_virtual_word(i->seg(), RMAddr(i));
  }

  Bit32u product_32  = ((Bit32u) op1_16) * ((Bit32u) op2_16);
  Bit16u product_16l = (product_32 & 0xFFFF);
  Bit16u product_16h =  product_32 >> 16;
  /* now write product back to destination */
  AX = product_16l;
  DX = product_16h;

  /* set EFLAGS */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16l);
  if(product_16h != 0)
  {
    ASSERT_FLAGS_OxxxxC();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_AXEw(bxInstruction_c *i)
{
  Bit16s op1_16, op2_16;

  op1_16 = AX;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    /* pointer, segment address pair */
    op2_16 = (Bit16s) read_virtual_word(i->seg(), RMAddr(i));
  }

  Bit32s product_32  = ((Bit32s) op1_16) * ((Bit32s) op2_16);
  Bit16u product_16l = (product_32 & 0xFFFF);
  Bit16u product_16h = product_32 >> 16;

  /* now write product back to destination */
  AX = product_16l;
  DX = product_16h;

  /* set eflags:
   * IMUL r/m16: condition for clearing CF & OF:
   *   DX:AX = sign-extend of AX
   */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16l);
  if(product_32 != (Bit16s)product_32)
  {
    ASSERT_FLAGS_OxxxxC();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::DIV_AXEw(bxInstruction_c *i)
{
  Bit16u op2_16, remainder_16, quotient_16l;
  Bit32u op1_32, quotient_32;

  op1_32 = (((Bit32u) DX) << 16) | ((Bit32u) AX);

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    /* pointer, segment address pair */
    op2_16 = read_virtual_word(i->seg(), RMAddr(i));
  }

  if (op2_16 == 0)
    exception(BX_DE_EXCEPTION, 0, 0);

  quotient_32  = op1_32 / op2_16;
  remainder_16 = op1_32 % op2_16;
  quotient_16l = quotient_32 & 0xFFFF;

  if (quotient_32 != quotient_16l)
    exception(BX_DE_EXCEPTION, 0, 0);

  /* now write quotient back to destination */
  AX = quotient_16l;
  DX = remainder_16;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IDIV_AXEw(bxInstruction_c *i)
{
  Bit16s op2_16, remainder_16, quotient_16l;
  Bit32s op1_32, quotient_32;

  op1_32 = ((((Bit32u) DX) << 16) | ((Bit32u) AX));

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    /* pointer, segment address pair */
    op2_16 = (Bit16s) read_virtual_word(i->seg(), RMAddr(i));
  }

  if (op2_16 == 0)
    exception(BX_DE_EXCEPTION, 0, 0);

  /* check MIN_INT case */
  if (op1_32 == ((Bit32s)0x80000000))
    exception(BX_DE_EXCEPTION, 0, 0);

  quotient_32  = op1_32 / op2_16;
  remainder_16 = op1_32 % op2_16;
  quotient_16l = quotient_32 & 0xFFFF;

  if (quotient_32 != quotient_16l)
    exception(BX_DE_EXCEPTION, 0, 0);

  /* now write quotient back to destination */
  AX = quotient_16l;
  DX = remainder_16;
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GwEwIw(bxInstruction_c *i)
{
  Bit16s op2_16, op3_16;

  op3_16 = i->Iw();

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    /* pointer, segment address pair */
    op2_16 = (Bit16s) read_virtual_word(i->seg(), RMAddr(i));
  }

  Bit32s product_32  = op2_16 * op3_16;
  Bit16u product_16 = (product_32 & 0xFFFF);

  /* now write product back to destination */
  BX_WRITE_16BIT_REG(i->nnn(), product_16);

  /* set eflags:
   * IMUL r16,r/m16,imm16: condition for clearing CF & OF:
   *   result exactly fits within r16
   */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16);
  if(product_32 != (Bit16s) product_32)
  {
    ASSERT_FLAGS_OxxxxC();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::IMUL_GwEw(bxInstruction_c *i)
{
  Bit16s op1_16, op2_16;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2_16 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
    /* pointer, segment address pair */
    op2_16 = (Bit16s) read_virtual_word(i->seg(), RMAddr(i));
  }

  op1_16 = BX_READ_16BIT_REG(i->nnn());

  Bit32s product_32 = op1_16 * op2_16;
  Bit16u product_16 = (product_32 & 0xFFFF);

  /* now write product back to destination */
  BX_WRITE_16BIT_REG(i->nnn(), product_16);

  /* set eflags:
   * IMUL r16,r/m16: condition for clearing CF & OF:
   *   result exactly fits within r16
   */
  SET_FLAGS_OSZAPC_LOGIC_16(product_16);
  if(product_32 != (Bit16s) product_32)
  {
    ASSERT_FLAGS_OxxxxC();
  }
}
