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

  void BX_CPP_AttrRegparmN(1)
BX_CPU_C::push_16(Bit16u value16)
{
  /* must use StackAddrSize, and either ESP or SP accordingly */
#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) { /* StackAddrSize = 32 */
    /* 32bit stack size: pushes use SS:ESP  */
    if (protected_mode()) {
      if (!can_push(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache, ESP, 2)) {
        BX_INFO(("push_16(): push outside stack limits"));
        exception(BX_SS_EXCEPTION, 0, 0);
      }
    }
    else 
    { /* real mode */
      if (ESP == 1)
        BX_PANIC(("CPU shutting down due to lack of stack space, ESP==1"));
    }

    write_virtual_word(BX_SEG_REG_SS, ESP-2, &value16);
    ESP -= 2;
  }
  else
#endif
  {
    /* 16bit stack size: pushes use SS:SP  */
#if BX_CPU_LEVEL >= 2
    if (protected_mode()) {
      if (!can_push(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache, SP, 2)) {
        BX_INFO(("push_16(): push outside stack limits"));
        exception(BX_SS_EXCEPTION, 0, 0);
      }
    }
    else 
#endif
    { /* real mode */
      if (SP == 1)
        BX_PANIC(("CPU shutting down due to lack of stack space, SP==1"));
    }

    write_virtual_word(BX_SEG_REG_SS, (Bit16u) (SP-2), &value16);
    SP -= 2;
  }
}

#if BX_CPU_LEVEL >= 3
/* push 32 bit operand size */
void BX_CPU_C::push_32(Bit32u value32)
{
  /* must use StackAddrSize, and either ESP or SP accordingly */
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) { /* StackAddrSize = 32 */
    /* 32bit stack size: pushes use SS:ESP  */
    if (protected_mode()) {
      if (!can_push(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache, ESP, 4)) {
        BX_INFO(("push_32(): push outside stack limits"));
        exception(BX_SS_EXCEPTION, 0, 0);
        }
      }
    else { /* real mode */
      if ((ESP>=1) && (ESP<=3)) {
        BX_PANIC(("push_32: ESP=%08x", (unsigned) ESP));
        }
      }

    write_virtual_dword(BX_SEG_REG_SS, ESP-4, &value32);
    ESP -= 4;
  }
  else { /* 16bit stack size: pushes use SS:SP  */
    if (protected_mode()) {
      if (!can_push(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache, SP, 4)) {
        BX_INFO(("push_32(): push outside stack limits"));
        exception(BX_SS_EXCEPTION, 0, 0);
        }
      }
    else { /* real mode */
      if ((SP>=1) && (SP<=3)) {
        BX_PANIC(("push_32: SP=%08x", (unsigned) SP));
        }
      }

    write_virtual_dword(BX_SEG_REG_SS, (Bit16u) (SP-4), &value32);
    SP -= 4;
  }
}

#if BX_SUPPORT_X86_64
void BX_CPU_C::push_64(Bit64u value64)
{
  /* 64bit stack size: pushes use SS:RSP, assume protected mode  */
#if BX_IGNORE_THIS
  if (!can_push(&BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache, RSP, 8)) {
    BX_INFO(("push_64(): push outside stack limits"));
    exception(BX_SS_EXCEPTION, 0, 0); /* #SS(0) */
    }
#endif

  write_virtual_qword(BX_SEG_REG_SS, RSP-8, &value64);
  RSP -= 8;
}
#endif  // #if BX_SUPPORT_X86_64

#endif /* BX_CPU_LEVEL >= 3 */

  void
BX_CPU_C::pop_16(Bit16u *value16_ptr)
{
  Bit32u temp_ESP;

#if BX_CPU_LEVEL >= 3
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
    temp_ESP = ESP;
  else
#endif
    temp_ESP = SP;

#if BX_CPU_LEVEL >= 2
  if (protected_mode()) {
    if ( !can_pop(2) ) {
      BX_ERROR(("pop_16(): can't pop from stack"));
      exception(BX_SS_EXCEPTION, 0, 0);
      return;
      }
    }
#endif

  /* access within limits */
  read_virtual_word(BX_SEG_REG_SS, temp_ESP, value16_ptr);

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
    ESP += 2;
  else
    SP += 2;
}

#if BX_CPU_LEVEL >= 3
  void
BX_CPU_C::pop_32(Bit32u *value32_ptr)
{
  Bit32u temp_ESP;

  /* 32 bit stack mode: use SS:ESP */
  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b)
    temp_ESP = ESP;
  else
    temp_ESP = SP;

  /* 16 bit stack mode: use SS:SP */
  if (protected_mode()) {
    if ( !can_pop(4) ) {
      BX_ERROR(("pop_32(): can't pop from stack"));
      exception(BX_SS_EXCEPTION, 0, 0);
      return;
      }
    }

  /* access within limits */
  read_virtual_dword(BX_SEG_REG_SS, temp_ESP, value32_ptr);

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b==1)
    ESP += 4;
  else
    SP += 4;
}

#if BX_SUPPORT_X86_64
void BX_CPU_C::pop_64(Bit64u *value64_ptr)
{
  if ( !can_pop(8) ) {
    BX_ERROR(("pop_64(): can't pop from stack"));
    exception(BX_SS_EXCEPTION, 0, 0);
    return;
    }

  /* access within limits */

  read_virtual_qword(BX_SEG_REG_SS, RSP, value64_ptr);

  RSP += 8;
}
#endif  // #if BX_SUPPORT_X86_64

#endif


#if BX_CPU_LEVEL >= 2
  bx_bool BX_CPP_AttrRegparmN(3)
BX_CPU_C::can_push(bx_descriptor_t *descriptor, Bit32u esp, Bit32u bytes)
{
#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    return(1);
    }
#endif

  if ( real_mode() ) { /* code not needed ??? */
    BX_PANIC(("can_push(): called in real mode"));
    return(0); /* never gets here */
    }

  // small stack compares against 16-bit SP
  if (!descriptor->u.segment.d_b)
    esp &= 0x0000ffff;


  if (descriptor->valid==0) {
    BX_PANIC(("can_push(): SS invalidated."));
    return(0);
    }

  if (descriptor->p==0) {
    BX_PANIC(("can_push(): descriptor not present"));
    return(0);
    }


  if (descriptor->u.segment.c_ed) { /* expand down segment */
    Bit32u expand_down_limit;

    if (descriptor->u.segment.d_b)
      expand_down_limit = 0xffffffff;
    else
      expand_down_limit = 0x0000ffff;

    if (esp==0) {
      BX_PANIC(("can_push(): esp=0, wraparound?"));
      return(0);
      }

    if (esp < bytes) {
      BX_PANIC(("can_push(): expand-down: esp < N"));
      return(0);
      }
    if ( (esp - bytes) <= descriptor->u.segment.limit_scaled ) {
      BX_PANIC(("can_push(): expand-down: esp-N < limit"));
      return(0);
      }
    if ( esp > expand_down_limit ) {
      BX_PANIC(("can_push(): esp > expand-down-limit"));
      return(0);
      }
    return(1);
    }
  else { /* normal (expand-up) segment */
    if (descriptor->u.segment.limit_scaled==0) {
      BX_PANIC(("can_push(): found limit of 0"));
      return(0);
      }

    // Look at case where esp==0.  Possibly, it's an intentional wraparound
    // If so, limit must be the maximum for the given stack size
    if (esp==0) {
      if (descriptor->u.segment.d_b && (descriptor->u.segment.limit_scaled==0xffffffff))
        return(1);
      if ((descriptor->u.segment.d_b==0) && (descriptor->u.segment.limit_scaled>=0xffff))
        return(1);
      BX_PANIC(("can_push(): esp=0, normal, wraparound? limit=%08x",
        descriptor->u.segment.limit_scaled));
      return(0);
      }

    if (esp < bytes) {
      BX_INFO(("can_push(): expand-up: esp < N"));
      return(0);
      }
    if ((esp-1) > descriptor->u.segment.limit_scaled) {
      BX_INFO(("can_push(): expand-up: SP > limit"));
      return(0);
      }
    /* all checks pass */
    return(1);
    }
}
#endif


#if BX_CPU_LEVEL >= 2
  bx_bool
BX_CPU_C::can_pop(Bit32u bytes)
{
  Bit32u temp_ESP, expand_down_limit;

#if BX_SUPPORT_X86_64
  if (BX_CPU_THIS_PTR cpu_mode == BX_MODE_LONG_64) {
    return(1);
    }
#endif

  /* ??? */
  if (real_mode()) BX_PANIC(("can_pop(): called in real mode?"));

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.d_b) { /* Big bit set: use ESP */
    temp_ESP = ESP;
    expand_down_limit = 0xFFFFFFFF;
    }
  else { /* Big bit clear: use SP */
    temp_ESP = SP;
    expand_down_limit = 0xFFFF;
    }

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.valid==0) {
    BX_PANIC(("can_pop(): SS invalidated."));
    return(0); /* never gets here */
    }

  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.p==0) { /* ??? */
    BX_PANIC(("can_pop(): SS.p = 0"));
    return(0);
    }


  if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.c_ed) { /* expand down segment */
    if ( temp_ESP == expand_down_limit ) {
      BX_PANIC(("can_pop(): found SP=ffff"));
      return(0);
      }
    if ( ((expand_down_limit - temp_ESP) + 1) >= bytes )
      return(1);
    return(0);
    }
  else { /* normal (expand-up) segment */
    if (BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled==0) {
      BX_PANIC(("can_pop(): SS.limit = 0"));
      }
    if ( temp_ESP == expand_down_limit ) {
      BX_PANIC(("can_pop(): found SP=ffff"));
      return(0);
      }
    if ( temp_ESP > BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled ) {
      BX_PANIC(("can_pop(): eSP > SS.limit"));
      return(0);
      }
    if ( ((BX_CPU_THIS_PTR sregs[BX_SEG_REG_SS].cache.u.segment.limit_scaled - temp_ESP) + 1) >= bytes )
      return(1);
    return(0);
    }
}
#endif
