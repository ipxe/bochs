/////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2004  MandrakeSoft S.A.
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
#define LOG_THIS BX_CPU_THIS_PTR


/* D9 D0 */
void BX_CPU_C::FNOP(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();
  BX_CPU_THIS_PTR FPU_check_pending_exceptions();

  // Perform no FPU operation. This instruction takes up space in the
  // instruction stream but does not affect the FPU or machine
  // context, except the EIP register.
#else
  BX_INFO(("FNOP: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FXCH_STi(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FXCH_STi: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FCHS(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FCHS: required FPU, configure --enable-fpu"));
#endif
}

void BX_CPU_C::FABS(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  fpu_execute(i);
#else
  BX_INFO(("FABS: required FPU, configure --enable-fpu"));
#endif
}

/* D9 F6 */
void BX_CPU_C::FDECSTP(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  if (BX_CPU_THIS_PTR the_i387.tos == 0)
  {
    BX_CPU_THIS_PTR the_i387.tos = 7;
  }
  else
  {
    BX_CPU_THIS_PTR the_i387.tos --;
  }

#else
  BX_INFO(("FDECSTP: required FPU, configure --enable-fpu"));
#endif
}

/* D9 F7 */
void BX_CPU_C::FINCSTP(bxInstruction_c *i)
{
#if BX_SUPPORT_FPU
  BX_CPU_THIS_PTR prepareFPU();

  clear_C1();

  BX_CPU_THIS_PTR the_i387.tos++;
  BX_CPU_THIS_PTR the_i387.tos &= 0x07;
#else
  BX_INFO(("FINCSTP: required FPU, configure --enable-fpu"));
#endif
}
