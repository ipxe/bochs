/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2002 Stanislav Shwartsman
//          Written by Stanislav Shwartsman <gate@fidonet.org.il>
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



#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#define LOG_THIS BX_CPU_THIS_PTR


#if BX_SUPPORT_SSE

void BX_CPU_C::prepareSSE(void)
{
  if(BX_CPU_THIS_PTR cr0.ts)
    exception(BX_NM_EXCEPTION, 0, 0);

  if(BX_CPU_THIS_PTR cr0.em)
    exception(BX_UD_EXCEPTION, 0, 0);

  if(! (BX_CPU_THIS_PTR cr4.get_OSFXSR()))
    exception(BX_UD_EXCEPTION, 0, 0);
}

#define BX_MXCSR_REGISTER (BX_CPU_THIS_PTR mxcsr.mxcsr)

#endif

/* 0F AE Grp15 010 */
void BX_CPU_C::LDMXCSR(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Bit32u new_mxcsr;

  read_virtual_dword(i->seg(), RMAddr(i), &new_mxcsr);
  if(new_mxcsr & ~MXCSR_MASK)
      exception(BX_GP_EXCEPTION, 0, 0);

  BX_MXCSR_REGISTER = new_mxcsr;
#else
  BX_INFO(("LDMXCSR: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 0F AE Grp15 011 */
void BX_CPU_C::STMXCSR(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  Bit32u mxcsr = BX_MXCSR_REGISTER & MXCSR_MASK;
  write_virtual_dword(i->seg(), RMAddr(i), &mxcsr);
#else
  BX_INFO(("STMXCSR: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 0F AE Grp15 000 */
void BX_CPU_C::FXSAVE(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BxPackedXmmRegister xmm;
  Bit16u twd = BX_CPU_THIS_PTR the_i387.soft.twd, tbd = 0;
  unsigned index;

  xmm.xmm16u(0) = BX_CPU_THIS_PTR the_i387.soft.cwd;
  xmm.xmm16u(1) = BX_CPU_THIS_PTR the_i387.soft.swd;

  if(twd & 0x0003 == 0x0003) tbd |= 0x0100;
  if(twd & 0x000c == 0x000c) tbd |= 0x0200;
  if(twd & 0x0030 == 0x0030) tbd |= 0x0400;
  if(twd & 0x00c0 == 0x00c0) tbd |= 0x0800;
  if(twd & 0x0300 == 0x0300) tbd |= 0x1000;
  if(twd & 0x0c00 == 0x0c00) tbd |= 0x2000;
  if(twd & 0x3000 == 0x3000) tbd |= 0x4000;
  if(twd & 0xc000 == 0xc000) tbd |= 0x8000;

  xmm.xmm16u(2) = tbd;

  /* x87 FPU Opcode (16 bits) */
  /* The lower 11 bits contain the FPU opcode, upper 5 bits are reserved */
  xmm.xmm16u(3) = 0;  /* still not implemented */

  /* 
   * x87 FPU IP Offset (32 bits)
   * The contents of this field differ depending on the current 
   * addressing mode (16/32 bit) when the FXSAVE instruction was executed:
   *   + 32-bit mode-32-bit IP offset
   *   + 16-bit mode-low 16 bits are IP offset; high 16 bits are reserved.
   *
   * x87 CS FPU IP Selector (16 bits)
   */
  xmm.xmm64u(1) = 0;  /* still not implemented */
 
  writeVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &xmm);

  /* 
   * x87 FPU Instruction Operand (Data) Pointer Offset (32 bits)
   * The contents of this field differ depending on the current 
   * addressing mode (16/32 bit) when the FXSAVE instruction was executed:
   *   + 32-bit mode-32-bit offset
   *   + 16-bit mode-low 16 bits are offset; high 16 bits are reserved.
   *
   * x87 DS FPU Instruction Operand (Data) Pointer Selector (16 bits)
   */
  xmm.xmm64u(0) = 0;  /* still not implemented */

  xmm.xmm32u(2) = BX_MXCSR_REGISTER;
  xmm.xmm32u(3) = MXCSR_MASK;

  writeVirtualDQwordAligned(i->seg(), RMAddr(i) + 16, (Bit8u *) &xmm);

  /* store i387 register file */
  for(index=0; index < 8; index++)
  {
    Bit8u *r387 = (Bit8u *) &(BX_CPU_THIS_PTR the_i387.soft.st_space[index]);
    writeVirtualDQwordAligned(i->seg(), RMAddr(i)+index*16+32, r387);
  }

  /* store XMM register file */
  for(index=0; index < BX_XMM_REGISTERS; index++)
  {
    Bit8u *r128 = (Bit8u *) &(BX_CPU_THIS_PTR xmm[index]);
    writeVirtualDQwordAligned(i->seg(), RMAddr(i)+index*16+160, r128);
  }

  /* do not touch reserved fields */
#else
  BX_INFO(("FXSAVE: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 0F AE Grp15 001 */
void BX_CPU_C::FXRSTOR(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BxPackedXmmRegister xmm;
  Bit32u tbd, twd = 0;
  unsigned index;

  readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &xmm);
  
  BX_CPU_THIS_PTR the_i387.soft.cwd = xmm.xmm16u(0);
  BX_CPU_THIS_PTR the_i387.soft.swd = xmm.xmm16u(1);

  /* TOS restore still not implemented */
  /* FOO/FPU IP restore still not implemented */

  /* 
   * Note that the original format for FTW can be recreated from the stored 
   * FTW valid bits and the stored 80-bit FP data (assuming the stored data 
   * was not the contents of MMX registers) using the following table:
    
     | Exponent | Exponent | Fraction | J,M bits | FTW valid | x87 FTW |
     |  all 1s  |  all 0s  |  all 0s  |          |           |         |
     -------------------------------------------------------------------
     |    0     |    0     |    0     |    0x    |     1     | S    10 |
     |    0     |    0     |    0     |    1x    |     1     | V    00 |
     -------------------------------------------------------------------
     |    0     |    0     |    1     |    00    |     1     | S    10 |
     |    0     |    0     |    1     |    10    |     1     | V    00 |
     -------------------------------------------------------------------
     |    0     |    1     |    0     |    0x    |     1     | S    10 |
     |    0     |    1     |    0     |    1x    |     1     | V    10 |
     -------------------------------------------------------------------
     |    0     |    1     |    1     |    00    |     1     | S    01 |
     |    0     |    1     |    1     |    10    |     1     | V    10 |
     -------------------------------------------------------------------
     |    1     |    0     |    0     |    1x    |     1     | S    10 |
     |    1     |    0     |    0     |    1x    |     1     | V    10 |
     -------------------------------------------------------------------
     |    1     |    0     |    1     |    00    |     1     | S    10 |
     |    1     |    0     |    1     |    10    |     1     | V    10 |
     -------------------------------------------------------------------
     |        all combinations above             |     1     | E    11 |

   *
   * The J-bit is defined to be the 1-bit binary integer to the left 
   * of the decimal place in the significand.
   * 
   * The M-bit is defined to be the most significant bit of the fractional 
   * portion of the significand (i.e., the bit immediately to the right of 
   * the decimal place). When the M-bit is the most significant bit of the 
   * fractional portion  of the significand, it must be  0 if the fraction 
   * is all 0's.  
   */                    /* still not implemented */

  tbd = xmm.xmm16u(2);
  if(tbd & 0x0100) twd |= 0x0003;
  if(tbd & 0x0200) twd |= 0x000c;
  if(tbd & 0x0400) twd |= 0x0030;
  if(tbd & 0x0800) twd |= 0x00c0;
  if(tbd & 0x1000) twd |= 0x0300;
  if(tbd & 0x2000) twd |= 0x0c00;
  if(tbd & 0x4000) twd |= 0x3000;
  if(tbd & 0x8000) twd |= 0xc000;
  BX_CPU_THIS_PTR the_i387.soft.twd = twd;

  /* FPU DP restore still not implemented */

  /* If the OSFXSR bit in CR4 is not set, the FXRSTOR instruction does
     not restore the states of the XMM and MXCSR registers. */
  if(! (BX_CPU_THIS_PTR cr4.get_OSFXSR())) 
  {
    readVirtualDQwordAligned(i->seg(), RMAddr(i) + 16, (Bit8u *) &xmm);

    Bit32u new_mxcsr = xmm.xmm32u(2), mxcsr_msk = xmm.xmm32u(3);
    if(! mxcsr_msk) mxcsr_msk = MXCSR_MASK; 
    if(new_mxcsr & ~mxcsr_msk)
       exception(BX_GP_EXCEPTION, 0, 0);

    BX_MXCSR_REGISTER = new_mxcsr;
  }

  /* load i387 register file */
  for(index=0; index < 8; index++)
  {
    Bit8u *r387 = (Bit8u *) &(BX_CPU_THIS_PTR the_i387.soft.st_space[index]);
    readVirtualDQwordAligned(i->seg(), RMAddr(i)+index*16+32, r387);
  }

  /* If the OSFXSR bit in CR4 is not set, the FXRSTOR instruction does
     not restore the states of the XMM and MXCSR registers. */
  if(! (BX_CPU_THIS_PTR cr4.get_OSFXSR())) 
  {
    /* load XMM register file */
    for(index=0; index < BX_XMM_REGISTERS; index++)
    {
      Bit8u *r128 = (Bit8u *) &(BX_CPU_THIS_PTR xmm[index]);
      readVirtualDQwordAligned(i->seg(), RMAddr(i)+index*16+160, r128);
    }
  }

#else
  BX_INFO(("FXRSTOR: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* ********************************************** */
/* SSE Integer Operations (128bit MMX extensions) */
/* ********************************************** */

/* 66 0F 63 */
void BX_CPU_C::PACKSSWB_VdqWq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmmsbyte(0x0) = SaturateWordSToByteS(op1.xmm16s(0));
  result.xmmsbyte(0x1) = SaturateWordSToByteS(op1.xmm16s(1));
  result.xmmsbyte(0x2) = SaturateWordSToByteS(op1.xmm16s(2));
  result.xmmsbyte(0x3) = SaturateWordSToByteS(op1.xmm16s(3));
  result.xmmsbyte(0x4) = SaturateWordSToByteS(op1.xmm16s(4));
  result.xmmsbyte(0x5) = SaturateWordSToByteS(op1.xmm16s(5));
  result.xmmsbyte(0x6) = SaturateWordSToByteS(op1.xmm16s(6));
  result.xmmsbyte(0x7) = SaturateWordSToByteS(op1.xmm16s(7));

  result.xmmsbyte(0x8) = SaturateWordSToByteS(op2.xmm16s(0));
  result.xmmsbyte(0x9) = SaturateWordSToByteS(op2.xmm16s(1));
  result.xmmsbyte(0xA) = SaturateWordSToByteS(op2.xmm16s(2));
  result.xmmsbyte(0xB) = SaturateWordSToByteS(op2.xmm16s(3));
  result.xmmsbyte(0xC) = SaturateWordSToByteS(op2.xmm16s(4));
  result.xmmsbyte(0xD) = SaturateWordSToByteS(op2.xmm16s(5));
  result.xmmsbyte(0xE) = SaturateWordSToByteS(op2.xmm16s(6));
  result.xmmsbyte(0xF) = SaturateWordSToByteS(op2.xmm16s(7));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PACKSSWB_VdqWq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 64 */
void BX_CPU_C::PCMPGTB_VdqWq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    result.xmmsbyte(j) = (op1.xmmsbyte(j) > op2.xmmsbyte(j)) ? 0xff : 0;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PCMPGTB_VdqWq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 65 */
void BX_CPU_C::PCMPGTW_VdqWq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16s(0) = (op1.xmm16s(0) > op2.xmm16s(0)) ? 0xffff : 0;
  result.xmm16s(1) = (op1.xmm16s(1) > op2.xmm16s(1)) ? 0xffff : 0;
  result.xmm16s(2) = (op1.xmm16s(2) > op2.xmm16s(2)) ? 0xffff : 0;
  result.xmm16s(3) = (op1.xmm16s(3) > op2.xmm16s(3)) ? 0xffff : 0;
  result.xmm16s(4) = (op1.xmm16s(4) > op2.xmm16s(4)) ? 0xffff : 0;
  result.xmm16s(5) = (op1.xmm16s(5) > op2.xmm16s(5)) ? 0xffff : 0;
  result.xmm16s(6) = (op1.xmm16s(6) > op2.xmm16s(6)) ? 0xffff : 0;
  result.xmm16s(7) = (op1.xmm16s(7) > op2.xmm16s(7)) ? 0xffff : 0;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PCMPGTW_VdqWq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 66 */
void BX_CPU_C::PCMPGTD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm32s(0) = (op1.xmm32s(0) > op2.xmm32s(0)) ? 0xffffffff : 0;
  result.xmm32s(1) = (op1.xmm32s(1) > op2.xmm32s(1)) ? 0xffffffff : 0;
  result.xmm32s(2) = (op1.xmm32s(2) > op2.xmm32s(2)) ? 0xffffffff : 0;
  result.xmm32s(3) = (op1.xmm32s(3) > op2.xmm32s(3)) ? 0xffffffff : 0;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PCMPGTD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 67 */
void BX_CPU_C::PACKUSWB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmmubyte(0x0) = SaturateWordSToByteU(op1.xmm16s(0));
  result.xmmubyte(0x1) = SaturateWordSToByteU(op1.xmm16s(1));
  result.xmmubyte(0x2) = SaturateWordSToByteU(op1.xmm16s(2));
  result.xmmubyte(0x3) = SaturateWordSToByteU(op1.xmm16s(3));
  result.xmmubyte(0x4) = SaturateWordSToByteU(op1.xmm16s(4));
  result.xmmubyte(0x5) = SaturateWordSToByteU(op1.xmm16s(5));
  result.xmmubyte(0x6) = SaturateWordSToByteU(op1.xmm16s(6));
  result.xmmubyte(0x7) = SaturateWordSToByteU(op1.xmm16s(7));

  result.xmmubyte(0x8) = SaturateWordSToByteU(op2.xmm16s(0));
  result.xmmubyte(0x9) = SaturateWordSToByteU(op2.xmm16s(1));
  result.xmmubyte(0xA) = SaturateWordSToByteU(op2.xmm16s(2));
  result.xmmubyte(0xB) = SaturateWordSToByteU(op2.xmm16s(3));
  result.xmmubyte(0xC) = SaturateWordSToByteU(op2.xmm16s(4));
  result.xmmubyte(0xD) = SaturateWordSToByteU(op2.xmm16s(5));
  result.xmmubyte(0xE) = SaturateWordSToByteU(op2.xmm16s(6));
  result.xmmubyte(0xF) = SaturateWordSToByteU(op2.xmm16s(7));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PACKUSWB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 6B */
void BX_CPU_C::PACKSSDW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16s(0) = SaturateDwordSToWordS(op1.xmm32s(0));
  result.xmm16s(1) = SaturateDwordSToWordS(op1.xmm32s(1));
  result.xmm16s(2) = SaturateDwordSToWordS(op1.xmm32s(2));
  result.xmm16s(3) = SaturateDwordSToWordS(op1.xmm32s(3));

  result.xmm16s(4) = SaturateDwordSToWordS(op2.xmm32s(0));
  result.xmm16s(5) = SaturateDwordSToWordS(op2.xmm32s(1));
  result.xmm16s(6) = SaturateDwordSToWordS(op2.xmm32s(2));
  result.xmm16s(7) = SaturateDwordSToWordS(op2.xmm32s(3));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PACKSSDW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 74 */
void BX_CPU_C::PCMPEQB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++)
  {
    result.xmmubyte(j) = (op1.xmmubyte(j) == op2.xmmubyte(j)) ? 0xff : 0;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PCMPEQB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 75 */
void BX_CPU_C::PCMPEQW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16u(0) = (op1.xmm16u(0) == op2.xmm16u(0)) ? 0xffff : 0;
  result.xmm16u(1) = (op1.xmm16u(1) == op2.xmm16u(1)) ? 0xffff : 0;
  result.xmm16u(2) = (op1.xmm16u(2) == op2.xmm16u(2)) ? 0xffff : 0;
  result.xmm16u(3) = (op1.xmm16u(3) == op2.xmm16u(3)) ? 0xffff : 0;
  result.xmm16u(4) = (op1.xmm16u(4) == op2.xmm16u(4)) ? 0xffff : 0;
  result.xmm16u(5) = (op1.xmm16u(5) == op2.xmm16u(5)) ? 0xffff : 0;
  result.xmm16u(6) = (op1.xmm16u(6) == op2.xmm16u(6)) ? 0xffff : 0;
  result.xmm16u(7) = (op1.xmm16u(7) == op2.xmm16u(7)) ? 0xffff : 0;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PCMPEQW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 76 */
void BX_CPU_C::PCMPEQD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm32u(0) = (op1.xmm32u(0) == op2.xmm32u(0)) ? 0xffffffff : 0;
  result.xmm32u(1) = (op1.xmm32u(1) == op2.xmm32u(1)) ? 0xffffffff : 0;
  result.xmm32u(2) = (op1.xmm32u(2) == op2.xmm32u(2)) ? 0xffffffff : 0;
  result.xmm32u(3) = (op1.xmm32u(3) == op2.xmm32u(3)) ? 0xffffffff : 0;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PCMPEQD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F C4 */
void BX_CPU_C::PINSRW_VdqEdIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn());
  Bit16u op2;
  Bit8u count = i->Ib() & 0x7;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_16BIT_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_word(i->seg(), RMAddr(i), &op2);
  }

  op1.xmm16u(count) = op2;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PINSRW_VdqEdIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F C5 */
void BX_CPU_C::PEXTRW_VdqEdIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->rm());
  Bit8u count = i->Ib() & 0x7;
  Bit32u result = (Bit32u) op.xmm16u(count);

  BX_WRITE_32BIT_REG(i->nnn(), result);
#else
  BX_INFO(("PEXTRW_VdqEdIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D1 */
void BX_CPU_C::PSRLW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 15)  /* looking only to low 64 bits */
  {
    op1.xmm64u(0) = 0;
    op1.xmm64u(1) = 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    op1.xmm16u(0) >>= shift;
    op1.xmm16u(1) >>= shift;
    op1.xmm16u(2) >>= shift;
    op1.xmm16u(3) >>= shift;
    op1.xmm16u(4) >>= shift;
    op1.xmm16u(5) >>= shift;
    op1.xmm16u(6) >>= shift;
    op1.xmm16u(7) >>= shift;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSRLW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D2 */
void BX_CPU_C::PSRLD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 31)  /* looking only to low 64 bits */
  {
    op1.xmm64u(0) = 0;
    op1.xmm64u(1) = 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    op1.xmm32u(0) >>= shift;
    op1.xmm32u(1) >>= shift;
    op1.xmm32u(2) >>= shift;
    op1.xmm32u(3) >>= shift;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSRLD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D3 */
void BX_CPU_C::PSRLQ_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 63)  /* looking only to low 64 bits */
  {
    op1.xmm64u(0) = 0;
    op1.xmm64u(1) = 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    op1.xmm64u(0) >>= shift;
    op1.xmm64u(1) >>= shift;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSRLQ_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D4 */
void BX_CPU_C::PADDQ_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm64u(0) += op2.xmm64u(0);
  op1.xmm64u(1) += op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PADDQ_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D5 */
void BX_CPU_C::PMULLW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  Bit32u product1 = Bit32u(op1.xmm16u(0)) * Bit32u(op2.xmm16u(0));
  Bit32u product2 = Bit32u(op1.xmm16u(1)) * Bit32u(op2.xmm16u(1));
  Bit32u product3 = Bit32u(op1.xmm16u(2)) * Bit32u(op2.xmm16u(2));
  Bit32u product4 = Bit32u(op1.xmm16u(3)) * Bit32u(op2.xmm16u(3));
  Bit32u product5 = Bit32u(op1.xmm16u(4)) * Bit32u(op2.xmm16u(4));
  Bit32u product6 = Bit32u(op1.xmm16u(5)) * Bit32u(op2.xmm16u(5));
  Bit32u product7 = Bit32u(op1.xmm16u(6)) * Bit32u(op2.xmm16u(6));
  Bit32u product8 = Bit32u(op1.xmm16u(7)) * Bit32u(op2.xmm16u(7));

  result.xmm16u(0) = product1 & 0xffff;
  result.xmm16u(1) = product2 & 0xffff;
  result.xmm16u(2) = product3 & 0xffff;
  result.xmm16u(3) = product4 & 0xffff;
  result.xmm16u(4) = product5 & 0xffff;
  result.xmm16u(5) = product6 & 0xffff;
  result.xmm16u(6) = product7 & 0xffff;
  result.xmm16u(7) = product8 & 0xffff;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PMULLW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D7 */
void BX_CPU_C::PMOVMSKB_GdVRdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->rm());
  Bit32u result = 0;

  if(op.xmmubyte(0x0) & 0x80) result |= 0x0001; 
  if(op.xmmubyte(0x1) & 0x80) result |= 0x0002; 
  if(op.xmmubyte(0x2) & 0x80) result |= 0x0004; 
  if(op.xmmubyte(0x3) & 0x80) result |= 0x0008; 
  if(op.xmmubyte(0x4) & 0x80) result |= 0x0010; 
  if(op.xmmubyte(0x5) & 0x80) result |= 0x0020; 
  if(op.xmmubyte(0x6) & 0x80) result |= 0x0040; 
  if(op.xmmubyte(0x7) & 0x80) result |= 0x0080; 
  if(op.xmmubyte(0x8) & 0x80) result |= 0x0100; 
  if(op.xmmubyte(0x9) & 0x80) result |= 0x0200; 
  if(op.xmmubyte(0xA) & 0x80) result |= 0x0400; 
  if(op.xmmubyte(0xB) & 0x80) result |= 0x0800; 
  if(op.xmmubyte(0xC) & 0x80) result |= 0x1000; 
  if(op.xmmubyte(0xD) & 0x80) result |= 0x2000; 
  if(op.xmmubyte(0xE) & 0x80) result |= 0x4000; 
  if(op.xmmubyte(0xF) & 0x80) result |= 0x8000; 

  /* now write result back to destination */
  BX_WRITE_32BIT_REG(i->nnn(), result);
  
#else
  BX_INFO(("PMOVMSKB_GdVRdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D8 */
void BX_CPU_C::PSUBUSB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm64u(0) = result.xmm64u(1) = 0;

  for(unsigned j=0; j<16; j++) 
  {
      if(op1.xmmubyte(j) > op2.xmmubyte(j)) 
      {
          result.xmmubyte(j) = op1.xmmubyte(j) - op2.xmmubyte(j);
      }
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSUBUSB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D9 */
void BX_CPU_C::PSUBUSW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm64u(0) = result.xmm64u(1) = 0;

  for(unsigned j=0; j<8; j++) 
  {
      if(op1.xmm16u(j) > op2.xmm16u(j)) 
      {
           result.xmm16u(j) = op1.xmm16u(j) - op2.xmm16u(j);
      }
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSUBUSW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F DA */
void BX_CPU_C::PMINUB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    if(op2.xmmubyte(j) < op1.xmmubyte(j)) op1.xmmubyte(j) = op2.xmmubyte(j);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PMINUB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* ANDPS:    0F 54 */
/* ANDPD: 66 0F 54 */
/* PAND:  66 0F DB */

void BX_CPU_C::PAND_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm64u(0) &= op2.xmm64u(0);
  op1.xmm64u(1) &= op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("ANDPS/PD/PAND_VdqWdq: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F DC */
void BX_CPU_C::PADDUSB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    result.xmmubyte(j) = SaturateWordSToByteU(Bit16s(op1.xmmubyte(j)) + Bit16s(op2.xmmubyte(j)));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PADDUSB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F DD */
void BX_CPU_C::PADDUSW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16u(0) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(0)) + Bit32s(op2.xmm16u(0)));
  result.xmm16u(1) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(1)) + Bit32s(op2.xmm16u(1)));
  result.xmm16u(2) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(2)) + Bit32s(op2.xmm16u(2)));
  result.xmm16u(3) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(3)) + Bit32s(op2.xmm16u(3)));
  result.xmm16u(4) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(4)) + Bit32s(op2.xmm16u(4)));
  result.xmm16u(5) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(5)) + Bit32s(op2.xmm16u(5)));
  result.xmm16u(6) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(6)) + Bit32s(op2.xmm16u(6)));
  result.xmm16u(7) = SaturateDwordSToWordU(Bit32s(op1.xmm16u(7)) + Bit32s(op2.xmm16u(7)));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PADDUSW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F DE */
void BX_CPU_C::PMAXUB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    if(op2.xmmubyte(j) > op1.xmmubyte(j)) op1.xmmubyte(j) = op2.xmmubyte(j);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PMAXUB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* ANDNPS:    0F 55 */
/* ANDNPD: 66 0F 55 */
/* PANDN:  66 0F DF */

void BX_CPU_C::PANDN_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm64u(0) = ~(op1.xmm64u(0)) & op2.xmm64u(0);
  result.xmm64u(1) = ~(op1.xmm64u(1)) & op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("ANDNPS/PD/PANDN_VdqWdq: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E0 */
void BX_CPU_C::PAVGB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++)
  {
    result.xmmubyte(j) = (op1.xmmubyte(j) + op2.xmmubyte(j) + 1) >> 1;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PAVGB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E1 */
void BX_CPU_C::PSRAW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 15)  /* looking only to low 64 bits */
  {
    result.xmm16u(0) = (op1.xmm16u(0) & 0x8000) ? 0xffff : 0;
    result.xmm16u(1) = (op1.xmm16u(1) & 0x8000) ? 0xffff : 0;
    result.xmm16u(2) = (op1.xmm16u(2) & 0x8000) ? 0xffff : 0;
    result.xmm16u(3) = (op1.xmm16u(3) & 0x8000) ? 0xffff : 0;
    result.xmm16u(4) = (op1.xmm16u(4) & 0x8000) ? 0xffff : 0;
    result.xmm16u(5) = (op1.xmm16u(5) & 0x8000) ? 0xffff : 0;
    result.xmm16u(6) = (op1.xmm16u(6) & 0x8000) ? 0xffff : 0;
    result.xmm16u(7) = (op1.xmm16u(7) & 0x8000) ? 0xffff : 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    result.xmm16u(0) = op1.xmm16u(0) >> shift;
    result.xmm16u(1) = op1.xmm16u(1) >> shift;
    result.xmm16u(2) = op1.xmm16u(2) >> shift;
    result.xmm16u(3) = op1.xmm16u(3) >> shift;
    result.xmm16u(4) = op1.xmm16u(4) >> shift;
    result.xmm16u(5) = op1.xmm16u(5) >> shift;
    result.xmm16u(6) = op1.xmm16u(6) >> shift;
    result.xmm16u(7) = op1.xmm16u(7) >> shift;

    if(op1.xmm16u(0) & 0x8000) result.xmm16u(0) |= (0xffff << (16 - shift));
    if(op1.xmm16u(1) & 0x8000) result.xmm16u(1) |= (0xffff << (16 - shift));
    if(op1.xmm16u(2) & 0x8000) result.xmm16u(2) |= (0xffff << (16 - shift));
    if(op1.xmm16u(3) & 0x8000) result.xmm16u(3) |= (0xffff << (16 - shift));
    if(op1.xmm16u(4) & 0x8000) result.xmm16u(4) |= (0xffff << (16 - shift));
    if(op1.xmm16u(5) & 0x8000) result.xmm16u(5) |= (0xffff << (16 - shift));
    if(op1.xmm16u(6) & 0x8000) result.xmm16u(6) |= (0xffff << (16 - shift));
    if(op1.xmm16u(7) & 0x8000) result.xmm16u(7) |= (0xffff << (16 - shift));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSRAW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E2 */
void BX_CPU_C::PSRAD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 31)  /* looking only to low 64 bits */
  {
    result.xmm32u(0) = (op1.xmm32u(0) & 0x80000000) ? 0xffffffff : 0;
    result.xmm32u(1) = (op1.xmm32u(1) & 0x80000000) ? 0xffffffff : 0;
    result.xmm32u(2) = (op1.xmm32u(2) & 0x80000000) ? 0xffffffff : 0;
    result.xmm32u(3) = (op1.xmm32u(3) & 0x80000000) ? 0xffffffff : 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    result.xmm32u(0) = op1.xmm32u(0) >> shift;
    result.xmm32u(1) = op1.xmm32u(1) >> shift;
    result.xmm32u(2) = op1.xmm32u(2) >> shift;
    result.xmm32u(3) = op1.xmm32u(3) >> shift;

    if(op1.xmm32u(0) & 0x80000000) result.xmm32u(0) |= (0xffffffff << (32-shift));
    if(op1.xmm32u(1) & 0x80000000) result.xmm32u(1) |= (0xffffffff << (32-shift));
    if(op1.xmm32u(2) & 0x80000000) result.xmm32u(2) |= (0xffffffff << (32-shift));
    if(op1.xmm32u(3) & 0x80000000) result.xmm32u(3) |= (0xffffffff << (32-shift));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSRAD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E3 */
void BX_CPU_C::PAVGW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16u(0) = (op1.xmm16u(0) + op2.xmm16u(0) + 1) >> 1;
  result.xmm16u(1) = (op1.xmm16u(1) + op2.xmm16u(1) + 1) >> 1;
  result.xmm16u(2) = (op1.xmm16u(2) + op2.xmm16u(2) + 1) >> 1;
  result.xmm16u(3) = (op1.xmm16u(3) + op2.xmm16u(3) + 1) >> 1;
  result.xmm16u(4) = (op1.xmm16u(4) + op2.xmm16u(4) + 1) >> 1;
  result.xmm16u(5) = (op1.xmm16u(5) + op2.xmm16u(5) + 1) >> 1;
  result.xmm16u(6) = (op1.xmm16u(6) + op2.xmm16u(6) + 1) >> 1;
  result.xmm16u(7) = (op1.xmm16u(7) + op2.xmm16u(7) + 1) >> 1;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PAVGW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E4 */
void BX_CPU_C::PMULHUW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  Bit32u product1 = Bit32u(op1.xmm16u(0)) * Bit32u(op2.xmm16u(0));
  Bit32u product2 = Bit32u(op1.xmm16u(1)) * Bit32u(op2.xmm16u(1));
  Bit32u product3 = Bit32u(op1.xmm16u(2)) * Bit32u(op2.xmm16u(2));
  Bit32u product4 = Bit32u(op1.xmm16u(3)) * Bit32u(op2.xmm16u(3));
  Bit32u product5 = Bit32u(op1.xmm16u(4)) * Bit32u(op2.xmm16u(4));
  Bit32u product6 = Bit32u(op1.xmm16u(5)) * Bit32u(op2.xmm16u(5));
  Bit32u product7 = Bit32u(op1.xmm16u(6)) * Bit32u(op2.xmm16u(6));
  Bit32u product8 = Bit32u(op1.xmm16u(7)) * Bit32u(op2.xmm16u(7));

  result.xmm16u(0) = (Bit16u)(product1 >> 16);
  result.xmm16u(1) = (Bit16u)(product2 >> 16);
  result.xmm16u(2) = (Bit16u)(product3 >> 16);
  result.xmm16u(3) = (Bit16u)(product4 >> 16);
  result.xmm16u(4) = (Bit16u)(product5 >> 16);
  result.xmm16u(5) = (Bit16u)(product6 >> 16);
  result.xmm16u(6) = (Bit16u)(product7 >> 16);
  result.xmm16u(7) = (Bit16u)(product8 >> 16);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else                                        
  BX_INFO(("PMULHUW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E5 */
void BX_CPU_C::PMULHW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  Bit32s product1 = Bit32s(op1.xmm16s(0)) * Bit32s(op2.xmm16s(0));
  Bit32s product2 = Bit32s(op1.xmm16s(1)) * Bit32s(op2.xmm16s(1));
  Bit32s product3 = Bit32s(op1.xmm16s(2)) * Bit32s(op2.xmm16s(2));
  Bit32s product4 = Bit32s(op1.xmm16s(3)) * Bit32s(op2.xmm16s(3));
  Bit32s product5 = Bit32s(op1.xmm16s(4)) * Bit32s(op2.xmm16s(4));
  Bit32s product6 = Bit32s(op1.xmm16s(5)) * Bit32s(op2.xmm16s(5));
  Bit32s product7 = Bit32s(op1.xmm16s(6)) * Bit32s(op2.xmm16s(6));
  Bit32s product8 = Bit32s(op1.xmm16s(7)) * Bit32s(op2.xmm16s(7));

  result.xmm16u(0) = (Bit16u)(product1 >> 16);
  result.xmm16u(1) = (Bit16u)(product2 >> 16);
  result.xmm16u(2) = (Bit16u)(product3 >> 16);
  result.xmm16u(3) = (Bit16u)(product4 >> 16);
  result.xmm16u(4) = (Bit16u)(product5 >> 16);
  result.xmm16u(5) = (Bit16u)(product6 >> 16);
  result.xmm16u(6) = (Bit16u)(product7 >> 16);
  result.xmm16u(7) = (Bit16u)(product8 >> 16);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PMULHW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E8 */
void BX_CPU_C::PSUBSB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    result.xmmsbyte(j) = SaturateWordSToByteS(Bit16s(op1.xmmsbyte(j)) - Bit16s(op2.xmmsbyte(j)));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSUBSB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F E9 */
void BX_CPU_C::PSUBSW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16s(0) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(0)) - Bit32s(op2.xmm16s(0)));
  result.xmm16s(1) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(1)) - Bit32s(op2.xmm16s(1)));
  result.xmm16s(2) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(2)) - Bit32s(op2.xmm16s(2)));
  result.xmm16s(3) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(3)) - Bit32s(op2.xmm16s(3)));
  result.xmm16s(4) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(4)) - Bit32s(op2.xmm16s(4)));
  result.xmm16s(5) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(5)) - Bit32s(op2.xmm16s(5)));
  result.xmm16s(6) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(6)) - Bit32s(op2.xmm16s(6)));
  result.xmm16s(7) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(7)) - Bit32s(op2.xmm16s(7)));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSUBSW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F EA */
void BX_CPU_C::PMINSW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm16s(0) < op1.xmm16s(0)) op1.xmm16s(0) = op2.xmm16s(0);
  if(op2.xmm16s(1) < op1.xmm16s(1)) op1.xmm16s(1) = op2.xmm16s(1);
  if(op2.xmm16s(2) < op1.xmm16s(2)) op1.xmm16s(2) = op2.xmm16s(2);
  if(op2.xmm16s(3) < op1.xmm16s(3)) op1.xmm16s(3) = op2.xmm16s(3);
  if(op2.xmm16s(4) < op1.xmm16s(4)) op1.xmm16s(4) = op2.xmm16s(4);
  if(op2.xmm16s(5) < op1.xmm16s(5)) op1.xmm16s(5) = op2.xmm16s(5);
  if(op2.xmm16s(6) < op1.xmm16s(6)) op1.xmm16s(6) = op2.xmm16s(6);
  if(op2.xmm16s(7) < op1.xmm16s(7)) op1.xmm16s(7) = op2.xmm16s(7);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PMINSW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* ORPS:    0F 56 */
/* ORPD: 66 0F 56 */
/* POR:  66 0F EB */

void BX_CPU_C::POR_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm64u(0) |= op2.xmm64u(0);
  op1.xmm64u(1) |= op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("ORPS/PD/POR_VdqWdq: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F EC */
void BX_CPU_C::PADDSB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    result.xmmsbyte(j) = SaturateWordSToByteS(Bit16s(op1.xmmsbyte(j)) + Bit16s(op2.xmmsbyte(j)));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PADDSB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F ED */
void BX_CPU_C::PADDSW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm16s(0) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(0)) + Bit32s(op2.xmm16s(0)));
  result.xmm16s(1) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(1)) + Bit32s(op2.xmm16s(1)));
  result.xmm16s(2) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(2)) + Bit32s(op2.xmm16s(2)));
  result.xmm16s(3) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(3)) + Bit32s(op2.xmm16s(3)));
  result.xmm16s(4) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(4)) + Bit32s(op2.xmm16s(4)));
  result.xmm16s(5) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(5)) + Bit32s(op2.xmm16s(5)));
  result.xmm16s(6) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(6)) + Bit32s(op2.xmm16s(6)));
  result.xmm16s(7) = SaturateDwordSToWordS(Bit32s(op1.xmm16s(7)) + Bit32s(op2.xmm16s(7)));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PADDSW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F EE */
void BX_CPU_C::PMAXSW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm16s(0) > op1.xmm16s(0)) op1.xmm16s(0) = op2.xmm16s(0);
  if(op2.xmm16s(1) > op1.xmm16s(1)) op1.xmm16s(1) = op2.xmm16s(1);
  if(op2.xmm16s(2) > op1.xmm16s(2)) op1.xmm16s(2) = op2.xmm16s(2);
  if(op2.xmm16s(3) > op1.xmm16s(3)) op1.xmm16s(3) = op2.xmm16s(3);
  if(op2.xmm16s(4) > op1.xmm16s(4)) op1.xmm16s(4) = op2.xmm16s(4);
  if(op2.xmm16s(5) > op1.xmm16s(5)) op1.xmm16s(5) = op2.xmm16s(5);
  if(op2.xmm16s(6) > op1.xmm16s(6)) op1.xmm16s(6) = op2.xmm16s(6);
  if(op2.xmm16s(7) > op1.xmm16s(7)) op1.xmm16s(7) = op2.xmm16s(7);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PMAXSW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* XORPS:    0F 57 */
/* XORPD: 66 0F 57 */
/* PXOR:  66 0F EF */

void BX_CPU_C::PXOR_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm64u(0) ^= op2.xmm64u(0);
  op1.xmm64u(1) ^= op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("XORPS/PD/PXOR_VdqWdq: SSE not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F1 */
void BX_CPU_C::PSLLW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 15)  /* looking only to low 64 bits */
  {
    op1.xmm64u(0) = 0;
    op1.xmm64u(1) = 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    op1.xmm16u(0) <<= shift;
    op1.xmm16u(1) <<= shift;
    op1.xmm16u(2) <<= shift;
    op1.xmm16u(3) <<= shift;
    op1.xmm16u(4) <<= shift;
    op1.xmm16u(5) <<= shift;
    op1.xmm16u(6) <<= shift;
    op1.xmm16u(7) <<= shift;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSLLW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F2 */
void BX_CPU_C::PSLLD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 31)  /* looking only to low 64 bits */
  {
    op1.xmm64u(0) = 0;
    op1.xmm64u(1) = 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    op1.xmm32u(0) <<= shift;
    op1.xmm32u(1) <<= shift;
    op1.xmm32u(2) <<= shift;
    op1.xmm32u(3) <<= shift;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSLLD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F3 */
void BX_CPU_C::PSLLQ_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  if(op2.xmm64u(0) > 63)  /* looking only to low 64 bits */
  {
    op1.xmm64u(0) = 0;
    op1.xmm64u(1) = 0;
  }
  else
  {
    Bit8u shift = op2.xmmubyte(0);

    op1.xmm64u(0) <<= shift;
    op1.xmm64u(1) <<= shift;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSLLQ_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F4 */
void BX_CPU_C::PMULUDQ_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm64u(0) = Bit64u(op1.xmm32u(0)) * Bit64u(op2.xmm32u(0));
  result.xmm64u(1) = Bit64u(op1.xmm32u(3)) * Bit64u(op2.xmm32u(3));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PMULUDQ_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F5 */
void BX_CPU_C::PMADDWD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<4; j++)
  {
    if(op1.xmm32u(j) == 0x80008000 && op2.xmm32u(j) == 0x80008000) {
      result.xmm32u(j) = 0x80000000;
    }
    else
      result.xmm32u(j) = 
		Bit32s(op1.xmm16s(2*j))   * Bit32s(op2.xmm16s(2*j)) +
		Bit32s(op1.xmm16s(2*j+1)) * Bit32s(op2.xmm16s(2*j+1));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PMADDWD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F6 */
void BX_CPU_C::PSADBW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;
  Bit16u temp1 = 0, temp2 = 0;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  temp1 += abs(op1.xmmubyte(0x0) - op2.xmmubyte(0x0));
  temp1 += abs(op1.xmmubyte(0x1) - op2.xmmubyte(0x1));
  temp1 += abs(op1.xmmubyte(0x2) - op2.xmmubyte(0x2));
  temp1 += abs(op1.xmmubyte(0x3) - op2.xmmubyte(0x3));
  temp1 += abs(op1.xmmubyte(0x4) - op2.xmmubyte(0x4));
  temp1 += abs(op1.xmmubyte(0x5) - op2.xmmubyte(0x5));
  temp1 += abs(op1.xmmubyte(0x6) - op2.xmmubyte(0x6));
  temp1 += abs(op1.xmmubyte(0x7) - op2.xmmubyte(0x7));

  temp2 += abs(op1.xmmubyte(0x8) - op2.xmmubyte(0x8));
  temp2 += abs(op1.xmmubyte(0x9) - op2.xmmubyte(0x9));
  temp2 += abs(op1.xmmubyte(0xA) - op2.xmmubyte(0xA));
  temp2 += abs(op1.xmmubyte(0xB) - op2.xmmubyte(0xB));
  temp2 += abs(op1.xmmubyte(0xC) - op2.xmmubyte(0xC));
  temp2 += abs(op1.xmmubyte(0xD) - op2.xmmubyte(0xD));
  temp2 += abs(op1.xmmubyte(0xE) - op2.xmmubyte(0xE));
  temp2 += abs(op1.xmmubyte(0xF) - op2.xmmubyte(0xF));

  result.xmm64u(0) = Bit64u(temp1);
  result.xmm64u(1) = Bit64u(temp2);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSADBW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F8 */
void BX_CPU_C::PSUBB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    op1.xmmubyte(j) -= op2.xmmubyte(j);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSUBB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F F9 */
void BX_CPU_C::PSUBW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm16u(0) -= op2.xmm16u(0);
  op1.xmm16u(1) -= op2.xmm16u(1);
  op1.xmm16u(2) -= op2.xmm16u(2);
  op1.xmm16u(3) -= op2.xmm16u(3);
  op1.xmm16u(4) -= op2.xmm16u(4);
  op1.xmm16u(5) -= op2.xmm16u(5);
  op1.xmm16u(6) -= op2.xmm16u(6);
  op1.xmm16u(7) -= op2.xmm16u(7);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSUBW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F FA */
void BX_CPU_C::PSUBD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm32u(0) -= op2.xmm32u(0);
  op1.xmm32u(1) -= op2.xmm32u(1);
  op1.xmm32u(2) -= op2.xmm32u(2);
  op1.xmm32u(3) -= op2.xmm32u(3);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSUBD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F FB */
void BX_CPU_C::PSUBQ_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm64u(0) -= op2.xmm64u(0);
  op1.xmm64u(1) -= op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PSUBQ_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F FC */
void BX_CPU_C::PADDB_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  for(unsigned j=0; j<16; j++) 
  {
    op1.xmmubyte(j) += op2.xmmubyte(j);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PADDB_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F FD */
void BX_CPU_C::PADDW_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm16u(0) += op2.xmm16u(0);
  op1.xmm16u(1) += op2.xmm16u(1);
  op1.xmm16u(2) += op2.xmm16u(2);
  op1.xmm16u(3) += op2.xmm16u(3);
  op1.xmm16u(4) += op2.xmm16u(4);
  op1.xmm16u(5) += op2.xmm16u(5);
  op1.xmm16u(6) += op2.xmm16u(6);
  op1.xmm16u(7) += op2.xmm16u(7);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PADDW_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F FE */
void BX_CPU_C::PADDD_VdqWdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  op1.xmm32u(0) += op2.xmm32u(0);
  op1.xmm32u(1) += op2.xmm32u(1);
  op1.xmm32u(2) += op2.xmm32u(2);
  op1.xmm32u(3) += op2.xmm32u(3);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PADDD_VdqWdq: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 71 Grp12 010 */
void BX_CPU_C::PSRLW_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm());
  Bit8u shift = i->Ib();

  op1.xmm16u(0) >>= shift;
  op1.xmm16u(1) >>= shift;
  op1.xmm16u(2) >>= shift;
  op1.xmm16u(3) >>= shift;
  op1.xmm16u(4) >>= shift;
  op1.xmm16u(5) >>= shift;
  op1.xmm16u(6) >>= shift;
  op1.xmm16u(7) >>= shift;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), op1);
#else
  BX_INFO(("PSRLW_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 0F 71 Grp12 100 */
void BX_CPU_C::PSRAW_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm()), result;
  Bit8u shift = i->Ib();

  if(shift > 15) {
    result.xmm16u(0) = (op1.xmm16u(0) & 0x8000) ? 0xffff : 0;
    result.xmm16u(1) = (op1.xmm16u(1) & 0x8000) ? 0xffff : 0;
    result.xmm16u(2) = (op1.xmm16u(2) & 0x8000) ? 0xffff : 0;
    result.xmm16u(3) = (op1.xmm16u(3) & 0x8000) ? 0xffff : 0;
    result.xmm16u(4) = (op1.xmm16u(4) & 0x8000) ? 0xffff : 0;
    result.xmm16u(5) = (op1.xmm16u(5) & 0x8000) ? 0xffff : 0;
    result.xmm16u(6) = (op1.xmm16u(6) & 0x8000) ? 0xffff : 0;
    result.xmm16u(7) = (op1.xmm16u(7) & 0x8000) ? 0xffff : 0;
  }
  else {
    result.xmm16u(0) = op1.xmm16u(0) >> shift;
    result.xmm16u(1) = op1.xmm16u(1) >> shift;
    result.xmm16u(2) = op1.xmm16u(2) >> shift;
    result.xmm16u(3) = op1.xmm16u(3) >> shift;
    result.xmm16u(4) = op1.xmm16u(4) >> shift;
    result.xmm16u(5) = op1.xmm16u(5) >> shift;
    result.xmm16u(6) = op1.xmm16u(6) >> shift;
    result.xmm16u(7) = op1.xmm16u(7) >> shift;

    if(op1.xmm16u(0) & 0x8000) result.xmm16u(0) |= (0xffff << (16 - shift));
    if(op1.xmm16u(1) & 0x8000) result.xmm16u(1) |= (0xffff << (16 - shift));
    if(op1.xmm16u(2) & 0x8000) result.xmm16u(2) |= (0xffff << (16 - shift));
    if(op1.xmm16u(3) & 0x8000) result.xmm16u(3) |= (0xffff << (16 - shift));
    if(op1.xmm16u(4) & 0x8000) result.xmm16u(4) |= (0xffff << (16 - shift));
    if(op1.xmm16u(5) & 0x8000) result.xmm16u(5) |= (0xffff << (16 - shift));
    if(op1.xmm16u(6) & 0x8000) result.xmm16u(6) |= (0xffff << (16 - shift));
    if(op1.xmm16u(7) & 0x8000) result.xmm16u(7) |= (0xffff << (16 - shift));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), result);
#else
  BX_INFO(("PSRAW_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 71 Grp12 110 */
void BX_CPU_C::PSLLW_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm());
  Bit8u shift = i->Ib();

  op1.xmm16u(0) <<= shift;
  op1.xmm16u(1) <<= shift;
  op1.xmm16u(2) <<= shift;
  op1.xmm16u(3) <<= shift;
  op1.xmm16u(4) <<= shift;
  op1.xmm16u(5) <<= shift;
  op1.xmm16u(6) <<= shift;
  op1.xmm16u(7) <<= shift;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), op1);
#else
  BX_INFO(("PSLLW_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 72 Grp13 010 */
void BX_CPU_C::PSRLD_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm());
  Bit8u shift = i->Ib();

  op1.xmm32u(0) >>= shift;
  op1.xmm32u(1) >>= shift;
  op1.xmm32u(2) >>= shift;
  op1.xmm32u(3) >>= shift;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), op1);
#else
  BX_INFO(("PSRLD_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 0F 72 Grp13 100 */
void BX_CPU_C::PSRAD_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm()), result;
  Bit8u shift = i->Ib();

  if(shift > 31) {
    result.xmm32u(0) = (op1.xmm32u(0) & 0x80000000) ? 0xffffffff : 0;
    result.xmm32u(1) = (op1.xmm32u(1) & 0x80000000) ? 0xffffffff : 0;
    result.xmm32u(2) = (op1.xmm32u(2) & 0x80000000) ? 0xffffffff : 0;
    result.xmm32u(3) = (op1.xmm32u(3) & 0x80000000) ? 0xffffffff : 0;
  }
  else {
    result.xmm32u(0) = op1.xmm32u(0) >> shift;
    result.xmm32u(1) = op1.xmm32u(1) >> shift;
    result.xmm32u(2) = op1.xmm32u(2) >> shift;
    result.xmm32u(3) = op1.xmm32u(3) >> shift;

    if(op1.xmm32u(0) & 0x80000000) result.xmm32u(0) |= (0xffffffff << (32-shift));
    if(op1.xmm32u(1) & 0x80000000) result.xmm32u(1) |= (0xffffffff << (32-shift));
    if(op1.xmm32u(2) & 0x80000000) result.xmm32u(2) |= (0xffffffff << (32-shift));
    if(op1.xmm32u(3) & 0x80000000) result.xmm32u(3) |= (0xffffffff << (32-shift));
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), result);
#else
  BX_INFO(("PSRAD_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 72 Grp13 110 */
void BX_CPU_C::PSLLD_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm());
  Bit8u shift = i->Ib();

  op1.xmm32u(0) <<= shift;
  op1.xmm32u(1) <<= shift;
  op1.xmm32u(2) <<= shift;
  op1.xmm32u(3) <<= shift;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), op1);
#else
  BX_INFO(("PSLLD_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 73 Grp14 010 */
void BX_CPU_C::PSRLQ_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm());
  Bit8u shift = i->Ib();

  op1.xmm64u(0) >>= shift;
  op1.xmm64u(1) >>= shift;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), op1);
#else
  BX_INFO(("PSRLQ_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::PSRLDQ_WdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm()), result;
  Bit8u shift = i->Ib();

  result.xmm64u(0) = 0;
  result.xmm64u(1) = 0;

  for(unsigned j=shift; j<16; j++)
  {
    result.xmmubyte(j-shift) = op1.xmmubyte(j);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), result);
#else
  BX_INFO(("PSRLDQ_WdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 73 Grp14 110 */
void BX_CPU_C::PSLLQ_PdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm());
  Bit8u shift = i->Ib();

  op1.xmm64u(0) <<= shift;
  op1.xmm64u(1) <<= shift;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), op1);
#else
  BX_INFO(("PSLLQ_PdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 73 Grp14 111 */
void BX_CPU_C::PSLLDQ_WdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->rm()), result;
  Bit8u shift = i->Ib();

  result.xmm64u(0) = 0;
  result.xmm64u(1) = 0;

  for(unsigned j=shift; j<16; j++)
  {
    result.xmmubyte(j) = op1.xmmubyte(j-shift);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->rm(), result);
#else
  BX_INFO(("PSLLDQ_WdqIb: SSE2 not supported in current configuration"));
  UndefinedOpcode(i);
#endif
}
