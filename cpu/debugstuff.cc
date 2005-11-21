/////////////////////////////////////////////////////////////////////////
// $Id: debugstuff.cc,v 1.42 2005/11/21 22:29:02 sshwarts Exp $
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


void BX_CPU_C::debug(bx_address offset)
{
  static const char *cpu_mode_name[] = {
     "real mode",
     "v8086 mode",
     "protected mode",
     "compatibility mode",
     "long mode"
  };

  if (BX_CPU_THIS_PTR cpu_mode < 5)
    BX_INFO(("%s", cpu_mode_name[BX_CPU_THIS_PTR cpu_mode]));
  BX_INFO(("CS.d_b = %u bit",
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b ? 32 : 16));
  BX_INFO(("SS.d_b = %u bit",
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b ? 32 : 16));
#if BX_SUPPORT_X86_64
  BX_INFO(("EFER   = 0x%08x", get_EFER()));

  BX_INFO(("| RAX=%08x%08x  RBX=%08x%08x",
          (unsigned) (RAX >> 32), (unsigned) EAX,
          (unsigned) (RBX >> 32), (unsigned) EBX));
  BX_INFO(("| RCX=%08x%08x  RDX=%08x%08x",
          (unsigned) (RCX >> 32), (unsigned) ECX,
          (unsigned) (RDX >> 32), (unsigned) EDX));
  BX_INFO(("| RSP=%08x%08x  RBP=%08x%08x",
          (unsigned) (RSP >> 32), (unsigned) ESP,
          (unsigned) (RBP >> 32), (unsigned) EBP));
  BX_INFO(("| RSI=%08x%08x  RDI=%08x%08x",
          (unsigned) (RSI >> 32), (unsigned) ESI,
          (unsigned) (RDI >> 32), (unsigned) EDI));
#else
  BX_INFO(("| EAX=%08x  EBX=%08x  ECX=%08x  EDX=%08x",
          (unsigned) EAX, (unsigned) EBX, (unsigned) ECX, (unsigned) EDX));
  BX_INFO(("| ESP=%08x  EBP=%08x  ESI=%08x  EDI=%08x",
          (unsigned) ESP, (unsigned) EBP, (unsigned) ESI, (unsigned) EDI));
#endif
  BX_INFO(("| IOPL=%1u %s %s %s %s %s %s %s %s",
    BX_CPU_THIS_PTR get_IOPL (),
    BX_CPU_THIS_PTR get_OF()          ? "OF" : "of",
    BX_CPU_THIS_PTR get_DF()   ? "DF" : "df",
    BX_CPU_THIS_PTR get_IF()   ? "IF" : "if",
    BX_CPU_THIS_PTR get_SF()          ? "SF" : "sf",
    BX_CPU_THIS_PTR get_ZF()          ? "ZF" : "zf",
    BX_CPU_THIS_PTR get_AF()          ? "AF" : "af",
    BX_CPU_THIS_PTR get_PF()          ? "PF" : "pf",
    BX_CPU_THIS_PTR get_CF()          ? "CF" : "cf"));
  BX_INFO(("| SEG selector     base    limit G D"));
  BX_INFO(("| SEG sltr(index|ti|rpl)     base    limit G D"));
  BX_INFO(("|  CS:%04x( %04x| %01u|  %1u) %08x %08x %1u %1u",
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.index,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.ti,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b));
  BX_INFO(("|  DS:%04x( %04x| %01u|  %1u) %08x %08x %1u %1u",
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.value,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.index,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.ti,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.rpl,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.d_b));
  BX_INFO(("|  SS:%04x( %04x| %01u|  %1u) %08x %08x %1u %1u",
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.index,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.ti,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.base,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.g,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b));
  BX_INFO(("|  ES:%04x( %04x| %01u|  %1u) %08x %08x %1u %1u",
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.value,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.index,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.ti,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.rpl,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.base,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.g,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.d_b));
  BX_INFO(("|  FS:%04x( %04x| %01u|  %1u) %08x %08x %1u %1u",
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.value,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.index,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.ti,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.rpl,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.g,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.d_b));
  BX_INFO(("|  GS:%04x( %04x| %01u|  %1u) %08x %08x %1u %1u",
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.value,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.index,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.ti,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.rpl,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.g,
    (unsigned) BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.d_b));

#if BX_SUPPORT_X86_64
  BX_INFO(("| RIP=%08x%08x (%08x%08x)", 
    (unsigned) BX_CPU_THIS_PTR dword.rip_upper, (unsigned) EIP,
    (unsigned) (BX_CPU_THIS_PTR prev_eip >> 32), 
    (unsigned) (BX_CPU_THIS_PTR prev_eip & 0xffffffff)));
  BX_INFO(("| CR0=0x%08x CR1=0x%x CR2=0x%08x%08x",
    (unsigned) (BX_CPU_THIS_PTR cr0.val32), 0,
    (unsigned) (BX_CPU_THIS_PTR cr2 >> 32),
    (unsigned) (BX_CPU_THIS_PTR cr2 & 0xffffffff)));
  BX_INFO(("| CR3=0x%08x%08x CR4=0x%08x",
    (unsigned) (BX_CPU_THIS_PTR cr3 >> 32),
    (unsigned) (BX_CPU_THIS_PTR cr3 & 0xffffffff),
    BX_CPU_THIS_PTR cr4.getRegister()));
#else
  BX_INFO(("| EIP=%08x (%08x)", (unsigned) EIP,
    (unsigned) BX_CPU_THIS_PTR prev_eip));

#if BX_CPU_LEVEL >= 2 && BX_CPU_LEVEL < 4
  BX_INFO(("| CR0=0x%08x CR1=%x CR2=0x%08x CR3=0x%08x",
    BX_CPU_THIS_PTR cr0.val32, 0,
    BX_CPU_THIS_PTR cr2,
    BX_CPU_THIS_PTR cr3));
#elif BX_CPU_LEVEL >= 4
  BX_INFO(("| CR0=0x%08x CR1=%x CR2=0x%08x",
    BX_CPU_THIS_PTR cr0.val32, 0,
    BX_CPU_THIS_PTR cr2));
  BX_INFO(("| CR3=0x%08x CR4=0x%08x",
    BX_CPU_THIS_PTR cr3,
    BX_CPU_THIS_PTR cr4.getRegister()));
#endif

#endif // BX_SUPPORT_X86_64


#if 0
  /* (mch) Hack to display the area round EIP and prev_EIP */
  char buf[100];
  sprintf(buf, "%04x:%08x  ", BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, EIP);
  for (int i = 0; i < 8; i++) {
    Bit8u data;
    BX_CPU_THIS_PTR read_virtual_byte(BX_SEG_REG_CS, EIP + i, &data);
    sprintf(buf+strlen(buf), "%02x ", data);
  }
  BX_INFO((buf));

  sprintf(buf, "%04x:%08x  ", BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value, BX_CPU_THIS_PTR prev_eip);
  for (int i = 0; i < 8; i++) {
    Bit8u data;
    BX_CPU_THIS_PTR read_virtual_byte(BX_SEG_REG_CS, BX_CPU_THIS_PTR prev_eip + i, &data);
    sprintf(buf+strlen(buf), "%02x ", data);
  }
  BX_INFO((buf));
#endif


#if BX_DISASM
  bx_bool valid;
  Bit32u  phy_addr, Base;
  Bit8u   instr_buf[32];
  char    char_buf[256];
  unsigned isize;

  static disassembler bx_disassemble;

  if (BX_CPU_THIS_PTR protected_mode()) { // 16bit & 32bit protected mode
    Base=BX_CPU_THIS_PTR get_segment_base(BX_SEG_REG_CS);
  }
  else {
    Base=BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value<<4;
  }

  dbg_xlate_linear2phy(BX_CPU_THIS_PTR get_segment_base(BX_SEG_REG_CS) + offset,
                       &phy_addr, &valid);
  if (valid && BX_CPU_THIS_PTR mem!=NULL) {
    BX_CPU_THIS_PTR mem->dbg_fetch_mem(phy_addr, 16, instr_buf);
    isize = bx_disassemble.disasm(
        BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b,
        BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64,
        Base, EIP, instr_buf, char_buf);
#if BX_SUPPORT_X86_64
    if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) isize = 16;
#endif
    for (unsigned j=0; j<isize; j++)
      BX_INFO((">> %02x", (unsigned) instr_buf[j]));
    BX_INFO((">> : %s", char_buf));
  }
  else {
    BX_INFO(("(instruction unavailable) page not present"));
  }
#else
  UNUSED(offset);
#endif  // #if BX_DISASM
}


#if BX_DEBUGGER
Bit32u BX_CPU_C::dbg_get_reg(unsigned reg)
{
  Bit32u return_val32;

  switch (reg) {
    case BX_DBG_REG_EAX: return(EAX);
    case BX_DBG_REG_ECX: return(ECX);
    case BX_DBG_REG_EDX: return(EDX);
    case BX_DBG_REG_EBX: return(EBX);
    case BX_DBG_REG_ESP: return(ESP);
    case BX_DBG_REG_EBP: return(EBP);
    case BX_DBG_REG_ESI: return(ESI);
    case BX_DBG_REG_EDI: return(EDI);
    case BX_DBG_REG_EIP: return(EIP);
    case BX_DBG_REG_EFLAGS:
      return_val32 = dbg_get_eflags();
      return(return_val32);
    case BX_DBG_REG_CS: return(BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value);
    case BX_DBG_REG_SS: return(BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value);
    case BX_DBG_REG_DS: return(BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.value);
    case BX_DBG_REG_ES: return(BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.value);
    case BX_DBG_REG_FS: return(BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.value);
    case BX_DBG_REG_GS: return(BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.value);
    default:
      BX_PANIC(("get_reg: request for unknown register"));
      return(0);
  }
}

bx_bool BX_CPU_C::dbg_set_reg(unsigned reg, Bit32u val)
{
  // returns 1=OK, 0=can't change
  bx_segment_reg_t *seg;
  Bit32u current_sys_bits;

  switch (reg) {
    case BX_DBG_REG_EAX: EAX = val; return(1);
    case BX_DBG_REG_ECX: ECX = val; return(1);
    case BX_DBG_REG_EDX: EDX = val; return(1);
    case BX_DBG_REG_EBX: EBX = val; return(1);
    case BX_DBG_REG_ESP: ESP = val; return(1);
    case BX_DBG_REG_EBP: EBP = val; return(1);
    case BX_DBG_REG_ESI: ESI = val; return(1);
    case BX_DBG_REG_EDI: EDI = val; return(1);
    case BX_DBG_REG_EIP: EIP = val; return(1);
    case BX_DBG_REG_EFLAGS:
      BX_INFO(("dbg_set_reg: can not handle eflags yet."));
      if ( val & 0xffff0000 ) {
        BX_INFO(("dbg_set_reg: can not set upper 16 bits of eflags."));
        return(0);
      }
      // make sure none of the system bits are being changed
      current_sys_bits = ((BX_CPU_THIS_PTR getB_NT()) << 14) |
                         (BX_CPU_THIS_PTR get_IOPL () << 12) |
                         ((BX_CPU_THIS_PTR getB_TF()) << 8);
      if ( current_sys_bits != (val & 0x0000f100) ) {
        BX_INFO(("dbg_set_reg: can not modify NT, IOPL, or TF."));
        return(0);
      }
      BX_CPU_THIS_PTR set_CF(val & 0x01); val >>= 2;
      BX_CPU_THIS_PTR set_PF(val & 0x01); val >>= 2;
      BX_CPU_THIS_PTR set_AF(val & 0x01); val >>= 2;
      BX_CPU_THIS_PTR set_ZF(val & 0x01); val >>= 1;
      BX_CPU_THIS_PTR set_SF(val & 0x01); val >>= 2;
      BX_CPU_THIS_PTR set_IF (val & 0x01); val >>= 1;
      BX_CPU_THIS_PTR set_DF (val & 0x01); val >>= 1;
      BX_CPU_THIS_PTR set_OF(val & 0x01);
      if (BX_CPU_THIS_PTR get_IF ())
        BX_CPU_THIS_PTR async_event = 1;
      return(1);
    case BX_DBG_REG_CS:
      seg = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS];
      break;
    case BX_DBG_REG_SS:
      seg = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS];
      break;
    case BX_DBG_REG_DS:
      seg = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS];
      break;
    case BX_DBG_REG_ES:
      seg = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES];
      break;
    case BX_DBG_REG_FS:
      seg = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS];
      break;
    case BX_DBG_REG_GS:
      seg = &BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS];
      break;
    default:
      BX_PANIC(("dbg_set_reg: unrecognized register ID (%u)", reg));
      return(0);
  }

  if (BX_CPU_THIS_PTR real_mode()) {
    seg->selector.value = val;
    seg->cache.valid = 1;
    seg->cache.p = 1;
    seg->cache.dpl = 0;
    seg->cache.segment = 1; // regular segment
    if (reg == BX_DBG_REG_CS) {
      seg->cache.u.segment.executable = 1; // code segment
    }
    else {
      seg->cache.u.segment.executable = 0; // data segment
    }
    seg->cache.u.segment.c_ed = 0;       // expand up/non-conforming
    seg->cache.u.segment.r_w = 1;        // writeable
    seg->cache.u.segment.a = 1;          // accessed
    seg->cache.u.segment.base = val << 4;
    seg->cache.u.segment.limit        = 0xffff;
    seg->cache.u.segment.limit_scaled = 0xffff;
    seg->cache.u.segment.g     = 0;      // byte granular
    seg->cache.u.segment.d_b   = 0;      // default 16bit size
    seg->cache.u.segment.avl   = 0;
    return(1); // ok
  }

  return(0); // can't change when not in real mode
}

unsigned BX_CPU_C::dbg_query_pending(void)
{
  unsigned ret = 0;

  if ( BX_HRQ ) {  // DMA Hold Request
    ret |= BX_DBG_PENDING_DMA;
  }

  if ( BX_CPU_THIS_PTR INTR && BX_CPU_THIS_PTR get_IF () ) {
    ret |= BX_DBG_PENDING_IRQ;
  }

  return(ret);
}

Bit32u BX_CPU_C::dbg_get_eflags(void)
{
  return (BX_CPU_THIS_PTR read_eflags());
}

Bit32u BX_CPU_C::dbg_get_descriptor_l(bx_descriptor_t *d)
{
  Bit32u val;

  if (d->valid == 0) {
    return(0);
  }

  if (d->segment) {
    val = ((d->u.segment.base & 0xffff) << 16) |
          (d->u.segment.limit & 0xffff);
    return(val);
  }
  else {
    switch (d->type) {
      case 0: // Reserved (not yet defined)
        BX_ERROR(( "#get_descriptor_l(): type %d not finished", d->type ));
        return(0);

      case BX_SYS_SEGMENT_AVAIL_286_TSS:
        val = ((d->u.tss286.base & 0xffff) << 16) |
               (d->u.tss286.limit & 0xffff);
        return(val);

      case BX_SYS_SEGMENT_LDT:
        val = ((d->u.ldt.base & 0xffff) << 16) |
              d->u.ldt.limit;
        return(val);

      case BX_SYS_SEGMENT_AVAIL_386_TSS:
        val = ((d->u.tss386.base & 0xffff) << 16) |
               (d->u.tss386.limit & 0xffff);
        return(val);

      default:
        BX_ERROR(( "#get_descriptor_l(): type %d not finished", d->type ));
        return(0);
    }
  }
}

Bit32u BX_CPU_C::dbg_get_descriptor_h(bx_descriptor_t *d)
{
  Bit32u val;

  if (d->valid == 0) {
    return(0);
  }

  if (d->segment) {
    val = (d->u.segment.base & 0xff000000) |
          ((d->u.segment.base >> 16) & 0x000000ff) |
          (d->u.segment.executable << 11) |
          (d->u.segment.c_ed << 10) |
          (d->u.segment.r_w << 9) |
          (d->u.segment.a << 8) |
          (d->segment << 12) |
          (d->dpl << 13) |
          (d->p << 15) |
          (d->u.segment.limit & 0xf0000) |
          (d->u.segment.avl << 20) |
          (d->u.segment.d_b << 22) |
          (d->u.segment.g << 23);
    return(val);
  }
  else {
    switch (d->type) {
      case 0: // Reserved (not yet defined)
        BX_ERROR(( "#get_descriptor_h(): type %d not finished", d->type ));
        return(0);

      case BX_SYS_SEGMENT_AVAIL_286_TSS:
        val = ((d->u.tss286.base >> 16) & 0xff) |
              (d->type << 8) |
              (d->dpl << 13) |
              (d->p << 15);
        return(val);

      case BX_SYS_SEGMENT_LDT:
        val = ((d->u.ldt.base >> 16) & 0xff) |
              (d->type << 8) |
              (d->dpl << 13) |
              (d->p << 15) |
              (d->u.ldt.base & 0xff000000);
        return(val);

      case BX_SYS_SEGMENT_AVAIL_386_TSS:
        val = ((d->u.tss386.base >> 16) & 0xff) |
              (d->type << 8) |
              (d->dpl << 13) |
              (d->p << 15) |
              (d->u.tss386.limit & 0xf0000) |
              (d->u.tss386.avl << 20) |
              (d->u.tss386.g << 23) |
              (d->u.tss386.base & 0xff000000);
        return(val);

      default:
        BX_ERROR(( "#get_descriptor_h(): type %d not finished", d->type ));
        return(0);
    }
  }
}

bx_bool BX_CPU_C::dbg_get_sreg(bx_dbg_sreg_t *sreg, unsigned sreg_no)
{
  if (sreg_no > 5)
    return(0);
  sreg->sel   = BX_CPU_THIS_PTR sregs[sreg_no].selector.value;
  sreg->des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[sreg_no].cache);
  sreg->des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[sreg_no].cache);
  sreg->valid = BX_CPU_THIS_PTR sregs[sreg_no].cache.valid;
  return(1);
}

bx_bool BX_CPU_C::dbg_get_cpu(bx_dbg_cpu_t *cpu)
{
  cpu->eax = EAX;
  cpu->ebx = EBX;
  cpu->ecx = ECX;
  cpu->edx = EDX;

  cpu->ebp = EBP;
  cpu->esi = ESI;
  cpu->edi = EDI;
  cpu->esp = ESP;

  cpu->eflags = dbg_get_eflags();
  cpu->eip    = EIP;

  cpu->cs.sel   = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value;
  cpu->cs.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache);
  cpu->cs.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache);
  cpu->cs.valid = BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid;

  cpu->ss.sel   = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value;
  cpu->ss.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache);
  cpu->ss.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache);
  cpu->ss.valid = BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid;

  cpu->ds.sel   = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.value;
  cpu->ds.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache);
  cpu->ds.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache);
  cpu->ds.valid = BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.valid;

  cpu->es.sel   = BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.value;
  cpu->es.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache);
  cpu->es.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache);
  cpu->es.valid = BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.valid;

  cpu->fs.sel   = BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.value;
  cpu->fs.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache);
  cpu->fs.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache);
  cpu->fs.valid = BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.valid;

  cpu->gs.sel   = BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.value;
  cpu->gs.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache);
  cpu->gs.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache);
  cpu->gs.valid = BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.valid;

  cpu->ldtr.sel   = BX_CPU_THIS_PTR ldtr.selector.value;
  cpu->ldtr.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR ldtr.cache);
  cpu->ldtr.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR ldtr.cache);
  cpu->ldtr.valid = BX_CPU_THIS_PTR ldtr.cache.valid;

  cpu->tr.sel   = BX_CPU_THIS_PTR tr.selector.value;
  cpu->tr.des_l = dbg_get_descriptor_l(&BX_CPU_THIS_PTR tr.cache);
  cpu->tr.des_h = dbg_get_descriptor_h(&BX_CPU_THIS_PTR tr.cache);
  cpu->tr.valid = BX_CPU_THIS_PTR tr.cache.valid;

  cpu->gdtr.base  = BX_CPU_THIS_PTR gdtr.base;
  cpu->gdtr.limit = BX_CPU_THIS_PTR gdtr.limit;

  cpu->idtr.base  = BX_CPU_THIS_PTR idtr.base;
  cpu->idtr.limit = BX_CPU_THIS_PTR idtr.limit;

  cpu->dr0 = BX_CPU_THIS_PTR dr0;
  cpu->dr1 = BX_CPU_THIS_PTR dr1;
  cpu->dr2 = BX_CPU_THIS_PTR dr2;
  cpu->dr3 = BX_CPU_THIS_PTR dr3;
  cpu->dr6 = BX_CPU_THIS_PTR dr6;
  cpu->dr7 = BX_CPU_THIS_PTR dr7;

  cpu->tr3 = 0;
  cpu->tr4 = 0;
  cpu->tr5 = 0;
  cpu->tr6 = 0;
  cpu->tr7 = 0;

#if BX_CPU_LEVEL >= 2
  // cr0:32=pg,cd,nw,am,wp,ne,ts,em,mp,pe
  cpu->cr0 = BX_CPU_THIS_PTR cr0.val32;
  cpu->cr1 = 0;
  cpu->cr2 = BX_CPU_THIS_PTR cr2;
  cpu->cr3 = BX_CPU_THIS_PTR cr3;
#endif
#if BX_CPU_LEVEL >= 4
  cpu->cr4 = BX_CPU_THIS_PTR cr4.getRegister();
#endif

  cpu->inhibit_mask = BX_CPU_THIS_PTR inhibit_mask;

  return(1);
}

bx_bool BX_CPU_C::dbg_set_cpu(bx_dbg_cpu_t *cpu)
{
  // returns 1=OK, 0=Error
  Bit32u val;
  Bit32u type;

  // =================================================
  // Do checks first, before setting any CPU registers
  // =================================================

  // CS, SS, DS, ES, FS, GS descriptor checks
  if (!cpu->cs.valid) {
    BX_ERROR(( "Error: CS not valid" ));
    return(0); // error
  }
  if ( (cpu->cs.des_h & 0x1000) == 0 ) {
    BX_ERROR(( "Error: CS not application type" ));
    return(0); // error
  }
  if ( (cpu->cs.des_h & 0x0800) == 0 ) {
    BX_ERROR(( "Error: CS not executable" ));
    return(0); // error
  }

  if (!cpu->ss.valid) {
    BX_ERROR(( "Error: SS not valid" ));
    return(0); // error
  }
  if ( (cpu->ss.des_h & 0x1000) == 0 ) {
    BX_ERROR(( "Error: SS not application type" ));
    return(0); // error
  }

  if (cpu->ds.valid) {
    if ( (cpu->ds.des_h & 0x1000) == 0 ) {
      BX_ERROR(( "Error: DS not application type" ));
      return(0); // error
    }
  }

  if (cpu->es.valid) {
    if ( (cpu->es.des_h & 0x1000) == 0 ) {
      BX_ERROR(( "Error: ES not application type" ));
      return(0); // error
    }
  }

  if (cpu->fs.valid) {
    if ( (cpu->fs.des_h & 0x1000) == 0 ) {
      BX_ERROR(( "Error: FS not application type" ));
      return(0); // error
    }
  }

  if (cpu->gs.valid) {
    if ( (cpu->gs.des_h & 0x1000) == 0 ) {
      BX_ERROR(( "Error: GS not application type" ));
      return(0); // error
    }
  }

  if (cpu->ldtr.valid) {
    if ( cpu->ldtr.des_h & 0x1000 ) {
      BX_ERROR(( "Error: LDTR not system type" ));
      return(0); // error
    }
    if ( ((cpu->ldtr.des_h >> 8) & 0x0f) != 2 ) {
      BX_ERROR(( "Error: LDTR descriptor type not LDT" ));
      return(0); // error
    }
  }

  if (cpu->tr.valid) {
    if ( cpu->tr.des_h & 0x1000 ) {
      BX_ERROR(( "Error: TR not system type"));
      return(0); // error
    }
    type = (cpu->tr.des_h >> 8) & 0x0f;

    if ( (type != 1) && (type != 9) ) {
      BX_ERROR(( "Error: TR descriptor type not TSS" ));
      return(0); // error
    }
  }

  // =============
  // end of checks
  // =============

  EAX = cpu->eax;
  EBX = cpu->ebx;
  ECX = cpu->ecx;
  EDX = cpu->edx;
  EBP = cpu->ebp;
  ESI = cpu->esi;
  EDI = cpu->edi;
  ESP = cpu->esp;

  // eflags
  val = cpu->eflags;
  BX_CPU_THIS_PTR set_CF(val & 0x01); val >>= 2;
  BX_CPU_THIS_PTR set_PF(val & 0x01); val >>= 2;
  BX_CPU_THIS_PTR set_AF(val & 0x01); val >>= 2;
  BX_CPU_THIS_PTR set_ZF(val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_SF(val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_TF (val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_IF (val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_DF (val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_OF(val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_IOPL (val & 0x03); val >>= 2;
  BX_CPU_THIS_PTR set_NT (val & 0x01); val >>= 2;
  BX_CPU_THIS_PTR set_RF (val & 0x01); val >>= 1;
  BX_CPU_THIS_PTR set_VM (val & 0x01); val >>= 1;
#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR set_AC (val & 0x01); val >>= 1;
  //BX_CPU_THIS_PTR eflags.set_VIF (val & 0x01);
  val >>= 1;
  //BX_CPU_THIS_PTR eflags.set_VIP (val & 0x01);
  val >>= 1;
  BX_CPU_THIS_PTR set_ID (val & 0x01);
#endif

  EIP = cpu->eip;

  // CS:
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.value = cpu->cs.sel;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.index = cpu->cs.sel >> 3;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.ti    = (cpu->cs.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].selector.rpl   = cpu->cs.sel & 0x03;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.valid            = cpu->cs.valid;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.p                = (cpu->cs.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.dpl              = (cpu->cs.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.segment          = (cpu->cs.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.type             = (cpu->cs.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.executable = (cpu->cs.des_h >> 11) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.c_ed   = (cpu->cs.des_h >> 10) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.r_w    = (cpu->cs.des_h >> 9) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.a      = (cpu->cs.des_h >> 8) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base   = (cpu->cs.des_l >> 16);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base  |= (cpu->cs.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.base  |= (cpu->cs.des_h & 0xff000000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit  = (cpu->cs.des_l & 0xffff);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit |= (cpu->cs.des_h & 0x000f0000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g      = (cpu->cs.des_h >> 23) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.d_b    = (cpu->cs.des_h >> 22) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.avl    = (cpu->cs.des_h >> 20) & 0x01;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.g)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled =
      (BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit << 12) | 0x0fff;
  else
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit_scaled =
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS].cache.u.segment.limit;


  // SS:
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.value = cpu->ss.sel;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.index = cpu->ss.sel >> 3;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.ti    = (cpu->ss.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].selector.rpl   = cpu->ss.sel & 0x03;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid            = cpu->ss.valid;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.p                = (cpu->ss.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.dpl              = (cpu->ss.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.segment          = (cpu->ss.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.type             = (cpu->ss.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.executable = (cpu->ss.des_h >> 11) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.c_ed   = (cpu->ss.des_h >> 10) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.r_w    = (cpu->ss.des_h >> 9) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.a      = (cpu->ss.des_h >> 8) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.base   = (cpu->ss.des_l >> 16);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.base  |= (cpu->ss.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.base  |= (cpu->ss.des_h & 0xff000000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit  = (cpu->ss.des_l & 0xffff);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit |= (cpu->ss.des_h & 0x000f0000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.g      = (cpu->ss.des_h >> 23) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b    = (cpu->ss.des_h >> 22) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.avl    = (cpu->ss.des_h >> 20) & 0x01;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.g)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled =
      (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit << 12) | 0x0fff;
  else
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled =
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit;


  // DS:
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.value = cpu->ds.sel;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.index = cpu->ds.sel >> 3;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.ti    = (cpu->ds.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].selector.rpl   = cpu->ds.sel & 0x03;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.valid            = cpu->ds.valid;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.p                = (cpu->ds.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.dpl              = (cpu->ds.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.segment          = (cpu->ds.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.type             = (cpu->ds.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.executable = (cpu->ds.des_h >> 11) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.c_ed   = (cpu->ds.des_h >> 10) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.r_w    = (cpu->ds.des_h >> 9) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.a      = (cpu->ds.des_h >> 8) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base   = (cpu->ds.des_l >> 16);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base  |= (cpu->ds.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.base  |= (cpu->ds.des_h & 0xff000000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit  = (cpu->ds.des_l & 0xffff);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit |= (cpu->ds.des_h & 0x000f0000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g      = (cpu->ds.des_h >> 23) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.d_b    = (cpu->ds.des_h >> 22) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.avl    = (cpu->ds.des_h >> 20) & 0x01;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.g)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled =
      (BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit << 12) | 0x0fff;
  else
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit_scaled =
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS].cache.u.segment.limit;


  // ES:
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.value = cpu->es.sel;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.index = cpu->es.sel >> 3;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.ti    = (cpu->es.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].selector.rpl   = cpu->es.sel & 0x03;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.valid            = cpu->es.valid;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.p                = (cpu->es.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.dpl              = (cpu->es.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.segment          = (cpu->es.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.type             = (cpu->es.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.executable = (cpu->es.des_h >> 11) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.c_ed   = (cpu->es.des_h >> 10) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.r_w    = (cpu->es.des_h >> 9) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.a      = (cpu->es.des_h >> 8) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.base   = (cpu->es.des_l >> 16);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.base  |= (cpu->es.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.base  |= (cpu->es.des_h & 0xff000000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit  = (cpu->es.des_l & 0xffff);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit |= (cpu->es.des_h & 0x000f0000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.g      = (cpu->es.des_h >> 23) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.d_b    = (cpu->es.des_h >> 22) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.avl    = (cpu->es.des_h >> 20) & 0x01;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.g)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit_scaled =
      (BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit << 12) | 0x0fff;
  else
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit_scaled =
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES].cache.u.segment.limit;


  // FS:
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.value = cpu->fs.sel;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.index = cpu->fs.sel >> 3;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.ti    = (cpu->fs.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].selector.rpl   = cpu->fs.sel & 0x03;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.valid            = cpu->fs.valid;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.p                = (cpu->fs.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.dpl              = (cpu->fs.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.segment          = (cpu->fs.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.type             = (cpu->fs.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.executable = (cpu->fs.des_h >> 11) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.c_ed   = (cpu->fs.des_h >> 10) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.r_w    = (cpu->fs.des_h >> 9) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.a      = (cpu->fs.des_h >> 8) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base   = (cpu->fs.des_l >> 16);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base  |= (cpu->fs.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.base  |= (cpu->fs.des_h & 0xff000000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit  = (cpu->fs.des_l & 0xffff);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit |= (cpu->fs.des_h & 0x000f0000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.g      = (cpu->fs.des_h >> 23) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.d_b    = (cpu->fs.des_h >> 22) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.avl    = (cpu->fs.des_h >> 20) & 0x01;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.g)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit_scaled =
      (BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit << 12) | 0x0fff;
  else
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit_scaled =
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS].cache.u.segment.limit;


  // GS:
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.value = cpu->gs.sel;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.index = cpu->gs.sel >> 3;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.ti    = (cpu->gs.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].selector.rpl   = cpu->gs.sel & 0x03;

  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.valid            = cpu->gs.valid;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.p                = (cpu->gs.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.dpl              = (cpu->gs.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.segment          = (cpu->gs.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.type             = (cpu->gs.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.executable = (cpu->gs.des_h >> 11) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.c_ed   = (cpu->gs.des_h >> 10) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.r_w    = (cpu->gs.des_h >> 9) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.a      = (cpu->gs.des_h >> 8) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base   = (cpu->gs.des_l >> 16);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base  |= (cpu->gs.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.base  |= (cpu->gs.des_h & 0xff000000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit  = (cpu->gs.des_l & 0xffff);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit |= (cpu->gs.des_h & 0x000f0000);
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.g      = (cpu->gs.des_h >> 23) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.d_b    = (cpu->gs.des_h >> 22) & 0x01;
  BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.avl    = (cpu->gs.des_h >> 20) & 0x01;
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.g)
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit_scaled =
      (BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit << 12) | 0x0fff;
  else
    BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit_scaled =
      BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS].cache.u.segment.limit;

  // LDTR:
  BX_CPU_THIS_PTR ldtr.selector.value = cpu->ldtr.sel;
  BX_CPU_THIS_PTR ldtr.selector.index = cpu->ldtr.sel >> 3;
  BX_CPU_THIS_PTR ldtr.selector.ti    = (cpu->ldtr.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR ldtr.selector.rpl   = cpu->ldtr.sel & 0x03;

  BX_CPU_THIS_PTR ldtr.cache.valid            = cpu->ldtr.valid;
  BX_CPU_THIS_PTR ldtr.cache.p                = (cpu->ldtr.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR ldtr.cache.dpl              = (cpu->ldtr.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR ldtr.cache.segment          = (cpu->ldtr.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR ldtr.cache.type             = (cpu->ldtr.des_h >> 8) & 0x0f;
  BX_CPU_THIS_PTR ldtr.cache.u.ldt.base       = (cpu->ldtr.des_l >> 16);
  BX_CPU_THIS_PTR ldtr.cache.u.ldt.base      |= (cpu->ldtr.des_h & 0xff) << 16;
  BX_CPU_THIS_PTR ldtr.cache.u.ldt.base      |= (cpu->ldtr.des_h & 0xff000000);
  BX_CPU_THIS_PTR ldtr.cache.u.ldt.limit      = (cpu->ldtr.des_l & 0xffff);

  // TR
  type = (cpu->tr.des_h >> 8) & 0x0f;
  type &= ~2; // never allow busy bit in tr.cache.type
  BX_CPU_THIS_PTR tr.selector.value = cpu->tr.sel;
  BX_CPU_THIS_PTR tr.selector.index = cpu->tr.sel >> 3;
  BX_CPU_THIS_PTR tr.selector.ti    = (cpu->tr.sel >> 2) & 0x01;
  BX_CPU_THIS_PTR tr.selector.rpl   = cpu->tr.sel & 0x03;

  BX_CPU_THIS_PTR tr.cache.valid            = cpu->tr.valid;
  BX_CPU_THIS_PTR tr.cache.p                = (cpu->tr.des_h >> 15) & 0x01;
  BX_CPU_THIS_PTR tr.cache.dpl              = (cpu->tr.des_h >> 13) & 0x03;
  BX_CPU_THIS_PTR tr.cache.segment          = (cpu->tr.des_h >> 12) & 0x01;
  BX_CPU_THIS_PTR tr.cache.type             = type;
  if (type == 1) { // 286 TSS
    BX_CPU_THIS_PTR tr.cache.u.tss286.base   = (cpu->tr.des_l >> 16);
    BX_CPU_THIS_PTR tr.cache.u.tss286.base  |= (cpu->tr.des_h & 0xff) << 16;
    BX_CPU_THIS_PTR tr.cache.u.tss286.limit  = (cpu->tr.des_l & 0xffff);
    }
  else { // type == 9, 386 TSS
    BX_CPU_THIS_PTR tr.cache.u.tss386.base   = (cpu->tr.des_l >> 16);
    BX_CPU_THIS_PTR tr.cache.u.tss386.base  |= (cpu->tr.des_h & 0xff) << 16;
    BX_CPU_THIS_PTR tr.cache.u.tss386.base  |= (cpu->tr.des_h & 0xff000000);
    BX_CPU_THIS_PTR tr.cache.u.tss386.limit  = (cpu->tr.des_l & 0xffff);
    BX_CPU_THIS_PTR tr.cache.u.tss386.limit |= (cpu->tr.des_h & 0x000f0000);
    BX_CPU_THIS_PTR tr.cache.u.tss386.g      = (cpu->tr.des_h >> 23) & 0x01;
    BX_CPU_THIS_PTR tr.cache.u.tss386.avl    = (cpu->tr.des_h >> 20) & 0x01;
    }


  // gdtr
  BX_CPU_THIS_PTR gdtr.base  = cpu->gdtr.base;
  BX_CPU_THIS_PTR gdtr.limit = cpu->gdtr.limit;

  // idtr
  BX_CPU_THIS_PTR idtr.base  = cpu->idtr.base;
  BX_CPU_THIS_PTR idtr.limit = cpu->idtr.limit;


  BX_CPU_THIS_PTR dr0 = cpu->dr0;
  BX_CPU_THIS_PTR dr1 = cpu->dr1;
  BX_CPU_THIS_PTR dr2 = cpu->dr2;
  BX_CPU_THIS_PTR dr3 = cpu->dr3;
  BX_CPU_THIS_PTR dr6 = cpu->dr6;
  BX_CPU_THIS_PTR dr7 = cpu->dr7;

  // BX_CPU_THIS_PTR tr3 = cpu->tr3;
  // BX_CPU_THIS_PTR tr4 = cpu->tr4;
  // BX_CPU_THIS_PTR tr5 = cpu->tr5;
  // BX_CPU_THIS_PTR tr6 = cpu->tr6;
  // BX_CPU_THIS_PTR tr7 = cpu->tr7;


#if BX_CPU_LEVEL >= 2
  // cr0, cr1, cr2, cr3, cr4
  SetCR0(cpu->cr0);
  BX_CPU_THIS_PTR cr1 = cpu->cr1;
  BX_CPU_THIS_PTR cr2 = cpu->cr2;
  BX_CPU_THIS_PTR cr3 = cpu->cr3;
#endif
#if BX_CPU_LEVEL >= 4
  BX_CPU_THIS_PTR cr4.setRegister(cpu->cr4);
#endif

  BX_CPU_THIS_PTR inhibit_mask = cpu->inhibit_mask;

  //
  // flush cached items, prefetch, paging, etc
  //
  BX_CPU_THIS_PTR CR3_change(cpu->cr3);
  BX_CPU_THIS_PTR invalidate_prefetch_q();
  BX_CPU_THIS_PTR async_event = 1;

  return(1);
}

#if BX_SIM_ID == 0
#  define BX_DBG_NULL_CALLBACK bx_dbg_null_callback0
#else
#  define BX_DBG_NULL_CALLBACK bx_dbg_null_callback1
#endif
void BX_DBG_NULL_CALLBACK(unsigned val)
{
  // bochs uses the pc_system variables, so this function is
  // a stub for notification by the debugger, that a change
  // occurred.
  UNUSED(val);
}

  void
#if BX_SIM_ID == 0
bx_dbg_init_cpu_mem_env0(bx_dbg_callback_t *callback, int argc, char *argv[])
#else
bx_dbg_init_cpu_mem_env1(bx_dbg_callback_t *callback, int argc, char *argv[])
#endif
{
  UNUSED(argc);
  UNUSED(argv);

#if 0
#ifdef __GNUC__
#warning hardcoding BX_CPU_THIS_PTR mem[0] and cpu[0]
#endif
  callback->setphymem           = BX_MEM(0)->dbg_set_mem;
  callback->getphymem           = BX_MEM(0)->dbg_fetch_mem;
  callback->xlate_linear2phy    = BX_CPU(0)->dbg_xlate_linear2phy;
  callback->set_reg             = BX_CPU(0)->dbg_set_reg;
  callback->get_reg             = BX_CPU(0)->dbg_get_reg;
  callback->get_sreg            = BX_CPU(0)->dbg_get_sreg;
  callback->get_cpu             = BX_CPU(0)->dbg_get_cpu;
  callback->set_cpu             = BX_CPU(0)->dbg_set_cpu;
  callback->dirty_page_tbl_size = sizeof(BX_MEM(0)->dbg_dirty_pages);
  callback->dirty_page_tbl      = BX_MEM(0)->dbg_dirty_pages;
  callback->atexit              = BX_CPU(0)->atexit;
  callback->query_pending       = BX_CPU(0)->dbg_query_pending;
  callback->execute             = BX_CPU(0)->cpu_loop;
  callback->take_irq            = BX_CPU(0)->dbg_take_irq;
  callback->take_dma            = BX_CPU(0)->dbg_take_dma;
  callback->reset_cpu           = BX_CPU(0)->reset;
  callback->init_mem            = BX_MEM(0)->init_memory;
  callback->load_ROM            = BX_MEM(0)->load_ROM;
  callback->set_A20             = NULL;
  callback->set_NMI             = BX_DBG_NULL_CALLBACK;
  callback->set_RESET           = BX_DBG_NULL_CALLBACK;
  callback->set_INTR            = BX_CPU(0)->set_INTR;
  callback->force_interrupt     = BX_CPU(0)->dbg_force_interrupt;

#if BX_INSTRUMENTATION
  callback->instr_start         = bx_instr_start;
  callback->instr_stop          = bx_instr_stop;
  callback->instr_reset         = bx_instr_reset;
  callback->instr_print         = bx_instr_print;
#endif
#if BX_USE_LOADER
  callback->loader              = bx_dbg_loader;
#endif
  callback->crc32               = BX_MEM(0)->dbg_crc32;
#endif
}

#endif  // #if BX_DEBUGGER

void BX_CPU_C::atexit(void)
{
  debug(BX_CPU_THIS_PTR prev_eip);
}
