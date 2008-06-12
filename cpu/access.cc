/////////////////////////////////////////////////////////////////////////
// $Id: access.cc,v 1.111 2008/06/12 19:14:39 sshwarts Exp $
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

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::write_virtual_checks(bx_segment_reg_t *seg, Bit32u offset, unsigned length)
{
  Bit32u upper_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    // Mark cache as being OK type for succeeding reads/writes
    seg->cache.valid |= SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    return;
  }
#endif

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

    default:
      BX_PANIC(("write_virtual_checks(): unknown descriptor type=%d", seg->cache.type));
  }
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::read_virtual_checks(bx_segment_reg_t *seg, Bit32u offset, unsigned length)
{
  Bit32u upper_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    // Mark cache as being OK type for succeeding reads/writes
    seg->cache.valid |= SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    return;
  }
#endif

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

    default:
      BX_PANIC(("read_virtual_checks(): unknown descriptor type=%d", seg->cache.type));
  }
}

  void BX_CPP_AttrRegparmN(3)
BX_CPU_C::execute_virtual_checks(bx_segment_reg_t *seg, Bit32u offset, unsigned length)
{
  Bit32u upper_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    // Mark cache as being OK type for succeeding reads/writes
    seg->cache.valid |= SegAccessROK | SegAccessWOK | SegAccessROK4G | SegAccessWOK4G;
    return;
  }
#endif

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

    default:
      BX_PANIC(("execute_virtual_checks(): unknown descriptor type=%d", seg->cache.type));
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
    BX_DEBUG(("read_virtual_dqword_aligned(): access not aligned to 16-byte"));
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
    BX_DEBUG(("write_virtual_dqword_aligned(): access not aligned to 16-byte"));
    exception(BX_GP_EXCEPTION, 0, 0);
  }

  write_virtual_dqword(s, offset, data);
}
