/////////////////////////////////////////////////////////////////////////
// $Id: protect_ctrl.cc,v 1.16.2.1 2002/10/20 22:26:02 zwane Exp $
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
BX_CPU_C::ARPL_EwGw(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("ARPL_EwRw: not supported on 8086!"));
#else /* 286+ */

  Bit16u op2_16, op1_16;


  if (protected_mode()) {
    /* op1_16 is a register or memory reference */
    if (i->modC0()) {
      op1_16 = BX_READ_16BIT_REG(i->rm());
      }
    else {
      /* pointer, segment address pair */
      read_RMW_virtual_word(i->seg(), RMAddr(i), &op1_16);
      }

    op2_16 = BX_READ_16BIT_REG(i->nnn());

    if ( (op1_16 & 0x03) < (op2_16 & 0x03) ) {
      op1_16 = (op1_16 & 0xfffc) | (op2_16 & 0x03);
      /* now write back to destination */
      if (i->modC0()) {
        if (i->os32L()) {
          // if 32bit opsize, then 0xff3f is or'd into
          // upper 16bits of register
          Bit32u op1_32;

          op1_32 = BX_READ_32BIT_REG(i->rm());
          op1_32 = (op1_32 & 0xffff0000) | op1_16;
          op1_32 |= 0xff3f0000;
          BX_WRITE_32BIT_REGZ(i->rm(), op1_32);
          }
        else {
          BX_WRITE_16BIT_REG(i->rm(), op1_16);
          }
        }
      else {
        Write_RMW_virtual_word(op1_16);
        }
      set_ZF(1);
      }
    else {
      set_ZF(0);
      }
    }
  else {
    // ARPL not recognized in real or v8086 mode
    UndefinedOpcode(i);
    return;
    }
#endif
}

  void
BX_CPU_C::LAR_GvEw(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  bx_descriptor_t descriptor;
  bx_selector_t   selector;
  Bit32u dword1, dword2;


  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  if (real_mode()) {
    BX_PANIC(("LAR_GvEw: not recognized in real mode"));
    UndefinedOpcode(i);
    return;
    }


  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->rm());
    }
  else {
    /* pointer, segment address pair */
    read_virtual_word(i->seg(), RMAddr(i), &raw_selector);
    }

  /* if selector null, clear ZF and done */
  if ( (raw_selector & 0xfffc) == 0 ) {
    set_ZF(0);
    return;
    }

  parse_selector(raw_selector, &selector);

  if ( !fetch_raw_descriptor2(&selector, &dword1, &dword2) ) {
    /* not within descriptor table */
    set_ZF(0);
    return;
    }

  parse_descriptor(dword1, dword2, &descriptor);

  if (descriptor.valid==0) {
    set_ZF(0);
    //BX_DEBUG(("lar(): descriptor valid bit cleared"));
    return;
    }

  /* if source selector is visible at CPL & RPL,
   * within the descriptor table, and of type accepted by LAR instruction,
   * then load register with segment limit and set ZF
   */

  if ( descriptor.segment ) { /* normal segment */
    if ( descriptor.u.segment.executable && descriptor.u.segment.c_ed ) {
      /* ignore DPL for conforming segments */
      }
    else {
      if ( (descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl) ) {
        set_ZF(0);
        return;
        }
      }
    set_ZF(1);
    if (i->os32L()) {
      /* masked by 00FxFF00, where x is undefined */
      BX_WRITE_32BIT_REGZ(i->nnn(), dword2 & 0x00ffff00);
      }
    else {
      BX_WRITE_16BIT_REG(i->nnn(), dword2 & 0xff00);
      }
    return;
    }
  else { /* system or gate segment */
    switch ( descriptor.type ) {
      case 1: /* available TSS */
      case 2: /* LDT */
      case 3: /* busy TSS */
      case 4: /* 286 call gate */
      case 5: /* task gate */
#if BX_CPU_LEVEL >= 3
      case 9:  /* available 32bit TSS */
      case 11: /* busy 32bit TSS */
      case 12: /* 32bit call gate */
#endif
        break;
      default: /* rest not accepted types to LAR */
        set_ZF(0);
        BX_DEBUG(("lar(): not accepted type"));
        return;
        break;
      }

    if ( (descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl) ) {
      set_ZF(0);
      return;
      }
    set_ZF(1);
    if (i->os32L()) {
      /* masked by 00FxFF00, where x is undefined ??? */
      BX_WRITE_32BIT_REGZ(i->nnn(), dword2 & 0x00ffff00);
      }
    else {
      BX_WRITE_16BIT_REG(i->nnn(), dword2 & 0xff00);
      }
    return;
    }
}

  void
BX_CPU_C::LSL_GvEw(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  Bit32u limit32;
  //bx_descriptor_t descriptor;
  bx_selector_t   selector;
  Bit32u dword1, dword2;
  Bit32u descriptor_dpl;

  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));


  if (real_mode()) {
    BX_PANIC(("LSL_GvEw: not recognized in real mode"));
    UndefinedOpcode(i);
    return;
    }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->rm());
    }
  else {
    /* pointer, segment address pair */
    read_virtual_word(i->seg(), RMAddr(i), &raw_selector);
    }


  /* if selector null, clear ZF and done */
  if ( (raw_selector & 0xfffc) == 0 ) {
    set_ZF(0);
    return;
    }

  parse_selector(raw_selector, &selector);

  if ( !fetch_raw_descriptor2(&selector, &dword1, &dword2) ) {
    /* not within descriptor table */
    set_ZF(0);
    return;
    }

  //parse_descriptor(dword1, dword2, &descriptor);

  descriptor_dpl = (dword2 >> 13) & 0x03;

  if ( (dword2 & 0x00001000) == 0 ) { // system segment
    Bit32u type;

    type = (dword2 >> 8) & 0x0000000f;
    switch (type) {
      case 1: // 16bit TSS
      case 3: // 16bit TSS
      case 2: // LDT
      case 9: // 32bit TSS    G00A
      case 11:// 32bit TSS    G00A
        limit32 = (dword1 & 0x0000ffff) | (dword2 & 0x000f0000);
        if ( dword2 & 0x00800000 )
          limit32 = (limit32 << 12) | 0x00000fff;
        if ( (descriptor_dpl<CPL) || (descriptor_dpl<selector.rpl) ) {
          set_ZF(0);
          return;
          }
        goto lsl_ok;
        break;
      default:
        set_ZF(0);
        return;
      }
    }
  else { // data & code segment
    limit32 = (dword1 & 0x0000ffff) | (dword2 & 0x000f0000);
    if ( dword2 & 0x00800000 )
      limit32 = (limit32 << 12) | 0x00000fff;
    if ( (dword2 & 0x00000c00) == 0x00000c00 ) {
      // conforming code segment, no check done
      goto lsl_ok;
      }

    if ( (descriptor_dpl<CPL) || (descriptor_dpl<selector.rpl) ) {
      set_ZF(0);
      return;
      }
    goto lsl_ok;
    }

lsl_ok:
  /* all checks pass, limit32 is now byte granular, write to op1 */
  set_ZF(1);

  if (i->os32L())
    BX_WRITE_32BIT_REGZ(i->nnn(), limit32)
  else
    // chop off upper 16 bits
    BX_WRITE_16BIT_REG(i->nnn(), (Bit16u) limit32)
}

  void
BX_CPU_C::SLDT_Ew(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("SLDT_Ew: not supported on 8086!"));
#else
  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  if (real_mode()) {
    /* not recognized in real address mode */
    BX_ERROR(("SLDT_Ew: encountered in real mode."));
    UndefinedOpcode(i);
    }
  else {
    Bit16u val16;

    val16 = BX_CPU_THIS_PTR ldtr.selector.value;
    if (i->modC0()) {
      BX_WRITE_16BIT_REG(i->rm(), val16);
      }
    else {
      write_virtual_word(i->seg(), RMAddr(i), &val16);
      }
    }
#endif
}

  void
BX_CPU_C::STR_Ew(bxInstruction_c *i)
{
  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  if (real_mode()) {
    // not recognized in real address mode
    BX_PANIC(("STR_Ew: encountered in real mode."));
    UndefinedOpcode(i);
    }
  else {
    Bit16u val16;

    val16 = BX_CPU_THIS_PTR tr.selector.value;
    if (i->modC0()) {
      BX_WRITE_16BIT_REG(i->rm(), val16);
      }
    else {
      write_virtual_word(i->seg(), RMAddr(i), &val16);
      }
    }
}

  void
BX_CPU_C::LLDT_Ew(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("LLDT_Ew: not supported on 8086!"));
#else
  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  invalidate_prefetch_q();

  if (real_mode()) {
    BX_PANIC(("lldt: not recognized in real mode"));
    UndefinedOpcode(i);
    return;
    }
  else { /* protected mode */
    bx_descriptor_t  descriptor;
    bx_selector_t    selector;
    Bit16u raw_selector;
    Bit32u dword1, dword2;


    /* #GP(0) if the current privilege level is not 0 */
    if (CPL != 0) {
      BX_PANIC(("LLDT: CPL != 0"));
      exception(BX_GP_EXCEPTION, 0, 0);
      return;
      }

    if (i->modC0()) {
      raw_selector = BX_READ_16BIT_REG(i->rm());
      }
    else {
      read_virtual_word(i->seg(), RMAddr(i), &raw_selector);
      }

    /* if selector is NULL, invalidate and done */
    if ((raw_selector & 0xfffc) == 0) {
      BX_CPU_THIS_PTR ldtr.selector.value = raw_selector;
      BX_CPU_THIS_PTR ldtr.cache.valid = 0;
      return;
      }

    /* parse fields in selector */
    parse_selector(raw_selector, &selector);

    // #GP(selector) if the selector operand does not point into GDT
    if (selector.ti != 0) {
      BX_ERROR(("LLDT: selector.ti != 0"));
      exception(BX_GP_EXCEPTION, raw_selector & 0xfffc, 0);
      }

    if ((selector.index*8 + 7) > BX_CPU_THIS_PTR gdtr.limit) {
      BX_PANIC(("lldt: GDT: index > limit"));
      exception(BX_GP_EXCEPTION, raw_selector & 0xfffc, 0);
      return;
      }

    access_linear(BX_CPU_THIS_PTR gdtr.base + selector.index*8,     4, 0,
      BX_READ, &dword1);
    access_linear(BX_CPU_THIS_PTR gdtr.base + selector.index*8 + 4, 4, 0,
      BX_READ, &dword2);

    parse_descriptor(dword1, dword2, &descriptor);

    /* if selector doesn't point to an LDT descriptor #GP(selector) */
    if ( (descriptor.valid==0) ||
        descriptor.segment  ||
        (descriptor.type!=2) ) {
      BX_ERROR(("lldt: doesn't point to an LDT descriptor!"));
      exception(BX_GP_EXCEPTION, raw_selector & 0xfffc, 0);
      }

    /* #NP(selector) if LDT descriptor is not present */
    if (descriptor.p==0) {
      BX_ERROR(("lldt: LDT descriptor not present!"));
      exception(BX_NP_EXCEPTION, raw_selector & 0xfffc, 0);
      }

    if (descriptor.u.ldt.limit < 7) {
      BX_ERROR(("lldt: ldtr.limit < 7"));
      }

    BX_CPU_THIS_PTR ldtr.selector = selector;
    BX_CPU_THIS_PTR ldtr.cache = descriptor;
    BX_CPU_THIS_PTR ldtr.cache.valid = 1;

    return;
    }
#endif
}

  void
BX_CPU_C::LTR_Ew(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("LTR_Ew: not supported on 8086!"));
#else
  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));


  invalidate_prefetch_q();

  if (protected_mode()) {
    bx_descriptor_t  descriptor;
    bx_selector_t    selector;
    Bit16u raw_selector;
    Bit32u dword1, dword2;
#if BX_SUPPORT_X86_64
    Bit32u dword3;
#endif


    /* #GP(0) if the current privilege level is not 0 */
    if (CPL != 0) {
      BX_PANIC(("LTR: CPL != 0"));
      exception(BX_GP_EXCEPTION, 0, 0);
      return;
      }

    if (i->modC0()) {
      raw_selector = BX_READ_16BIT_REG(i->rm());
      }
    else {
      read_virtual_word(i->seg(), RMAddr(i), &raw_selector);
      }

    /* if selector is NULL, invalidate and done */
    if ((raw_selector & 0xfffc) == 0) {
      BX_PANIC(("ltr: loading with NULL selector!"));
      /* if this is OK, then invalidate and load selector & descriptor cache */
      /* load here */
      BX_CPU_THIS_PTR tr.selector.value = raw_selector;
      BX_CPU_THIS_PTR tr.cache.valid = 0;
      return;
      }

    /* parse fields in selector, then check for null selector */
    parse_selector(raw_selector, &selector);

    if (selector.ti) {
      BX_PANIC(("ltr: selector.ti != 0"));
      return;
      }

    /* fetch 2 dwords of descriptor; call handles out of limits checks */
    fetch_raw_descriptor(&selector, &dword1, &dword2, BX_GP_EXCEPTION);

    parse_descriptor(dword1, dword2, &descriptor);

#if BX_SUPPORT_X86_64
    if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
      // set upper 32 bits of tss base
      access_linear(BX_CPU_THIS_PTR gdtr.base + selector.index*8 + 8, 4, 0,
        BX_READ, &dword3);
      descriptor.u.tss386.base |= ((Bit64u)dword3 << 32);
      }
#endif


    /* #GP(selector) if object is not a TSS or is already busy */
    if ( (descriptor.valid==0) || descriptor.segment  ||
         (descriptor.type!=1 && descriptor.type!=9) ) {
      BX_PANIC(("ltr: doesn't point to an available TSS descriptor!"));
      exception(BX_GP_EXCEPTION, raw_selector & 0xfffc, 0); /* 0 ??? */
      return;
      }

    /* #NP(selector) if TSS descriptor is not present */
    if (descriptor.p==0) {
      BX_PANIC(("ltr: LDT descriptor not present!"));
      exception(BX_NP_EXCEPTION, raw_selector & 0xfffc, 0); /* 0 ??? */
      return;
      }

    if (descriptor.type==1 && descriptor.u.tss286.limit<43) {
      BX_PANIC(("ltr:286TSS: loading tr.limit < 43"));
      }
    else if (descriptor.type==9 && descriptor.u.tss386.limit_scaled<103) {
      BX_PANIC(("ltr:386TSS: loading tr.limit < 103"));
      }

    BX_CPU_THIS_PTR tr.selector = selector;
    BX_CPU_THIS_PTR tr.cache    = descriptor;
    BX_CPU_THIS_PTR tr.cache.valid = 1;
    // tr.cache.type should not have busy bit, or it would not get 
    // through the conditions above.
    BX_ASSERT((BX_CPU_THIS_PTR tr.cache.type & 2) == 0);

    /* mark as busy */
    dword2 |= 0x00000200; /* set busy bit */
    access_linear(BX_CPU_THIS_PTR gdtr.base + selector.index*8 + 4, 4, 0,
      BX_WRITE, &dword2);

    return;
    }
  else {
    BX_PANIC(("ltr_ew: not recognized in real-mode!"));
    UndefinedOpcode(i);
    return;
    }
#endif
}

  void
BX_CPU_C::VERR_Ew(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  bx_descriptor_t descriptor;
  bx_selector_t   selector;
  Bit32u dword1, dword2;

  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));


  if (real_mode()) {
    BX_PANIC(("VERR_Ew: not recognized in real mode"));
    UndefinedOpcode(i);
    return;
    }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->rm());
    }
  else {
    /* pointer, segment address pair */
    read_virtual_word(i->seg(), RMAddr(i), &raw_selector);
    }

  /* if selector null, clear ZF and done */
  if ( (raw_selector & 0xfffc) == 0 ) {
    set_ZF(0);
    BX_ERROR(("VERR: null selector"));
    return;
    }

  /* if source selector is visible at CPL & RPL,
   * within the descriptor table, and of type accepted by VERR instruction,
   * then load register with segment limit and set ZF */
  parse_selector(raw_selector, &selector);

  if ( !fetch_raw_descriptor2(&selector, &dword1, &dword2) ) {
    /* not within descriptor table */
    set_ZF(0);
    BX_ERROR(("VERR: not in table"));
    return;
    }

  parse_descriptor(dword1, dword2, &descriptor);

  if ( descriptor.segment==0 ) { /* system or gate descriptor */
    set_ZF(0); /* inaccessible */
    BX_ERROR(("VERR: system descriptor"));
    return;
    }

  if ( descriptor.valid==0 ) {
    set_ZF(0);
    BX_INFO(("VERR: valid bit cleared"));
    return;
    }

  /* normal data/code segment */
  if ( descriptor.u.segment.executable ) { /* code segment */
    /* ignore DPL for readable conforming segments */
    if ( descriptor.u.segment.c_ed &&
         descriptor.u.segment.r_w) {
      set_ZF(1); /* accessible */
      BX_INFO(("VERR: conforming code, OK"));
      return;
      }
    if ( descriptor.u.segment.r_w==0 ) {
      set_ZF(0); /* inaccessible */
      BX_INFO(("VERR: code not readable"));
      return;
      }
    /* readable, non-conforming code segment */
    if ( (descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl) ) {
      set_ZF(0); /* inaccessible */
      BX_INFO(("VERR: non-coforming code not withing priv level"));
      return;
      }
    set_ZF(1); /* accessible */
    BX_INFO(("VERR: code seg readable"));
    return;
    }
  else { /* data segment */
    if ( (descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl) ) {
      set_ZF(0); /* not accessible */
      BX_INFO(("VERR: data seg not withing priv level"));
      return;
      }
    set_ZF(1); /* accessible */
    BX_DEBUG(("VERR: data segment OK"));
    return;
    }
}

  void
BX_CPU_C::VERW_Ew(bxInstruction_c *i)
{
  /* for 16 bit operand size mode */
  Bit16u raw_selector;
  bx_descriptor_t descriptor;
  bx_selector_t   selector;
  Bit32u dword1, dword2;

  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));


  if (real_mode()) {
    BX_PANIC(("VERW_Ew: not recognized in real mode"));
    UndefinedOpcode(i);
    return;
    }

  if (i->modC0()) {
    raw_selector = BX_READ_16BIT_REG(i->rm());
    }
  else {
    /* pointer, segment address pair */
    read_virtual_word(i->seg(), RMAddr(i), &raw_selector);
    }

  /* if selector null, clear ZF and done */
  if ( (raw_selector & 0xfffc) == 0 ) {
    set_ZF(0);
    BX_ERROR(("VERW: null selector"));
    return;
    }

  /* if source selector is visible at CPL & RPL,
   * within the descriptor table, and of type accepted by VERW instruction,
   * then load register with segment limit and set ZF */
  parse_selector(raw_selector, &selector);

  if ( !fetch_raw_descriptor2(&selector, &dword1, &dword2) ) {
    /* not within descriptor table */
    set_ZF(0);
    BX_ERROR(("VERW: not in table"));
    return;
    }

  parse_descriptor(dword1, dword2, &descriptor);

  /* rule out system segments & code segments */
  if ( descriptor.segment==0 || descriptor.u.segment.executable ) {
    set_ZF(0);
    BX_ERROR(("VERW: system seg or code"));
    return;
    }

  if ( descriptor.valid==0 ) {
    set_ZF(0);
    BX_INFO(("VERW: valid bit cleared"));
    return;
    }

  /* data segment */
  if ( descriptor.u.segment.r_w ) { /* writable */
    if ( (descriptor.dpl<CPL) || (descriptor.dpl<selector.rpl) ) {
      set_ZF(0); /* not accessible */
      BX_INFO(("VERW: writable data seg not within priv level"));
      return;
      }
    set_ZF(1); /* accessible */
    BX_DEBUG(("VERW: data seg writable"));
    return;
    }

  set_ZF(0); /* not accessible */
  BX_INFO(("VERW: data seg not writable"));
  return;
}

  void
BX_CPU_C::SGDT_Ms(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("SGDT_Ms: not supported on 8086!"));
#else
  Bit16u limit_16;
  Bit32u base_32;

  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));


  /* op1 is a register or memory reference */
  if (i->modC0()) {

    /* undefined opcode exception */
    BX_PANIC(("SGDT_Ms: use of register is undefined opcode."));
    UndefinedOpcode(i);
    return;
    }

  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    Bit64u base_64;

    limit_16 = BX_CPU_THIS_PTR gdtr.limit;
    base_64  = BX_CPU_THIS_PTR gdtr.base;

    write_virtual_word(i->seg(), RMAddr(i), &limit_16);

    write_virtual_qword(i->seg(), RMAddr(i)+2, &base_64);

    }
  else
    {
    limit_16 = BX_CPU_THIS_PTR gdtr.limit;
    base_32  = BX_CPU_THIS_PTR gdtr.base;
#if BX_CPU_LEVEL == 2
    base_32 |= 0xff000000; /* ??? */
#else /* 386+ */
    /* 32bit processors always write 32bits of base */
#endif
    write_virtual_word(i->seg(), RMAddr(i), &limit_16);

    write_virtual_dword(i->seg(), RMAddr(i)+2, &base_32);
    }

#endif
}

  void
BX_CPU_C::SIDT_Ms(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("SIDT_Ms: not supported on 8086!"));
#else
  Bit16u limit_16;
  Bit32u base_32;

  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  /* op1 is a register or memory reference */
  if (i->modC0()) {
    /* undefined opcode exception */
    BX_PANIC(("SIDT: use of register is undefined opcode."));
    UndefinedOpcode(i);
    return;
    }

  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    Bit64u base_64;

    limit_16 = BX_CPU_THIS_PTR idtr.limit;
    base_64  = BX_CPU_THIS_PTR idtr.base;

    write_virtual_word(i->seg(), RMAddr(i), &limit_16);

    write_virtual_qword(i->seg(), RMAddr(i)+2, &base_64);

    }
  else
    {
    limit_16 = BX_CPU_THIS_PTR idtr.limit;
    base_32  = BX_CPU_THIS_PTR idtr.base;

#if BX_CPU_LEVEL == 2
    base_32 |= 0xff000000;
#else /* 386+ */
    /* ??? regardless of operand size, all 32bits of base are stored */
#endif

    write_virtual_word(i->seg(), RMAddr(i), &limit_16);

    write_virtual_dword(i->seg(), RMAddr(i)+2, &base_32);
    }

#endif
}

  void
BX_CPU_C::LGDT_Ms(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("LGDT_Ms: not supported on 8086!"));
#else

  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  invalidate_prefetch_q();

  if (protected_mode() && (CPL!=0)) {
    BX_PANIC(("LGDT: protected mode: CPL!=0"));
    exception(BX_GP_EXCEPTION, 0, 0);
    return;
    }

  /* op1 is a register or memory reference */
  if (i->modC0()) {
    BX_PANIC(("LGDT generating exception 6"));
    UndefinedOpcode(i);
    return;
    }

#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    Bit16u limit_16;
    Bit64u base_64;

    read_virtual_word(i->seg(), RMAddr(i), &limit_16);

    read_virtual_qword(i->seg(), RMAddr(i) + 2, &base_64);

    BX_CPU_THIS_PTR gdtr.limit = limit_16;
    BX_CPU_THIS_PTR gdtr.base = base_64;
    }
  else
  if (i->os32L()) {
    Bit16u limit_16;
    Bit32u base0_31;

    read_virtual_word(i->seg(), RMAddr(i), &limit_16);

    read_virtual_dword(i->seg(), RMAddr(i) + 2, &base0_31);

    BX_CPU_THIS_PTR gdtr.limit = limit_16;
    BX_CPU_THIS_PTR gdtr.base = base0_31;
    }
  else
#endif
    {
    Bit16u limit_16, base0_15;
    Bit8u base16_23;

    read_virtual_word(i->seg(), RMAddr(i), &limit_16);

    read_virtual_word(i->seg(), RMAddr(i) + 2, &base0_15);

    read_virtual_byte(i->seg(), RMAddr(i) + 4, &base16_23);

    /* ignore high 8 bits */

    BX_CPU_THIS_PTR gdtr.limit = limit_16;
    BX_CPU_THIS_PTR gdtr.base = (base16_23 << 16) | base0_15;
    }
#endif
}

  void
BX_CPU_C::LIDT_Ms(bxInstruction_c *i)
{
#if BX_CPU_LEVEL < 2
  BX_PANIC(("LIDT_Ms: not supported on 8086!"));
#else
  Bit16u limit_16;
  Bit32u base_32;


  if (v8086_mode()) BX_PANIC(("protect_ctrl: v8086 mode unsupported"));

  invalidate_prefetch_q();

  if (protected_mode()) {
    if (CPL != 0) {
      BX_PANIC(("LIDT(): CPL(%u) != 0", (unsigned) CPL));
      exception(BX_GP_EXCEPTION, 0, 0);
      return;
      }
    }

  /* op1 is a register or memory reference */
  if (i->modC0()) {
    /* undefined opcode exception */
    BX_PANIC(("LIDT generating exception 6"));
    UndefinedOpcode(i);
    return;
    }

#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    Bit64u base_64;

    read_virtual_word(i->seg(), RMAddr(i), &limit_16);
    read_virtual_qword(i->seg(), RMAddr(i) + 2, &base_64);

    BX_CPU_THIS_PTR idtr.limit = limit_16;
    BX_CPU_THIS_PTR idtr.base = base_64;
    }
  else if (i->os32L()) {
    read_virtual_word(i->seg(), RMAddr(i), &limit_16);
    read_virtual_dword(i->seg(), RMAddr(i) + 2, &base_32);

    BX_CPU_THIS_PTR idtr.limit = limit_16;
    BX_CPU_THIS_PTR idtr.base = base_32;
    }
  else
#endif
    {
    read_virtual_word(i->seg(), RMAddr(i), &limit_16);
    read_virtual_dword(i->seg(), RMAddr(i) + 2, &base_32);

    BX_CPU_THIS_PTR idtr.limit = limit_16;
    BX_CPU_THIS_PTR idtr.base = base_32 & 0x00ffffff; /* ignore upper 8 bits */
    }
#endif
}
