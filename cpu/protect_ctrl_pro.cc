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







#define BX_IN_CPU_METHOD 1
#include "bochs.h"




  void
BX_CPU_C::enter_protected_mode(void)
{
// bx_printf("processor switching into PROTECTED mode!!!\n");
// debug(BX_CPU_THIS_PTR prev_eip);
  if (v8086_mode()) bx_panic("protect_ctrl: v8086 mode unsupported\n");

  if (bx_dbg.reset)
    bx_printf("processor switching into PROTECTED mode!!!\n");

if ( BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl!=0 || BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl!=0 )
  bx_panic("enter_protected_mode: CS or SS rpl != 0\n");
}


  void
BX_CPU_C::enter_real_mode(void)
{
// ???
// bx_printf("processor switching into REAL mode!!!\n");
// debug(BX_CPU_THIS_PTR prev_eip);
  if (v8086_mode()) bx_panic("protect_ctrl: v8086 mode unsupported\n");

  if (bx_dbg.reset)
    bx_printf("processor switching into REAL mode!!!\n");

if ( BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl!=0 || BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl!=0 )
  bx_panic("enter_real_mode: CS or SS rpl != 0\n");
}
