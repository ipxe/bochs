/////////////////////////////////////////////////////////////////////////
// $Id: access.cc,v 1.105 2008/05/07 16:45:07 sshwarts Exp $
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
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

// The macro was made in order to optimize access alignment into TLB lookup - 
// when aligment check is enabled a misaligned access will miss the TLB.
// BX_CPU_THIS_PTR alignment_check_mask must be initialized to all'ones if
// alignment check exception is enabled and LPF_MASK if not.
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
#define AlignedAccessLPFOf(laddr, alignment_mask) \
          ((laddr) & (LPF_MASK | (alignment_mask))) & (BX_CPU_THIS_PTR alignment_check_mask)
#else
#define AlignedAccessLPFOf(laddr, alignment_mask) LPFOf(laddr)
#endif

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_checks(bx_segment_reg_t *seg, bx_address offset, unsigned length)
{
  Bit32u upper_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    // do canonical checks
    if (!IsCanonical(offset)) {
      BX_ERROR(("write_virtual_checks(): canonical failure 0x%08x:%08x", GET32H(offset), GET32L(offset)));
      exception(int_number(seg), 0, 0);
    }
    // Mark cache as being OK type for succeeding reads/writes
    seg->cache.valid |= SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    return;
  }
#endif
  if (protected_mode()) {
    if (seg->cache.valid==0) {
      BX_DEBUG(("write_virtual_checks(): segment descriptor not valid"));
      exception(int_number(seg), 0, 0);
    }

    if (seg->cache.p == 0) { /* not present */
      BX_ERROR(("write_virtual_checks(): segment not present"));
      exception(int_number(seg), 0, 0);
    }

    switch (seg->cache.type) {
      case 0: case 1:   // read only
      case 4: case 5:   // read only, expand down
      case 8: case 9:   // execute only
      case 10: case 11: // execute/read
      case 12: case 13: // execute only, conforming
      case 14: case 15: // execute/read-only, conforming
        BX_ERROR(("write_virtual_checks(): no write access to seg"));
        exception(int_number(seg), 0, 0);

      case 2: case 3: /* read/write */
        if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
            || (length-1 > seg->cache.u.segment.limit_scaled))
        {
          BX_ERROR(("write_virtual_checks(): write beyond limit, r/w"));
          exception(int_number(seg), 0, 0);
        }
        if (seg->cache.u.segment.limit_scaled >= 7) {
          // Mark cache as being OK type for succeeding read/writes. The limit
          // checks still needs to be done though, but is more simple. We
          // could probably also optimize that out with a flag for the case
          // when limit is the maximum 32bit value. Limit should accomodate
          // at least a dword, since we subtract from it in the simple
          // limit check in other functions, and we don't want the value to roll.
          // Only normal segments (not expand down) are handled this way.
          seg->cache.valid |= SegAccessROK | SegAccessWOK;

          if (seg->cache.u.segment.limit_scaled == 0xffffffff)
            seg->cache.valid |= SegAccessROK4G | SegAccessWOK4G;
        }
        break;

      case 6: case 7: /* read/write, expand down */
        if (seg->cache.u.segment.d_b)
          upper_limit = 0xffffffff;
        else
          upper_limit = 0x0000ffff;
        if ((offset <= seg->cache.u.segment.limit_scaled) ||
             (offset > upper_limit) || ((upper_limit - offset) < (length - 1)))
        {
          BX_ERROR(("write_virtual_checks(): write beyond limit, r/w ED"));
          exception(int_number(seg), 0, 0);
        }
        break;
    }

    return;
  }
  else { /* real mode */
    if (seg->cache.valid==0) {
      BX_DEBUG(("write_virtual_checks(): segment descriptor not valid (real mode)"));
      exception(int_number(seg), 0, 0);
    }

    if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
          || (length-1 > seg->cache.u.segment.limit_scaled))
    {
      BX_DEBUG(("write_virtual_checks(): write beyond limit (real mode)"));
      exception(int_number(seg), 0, 0);
    }
    if (seg->cache.u.segment.limit_scaled >= 7) {
      // Mark cache as being OK type for succeeding reads/writes.
      // See notes above.
      seg->cache.valid |= SegAccessROK | SegAccessWOK;

      if (seg->cache.u.segment.limit_scaled == 0xffffffff)
        seg->cache.valid |= SegAccessROK4G | SegAccessWOK4G;
    }
  }
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_virtual_checks(bx_segment_reg_t *seg, bx_address offset, unsigned length)
{
  Bit32u upper_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    // do canonical checks
    if (!IsCanonical(offset)) {
      BX_ERROR(("read_virtual_checks(): canonical failure 0x%08x:%08x", GET32H(offset), GET32L(offset)));
      exception(int_number(seg), 0, 0);
    }
    // Mark cache as being OK type for succeeding reads/writes
    seg->cache.valid |= SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    return;
  }
#endif
  if (protected_mode()) {
    if (seg->cache.valid==0) {
      BX_DEBUG(("read_virtual_checks(): segment descriptor not valid"));
      exception(int_number(seg), 0, 0);
    }

    if (seg->cache.p == 0) { /* not present */
      BX_ERROR(("read_virtual_checks(): segment not present"));
      exception(int_number(seg), 0, 0);
    }

    switch (seg->cache.type) {
      case 0: case 1: /* read only */
      case 2: case 3: /* read/write */
      case 10: case 11: /* execute/read */
      case 14: case 15: /* execute/read-only, conforming */
        if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
            || (length-1 > seg->cache.u.segment.limit_scaled))
        {
          BX_ERROR(("read_virtual_checks(): read beyond limit"));
          exception(int_number(seg), 0, 0);
        }
        if (seg->cache.u.segment.limit_scaled >= 7) {
          // Mark cache as being OK type for succeeding reads. See notes for
          // write checks; similar code.
          seg->cache.valid |= SegAccessROK;
          if (seg->cache.u.segment.limit_scaled == 0xffffffff)
            seg->cache.valid |= SegAccessROK4G;
        }
        break;

      case 4: case 5: /* read only, expand down */
      case 6: case 7: /* read/write, expand down */
        if (seg->cache.u.segment.d_b)
          upper_limit = 0xffffffff;
        else
          upper_limit = 0x0000ffff;
        if ((offset <= seg->cache.u.segment.limit_scaled) ||
             (offset > upper_limit) || ((upper_limit - offset) < (length - 1)))
        {
          BX_ERROR(("read_virtual_checks(): read beyond limit ED"));
          exception(int_number(seg), 0, 0);
        }
        break;

      case 8: case 9: /* execute only */
      case 12: case 13: /* execute only, conforming */
        /* can't read or write an execute-only segment */
        BX_ERROR(("read_virtual_checks(): execute only"));
        exception(int_number(seg), 0, 0);
    }
    return;
  }
  else { /* real mode */
    if (seg->cache.valid==0) {
      BX_DEBUG(("read_virtual_checks(): segment descriptor not valid (real mode)"));
      exception(int_number(seg), 0, 0);
    }

    if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
        || (length-1 > seg->cache.u.segment.limit_scaled))
    {
      BX_DEBUG(("read_virtual_checks(): read beyond limit (real mode)"));
      exception(int_number(seg), 0, 0);
    }
    if (seg->cache.u.segment.limit_scaled >= 7) {
      // Mark cache as being OK type for succeeding reads/writes. See notes for
      // write checks; similar code.
      seg->cache.valid |= SegAccessROK | SegAccessWOK;

      if (seg->cache.u.segment.limit_scaled == 0xffffffff)
        seg->cache.valid |= SegAccessROK4G | SegAccessWOK4G;
    }
  }
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::execute_virtual_checks(bx_segment_reg_t *seg, bx_address offset, unsigned length)
{
  Bit32u upper_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    // do canonical checks
    if (!IsCanonical(offset)) {
      BX_ERROR(("execute_virtual_checks(): canonical failure 0x%08x:%08x", GET32H(offset), GET32L(offset)));
      exception(int_number(seg), 0, 0);
    }
    // Mark cache as being OK type for succeeding reads/writes
    seg->cache.valid |= SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    return;
  }
#endif
  if (protected_mode()) {
    if (seg->cache.valid==0) {
      BX_DEBUG(("execute_virtual_checks(): segment descriptor not valid"));
      exception(int_number(seg), 0, 0);
    }

    if (seg->cache.p == 0) { /* not present */
      BX_ERROR(("execute_virtual_checks(): segment not present"));
      exception(int_number(seg), 0, 0);
    }

    switch (seg->cache.type) {
      case 0: case 1: /* read only */
      case 2: case 3: /* read/write */
      case 10: case 11: /* execute/read */
      case 14: case 15: /* execute/read-only, conforming */
        if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
            || (length-1 > seg->cache.u.segment.limit_scaled))
        {
          BX_ERROR(("execute_virtual_checks(): read beyond limit"));
          exception(int_number(seg), 0, 0);
        }
        if (seg->cache.u.segment.limit_scaled >= 7) {
          // Mark cache as being OK type for succeeding reads. See notes for
          // write checks; similar code.
          seg->cache.valid |= SegAccessROK;
          if (seg->cache.u.segment.limit_scaled == 0xffffffff)
            seg->cache.valid |= SegAccessROK4G;
        }
        break;

      case 8: case 9: /* execute only */
      case 12: case 13: /* execute only, conforming */
        if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
            || (length-1 > seg->cache.u.segment.limit_scaled))
        {
          BX_ERROR(("execute_virtual_checks(): read beyond limit execute only"));
          exception(int_number(seg), 0, 0);
        }
        break;

      case 4: case 5: /* read only, expand down */
      case 6: case 7: /* read/write, expand down */
        if (seg->cache.u.segment.d_b)
          upper_limit = 0xffffffff;
        else
          upper_limit = 0x0000ffff;
        if ((offset <= seg->cache.u.segment.limit_scaled) ||
             (offset > upper_limit) || ((upper_limit - offset) < (length - 1)))
        {
          BX_ERROR(("execute_virtual_checks(): read beyond limit ED"));
          exception(int_number(seg), 0, 0);
        }
        break;
    }
    return;
  }
  else { /* real mode */
    if (seg->cache.valid==0) {
      BX_DEBUG(("execute_virtual_checks(): segment descriptor not valid (real mode)"));
      exception(int_number(seg), 0, 0);
    }

    if (offset > (seg->cache.u.segment.limit_scaled - length + 1)
        || (length-1 > seg->cache.u.segment.limit_scaled))
    {
      BX_DEBUG(("execute_virtual_checks(): read beyond limit (real mode)"));
      exception(int_number(seg), 0, 0);
    }
    if (seg->cache.u.segment.limit_scaled >= 7) {
      // Mark cache as being OK type for succeeding reads/writes. See notes for
      // write checks; similar code.
      seg->cache.valid |= SegAccessROK | SegAccessWOK;

      if (seg->cache.u.segment.limit_scaled == 0xffffffff)
        seg->cache.valid |= SegAccessROK4G | SegAccessWOK4G;
    }
  }
}

const char *BX_CPU_C::strseg(bx_segment_reg_t *seg)
{
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES]) return("ES");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS]) return("CS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS]) return("SS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS]) return("DS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS]) return("FS");
  else if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS]) return("GS");
  else {
    BX_PANIC(("undefined segment passed to strseg()!"));
    return("??");
  }
}

int BX_CPU_C::int_number(bx_segment_reg_t *seg)
{
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_ES]) return BX_GP_EXCEPTION;
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_CS]) return BX_GP_EXCEPTION;
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS]) return BX_SS_EXCEPTION;
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_DS]) return BX_GP_EXCEPTION;
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_FS]) return BX_GP_EXCEPTION;
  if (seg == &BX_CPU_THIS_PTR sregs[BX_SEG_REG_GS]) return BX_GP_EXCEPTION;

  // undefined segment, this must be a new stack segment
  return BX_SS_EXCEPTION;
}

int BX_CPU_C::int_number(unsigned s)
{
  if (s == BX_SEG_REG_SS)
    return BX_SS_EXCEPTION;
  else
    return BX_GP_EXCEPTION;
}

#if BX_SupportGuest2HostTLB
  Bit8u* BX_CPP_AttrRegparmN(2)
BX_CPU_C::v2h_read_byte(bx_address laddr, unsigned curr_pl)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
  bx_address lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us read access
    // from this CPL.
    if (tlbEntry->accessBits & (1<<curr_pl)) { // Read this pl OK.
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
      return hostAddr;
    }
  }

  return 0;
}

  Bit8u* BX_CPP_AttrRegparmN(2)
BX_CPU_C::v2h_write_byte(bx_address laddr, unsigned curr_pl)
{
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
  bx_address lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf)
  {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x10 << curr_pl)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
      pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
      return hostAddr;
    }
  }

  return 0;
}

  Bit8u* BX_CPP_AttrRegparmN(3)
BX_CPU_C::v2h_read(bx_address laddr, unsigned curr_pl, unsigned len)
{
  // Make sure access does not span 2 pages.
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, len);
  bx_address lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us read access
    // from this CPL.
    if (tlbEntry->accessBits & (1<<curr_pl)) { // Read this pl OK.
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
      return hostAddr;
    }
  }

  return 0;
}

  Bit8u* BX_CPP_AttrRegparmN(3)
BX_CPU_C::v2h_write(bx_address laddr, unsigned curr_pl, unsigned len)
{
  // Make sure access does not span 2 pages.
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, len);
  bx_address lpf = LPFOf(laddr);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf)
  {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x10 << curr_pl)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
      pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
      return hostAddr;
    }
  }

  return 0;
}
#endif   // BX_SupportGuest2HostTLB

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_byte(unsigned s, bx_address offset, Bit8u data)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 1, BX_WRITE);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 1, BX_WRITE);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 1, CPL, BX_WRITE, (Bit8u*) &data);
        Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        *hostAddr = data;
        return;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("write_virtual_byte(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
    access_write_linear(laddr, 1, CPL, (void *) &data);
    return;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset <= seg->cache.u.segment.limit_scaled))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 1);
  goto accessOK;
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_word(unsigned s, bx_address offset, Bit16u data)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 2, BX_WRITE);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 1);
    bx_address lpf = AlignedAccessLPFOf(laddr, 1);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 2, BX_WRITE);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 2, CPL, BX_WRITE, (Bit8u*) &data);
        Bit16u *hostAddr = (Bit16u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        WriteHostWordToLittleEndian(hostAddr, data);
        return;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("write_virtual_word(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 1) {
        BX_ERROR(("write_virtual_word(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_write_linear(laddr, 2, CPL, (void *) &data);
    return;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset < seg->cache.u.segment.limit_scaled))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 2);
  goto accessOK;
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_dword(unsigned s, bx_address offset, Bit32u data)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 4, BX_WRITE);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 3);
    bx_address lpf = AlignedAccessLPFOf(laddr, 3);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 4, BX_WRITE);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 4, CPL, BX_WRITE, (Bit8u*) &data);
        Bit32u *hostAddr = (Bit32u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        WriteHostDWordToLittleEndian(hostAddr, data);
        return;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("write_virtual_dword(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 3) {
        BX_ERROR(("write_virtual_dword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_write_linear(laddr, 4, CPL, (void *) &data);
    return;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset < (seg->cache.u.segment.limit_scaled-2)))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 4);
  goto accessOK;
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_qword(unsigned s, bx_address offset, Bit64u data)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 8, BX_WRITE);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 7);
    bx_address lpf = AlignedAccessLPFOf(laddr, 7);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 8, BX_WRITE);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 8, CPL, BX_WRITE, (Bit8u*) &data);
        Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        WriteHostQWordToLittleEndian(hostAddr, data);
        return;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("write_virtual_qword(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 7) {
        BX_ERROR(("write_virtual_qword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_write_linear(laddr, 8, CPL, (void *) &data);
    return;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset <= (seg->cache.u.segment.limit_scaled-7)))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 8);
  goto accessOK;
}

  Bit8u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_virtual_byte(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit8u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 1, BX_READ);

  if (seg->cache.valid & SegAccessROK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us read access
      // from this CPL.
      if (tlbEntry->accessBits & (1<<CPL)) { // Read this pl OK.
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 1, BX_READ);
        Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
        data = *hostAddr;
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 1, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_virtual_byte(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
    access_read_linear(laddr, 1, CPL, BX_READ, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessROK) {
    if (Is64BitMode() || (offset <= seg->cache.u.segment.limit_scaled))
      goto accessOK;
  }
  read_virtual_checks(seg, offset, 1);
  goto accessOK;
}

  Bit16u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_virtual_word(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit16u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 2, BX_READ);

  if (seg->cache.valid & SegAccessROK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 1);
    bx_address lpf = AlignedAccessLPFOf(laddr, 1);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us read access
      // from this CPL.
      if (tlbEntry->accessBits & (1<<CPL)) { // Read this pl OK.
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 2, BX_READ);
        Bit16u *hostAddr = (Bit16u*) (hostPageAddr | pageOffset);
        ReadHostWordFromLittleEndian(hostAddr, data);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 2, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_virtual_word(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 1) {
        BX_ERROR(("read_virtual_word(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_read_linear(laddr, 2, CPL, BX_READ, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessROK) {
    if (Is64BitMode() || (offset < seg->cache.u.segment.limit_scaled))
      goto accessOK;
  }
  read_virtual_checks(seg, offset, 2);
  goto accessOK;
}

  Bit32u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_virtual_dword(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit32u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 4, BX_READ);

  if (seg->cache.valid & SegAccessROK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 3);
    bx_address lpf = AlignedAccessLPFOf(laddr, 3);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us read access
      // from this CPL.
      if (tlbEntry->accessBits & (1<<CPL)) { // Read this pl OK.
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 4, BX_READ);
        Bit32u *hostAddr = (Bit32u*) (hostPageAddr | pageOffset);
        ReadHostDWordFromLittleEndian(hostAddr, data);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 4, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_virtual_dword(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 3) {
        BX_ERROR(("read_virtual_dword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_read_linear(laddr, 4, CPL, BX_READ, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessROK) {
    if (Is64BitMode() || (offset < (seg->cache.u.segment.limit_scaled-2)))
      goto accessOK;
  }
  read_virtual_checks(seg, offset, 4);
  goto accessOK;
}

  Bit64u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_virtual_qword(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit64u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 8, BX_READ);

  if (seg->cache.valid & SegAccessROK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 7);
    bx_address lpf = AlignedAccessLPFOf(laddr, 7);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us read access
      // from this CPL.
      if (tlbEntry->accessBits & (1<<CPL)) { // Read this pl OK.
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 8, BX_READ);
        Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
        ReadHostQWordFromLittleEndian(hostAddr, data);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 8, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_virtual_qword(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 7) {
        BX_ERROR(("read_virtual_qword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_read_linear(laddr, 8, CPL, BX_READ, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessROK) {
    if (Is64BitMode() || (offset <= (seg->cache.u.segment.limit_scaled-7)))
      goto accessOK;
  }
  read_virtual_checks(seg, offset, 8);
  goto accessOK;
}

//////////////////////////////////////////////////////////////
// special Read-Modify-Write operations                     //
// address translation info is kept across read/write calls //
//////////////////////////////////////////////////////////////

  Bit8u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_virtual_byte(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit8u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 1, BX_RW);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 0);
    bx_address lpf = LPFOf(laddr);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 1, BX_RW);
        Bit8u *hostAddr = (Bit8u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        data = *hostAddr;
        BX_CPU_THIS_PTR address_xlation.pages = (bx_ptr_equiv_t) hostAddr;
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 1, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
    // Accelerated attempt falls through to long path.  Do it the
    // old fashioned way...
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_RMW_virtual_byte(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
    access_read_linear(laddr, 1, CPL, BX_RW, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset <= seg->cache.u.segment.limit_scaled))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 1);
  goto accessOK;
}

  Bit16u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_virtual_word(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit16u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 2, BX_RW);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 1);
    bx_address lpf = AlignedAccessLPFOf(laddr, 1);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 2, BX_RW);
        Bit16u *hostAddr = (Bit16u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        ReadHostWordFromLittleEndian(hostAddr, data);
        BX_CPU_THIS_PTR address_xlation.pages = (bx_ptr_equiv_t) hostAddr;
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 2, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_RMW_virtual_word(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 1) {
        BX_ERROR(("read_RMW_virtual_word(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_read_linear(laddr, 2, CPL, BX_RW, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset < seg->cache.u.segment.limit_scaled))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 2);
  goto accessOK;
}

  Bit32u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_virtual_dword(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit32u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 4, BX_RW);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 3);
    bx_address lpf = AlignedAccessLPFOf(laddr, 3);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 4, BX_RW);
        Bit32u *hostAddr = (Bit32u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        ReadHostDWordFromLittleEndian(hostAddr, data);
        BX_CPU_THIS_PTR address_xlation.pages = (bx_ptr_equiv_t) hostAddr;
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 4, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_RMW_virtual_dword(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 3) {
        BX_ERROR(("read_RMW_virtual_dword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_read_linear(laddr, 4, CPL, BX_RW, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset < (seg->cache.u.segment.limit_scaled-2)))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 4);
  goto accessOK;
}

  Bit64u BX_CPP_AttrRegparmN(2)
BX_CPU_C::read_RMW_virtual_qword(unsigned s, bx_address offset)
{
  bx_address laddr;
  bx_segment_reg_t *seg = &BX_CPU_THIS_PTR sregs[s];
  Bit64u data;
  BX_INSTR_MEM_DATA_ACCESS(BX_CPU_ID, s, offset, 8, BX_RW);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 7);
    bx_address lpf = AlignedAccessLPFOf(laddr, 7);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 8, BX_RW);
        Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        ReadHostQWordFromLittleEndian(hostAddr, data);
        BX_CPU_THIS_PTR address_xlation.pages = (bx_ptr_equiv_t) hostAddr;
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 8, CPL, BX_READ, (Bit8u*) &data);
        return data;
      }
    }
#endif
#if BX_SUPPORT_X86_64
    if (! IsCanonical(laddr)) {
      BX_ERROR(("read_RMW_virtual_qword(): canonical failure"));
      exception(int_number(seg), 0, 0);
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check()) {
      if (laddr & 7) {
        BX_ERROR(("read_RMW_virtual_qword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_read_linear(laddr, 8, CPL, BX_RW, (void *) &data);
    return data;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (Is64BitMode() || (offset <= (seg->cache.u.segment.limit_scaled-7)))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 8);
  goto accessOK;
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_virtual_byte(Bit8u val8)
{
  BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
    BX_CPU_THIS_PTR address_xlation.paddress1, 2, BX_WRITE, (Bit8u*) &val8);

  if (BX_CPU_THIS_PTR address_xlation.pages > 2) {
    // Pages > 2 means it stores a host address for direct access.
    Bit8u *hostAddr = (Bit8u *) BX_CPU_THIS_PTR address_xlation.pages;
    *hostAddr = val8;
  }
  else {
    // address_xlation.pages must be 1
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1, 1, &val8);
  }
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_virtual_word(Bit16u val16)
{
  if (BX_CPU_THIS_PTR address_xlation.pages > 2) {
    // Pages > 2 means it stores a host address for direct access.
    Bit16u *hostAddr = (Bit16u *) BX_CPU_THIS_PTR address_xlation.pages;
    WriteHostWordToLittleEndian(hostAddr, val16);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 2, BX_WRITE, (Bit8u*) &val16);
  }
  else if (BX_CPU_THIS_PTR address_xlation.pages == 1) {
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1, 2, &val16);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 2, BX_WRITE, (Bit8u*) &val16);
  }
  else {
#ifdef BX_LITTLE_ENDIAN
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1, 1, &val16);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 1, BX_WRITE,  (Bit8u*) &val16);
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress2, 1, ((Bit8u *) &val16) + 1);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress2, 1, BX_WRITE, ((Bit8u*) &val16)+1);
#else
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1, 1, ((Bit8u *) &val16) + 1);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 1, BX_WRITE, ((Bit8u*) &val16)+1);
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress2, 1, &val16);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress2, 1, BX_WRITE,  (Bit8u*) &val16);
#endif
  }
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_virtual_dword(Bit32u val32)
{
  if (BX_CPU_THIS_PTR address_xlation.pages > 2) {
    // Pages > 2 means it stores a host address for direct access.
    Bit32u *hostAddr = (Bit32u *) BX_CPU_THIS_PTR address_xlation.pages;
    WriteHostDWordToLittleEndian(hostAddr, val32);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 4, BX_WRITE, (Bit8u*) &val32);
  }
  else if (BX_CPU_THIS_PTR address_xlation.pages == 1) {
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1, 4, &val32);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 4, BX_WRITE, (Bit8u*) &val32);
  }
  else {
#ifdef BX_LITTLE_ENDIAN
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1,
        &val32);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1, BX_WRITE, (Bit8u*) &val32);
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2,
        ((Bit8u *) &val32) + BX_CPU_THIS_PTR address_xlation.len1);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2, BX_WRITE,
        ((Bit8u *) &val32) + BX_CPU_THIS_PTR address_xlation.len1);
#else
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1,
        ((Bit8u *) &val32) + (4 - BX_CPU_THIS_PTR address_xlation.len1));
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1, BX_WRITE,
        ((Bit8u *) &val32) + (4 - BX_CPU_THIS_PTR address_xlation.len1));
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2,
        &val32);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2, BX_WRITE, (Bit8u*) &val32);
#endif
  }
}

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::write_RMW_virtual_qword(Bit64u val64)
{
  if (BX_CPU_THIS_PTR address_xlation.pages > 2) {
    // Pages > 2 means it stores a host address for direct access.
    Bit64u *hostAddr = (Bit64u *) BX_CPU_THIS_PTR address_xlation.pages;
    WriteHostQWordToLittleEndian(hostAddr, val64);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 8, BX_WRITE, (Bit8u*) &val64);
  }
  else if (BX_CPU_THIS_PTR address_xlation.pages == 1) {
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1, 8, &val64);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1, 8, BX_WRITE, (Bit8u*) &val64);
  }
  else {
#ifdef BX_LITTLE_ENDIAN
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1,
        &val64);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1, BX_WRITE, (Bit8u*) &val64);
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2,
        ((Bit8u *) &val64) + BX_CPU_THIS_PTR address_xlation.len1);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2, BX_WRITE,
        ((Bit8u *) &val64) + BX_CPU_THIS_PTR address_xlation.len1);
#else
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1,
        ((Bit8u *) &val64) + (8 - BX_CPU_THIS_PTR address_xlation.len1));
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress1,
        BX_CPU_THIS_PTR address_xlation.len1, BX_WRITE,
        ((Bit8u *) &val32) + (8 - BX_CPU_THIS_PTR address_xlation.len1));
    BX_MEM(0)->writePhysicalPage(BX_CPU_THIS,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2,
        &val64);
    BX_DBG_PHY_MEMORY_ACCESS(BX_CPU_ID,
        BX_CPU_THIS_PTR address_xlation.paddress2,
        BX_CPU_THIS_PTR address_xlation.len2, BX_WRITE, (Bit8u*) &val64);
#endif
  }
}

//
// Some macro defs to make things cleaner for endian-ness issues.
// The following routines access a double qword, ie 16-bytes.
// For the moment, I redirect these to use the single qword routines
// by splitting one access into two.
//
// Endian  Host byte order         Guest (x86) byte order
// ======================================================
// Little  0..7 8..15               0..7 8..15
// Big    15..8 7...0               0..7 8..15
//
// Below are the host memory offsets to each of 2 single quadwords, which
// are different across big an little endian machines.  The memory
// accessing routines take care of the access endian issues when accessing
// the physical memory image.
//


#ifdef BX_LITTLE_ENDIAN
#  define Host1stDWordOffset 0
#  define Host2ndDWordOffset 8
#else
#  define Host1stDWordOffset 8
#  define Host2ndDWordOffset 0
#endif


  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_virtual_dqword(unsigned s, bx_address offset, Bit8u *data)
{
  // Read Double Quadword.
  Bit64u *qwords = (Bit64u*) data;

  qwords[0] = read_virtual_qword(s, offset+Host1stDWordOffset);
  qwords[1] = read_virtual_qword(s, offset+Host2ndDWordOffset);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_virtual_dqword_aligned(unsigned s, bx_address offset, Bit8u *data)
{
  // If double quadword access is unaligned, #GP(0).
  bx_address laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
  if (laddr & 0xf) {
    BX_DEBUG(("read_virtual_dqword_aligned: access not aligned to 16-byte"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  read_virtual_dqword(s, offset, data);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_dqword(unsigned s, bx_address offset, Bit8u *data)
{
  // Write Double Quadword.
  Bit64u *qwords = (Bit64u*) data;

  write_virtual_qword(s, offset+Host1stDWordOffset, qwords[0]);
  write_virtual_qword(s, offset+Host2ndDWordOffset, qwords[1]);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_dqword_aligned(unsigned s, bx_address offset, Bit8u *data)
{
  // If double quadword access is unaligned, #GP(0).
  bx_address laddr = BX_CPU_THIS_PTR get_laddr(s, offset);
  if (laddr & 0xf) {
    BX_DEBUG(("write_virtual_dqword_aligned: access not aligned to 16-byte"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  write_virtual_dqword(s, offset, data);
}

#if BX_SUPPORT_FPU

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_virtual_tword(unsigned s, bx_address offset, floatx80 *data)
{
  // read floating point register
  data->fraction = read_virtual_qword(s, offset+0);
  data->exp      = read_virtual_word (s, offset+8);
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_tword(unsigned s, bx_address offset, floatx80 *data)
{
  // store floating point register
  write_virtual_qword(s, offset+0, data->fraction);
  write_virtual_word (s, offset+8, data->exp);
}

#endif

//
// Write data to new stack, these methods are required for emulation
// correctness but not performance critical.
//

// assuming the write happens in legacy mode
void BX_CPU_C::write_new_stack_word(bx_segment_reg_t *seg, bx_address offset, unsigned curr_pl, Bit16u data)
{
  Bit32u laddr;

  BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = (Bit32u)(seg->cache.u.segment.base + offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 1);
    bx_address lpf = AlignedAccessLPFOf(laddr, 1);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 2, BX_WRITE);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 2, curr_pl, BX_WRITE, (Bit8u*) &data);
        Bit16u *hostAddr = (Bit16u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        WriteHostWordToLittleEndian(hostAddr, data);
        return;
      }
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check() && curr_pl == 3) {
      if (laddr & 1) {
        BX_ERROR(("write_new_stack_word(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_write_linear(laddr, 2, curr_pl, (void *) &data);
    return;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (offset < seg->cache.u.segment.limit_scaled)
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 2);
  goto accessOK;
}

// assuming the write happens in legacy mode
void BX_CPU_C::write_new_stack_dword(bx_segment_reg_t *seg, bx_address offset, unsigned curr_pl, Bit32u data)
{
  Bit32u laddr;

  BX_ASSERT(BX_CPU_THIS_PTR cpu_mode != BX_MODE_LONG_64);

  if (seg->cache.valid & SegAccessWOK4G) {
accessOK:
    laddr = (Bit32u)(seg->cache.u.segment.base + offset);
#if BX_SupportGuest2HostTLB
    unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 3);
    bx_address lpf = AlignedAccessLPFOf(laddr, 3);
    bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
    if (tlbEntry->lpf == lpf) {
      // See if the TLB entry privilege level allows us write access
      // from this CPL.
      if (tlbEntry->accessBits & (0x10 << CPL)) {
        bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
        Bit32u pageOffset = PAGE_OFFSET(laddr);
        BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 4, BX_WRITE);
        BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
            tlbEntry->ppf | pageOffset, 4, curr_pl, BX_WRITE, (Bit8u*) &data);
        Bit32u *hostAddr = (Bit32u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
        pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
        WriteHostDWordToLittleEndian(hostAddr, data);
        return;
      }
    }
#endif
#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
    if (BX_CPU_THIS_PTR alignment_check() && curr_pl == 3) {
      if (laddr & 3) {
        BX_ERROR(("write_new_stack_dword(): #AC misaligned access"));
        exception(BX_AC_EXCEPTION, 0, 0);
      }
    }
#endif
    access_write_linear(laddr, 4, curr_pl, (void *) &data);
    return;
  }

  if (seg->cache.valid & SegAccessWOK) {
    if (offset < (seg->cache.u.segment.limit_scaled-2))
      goto accessOK;
  }
  write_virtual_checks(seg, offset, 4);
  goto accessOK;
}

// assuming the write happens in 64-bit mode
#if BX_SUPPORT_X86_64
void BX_CPU_C::write_new_stack_qword(bx_address laddr, unsigned curr_pl, Bit64u data)
{
#if BX_SupportGuest2HostTLB
  unsigned tlbIndex = BX_TLB_INDEX_OF(laddr, 7);
  bx_address lpf = AlignedAccessLPFOf(laddr, 7);
  bx_TLB_entry *tlbEntry = &BX_CPU_THIS_PTR TLB.entry[tlbIndex];
  if (tlbEntry->lpf == lpf) {
    // See if the TLB entry privilege level allows us write access
    // from this CPL.
    if (tlbEntry->accessBits & (0x10 << CPL)) {
      bx_hostpageaddr_t hostPageAddr = tlbEntry->hostPageAddr;
      Bit32u pageOffset = PAGE_OFFSET(laddr);
      BX_INSTR_LIN_ACCESS(BX_CPU_ID, laddr, tlbEntry->ppf | pageOffset, 8, BX_WRITE);
      BX_DBG_LIN_MEMORY_ACCESS(BX_CPU_ID, laddr,
          tlbEntry->ppf | pageOffset, 8, curr_pl, BX_WRITE, (Bit8u*) &data);
      Bit64u *hostAddr = (Bit64u*) (hostPageAddr | pageOffset);
#if BX_SUPPORT_ICACHE
      pageWriteStampTable.decWriteStamp(tlbEntry->ppf);
#endif
      WriteHostQWordToLittleEndian(hostAddr, data);
      return;
    }
  }
#endif

  if (! IsCanonical(laddr)) {
    BX_ERROR(("write_new_stack_qword(): canonical failure"));
    exception(BX_SS_EXCEPTION, 0, 0);
  }

#if BX_CPU_LEVEL >= 4 && BX_SUPPORT_ALIGNMENT_CHECK
  if (BX_CPU_THIS_PTR alignment_check() && curr_pl == 3) {
    if (laddr & 7) {
      BX_ERROR(("write_new_stack_qword(): #AC misaligned access"));
      exception(BX_AC_EXCEPTION, 0, 0);
    }
  }
#endif

  access_write_linear(laddr, 8, curr_pl, (void *) &data);
}
#endif
