/////////////////////////////////////////////////////////////////////////
// $Id: segment_ctrl_pro.cc,v 1.43 2005/08/01 21:40:17 sshwarts Exp $
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


  void BX_CPP_AttrRegparmN(2)
BX_CPU_C::load_seg_reg(bx_segment_reg_t *seg, Bit16u new_value)
{
#if BX_CPU_LEVEL >= 3
  if (v8086_mode()) {
    /* ??? don't need to set all these fields */
    seg->selector.value = new_value;
    seg->selector.rpl   = 3;
    seg->cache.valid    = 1;
    seg->cache.p        = 1;
    seg->cache.dpl      = 3;
    seg->cache.segment  = 1; /* regular segment */
    if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS]) {
      seg->cache.u.segment.executable = 1; /* code segment */
#if BX_SUPPORT_ICACHE
      BX_CPU_THIS_PTR iCache.fetchModeMask = createFetchModeMask(BX_CPU_THIS);
#endif
      invalidate_prefetch_q();
    }
    else
      seg->cache.u.segment.executable = 0; /* data segment */
    seg->cache.u.segment.c_ed = 0; /* expand up */
    seg->cache.u.segment.r_w = 1; /* writeable */
    seg->cache.u.segment.a = 1; /* accessed */
    seg->cache.u.segment.base = new_value << 4;
    seg->cache.u.segment.limit        = 0xffff;
    seg->cache.u.segment.limit_scaled = 0xffff;
    seg->cache.u.segment.g     = 0; /* byte granular */
    seg->cache.u.segment.d_b   = 0; /* default 16bit size */
#if BX_SUPPORT_X86_64
    seg->cache.u.segment.l     = 0; /* default 16bit size */
#endif
    seg->cache.u.segment.avl   = 0;

    return;
  }
#endif

#if BX_CPU_LEVEL >= 2
  if (protected_mode())
  {
    if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS])
    {
      if ((new_value & 0xfffc) == 0) { /* null selector */
#if BX_SUPPORT_X86_64
        // allow SS = 0 in 64 bit mode with cpl != 3
        if (BX_CPU_THIS_PTR msr.lma && CPL != 3) {
          seg->selector.index = 0;
          seg->selector.ti    = 0;
          seg->selector.rpl   = 0;
          seg->selector.value = 0;
          seg->cache.valid    = 0; /* invalidate null selector */
          return;
        }
#endif
        BX_ERROR(("load_seg_reg: SS: new_value == 0"));
        exception(BX_GP_EXCEPTION, 0, 0);
      }

      bx_descriptor_t descriptor;
      Bit32u dword1, dword2;

      Bit16u index = new_value >> 3;
      Bit8u ti = (new_value >> 2) & 0x01;
      Bit8u rpl = (new_value & 0x03);

      /* examine AR byte of destination selector for legal values: */

      if (ti == 0) { /* GDT */
        if ((index*8 + 7) > BX_CPU_THIS_PTR gdtr.limit) {
          BX_ERROR(("load_seg_reg: GDT: %s: index(%04x*8+7) > limit(%06x)",
            BX_CPU_THIS_PTR strseg(seg), (unsigned) index, (unsigned) BX_CPU_THIS_PTR gdtr.limit));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
        access_linear(BX_CPU_THIS_PTR gdtr.base + index*8,     4, 0,
          BX_READ, &dword1);
        access_linear(BX_CPU_THIS_PTR gdtr.base + index*8 + 4, 4, 0,
          BX_READ, &dword2);
      }
      else { /* LDT */
        if (BX_CPU_THIS_PTR ldtr.cache.valid==0) { /* ??? */
          BX_ERROR(("load_seg_reg: LDT invalid"));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
        if ((index*8 + 7) > BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit) {
          BX_ERROR(("load_seg_reg ss: LDT: index > limit"));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
        access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + index*8,     4, 0,
          BX_READ, &dword1);
        access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + index*8 + 4, 4, 0,
          BX_READ, &dword2);
      }

      /* selector's RPL must = CPL, else #GP(selector) */
      if (rpl != CPL) {
        BX_ERROR(("load_seg_reg(): rpl != CPL"));
        exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
      }

      parse_descriptor(dword1, dword2, &descriptor);

      if (descriptor.valid==0) {
        BX_ERROR(("load_seg_reg(): valid bit cleared"));
        exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* AR byte must indicate a writable data segment else #GP(selector) */
      if ( (descriptor.segment==0) ||
           descriptor.u.segment.executable ||
           descriptor.u.segment.r_w==0 )
      {
        BX_ERROR(("load_seg_reg(): not writable data segment"));
        exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* DPL in the AR byte must equal CPL else #GP(selector) */
      if (descriptor.dpl != CPL) {
        BX_ERROR(("load_seg_reg(): dpl != CPL"));
        exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* segment must be marked PRESENT else #SS(selector) */
      if (! IS_PRESENT(descriptor)) {
        BX_ERROR(("load_seg_reg(): not present"));
        exception(BX_SS_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* load SS with selector, load SS cache with descriptor */
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value = new_value;
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.index = index;
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.ti    = ti;
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl   = rpl;
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache          = descriptor;
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid    = 1;

      /* now set accessed bit in descriptor */
      dword2 |= 0x0100;
      if (ti == 0) { /* GDT */
        access_linear(BX_CPU_THIS_PTR gdtr.base + index*8 + 4, 4, 0,
          BX_WRITE, &dword2);
      }
      else { /* LDT */
        access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + index*8 + 4, 4, 0,
          BX_WRITE, &dword2);
      }

      return;
    }
    else if ( (seg==&BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS]) ||
              (seg==&BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES])
#if BX_CPU_LEVEL >= 3
           || (seg==&BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS]) ||
              (seg==&BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS])
#endif
            )
    {
      Bit16u index;
      Bit8u ti;
      Bit8u rpl;
      bx_descriptor_t descriptor;
      Bit32u dword1, dword2;

      if ((new_value & 0xfffc) == 0) { /* null selector */
        seg->selector.index = 0;
        seg->selector.ti = 0;
        seg->selector.rpl = 0;
        seg->selector.value = 0;
        seg->cache.valid = 0; /* invalidate null selector */
        return;
      }

      index = new_value >> 3;
      ti = (new_value >> 2) & 0x01;
      rpl = (new_value & 0x03);

      /* selector index must be within descriptor limits, else #GP(selector) */

      if (ti == 0) { /* GDT */
        if ((index*8 + 7) > BX_CPU_THIS_PTR gdtr.limit) {
          BX_ERROR(("load_seg_reg: GDT: %s: index(%04x) > limit(%06x)",
            BX_CPU_THIS_PTR strseg(seg), (unsigned) index, (unsigned) BX_CPU_THIS_PTR gdtr.limit));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
        access_linear(BX_CPU_THIS_PTR gdtr.base + index*8,     4, 0,
          BX_READ, &dword1);
        access_linear(BX_CPU_THIS_PTR gdtr.base + index*8 + 4, 4, 0,
          BX_READ, &dword2);
      }
      else { /* LDT */
        if (BX_CPU_THIS_PTR ldtr.cache.valid==0) {
          BX_ERROR(("load_seg_reg: LDT invalid"));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
        if ((index*8 + 7) > BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit) {
          BX_ERROR(("load_seg_reg ds,es: LDT: index > limit"));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
        access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + index*8,     4, 0,
          BX_READ, &dword1);
        access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + index*8 + 4, 4, 0,
          BX_READ, &dword2);
      }

      parse_descriptor(dword1, dword2, &descriptor);

      if (descriptor.valid==0) {
        BX_ERROR(("load_seg_reg(): valid bit cleared"));
        exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* AR byte must indicate data or readable code segment else #GP(selector) */
      if ( descriptor.segment==0 ||
           (descriptor.u.segment.executable==1 &&
            descriptor.u.segment.r_w==0) )
      {
        BX_ERROR(("load_seg_reg(): not data or readable code"));
        exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* If data or non-conforming code, then both the RPL and the CPL
       * must be less than or equal to DPL in AR byte else #GP(selector) */
      if ( descriptor.u.segment.executable==0 ||
           descriptor.u.segment.c_ed==0 )
      {
        if ((rpl > descriptor.dpl) || (CPL > descriptor.dpl)) {
          BX_ERROR(("load_seg_reg: RPL & CPL must be <= DPL"));
          exception(BX_GP_EXCEPTION, new_value & 0xfffc, 0);
        }
      }

      /* segment must be marked PRESENT else #NP(selector) */
      if (! IS_PRESENT(descriptor)) {
        BX_ERROR(("load_seg_reg: segment not present"));
        exception(BX_NP_EXCEPTION, new_value & 0xfffc, 0);
      }

      /* load segment register with selector */
      /* load segment register-cache with descriptor */
      seg->selector.value  = new_value;
      seg->selector.index  = index;
      seg->selector.ti     = ti;
      seg->selector.rpl    = rpl;
      seg->cache           = descriptor;
      seg->cache.valid     = 1;

      /* now set accessed bit in descriptor                   */
      /* wmr: don't bother if it's already set (thus allowing */ 
      /* GDT to be in read-only pages like real hdwe does)    */

      if (!(dword2 & 0x0100)) {
        dword2 |= 0x0100;
        if (ti == 0) { /* GDT */
          access_linear(BX_CPU_THIS_PTR gdtr.base + index*8 + 4, 4, 0,
            BX_WRITE, &dword2);
        }
        else { /* LDT */
         access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + index*8 + 4, 4, 0,
            BX_WRITE, &dword2);
        }
      }
      return;
    }
    else {
      BX_PANIC(("load_seg_reg(): invalid segment register passed!"));
      return;
    }
  }

  /* real mode */

  /* www.x86.org:
    According  to  Intel, each time any segment register is loaded in real
    mode,  the  base  address is calculated as 16 times the segment value,
    while  the  access  rights  and size limit attributes are given fixed,
    "real-mode  compatible" values. This is not true. In fact, only the CS
    descriptor  caches  for  the  286,  386, and 486 get loaded with fixed
    values  each  time  the segment register is loaded. Loading CS, or any
    other segment register in real mode, on later Intel processors doesn't
    change  the  access rights or the segment size limit attributes stored
    in  the  descriptor  cache  registers.  For these segments, the access
    rights and segment size limit attributes from any previous setting are
    honored. */

  /* ??? */
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS]) {
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value = new_value;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p = 1;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl = 0;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment = 1; /* regular segment */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.executable = 1; /* code segment */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.c_ed = 0; /* expand up */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.r_w = 1; /* writeable */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.a = 1; /* accessed */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base = new_value << 4;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit        = 0xffff;
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled = 0xffff;
#if BX_CPU_LEVEL >= 3
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g     = 0; /* byte granular */
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b   = 0; /* default 16bit size */
#if BX_SUPPORT_X86_64
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.l     = 0; /* default 16bit size */
#endif
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl   = 0;
#endif

#if BX_SUPPORT_ICACHE
    BX_CPU_THIS_PTR iCache.fetchModeMask = createFetchModeMask(BX_CPU_THIS);
#endif
    invalidate_prefetch_q();
  }
  else { /* SS, DS, ES, FS, GS */
    seg->selector.value = new_value;
    seg->cache.valid = 1;
    seg->cache.p = 1; // set this???
    seg->cache.u.segment.base = new_value << 4;
    seg->cache.segment = 1; /* regular segment */
    seg->cache.u.segment.a = 1; /* accessed */
    /* set G, D_B, AVL bits here ??? */
  }
#else /* 8086 */

  seg->selector.value = new_value;
  seg->cache.u.segment.base = new_value << 4;
#endif
}

#if BX_SUPPORT_X86_64
  void
BX_CPU_C::loadSRegLMNominal(unsigned segI, unsigned selector, bx_address base,
                            unsigned dpl)
{
  bx_segment_reg_t *seg = & BX_CPU_THIS_PTR sregs[segI];

  // Load a segment register in long-mode with nominal values,
  // so descriptor cache values are compatible with existing checks.
  seg->cache.u.segment.base = base;
  // I doubt we need limit_scaled.  If we do, it should be
  // of type bx_addr and be maxed to 64bits, not 32.
  seg->cache.u.segment.limit_scaled = 0xffffffff;
  seg->cache.valid = 1;
  seg->cache.dpl = dpl; // (KPL) Not sure if we need this.

  seg->selector.value = selector;
}
#endif


#if BX_CPU_LEVEL >= 2
  void BX_CPP_AttrRegparmN(2)
BX_CPU_C::parse_selector(Bit16u raw_selector, bx_selector_t *selector)
{
  selector->value  = raw_selector;
  selector->index  = raw_selector >> 3;
  selector->ti     = (raw_selector >> 2) & 0x01;
  selector->rpl    = raw_selector & 0x03;
}
#endif

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::parse_descriptor(Bit32u dword1, Bit32u dword2, bx_descriptor_t *temp)
{
  Bit8u AR_byte;

  AR_byte        = dword2 >> 8;
  temp->p        = (AR_byte >> 7) & 0x01;
  temp->dpl      = (AR_byte >> 5) & 0x03;
  temp->segment  = (AR_byte >> 4) & 0x01;
  temp->type     = (AR_byte & 0x0f);
  temp->valid    = 0; /* start out invalid */

  if (temp->segment) { /* data/code segment descriptors */
    temp->u.segment.executable = (AR_byte >> 3) & 0x01;
    temp->u.segment.c_ed       = (AR_byte >> 2) & 0x01;
    temp->u.segment.r_w        = (AR_byte >> 1) & 0x01;
    temp->u.segment.a          = (AR_byte >> 0) & 0x01;

    temp->u.segment.limit      = (dword1 & 0xffff);
    temp->u.segment.base       = (dword1 >> 16) | ((dword2 & 0xFF) << 16);

#if BX_CPU_LEVEL >= 3
    temp->u.segment.limit        |= (dword2 & 0x000F0000);
    temp->u.segment.g            =  (dword2 & 0x00800000) > 0;
    temp->u.segment.d_b          =  (dword2 & 0x00400000) > 0;
#if BX_SUPPORT_X86_64
    temp->u.segment.l            =  (dword2 & 0x00200000) > 0;
#endif
    temp->u.segment.avl          =  (dword2 & 0x00100000) > 0;
    temp->u.segment.base         |= (dword2 & 0xFF000000);

    if (temp->u.segment.g) {
      if ( (temp->u.segment.executable==0) && (temp->u.segment.c_ed) )
        temp->u.segment.limit_scaled = (temp->u.segment.limit << 12);
      else
        temp->u.segment.limit_scaled = (temp->u.segment.limit << 12) | 0x0fff;
      }
    else
#endif
      temp->u.segment.limit_scaled = temp->u.segment.limit;

    temp->valid    = 1;
  }
  else { // system & gate segment descriptors
    switch ( temp->type ) {
      case  0: // reserved
      case  8: // reserved
      case 10: // reserved
      case 13: // reserved
        temp->valid    = 0;
        break;
      case BX_SYS_SEGMENT_AVAIL_286_TSS:
      case BX_SYS_SEGMENT_BUSY_286_TSS:
        temp->u.tss286.base  = (dword1 >> 16) | ((dword2 & 0xff) << 16);
        temp->u.tss286.limit = (dword1 & 0xffff);
        temp->valid    = 1;
        break;
      case BX_SYS_SEGMENT_LDT:
        temp->u.ldt.base = (dword1 >> 16) | ((dword2 & 0xFF) << 16);
#if BX_CPU_LEVEL >= 3
        temp->u.ldt.base |= (dword2 & 0xff000000);
#endif
        temp->u.ldt.limit = (dword1 & 0xffff);
        temp->valid    = 1;
        break;
      case BX_286_CALL_GATE:
      case BX_286_INTERRUPT_GATE:
      case BX_286_TRAP_GATE:
        /* word count only used for call gate */
        temp->u.gate286.word_count = dword2 & 0x1f;
        temp->u.gate286.dest_selector = dword1 >> 16;
        temp->u.gate286.dest_offset   = dword1 & 0xffff;
        temp->valid = 1;
        break;
      case BX_TASK_GATE:
        temp->u.taskgate.tss_selector = dword1 >> 16;
        temp->valid = 1;
        break;
#if BX_CPU_LEVEL >= 3
      case BX_SYS_SEGMENT_AVAIL_386_TSS:
      case BX_SYS_SEGMENT_BUSY_386_TSS:
        temp->u.tss386.base  = (dword1 >> 16) |
                               ((dword2 & 0xff) << 16) |
                               (dword2 & 0xff000000);
        temp->u.tss386.limit = (dword1 & 0x0000ffff) |
                               (dword2 & 0x000f0000);
        temp->u.tss386.g     = (dword2 & 0x00800000) > 0;
        temp->u.tss386.avl   = (dword2 & 0x00100000) > 0;
        if (temp->u.tss386.g)
          temp->u.tss386.limit_scaled = (temp->u.tss386.limit << 12) | 0x0fff;
        else
          temp->u.tss386.limit_scaled = temp->u.tss386.limit;
        temp->valid = 1;
        break;

      case BX_386_CALL_GATE:
      case BX_386_INTERRUPT_GATE:
      case BX_386_TRAP_GATE:
        // word count only used for call gate
        temp->u.gate386.dword_count   = dword2 & 0x1f;
        temp->u.gate386.dest_selector = dword1 >> 16;
        temp->u.gate386.dest_offset   = (dword2 & 0xffff0000) |
                                        (dword1 & 0x0000ffff);
        temp->valid = 1;
        break;
#endif
      default: 
        BX_PANIC(("parse_descriptor(): case %u unfinished", (unsigned) temp->type));
        temp->valid = 0;
    }
  }
}

  void BX_CPP_AttrRegparmN(2)
BX_CPU_C::load_ldtr(bx_selector_t *selector, bx_descriptor_t *descriptor)
{
  /* check for null selector, if so invalidate LDTR */
  if ( (selector->value & 0xfffc)==0 ) {
    BX_CPU_THIS_PTR ldtr.selector = *selector;
    BX_CPU_THIS_PTR ldtr.cache.valid = 0;
    return;
  }

  if (!descriptor)
    BX_PANIC(("load_ldtr(): descriptor == NULL !"));

  BX_CPU_THIS_PTR ldtr.cache = *descriptor; /* whole structure copy */
  BX_CPU_THIS_PTR ldtr.selector = *selector;

  if (BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit < 7) {
    BX_PANIC(("load_ldtr(): ldtr.limit < 7"));
  }

  BX_CPU_THIS_PTR ldtr.cache.valid = 1;
}

/* pass zero in check_rpl if no needed selector RPL checking for 
   non-conforming segments */
  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::check_cs(bx_descriptor_t *descriptor, Bit16u cs_raw, Bit8u check_rpl)
{
#if BX_SUPPORT_X86_64
  if (descriptor->u.segment.l)
  {
    if (! BX_CPU_THIS_PTR msr.lma) {
      BX_PANIC(("check_cs: attempt to jump to long mode without enabling EFER.LMA !"));
    }

    if (descriptor->u.segment.d_b) {
      BX_ERROR(("check_cs: Both L and D bits enabled for segment descriptor !"));
      exception(BX_GP_EXCEPTION, cs_raw & 0xfffc, 0);
    }
  }
#endif

  // descriptor AR byte must indicate code segment else #GP(selector)
  if ((descriptor->valid==0) || (descriptor->segment==0) ||
      (descriptor->u.segment.executable==0))
  {
    BX_ERROR(("check_cs: not a valid code segment !"));
    exception(BX_GP_EXCEPTION, cs_raw & 0xfffc, 0);
  }

  // if non-conforming, code segment descriptor DPL must = CPL else #GP(selector)
  if (descriptor->u.segment.c_ed==0) {
    if (descriptor->dpl != CPL) {
      BX_ERROR(("check_cs: non-conforming code seg descriptor DPL != CPL"));
      exception(BX_GP_EXCEPTION, cs_raw & 0xfffc, 0);
    }

    /* RPL of destination selector must be <= CPL else #GP(selector) */
    if (check_rpl > CPL) {
      BX_ERROR(("check_cs: non-conforming code seg selector rpl > CPL"));
      exception(BX_GP_EXCEPTION, cs_raw & 0xfffc, 0);
    }
  }
  // if conforming, then code segment descriptor DPL must <= CPL else #GP(selector)
  else {
    if (descriptor->dpl > CPL) {
      BX_ERROR(("check_cs: conforming code seg descriptor DPL > CPL"));
      exception(BX_GP_EXCEPTION, cs_raw & 0xfffc, 0);
    }
  }

  // code segment must be present else #NP(selector)
  if (! descriptor->p) {
    BX_ERROR(("check_cs: code segment not present !"));
    exception(BX_NP_EXCEPTION, cs_raw & 0xfffc, 0);
  }
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::load_cs(bx_selector_t *selector, bx_descriptor_t *descriptor, Bit8u cpl)
{
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector = *selector;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache    = *descriptor;

  /* caller may request different CPL then in selector */
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl = cpl;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid  = 1;
  // Added cpl to the selector value.
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value =
    (0xfffc & BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value) | cpl;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR msr.lma) {
    if (descriptor->u.segment.l) {
      BX_CPU_THIS_PTR cpu_mode = BX_MODE_LONG_64;
      BX_DEBUG(("Long Mode Activated"));
      loadSRegLMNominal(BX_SEG_REG_CS, selector->value, 0, cpl);
    }
    else {
      BX_DEBUG(("Compatibility Mode Activated"));
      BX_CPU_THIS_PTR cpu_mode = BX_MODE_LONG_COMPAT;
    }
  }
#endif

#if BX_SUPPORT_ICACHE
  BX_CPU_THIS_PTR iCache.fetchModeMask = createFetchModeMask(BX_CPU_THIS);
#endif

  // Loading CS will invalidate the EIP fetch window.
  invalidate_prefetch_q();
}

#if BX_SUPPORT_X86_64
  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::load_ss64(Bit8u cpl)
{
  bx_selector_t selector;
  bx_descriptor_t descriptor;

  // set up a null descriptor
  parse_selector(0, &selector);
  parse_descriptor(0, 0, &descriptor);

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector = selector;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache = descriptor;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl = cpl;

  loadSRegLMNominal(BX_SEG_REG_SS, selector.value, 0, cpl);
}
#endif

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::load_ss(bx_selector_t *selector, bx_descriptor_t *descriptor, Bit8u cpl)
{
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector = *selector;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache = *descriptor;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl = cpl;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    loadSRegLMNominal(BX_SEG_REG_SS, selector->value, 0, cpl);
    return;
  }
#endif
  if ( (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value & 0xfffc) == 0 )
    BX_PANIC(("load_ss(): null selector passed"));

  if ( !BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid ) {
    BX_PANIC(("load_ss(): invalid selector/descriptor passed."));
  }
}

#if BX_CPU_LEVEL >= 2
  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::fetch_raw_descriptor(bx_selector_t *selector,
                        Bit32u *dword1, Bit32u *dword2, Bit8u exception_no)
{
  if (selector->ti == 0) { /* GDT */
    if ((selector->index*8 + 7) > BX_CPU_THIS_PTR gdtr.limit) {
      BX_ERROR(("fetch_raw_descriptor: GDT: index (%x)%x > limit (%x)",
          (selector->index*8 + 7), selector->index,
          BX_CPU_THIS_PTR gdtr.limit));
      exception(exception_no, selector->value & 0xfffc, 0);
    }
    access_linear(BX_CPU_THIS_PTR gdtr.base + selector->index*8,     4, 0,
      BX_READ, dword1);
    access_linear(BX_CPU_THIS_PTR gdtr.base + selector->index*8 + 4, 4, 0,
      BX_READ, dword2);
  }
  else { /* LDT */
    if (BX_CPU_THIS_PTR ldtr.cache.valid==0) {
      BX_PANIC(("fetch_raw_descriptor: LDTR.valid=0"));
      debug(BX_CPU_THIS_PTR prev_eip);
    }
    if ((selector->index*8 + 7) > BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit) {
      BX_ERROR(("fetch_raw_descriptor: LDT: index (%x)%x > limit (%x)",
          (selector->index*8 + 7), selector->index,
          BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit));
      exception(exception_no, selector->value & 0xfffc, 0);
    }
    access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + selector->index*8,     4, 0,
      BX_READ, dword1);
    access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + selector->index*8 + 4, 4, 0,
      BX_READ, dword2);
  }
}

#endif

  bx_bool BX_CPP_AttrRegparmN(3)
BX_CPU_C::fetch_raw_descriptor2(bx_selector_t *selector,
                        Bit32u *dword1, Bit32u *dword2)
{
  if (selector->ti == 0) { /* GDT */
    if ((selector->index*8 + 7) > BX_CPU_THIS_PTR gdtr.limit)
      return(0);
    access_linear(BX_CPU_THIS_PTR gdtr.base + selector->index*8,     4, 0,
      BX_READ, dword1);
    access_linear(BX_CPU_THIS_PTR gdtr.base + selector->index*8 + 4, 4, 0,
      BX_READ, dword2);
    return(1);
  }
  else { /* LDT */
    if (BX_CPU_THIS_PTR ldtr.cache.valid==0) {
      BX_PANIC(("fetch_raw_descriptor2: LDTR.valid=0"));
      return(0);
    }
    if ((selector->index*8 + 7) > BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit)
      return(0);
    access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + selector->index*8,     4, 0,
      BX_READ, dword1);
    access_linear(BX_CPU_THIS_PTR ldtr.cache.u.ldt.base + selector->index*8 + 4, 4, 0,
      BX_READ, dword2);
    return(1);
  }
}
