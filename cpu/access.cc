/////////////////////////////////////////////////////////////////////////
// $Id: access.cc,v 1.13 2002/09/01 20:12:09 kevinlawton Exp $
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

#if BX_USE_CPU_SMF
#define this (BX_CPU(0))
#endif



  void
BX_CPU_C::write_virtual_checks(bx_segment_reg_t *seg, Bit32u offset,
                               unsigned length)
{
  Bit32u upper_limit;

  if ( protected_mode() ) {
    if ( seg->cache.valid==0 ) {
      BX_ERROR(("seg = %s", BX_CPU_THIS_PTR strseg(seg)));
      BX_ERROR(("seg->selector.value = %04x", (unsigned) seg->selector.value));
      BX_ERROR(("write_virtual_checks: valid bit = 0"));
	  BX_ERROR(("CS: %04x", (unsigned) BX_CPU_THIS_PTR sregs[1].selector.value));
	  BX_ERROR(("IP: %04x", (unsigned) BX_CPU_THIS_PTR prev_eip));
      exception(BX_GP_EXCEPTION, 0, 0);
      return;
      }

    if (seg->cache.p == 0) { /* not present */
	  BX_INFO(("write_virtual_checks(): segment not present"));
      exception(int_number(seg), 0, 0);
      return;
      }

    switch ( seg->cache.type ) {
      case 0: case 1:   // read only
      case 4: case 5:   // read only, expand down
      case 8: case 9:   // execute only
      case 10: case 11: // execute/read
      case 12: case 13: // execute only, conforming
      case 14: case 15: // execute/read-only, conforming
		BX_INFO(("write_virtual_checks(): no write access to seg"));
        exception(int_number(seg), 0, 0);
        return;

      case 2: case 3: /* read/write */
	if (offset > (seg->cache.u.segment.limit_scaled - length + 1) 
	    || (length-1 > seg->cache.u.segment.limit_scaled)) {
		  BX_INFO(("write_virtual_checks(): write beyond limit, r/w"));
          exception(int_number(seg), 0, 0);
          return;
          }
        if (seg->cache.u.segment.limit_scaled >= 3) {
          // Mark cache as being OK type for succeeding writes.  The limit
          // checks still needs to be done though, but is more simple.  We
          // could probably also optimize that out with a flag for the case
          // when limit is the maximum 32bit value.  Limit should accomodate
          // at least a dword, since we subtract from it in the simple
          // limit check in other functions, and we don't want the value to roll.
          // Only normal segments (not expand down) are handled this way.
          seg->cache.valid |= SegAccessWOK;
          }
        break;

      case 6: case 7: /* read write, expand down */
        if (seg->cache.u.segment.d_b)
          upper_limit = 0xffffffff;
        else
          upper_limit = 0x0000ffff;
        if ( (offset <= seg->cache.u.segment.limit_scaled) ||
             (offset > upper_limit) ||
             ((upper_limit - offset) < (length - 1)) ) {
		  BX_INFO(("write_virtual_checks(): write beyond limit, r/w ED"));
          exception(int_number(seg), 0, 0);
          return;
          }
        break;
      }

    return;
    }

  else { /* real mode */
      if (offset > (seg->cache.u.segment.limit_scaled - length + 1) 
	  || (length-1 > seg->cache.u.segment.limit_scaled)) {
      //BX_INFO(("write_virtual_checks() SEG EXCEPTION:  %x:%x + %x",
      //  (unsigned) seg->selector.value, (unsigned) offset, (unsigned) length));
      if (seg == & BX_CPU_THIS_PTR sregs[2]) exception(BX_SS_EXCEPTION, 0, 0);
      else exception(BX_GP_EXCEPTION, 0, 0);
      }
    if (seg->cache.u.segment.limit_scaled >= 3) {
      // Mark cache as being OK type for succeeding writes.  See notes above.
      seg->cache.valid |= SegAccessWOK;
      }
    }
}

  void
BX_CPU_C::read_virtual_checks(bx_segment_reg_t *seg, Bit32u offset,
                              unsigned length)
{
  Bit32u upper_limit;

  if ( protected_mode() ) {
    if ( seg->cache.valid==0 ) {
      BX_ERROR(("seg = %s", BX_CPU_THIS_PTR strseg(seg)));
      BX_ERROR(("seg->selector.value = %04x", (unsigned) seg->selector.value));
      //BX_ERROR(("read_virtual_checks: valid bit = 0"));
      //BX_ERROR(("CS: %04x", (unsigned)
      //   BX_CPU_THIS_PTR sregs[1].selector.value));
      //BX_ERROR(("IP: %04x", (unsigned) BX_CPU_THIS_PTR prev_eip));
      //debug(BX_CPU_THIS_PTR eip);
      exception(BX_GP_EXCEPTION, 0, 0);
      return;
      }

    if (seg->cache.p == 0) { /* not present */
	  BX_INFO(("read_virtual_checks(): segment not present"));
      exception(int_number(seg), 0, 0);
      return;
      }

    switch ( seg->cache.type ) {
      case 0: case 1: /* read only */
      case 10: case 11: /* execute/read */
      case 14: case 15: /* execute/read-only, conforming */
	if (offset > (seg->cache.u.segment.limit_scaled - length + 1) 
	    || (length-1 > seg->cache.u.segment.limit_scaled)) {
		  BX_INFO(("read_virtual_checks(): write beyond limit"));
          exception(int_number(seg), 0, 0);
          return;
          }
        if (seg->cache.u.segment.limit_scaled >= 3) {
          // Mark cache as being OK type for succeeding writes.  See notes for
          // write checks; similar code.
          seg->cache.valid |= SegAccessROK;
          }
        break;

      case 2: case 3: /* read/write */
	if (offset > (seg->cache.u.segment.limit_scaled - length + 1) 
	    || (length-1 > seg->cache.u.segment.limit_scaled)) {
		  BX_INFO(("read_virtual_checks(): write beyond limit"));
          exception(int_number(seg), 0, 0);
          return;
          }
        if (seg->cache.u.segment.limit_scaled >= 3) {
          // Mark cache as being OK type for succeeding writes.  See notes for
          // write checks; similar code.
          seg->cache.valid |= SegAccessROK;
          }
        break;

      case 4: case 5: /* read only, expand down */
        if (seg->cache.u.segment.d_b)
          upper_limit = 0xffffffff;
        else
          upper_limit = 0x0000ffff;
        if ( (offset <= seg->cache.u.segment.limit_scaled) ||
             (offset > upper_limit) ||
             ((upper_limit - offset) < (length - 1)) ) {
		  BX_INFO(("read_virtual_checks(): write beyond limit"));
          exception(int_number(seg), 0, 0);
          return;
          }
        break;

      case 6: case 7: /* read write, expand down */
        if (seg->cache.u.segment.d_b)
          upper_limit = 0xffffffff;
        else
          upper_limit = 0x0000ffff;
        if ( (offset <= seg->cache.u.segment.limit_scaled) ||
             (offset > upper_limit) ||
             ((upper_limit - offset) < (length - 1)) ) {
		  BX_INFO(("read_virtual_checks(): write beyond limit"));
          exception(int_number(seg), 0, 0);
          return;
          }
        break;

      case 8: case 9: /* execute only */
      case 12: case 13: /* execute only, conforming */
        /* can't read or write an execute-only segment */
		BX_INFO(("read_virtual_checks(): execute only"));
        exception(int_number(seg), 0, 0);
        return;
        break;
      }
    return;
    }

  else { /* real mode */
    if (offset > (seg->cache.u.segment.limit_scaled - length + 1) 
	|| (length-1 > seg->cache.u.segment.limit_scaled)) {
      //BX_ERROR(("read_virtual_checks() SEG EXCEPTION:  %x:%x + %x",
      //  (unsigned) seg->selector.value, (unsigned) offset, (unsigned) length));
      if (seg == & BX_CPU_THIS_PTR sregs[2]) exception(BX_SS_EXCEPTION, 0, 0);
      else exception(BX_GP_EXCEPTION, 0, 0);
      }
    if (seg->cache.u.segment.limit_scaled >= 3) {
      // Mark cache as being OK type for succeeding writes.  See notes for
      // write checks; similar code.
      seg->cache.valid |= SegAccessROK;
      }
    return;
    }
}




  char *
BX_CPU_C::strseg(bx_segment_reg_t *seg)
{
  if (seg == &BX_CPU_THIS_PTR sregs[0]) return("ES");
  else if (seg == & BX_CPU_THIS_PTR sregs[1]) return("CS");
  else if (seg == & BX_CPU_THIS_PTR sregs[2]) return("SS");
  else if (seg == &BX_CPU_THIS_PTR sregs[3]) return("DS");
  else if (seg == &BX_CPU_THIS_PTR sregs[4]) return("FS");
  else if (seg == &BX_CPU_THIS_PTR sregs[5]) return("GS");
  else {
    BX_ERROR(("undefined segment passed to strseg()!"));
    return("??");
    }
}


extern unsigned priv_check[];



  void
BX_CPU_C::write_virtual_byte(unsigned s, Bit32u offset, Bit8u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessWOK) {
    if (offset <= seg->cache.u.segment.limit_scaled) {
      unsigned pl;
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 1, BX_WRITE);
      pl = (CPL==3);

#if BX_SupportGuest2HostTLB
      if (BX_CPU_THIS_PTR cr0.pg) {
        Bit32u lpf, tlbIndex, pageOffset;

        pageOffset = laddr & 0xfff;
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = laddr & 0xfffff000;
        if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == lpf) {
          Bit32u combined_access;

          // See if the TLB entry privilege level allows us write access
          // from this CPL.
          combined_access = BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access;
          if (combined_access & 1) { // TLB has seen a write already.
            unsigned privIndex;
            privIndex =
#if BX_CPU_LEVEL >= 4
              (BX_CPU_THIS_PTR cr0.wp<<4) | // bit 4
#endif
              (pl<<3) |                     // bit 3
              (combined_access & 0x07);     // bit 2,1,0
                                            // Let bit0 slide through since
                                            // we know it's 1 (W) from the
                                            // check above.
            if ( priv_check[privIndex] ) {
              // Current write access has privilege.
              Bit32u hostPageAddr;
              Bit8u *hostAddr;
              hostPageAddr =
                  BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access &
                  0xfffffff8;
              if (hostPageAddr) {
                hostAddr = (Bit8u*) (hostPageAddr + pageOffset);
                *hostAddr = *data;
                return;
                }
              }
            }
          }
        }
#endif  // BX_SupportGuest2HostTLB

      // all checks OK
      access_linear(laddr, 1, pl, BX_WRITE, (void *) data);
      return;
      }
    }
  write_virtual_checks(seg, offset, 1);
  goto accessOK;
}

  void
BX_CPU_C::write_virtual_word(unsigned s, Bit32u offset, Bit16u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessWOK) {
    if (offset < seg->cache.u.segment.limit_scaled) {
      unsigned pl;
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 2, BX_WRITE);
      pl = (CPL==3);

#if BX_SupportGuest2HostTLB
      if (BX_CPU_THIS_PTR cr0.pg) {
        Bit32u lpf, tlbIndex, pageOffset;

        pageOffset = laddr & 0xfff;
        if (pageOffset <= 0xffe) { // Make sure access does not span 2 pages.
          tlbIndex = BX_TLB_INDEX_OF(laddr);
          lpf = laddr & 0xfffff000;
          if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == lpf) {
            Bit32u combined_access;
            // See if the TLB entry privilege level allows us write access
            // from this CPL.
  
            combined_access =
                BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access;
            if (combined_access & 1) { // TLB has seen a write already.
              unsigned privIndex;
              privIndex =
#if BX_CPU_LEVEL >= 4
                (BX_CPU_THIS_PTR cr0.wp<<4) | // b4
#endif
                (pl<<3) |                     // b3
                (combined_access & 0x07);     // b{2,1,0}
                                              // Let b0 slide through since
                                              // we know it's 1 (W) from the
                                              // check above.
              if ( priv_check[privIndex] ) {
                // Current write access has privilege.
                Bit32u  hostPageAddr;
                Bit16u *hostAddr;
                hostPageAddr =
                    BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access &
                    0xfffffff8;
                if (hostPageAddr) {
                  hostAddr = (Bit16u*) (hostPageAddr + pageOffset);
                  *hostAddr = *data;
                  return;
                  }
                }
              }
            }
          }
        }
#endif  // BX_SupportGuest2HostTLB

      // all checks OK
      access_linear(laddr, 2, CPL==3, BX_WRITE, (void *) data);
      return;
      }
    }
  write_virtual_checks(seg, offset, 2);
  goto accessOK;
}

  void
BX_CPU_C::write_virtual_dword(unsigned s, Bit32u offset, Bit32u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessWOK) {
    if (offset < (seg->cache.u.segment.limit_scaled-2)) {
      unsigned pl;
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 4, BX_WRITE);
      pl = (CPL==3);

#if BX_SupportGuest2HostTLB
      if (BX_CPU_THIS_PTR cr0.pg) {
        Bit32u lpf, tlbIndex, pageOffset;
  
        pageOffset = laddr & 0xfff;
        if (pageOffset <= 0xffc) { // Make sure access does not span 2 pages.
          tlbIndex = BX_TLB_INDEX_OF(laddr);
          lpf = laddr & 0xfffff000;
          if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == lpf) {
            Bit32u combined_access;
            // See if the TLB entry privilege level allows us write access
            // from this CPL.
  
            combined_access =
                BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access;
            if (combined_access & 1) { // TLB has seen a write already.
              unsigned privIndex;
              privIndex =
#if BX_CPU_LEVEL >= 4
                (BX_CPU_THIS_PTR cr0.wp<<4) | // b4
#endif
                (pl<<3) |                     // b3
                (combined_access & 0x07);     // b{2,1,0}
                                              // Let b0 slide through since
                                              // we know it's 1 (W) from the
                                              // check above.
              if ( priv_check[privIndex] ) {
                // Current write access has privilege.
                Bit32u  hostPageAddr;
                Bit32u *hostAddr;
                hostPageAddr =
                    BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access &
                    0xfffffff8;
                if (hostPageAddr) {
                  hostAddr = (Bit32u*) (hostPageAddr + pageOffset);
                  *hostAddr = *data;
                  return;
                  }
                }
              }
            }
          }
        }
#endif  // BX_SupportGuest2HostTLB

      // all checks OK
      access_linear(laddr, 4, CPL==3, BX_WRITE, (void *) data);
      return;
      }
    }
  write_virtual_checks(seg, offset, 4);
  goto accessOK;
}

  void
BX_CPU_C::read_virtual_byte(unsigned s, Bit32u offset, Bit8u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessROK) {
    if (offset <= seg->cache.u.segment.limit_scaled) {
      unsigned pl;
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 1, BX_READ);
      pl = (CPL==3);

#if BX_SupportGuest2HostTLB
      if (BX_CPU_THIS_PTR cr0.pg) {
        Bit32u lpf, tlbIndex, pageOffset;
  
        pageOffset = laddr & 0xfff;
        tlbIndex = BX_TLB_INDEX_OF(laddr);
        lpf = laddr & 0xfffff000;
        if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == lpf) {
          // See if the TLB entry privilege level allows us read access
          // from this CPL.
          if ( ((BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access>>2) & 1)
               == pl ) {
            Bit32u hostPageAddr;
            Bit8u *hostAddr;
            hostPageAddr =
                BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access &
                0xfffffff8;
            if (hostPageAddr) {
              hostAddr = (Bit8u*) (hostPageAddr + pageOffset);
              *data = *hostAddr;
              return;
              }
            }
          }
        }
#endif  // BX_SupportGuest2HostTLB

      // all checks OK
      access_linear(laddr, 1, pl, BX_READ, (void *) data);
      return;
      }
    }
  read_virtual_checks(seg, offset, 1);
  goto accessOK;
}


  void
BX_CPU_C::read_virtual_word(unsigned s, Bit32u offset, Bit16u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessROK) {
    if (offset < seg->cache.u.segment.limit_scaled) {
      unsigned pl;
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 2, BX_READ);
      pl = (CPL==3);

#if BX_SupportGuest2HostTLB
      if (BX_CPU_THIS_PTR cr0.pg) {
        Bit32u pageOffset;

        pageOffset = laddr & 0xfff;
        if (pageOffset <= 0xffe) { // Make sure access does not span 2 pages.
          Bit32u lpf, tlbIndex;
          tlbIndex = BX_TLB_INDEX_OF(laddr);
          lpf = laddr & 0xfffff000;
          if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == lpf) {
            // See if the TLB entry privilege level allows us read access
            // from this CPL.
            if ( ((BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access>>2) & 1)
                 == pl ) {
              Bit32u hostPageAddr;
              Bit16u *hostAddr;
              hostPageAddr =
                  BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access & 0xfffffff8;
              if (hostPageAddr) {
                hostAddr = (Bit16u*) (hostPageAddr + pageOffset);
                *data = *hostAddr;
                return;
                }
              }
            }
          }
        }
#endif  // BX_SupportGuest2HostTLB

      // all checks OK
      access_linear(laddr, 2, pl, BX_READ, (void *) data);
      return;
      }
    }
  read_virtual_checks(seg, offset, 2);
  goto accessOK;
}


  void
BX_CPU_C::read_virtual_dword(unsigned s, Bit32u offset, Bit32u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessROK) {
    if (offset < (seg->cache.u.segment.limit_scaled-2)) {
      unsigned pl;
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 4, BX_READ);
      pl = (CPL==3);

#if BX_SupportGuest2HostTLB
      if (BX_CPU_THIS_PTR cr0.pg) {
        Bit32u pageOffset;

        pageOffset = laddr & 0xfff;
        if (pageOffset <= 0xffc) { // Make sure access does not span 2 pages.
          Bit32u lpf, tlbIndex;
          tlbIndex = BX_TLB_INDEX_OF(laddr);
          lpf = laddr & 0xfffff000;
          if (BX_CPU_THIS_PTR TLB.entry[tlbIndex].lpf == lpf) {
            // See if the TLB entry privilege level allows us read access
            // from this CPL.
            if ( ((BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access>>2) & 1)
                 == pl ) {
              Bit32u hostPageAddr;
              Bit32u *hostAddr;
              hostPageAddr =
                  BX_CPU_THIS_PTR TLB.entry[tlbIndex].combined_access & 0xfffffff8;
              if (hostPageAddr) {
                hostAddr = (Bit32u*) (hostPageAddr + pageOffset);
                *data = *hostAddr;
                return;
                }
              }
            }
          }
        }
#endif  // BX_SupportGuest2HostTLB

      // all checks OK
      access_linear(laddr, 4, CPL==3, BX_READ, (void *) data);
      return;
      }
    }
  read_virtual_checks(seg, offset, 4);
  goto accessOK;
}

//////////////////////////////////////////////////////////////
// special Read-Modify-Write operations                     //
// address translation info is kept across read/write calls //
//////////////////////////////////////////////////////////////

  void
BX_CPU_C::read_RMW_virtual_byte(unsigned s, Bit32u offset, Bit8u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessWOK) {
    if (offset <= seg->cache.u.segment.limit_scaled) {
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 1, BX_READ);

      // all checks OK
#if BX_CPU_LEVEL >= 3
      if (BX_CPU_THIS_PTR cr0.pg)
        access_linear(laddr, 1, CPL==3, BX_RW, (void *) data);
      else
#endif
        {
        BX_CPU_THIS_PTR address_xlation.paddress1 = laddr;
        BX_INSTR_LIN_READ(laddr, laddr, 1);
        BX_INSTR_LIN_WRITE(laddr, laddr, 1);
        BX_CPU_THIS_PTR mem->read_physical(this, laddr, 1, (void *) data);
        }
      return;
      }
    }
  write_virtual_checks(seg, offset, 1);
  goto accessOK;
}


  void
BX_CPU_C::read_RMW_virtual_word(unsigned s, Bit32u offset, Bit16u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessWOK) {
    if (offset < seg->cache.u.segment.limit_scaled) {
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 2, BX_READ);

      // all checks OK
#if BX_CPU_LEVEL >= 3
      if (BX_CPU_THIS_PTR cr0.pg)
        access_linear(laddr, 2, CPL==3, BX_RW, (void *) data);
      else
#endif
        {
        BX_CPU_THIS_PTR address_xlation.paddress1 = laddr;
        BX_INSTR_LIN_READ(laddr, laddr, 2);
        BX_INSTR_LIN_WRITE(laddr, laddr, 2);
        BX_CPU_THIS_PTR mem->read_physical(this, laddr, 2, data);
        }
      return;
      }
    }
  write_virtual_checks(seg, offset, 2);
  goto accessOK;
}

  void
BX_CPU_C::read_RMW_virtual_dword(unsigned s, Bit32u offset, Bit32u *data)
{
  Bit32u laddr;
  bx_segment_reg_t *seg;

  seg = &BX_CPU_THIS_PTR sregs[s];
  if (seg->cache.valid & SegAccessWOK) {
    if (offset < (seg->cache.u.segment.limit_scaled-2)) {
accessOK:
      laddr = seg->cache.u.segment.base + offset;
      BX_INSTR_MEM_DATA(laddr, 4, BX_READ);

      // all checks OK
#if BX_CPU_LEVEL >= 3
      if (BX_CPU_THIS_PTR cr0.pg)
        access_linear(laddr, 4, CPL==3, BX_RW, (void *) data);
      else
#endif
        {
        BX_CPU_THIS_PTR address_xlation.paddress1 = laddr;
        BX_INSTR_LIN_READ(laddr, laddr, 4);
        BX_INSTR_LIN_WRITE(laddr, laddr, 4);
        BX_CPU_THIS_PTR mem->read_physical(this, laddr, 4, data);
        }
      return;
      }
    }
  write_virtual_checks(seg, offset, 4);
  goto accessOK;
}

  void
BX_CPU_C::write_RMW_virtual_byte(Bit8u val8)
{
  BX_INSTR_MEM_DATA(BX_CPU_THIS_PTR address_xlation.paddress1, 1, BX_WRITE);

#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR cr0.pg) {
    // BX_CPU_THIS_PTR address_xlation.pages must be 1
    BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 1, &val8);
    }
  else
#endif
    {
    BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 1, &val8);
    }
}

  void
BX_CPU_C::write_RMW_virtual_word(Bit16u val16)
{
  BX_INSTR_MEM_DATA(BX_CPU_THIS_PTR address_xlation.paddress1, 2, BX_WRITE);

#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR cr0.pg) {
    if (BX_CPU_THIS_PTR address_xlation.pages == 1) {
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 2, &val16);
      }
    else {
#ifdef BX_LITTLE_ENDIAN
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 1,
                            &val16);
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress2, 1,
                            ((Bit8u *) &val16) + 1);
#else
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 1,
                            ((Bit8u *) &val16) + 1);
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress2, 1,
                            &val16);
#endif
      }
    }
  else
#endif
    {
    BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 2, &val16);
    }
}

  void
BX_CPU_C::write_RMW_virtual_dword(Bit32u val32)
{
  BX_INSTR_MEM_DATA(BX_CPU_THIS_PTR address_xlation.paddress1, 4, BX_WRITE);

#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR cr0.pg) {
    if (BX_CPU_THIS_PTR address_xlation.pages == 1) {
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 4, &val32);
      }
    else {
#ifdef BX_LITTLE_ENDIAN
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1,
                            BX_CPU_THIS_PTR address_xlation.len1,
                            &val32);
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress2,
                            BX_CPU_THIS_PTR address_xlation.len2,
                            ((Bit8u *) &val32) + BX_CPU_THIS_PTR address_xlation.len1);
#else
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1,
                            BX_CPU_THIS_PTR address_xlation.len1,
                            ((Bit8u *) &val32) + (4 - BX_CPU_THIS_PTR address_xlation.len1));
      BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress2,
                            BX_CPU_THIS_PTR address_xlation.len2,
                            &val32);
#endif
      }
    }
  else
#endif
    {
    BX_CPU_THIS_PTR mem->write_physical(this, BX_CPU_THIS_PTR address_xlation.paddress1, 4, &val32);
    }
}
