/////////////////////////////////////////////////////////////////////////
// $Id: fpu_compare.cc,v 1.30 2010/11/11 15:48:56 sshwarts Exp $
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2003-2009 Stanislav Shwartsman
//          Written by Stanislav Shwartsman [sshwarts at sourceforge net]
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu/cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_FPU

extern float_status_t FPU_pre_exception_handling(Bit16u control_word);

#include "softfloatx80.h"

static int status_word_flags_fpu_compare(int float_relation)
{
  switch(float_relation) {
     case float_relation_unordered:
         return (FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

     case float_relation_greater:
         return (0);

     case float_relation_less:
         return (FPU_SW_C0);

     case float_relation_equal:
         return (FPU_SW_C3);
  }

  return (-1);        // should never get here
}

void BX_CPU_C::write_eflags_fpu_compare(int float_relation)
{
  switch(float_relation) {
   case float_relation_unordered:
      setEFlagsOSZAPC(EFlagsZFMask | EFlagsPFMask | EFlagsCFMask);
      break;

   case float_relation_greater:
      setEFlagsOSZAPC(0);
      break;

   case float_relation_less:
      setEFlagsOSZAPC(EFlagsCFMask);
      break;

   case float_relation_equal:
      setEFlagsOSZAPC(EFlagsZFMask);
      break;

   default:
      BX_PANIC(("write_eflags: unknown floating point compare relation"));
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FCOM_STi(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  int pop_stack = i->nnn() & 1;
  // handle special case of FSTP opcode @ 0xDE 0xD0..D7
  if (i->b1() == 0xde)
    pop_stack = 1;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
     FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare(BX_READ_FPU_REG(0), BX_READ_FPU_REG(i->rm()), status);
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
        BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FCOMI_ST0_STj(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  int pop_stack = i->b1() & 4;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setEFlagsOSZAPC(EFlagsZFMask | EFlagsPFMask | EFlagsCFMask);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare(BX_READ_FPU_REG(0), BX_READ_FPU_REG(i->rm()), status);
  BX_CPU_THIS_PTR write_eflags_fpu_compare(rc);

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FUCOMI_ST0_STj(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  int pop_stack = i->b1() & 4;

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setEFlagsOSZAPC(EFlagsZFMask | EFlagsPFMask | EFlagsCFMask);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare_quiet(BX_READ_FPU_REG(0), BX_READ_FPU_REG(i->rm()), status);
  BX_CPU_THIS_PTR write_eflags_fpu_compare(rc);

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FUCOM_STi(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  int pop_stack = i->nnn() & 1;

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare_quiet(BX_READ_FPU_REG(0), BX_READ_FPU_REG(i->rm()), status);
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FCOM_SINGLE_REAL(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);

  int pop_stack = i->nnn() & 1, rc;

  RMAddr(i) = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  float32 load_reg = read_virtual_dword(i->seg(), RMAddr(i));

  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  clear_C1();

  if (IS_TAG_EMPTY(0))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  floatx80 a = BX_READ_FPU_REG(0);

  if (floatx80_is_nan(a) || floatx80_is_unsupported(a) || float32_is_nan(load_reg)) {
    rc = float_relation_unordered;
    float_raise(status, float_flag_invalid);
  }
  else {
    rc = floatx80_compare(a, float32_to_floatx80(load_reg, status), status);
  }
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FCOM_DOUBLE_REAL(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);

  int pop_stack = i->nnn() & 1, rc;

  RMAddr(i) = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  float64 load_reg = read_virtual_qword(i->seg(), RMAddr(i));

  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  clear_C1();

  if (IS_TAG_EMPTY(0))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  floatx80 a = BX_READ_FPU_REG(0);

  if (floatx80_is_nan(a) || floatx80_is_unsupported(a) || float64_is_nan(load_reg)) {
    rc = float_relation_unordered;
    float_raise(status, float_flag_invalid);
  }
  else {
    rc = floatx80_compare(a, float64_to_floatx80(load_reg, status), status);
  }
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FICOM_WORD_INTEGER(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);

  int pop_stack = i->nnn() & 1;

  RMAddr(i) = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  Bit16s load_reg = (Bit16s) read_virtual_word(i->seg(), RMAddr(i));

  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  clear_C1();

  if (IS_TAG_EMPTY(0))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare(BX_READ_FPU_REG(0),
                      int32_to_floatx80((Bit32s)(load_reg)), status);
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FICOM_DWORD_INTEGER(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);

  int pop_stack = i->nnn() & 1;

  RMAddr(i) = BX_CPU_CALL_METHODR(i->ResolveModrm, (i));
  Bit32s load_reg = (Bit32s) read_virtual_dword(i->seg(), RMAddr(i));

  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  clear_C1();

  if (IS_TAG_EMPTY(0))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          if (pop_stack)
              BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare(BX_READ_FPU_REG(0), int32_to_floatx80(load_reg), status);
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     if (pop_stack)
         BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

/* DE D9 */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::FCOMPP(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  clear_C1();

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(1))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          BX_CPU_THIS_PTR the_i387.FPU_pop();
          BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare(BX_READ_FPU_REG(0), BX_READ_FPU_REG(1), status);
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     BX_CPU_THIS_PTR the_i387.FPU_pop();
     BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

/* DA E9 */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::FUCOMPP(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(1))
  {
      FPU_exception(FPU_EX_Stack_Underflow);
      setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);

      if(BX_CPU_THIS_PTR the_i387.is_IA_masked())
      {
          BX_CPU_THIS_PTR the_i387.FPU_pop();
          BX_CPU_THIS_PTR the_i387.FPU_pop();
      }
      return;
  }

  float_status_t status =
      FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

  int rc = floatx80_compare_quiet(BX_READ_FPU_REG(0), BX_READ_FPU_REG(1), status);
  setcc(status_word_flags_fpu_compare(rc));

  if (! FPU_exception(status.float_exception_flags)) {
     BX_CPU_THIS_PTR the_i387.FPU_pop();
     BX_CPU_THIS_PTR the_i387.FPU_pop();
  }
}

void BX_CPP_AttrRegparmN(1) BX_CPU_C::FCMOV_ST0_STj(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  if (IS_TAG_EMPTY(0) || IS_TAG_EMPTY(i->rm()))
  {
     FPU_stack_underflow(0);
     return;
  }

  floatx80 sti_reg = BX_READ_FPU_REG(i->rm());

  bx_bool condition = 0;
  switch(i->nnn() & 3)
  {
     case 0: condition = get_CF(); break;
     case 1: condition = get_ZF(); break;
     case 2: condition = get_CF() || get_ZF(); break;
     case 3: condition = get_PF(); break;
     default:
        BX_PANIC(("FCMOV_ST0_STj: default case"));
  }
  if (i->b1() & 1)
     condition = !condition;

  if (condition)
     BX_WRITE_FPU_REG(sti_reg, 0);
}

/* D9 E4 */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::FTST(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  clear_C1();

  if (IS_TAG_EMPTY(0)) {
     FPU_exception(FPU_EX_Stack_Underflow);
     setcc(FPU_SW_C0|FPU_SW_C2|FPU_SW_C3);
  }
  else {
     extern const floatx80 Const_Z;

     float_status_t status =
        FPU_pre_exception_handling(BX_CPU_THIS_PTR the_i387.get_control_word());

     int rc = floatx80_compare(BX_READ_FPU_REG(0), Const_Z, status);
     setcc(status_word_flags_fpu_compare(rc));
     FPU_exception(status.float_exception_flags);
  }
}

/* D9 E5 */
void BX_CPP_AttrRegparmN(1) BX_CPU_C::FXAM(bxInstruction_c *i)
{
  BX_CPU_THIS_PTR prepareFPU(i);
  BX_CPU_THIS_PTR FPU_update_last_instruction(i);

  floatx80 reg = BX_READ_FPU_REG(0);
  int sign = floatx80_sign(reg);

  /*
   * Examine the contents of the ST(0) register and sets the condition
   * code flags C0, C2 and C3 in the FPU status word to indicate the
   * class of value or number in the register.
   */

  if (IS_TAG_EMPTY(0))
  {
      setcc(FPU_SW_C3|FPU_SW_C1|FPU_SW_C0);
  }
  else
  {
      float_class_t aClass = floatx80_class(reg);

      switch(aClass)
      {
        case float_zero:
           setcc(FPU_SW_C3|FPU_SW_C1);
           break;

        case float_NaN:
           // unsupported handled as NaNs
           if (floatx80_is_unsupported(reg)) {
               setcc(FPU_SW_C1);
           } else {
               setcc(FPU_SW_C1|FPU_SW_C0);
           }
           break;

        case float_negative_inf:
        case float_positive_inf:
           setcc(FPU_SW_C2|FPU_SW_C1|FPU_SW_C0);
           break;

        case float_denormal:
           setcc(FPU_SW_C3|FPU_SW_C2|FPU_SW_C1);
           break;

        case float_normalized:
           setcc(FPU_SW_C2|FPU_SW_C1);
           break;
      }
  }

  /*
   * The C1 flag is set to the sign of the value in ST(0), regardless
   * of whether the register is empty or full.
   */
  if (! sign)
    clear_C1();
}

#endif
