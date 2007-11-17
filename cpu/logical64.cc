/////////////////////////////////////////////////////////////////////////
// $Id: logical64.cc,v 1.21 2007/11/17 23:28:31 sshwarts Exp $
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

#if BX_SUPPORT_X86_64

void BX_CPU_C::XOR_EqGq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op2_64 = BX_READ_64BIT_REG(i->nnn());

  /* op1_64 is a register or memory reference */
  if (i->modC0()) {
    op1_64 = BX_READ_64BIT_REG(i->rm());
    op1_64 ^= op2_64;
    BX_WRITE_64BIT_REG(i->rm(), op1_64);
  }
  else {
    /* pointer, segment address pair */
    read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
    op1_64 ^= op2_64;
    write_RMW_virtual_qword(op1_64);
  }

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::XOR_GqEq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->nnn());

  /* op2_64 is a register or memory reference */
  if (i->modC0()) {
    op2_64 = BX_READ_64BIT_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2_64);
  }

  op1_64 ^= op2_64;

  /* now write result back to destination */
  BX_WRITE_64BIT_REG(i->nnn(), op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::XOR_RAXId(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = RAX;
  op2_64 = (Bit32s) i->Id();
  op1_64 ^= op2_64;

  RAX = op1_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::XOR_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  /* pointer, segment address pair */
  read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
  op1_64 ^= op2_64;
  write_RMW_virtual_qword(op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::XOR_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  op1_64 = BX_READ_64BIT_REG(i->rm());
  op1_64 ^= op2_64;
  BX_WRITE_64BIT_REG(i->rm(), op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::OR_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  /* pointer, segment address pair */
  read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
  op1_64 |= op2_64;
  write_RMW_virtual_qword(op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::OR_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  op1_64 = BX_READ_64BIT_REG(i->rm());
  op1_64 |= op2_64;
  BX_WRITE_64BIT_REG(i->rm(), op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::NOT_EqM(bxInstruction_c *i)
{
  Bit64u op1_64;

  /* pointer, segment address pair */
  read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
  op1_64 = ~op1_64;
  write_RMW_virtual_qword(op1_64);
}

void BX_CPU_C::NOT_EqR(bxInstruction_c *i)
{
  Bit64u op1_64 = BX_READ_64BIT_REG(i->rm());
  op1_64 = ~op1_64;
  BX_WRITE_64BIT_REG(i->rm(), op1_64);
}

void BX_CPU_C::OR_EqGq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op2_64 = BX_READ_64BIT_REG(i->nnn());

  /* op1_64 is a register or memory reference */
  if (i->modC0()) {
    op1_64 = BX_READ_64BIT_REG(i->rm());
    op1_64 |= op2_64;
    BX_WRITE_64BIT_REG(i->rm(), op1_64);
  }
  else {
    /* pointer, segment address pair */
    read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
    op1_64 |= op2_64;
    write_RMW_virtual_qword(op1_64);
  }

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::OR_GqEq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->nnn());

  /* op2_64 is a register or memory reference */
  if (i->modC0()) {
    op2_64 = BX_READ_64BIT_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2_64);
  }

  op1_64 |= op2_64;

  /* now write result back to destination */
  BX_WRITE_64BIT_REG(i->nnn(), op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::OR_RAXId(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = RAX;
  op2_64 = (Bit32s) i->Id();
  op1_64 |= op2_64;

  RAX = op1_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::AND_EqGq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op2_64 = BX_READ_64BIT_REG(i->nnn());

  /* op1_64 is a register or memory reference */
  if (i->modC0()) {
    op1_64 = BX_READ_64BIT_REG(i->rm());
    op1_64 &= op2_64;
    BX_WRITE_64BIT_REG(i->rm(), op1_64);
  }
  else {
    /* pointer, segment address pair */
    read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
    op1_64 &= op2_64;
    write_RMW_virtual_qword(op1_64);
  }

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::AND_GqEq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->nnn());

  /* op2_64 is a register or memory reference */
  if (i->modC0()) {
    op2_64 = BX_READ_64BIT_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op2_64);
  }

  op1_64 &= op2_64;

  /* now write result back to destination */
  BX_WRITE_64BIT_REG(i->nnn(), op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::AND_RAXId(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = RAX;
  op2_64 = (Bit32s) i->Id();
  op1_64 &= op2_64;

  RAX = op1_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::AND_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  /* pointer, segment address pair */
  read_RMW_virtual_qword(i->seg(), RMAddr(i), &op1_64);
  op1_64 &= op2_64;
  write_RMW_virtual_qword(op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::AND_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64 = (Bit32s) i->Id();

  op1_64 = BX_READ_64BIT_REG(i->rm());
  op1_64 &= op2_64;
  BX_WRITE_64BIT_REG(i->rm(), op1_64);

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::TEST_EqGq(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op2_64 = BX_READ_64BIT_REG(i->nnn());

  /* op1_64 is a register or memory reference */
  if (i->modC0()) {
    op1_64 = BX_READ_64BIT_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &op1_64);
  }

  op1_64 &= op2_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::TEST_RAXId(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = RAX;
  op2_64 = (Bit32s) i->Id();
  op1_64 &= op2_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::TEST_EqIdM(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  /* pointer, segment address pair */
  read_virtual_qword(i->seg(), RMAddr(i), &op1_64);
  op2_64 = (Bit32s) i->Id();
  op1_64 &= op2_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

void BX_CPU_C::TEST_EqIdR(bxInstruction_c *i)
{
  Bit64u op1_64, op2_64;

  op1_64 = BX_READ_64BIT_REG(i->rm());
  op2_64 = (Bit32s) i->Id();
  op1_64 &= op2_64;

  SET_FLAGS_OSZAPC_RESULT_64(op1_64, BX_INSTR_LOGIC64);
}

#endif /* if BX_SUPPORT_X86_64 */
