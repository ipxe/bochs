/////////////////////////////////////////////////////////////////////////
// $Id: data_xfer8.cc,v 1.19 2004/08/09 21:28:47 sshwarts Exp $
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
BX_CPU_C::MOV_RLIb(bxInstruction_c *i)
{
  BX_READ_8BIT_REGx(i->opcodeReg(),i->extend8bitL()) = i->Ib();
}

  void
BX_CPU_C::MOV_RHIb(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR gen_reg[i->b1() & 0x03].word.byte.rh = i->Ib();
}

  void
BX_CPU_C::MOV_EEbGb(bxInstruction_c *i)
{
  write_virtual_byte(i->seg(), RMAddr(i), &BX_READ_8BIT_REGx(i->nnn(),i->extend8bitL()));
}

  void
BX_CPU_C::MOV_EGbGb(bxInstruction_c *i)
{
  Bit8u op2 = BX_READ_8BIT_REGx(i->nnn(),i->extend8bitL());

  BX_WRITE_8BIT_REGx(i->rm(), i->extend8bitL(), op2);
}


  void
BX_CPU_C::MOV_GbEEb(bxInstruction_c *i)
{
  read_virtual_byte(i->seg(), RMAddr(i), &BX_READ_8BIT_REGx(i->nnn(),i->extend8bitL()));
}

  void
BX_CPU_C::MOV_GbEGb(bxInstruction_c *i)
{
  Bit8u op2 = BX_READ_8BIT_REGx(i->rm(),i->extend8bitL());
  BX_WRITE_8BIT_REGx(i->nnn(), i->extend8bitL(), op2);
}

  void
BX_CPU_C::MOV_ALOb(bxInstruction_c *i)
{
  bx_address addr = i->Id();

  /* read from memory address and write to register */
  if (!BX_NULL_SEG_REG(i->seg())) {
    read_virtual_byte(i->seg(), addr, &AL);
    }
  else {
    read_virtual_byte(BX_SEG_REG_DS, addr, &AL);
    }
}


  void
BX_CPU_C::MOV_ObAL(bxInstruction_c *i)
{
  bx_address addr = i->Id();

  /* write to memory address and write to register */
  if (!BX_NULL_SEG_REG(i->seg())) {
    write_virtual_byte(i->seg(), addr, &AL);
    }
  else {
    write_virtual_byte(BX_SEG_REG_DS, addr, &AL);
    }
}

  void
BX_CPU_C::MOV_EbIb(bxInstruction_c *i)
{
  Bit8u op2 = i->Ib();

  if (i->modC0()) {
    BX_WRITE_8BIT_REGx(i->rm(), i->extend8bitL(), op2);
    }
  else {
    write_virtual_byte(i->seg(), RMAddr(i), &op2);
    }
}

  void
BX_CPU_C::XLAT(bxInstruction_c *i)
{
  Bit32u offset_32;

#if BX_CPU_LEVEL >= 3
  if (i->as32L()) {
    offset_32 = EBX + AL;
    }
  else
#endif /* BX_CPU_LEVEL >= 3 */
    {
    offset_32 = BX + AL;
    }

  if (!BX_NULL_SEG_REG(i->seg())) {
    read_virtual_byte(i->seg(), offset_32, &AL);
    }
  else {
    read_virtual_byte(BX_SEG_REG_DS, offset_32, &AL);
    }
}

  void
BX_CPU_C::XCHG_EbGb(bxInstruction_c *i)
{
  Bit8u op2, op1;

  /* op2 is a register, op2_addr is an index of a register */
  op2 = BX_READ_8BIT_REGx(i->nnn(),i->extend8bitL());

  /* op1 is a register or memory reference */
  if (i->modC0()) {
    op1 = BX_READ_8BIT_REGx(i->rm(),i->extend8bitL());
    BX_WRITE_8BIT_REGx(i->rm(), i->extend8bitL(), op2);
    }
  else {
    /* pointer, segment address pair */
    read_RMW_virtual_byte(i->seg(), RMAddr(i), &op1);
    Write_RMW_virtual_byte(op2);
    }

  BX_WRITE_8BIT_REGx(i->nnn(), i->extend8bitL(), op1);
}
