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

/* ************************************ */
/* SSE: SAVE/RESTORE FPU/MMX/SSEx STATE */
/* ************************************ */

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
  BX_INFO(("LDMXCSR: required SSE, use --enable-sse option"));
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
  BX_INFO(("STMXCSR: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 0F AE Grp15 000 */
void BX_CPU_C::FXSAVE(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BxPackedXmmRegister xmm;
  Bit16u twd = BX_CPU_THIS_PTR the_i387.soft.twd, tag_byte = 0;
  Bit16u status_w = BX_CPU_THIS_PTR the_i387.soft.swd;
  Bit16u tos = BX_CPU_THIS_PTR the_i387.soft.tos;
  unsigned index;

  BX_INFO(("FXSAVE: save FPU/MMX/SSE state"));

#define SW_TOP (0x3800)

  xmm.xmm16u(0) = (BX_CPU_THIS_PTR the_i387.soft.cwd);
  xmm.xmm16u(1) = (status_w & ~SW_TOP & 0xffff) | ((tos << 11) & SW_TOP);

  if(twd & 0x0003 != 0x0003) tag_byte |= 0x0100;
  if(twd & 0x000c != 0x000c) tag_byte |= 0x0200;
  if(twd & 0x0030 != 0x0030) tag_byte |= 0x0400;
  if(twd & 0x00c0 != 0x00c0) tag_byte |= 0x0800;
  if(twd & 0x0300 != 0x0300) tag_byte |= 0x1000;
  if(twd & 0x0c00 != 0x0c00) tag_byte |= 0x2000;
  if(twd & 0x3000 != 0x3000) tag_byte |= 0x4000;
  if(twd & 0xc000 != 0xc000) tag_byte |= 0x8000;

  xmm.xmm16u(2) = tag_byte;

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
    writeVirtualDQwordAligned(i->seg(), 
           RMAddr(i)+index*16+32, (Bit8u *) &(BX_FPU_REG(index)));
  }

  /* store XMM register file */
  for(index=0; index < BX_XMM_REGISTERS; index++)
  {
    writeVirtualDQwordAligned(i->seg(), 
           RMAddr(i)+index*16+160, (Bit8u *) &(BX_CPU_THIS_PTR xmm[index]));
  }

  /* do not touch reserved fields */
#else
  BX_INFO(("FXSAVE: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 0F AE Grp15 001 */
void BX_CPU_C::FXRSTOR(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BxPackedXmmRegister xmm;
  Bit32u tag_byte, tag_byte_mask, twd = 0;
  unsigned index;

  BX_INFO(("FXRSTOR: restore FPU/MMX/SSE state"));

  readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &xmm);
  
  BX_CPU_THIS_PTR the_i387.soft.cwd = xmm.xmm16u(0);
  BX_CPU_THIS_PTR the_i387.soft.swd = xmm.xmm16u(1);
  BX_CPU_THIS_PTR the_i387.soft.tos = (xmm.xmm16u(1) >> 11) & 0x07;

  /* FOO/FPU IP restore still not implemented */

  tag_byte = xmm.xmm16u(2);

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
    readVirtualDQwordAligned(i->seg(), 
           RMAddr(i)+index*16+32, (Bit8u *) &(BX_FPU_REG(index)));
  }

  /*                                 FTW
   *
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
     |    0     |    1     |    0     |    1x    |     1     | S    10 |
     -------------------------------------------------------------------
     |    0     |    1     |    1     |    00    |     1     | Z    01 |
     |    0     |    1     |    1     |    10    |     1     | S    10 |
     -------------------------------------------------------------------
     |    1     |    0     |    0     |    1x    |     1     | S    10 |
     |    1     |    0     |    0     |    1x    |     1     | S    10 |
     -------------------------------------------------------------------
     |    1     |    0     |    1     |    00    |     1     | S    10 |
     |    1     |    0     |    1     |    10    |     1     | S    10 |
     -------------------------------------------------------------------
     |        all combinations above             |     0     | E    11 |

   *
   * The J-bit is defined to be the 1-bit binary integer to the left of
   * the decimal place in the significand.
   * 
   * The M-bit is defined to be the most significant bit of the fractional 
   * portion of the significand (i.e., the bit immediately to the right of 
   * the decimal place). When the M-bit is the most significant bit of the 
   * fractional portion  of the significand, it must be  0 if the fraction 
   * is all 0's.  
   */

  tag_byte_mask = 0x0100;

#define FPU_TAG_VALID   0x00
#define FPU_TAG_ZERO    0x01
#define FPU_TAG_SPECIAL 0x02
#define FPU_TAG_EMPTY   0x03

  for(index = 0;index < 8; index++, twd <<= 2, tag_byte_mask <<= 1)
  {
      if(tag_byte & tag_byte_mask) {
          BxFpuRegister *fpu_reg = (BxFpuRegister *) &(BX_FPU_REG(index));

          if (fpu_reg->exp == 0) {
              if(!(fpu_reg->sigl | fpu_reg->sigh)) twd |= FPU_TAG_ZERO;
              else twd |= FPU_TAG_SPECIAL; 
          }
          else if (fpu_reg->exp == 0x7fff) twd |= FPU_TAG_SPECIAL;
          else if (fpu_reg->sigh & 0x80000000) twd |= FPU_TAG_VALID;
          else twd |= FPU_TAG_SPECIAL;
      }
      else {
         twd |= FPU_TAG_EMPTY;
      }
  }

  BX_CPU_THIS_PTR the_i387.soft.twd = (twd >> 2);

  /* If the OSFXSR bit in CR4 is not set, the FXRSTOR instruction does
     not restore the states of the XMM and MXCSR registers. */
  if(! (BX_CPU_THIS_PTR cr4.get_OSFXSR())) 
  {
    /* load XMM register file */
    for(index=0; index < BX_XMM_REGISTERS; index++)
    {
      readVirtualDQwordAligned(i->seg(), 
           RMAddr(i)+index*16+160, (Bit8u *) &(BX_CPU_THIS_PTR xmm[index]));
    }
  }

#else
  BX_INFO(("FXRSTOR: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* *************************** */
/* SSE: MEMORY MOVE OPERATIONS */
/* *************************** */

/* All these opcodes never generate SIMD floating point exeptions */

/* MOVUPS:    0F 10 */
/* MOVUPD: 66 0F 10 */
/* MOVDQU: F3 0F 6F */

void BX_CPU_C::MOVUPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op;

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQword(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op);
#else
  BX_INFO(("MOVUPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVUPS:    0F 11 */
/* MOVUPD: 66 0F 11 */
/* MOVDQU: F3 0F 7F */

void BX_CPU_C::MOVUPS_WpsVps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());

  /* op is a register or memory reference */
  if (i->modC0()) {
    BX_WRITE_XMM_REG(i->rm(), op);
  }
  else {
    writeVirtualDQword(i->seg(), RMAddr(i), (Bit8u *) &op);
  }
#else
  BX_INFO(("MOVUPS_WpsVps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVAPS:    0F 28 */
/* MOVAPD: 66 0F 28 */
/* MOVDQA: F3 0F 6F */

void BX_CPU_C::MOVAPS_VpsWps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op;

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op);
#else
  BX_INFO(("MOVAPS_VpsWps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVAPS:    0F 29 */
/* MOVAPD: 66 0F 29 */
/* MOVDQA: F3 0F 7F */

void BX_CPU_C::MOVAPS_WpsVps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());

  /* op is a register or memory reference */
  if (i->modC0()) {
    BX_WRITE_XMM_REG(i->rm(), op);
  }
  else {
    writeVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op);
  }
#else
  BX_INFO(("MOVAPS_WpsVps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F3 0F 10 */
void BX_CPU_C::MOVSS_VssWss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;
  Bit32u val32;

  /* op2 is a register or memory reference */
  if (i->modC0()) 
  {
    op2 = BX_READ_XMM_REG(i->rm());

    /* If the source operand is an XMM register, the high-order 
            96 bits of the destination XMM register are not modified. */
    op1.xmm32u(0) = op2.xmm32u(0);
  }
  else {
    /* pointer, segment address pair */
    read_virtual_dword(i->seg(), RMAddr(i), &val32);

    /* If the source operand is a memory location, the high-order
            96 bits of the destination XMM register are cleared to 0s */
    op1.xmm64u(0) = (Bit64u)(val32);
    op1.xmm64u(1) = 0;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("MOVSS_VssWss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F3 0F 11 */
void BX_CPU_C::MOVSS_WssVss(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) 
  {
    op2 = BX_READ_XMM_REG(i->rm());

    /* If the source operand is an XMM register, the high-order 
            96 bits of the destination XMM register are not modified. */
    op2.xmm32u(0) = op1.xmm32u(0);

    /* now write result back to destination */
    BX_WRITE_XMM_REG(i->rm(), op2);
  }
  else {
    /* pointer, segment address pair */
    write_virtual_dword(i->seg(), RMAddr(i), &(op1.xmm32u(0)));
  }
#else
  BX_INFO(("MOVSS_WssVss: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F2 0F 10 */
void BX_CPU_C::MOVSD_VsdWsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;
  Bit64u val64;

  /* op2 is a register or memory reference */
  if (i->modC0()) 
  {
    op2 = BX_READ_XMM_REG(i->rm());

    /* If the source operand is an XMM register, the high-order 
            64 bits of the destination XMM register are not modified. */
    op1.xmm64u(0) = op2.xmm64u(0);
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &val64);

    /* If the source operand is a memory location, the high-order
            64 bits of the destination XMM register are cleared to 0s */
    op1.xmm64u(0) = val64;
    op1.xmm64u(1) = 0;
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("MOVSD_VsdWsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F2 0F 11 */
void BX_CPU_C::MOVSD_WsdVsd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2;

  /* op2 is a register or memory reference */
  if (i->modC0()) 
  {
    op2 = BX_READ_XMM_REG(i->rm());

    /* If the source operand is an XMM register, the high-order 
            64 bits of the destination XMM register are not modified. */
    op2.xmm64u(0) = op1.xmm64u(0);

    /* now write result back to destination */
    BX_WRITE_XMM_REG(i->rm(), op2);
  }
  else {
    /* pointer, segment address pair */
    write_virtual_qword(i->seg(), RMAddr(i), &(op1.xmm64u(0)));
  }
#else
  BX_INFO(("MOVSD_WsdVsd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVLPS:    0F 12 */
/* MOVLPD: 66 0F 12 */

void BX_CPU_C::MOVLPS_VpsMq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn());
  Bit64u v64;

  if (i->modC0()) /* MOVHLPS xmm1, xmm2 opcode */
  {
    BxPackedXmmRegister op2 = BX_READ_XMM_REG(i->rm());
    v64 = op2.xmm64u(1); 
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &v64);
  }

  /* The high-order 64 bits of the destination 
                          XMM register are not modified. */
  op1.xmm64u(0) = v64;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("MOVLPS_VpsMq: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVLPS:    0F 13 */
/* MOVLPD: 66 0F 13 */

void BX_CPU_C::MOVLPS_MqVps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  if (i->modC0()) 
  {
    BX_INFO(("MOVLPS_MqVps: must be memory reference"));
    UndefinedOpcode(i);
  }

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());
  Bit64u v64 = op.xmm64u(0);
  write_virtual_qword(i->seg(), RMAddr(i), &v64);

#else
  BX_INFO(("MOVLPS_MqVps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVHPS:    0F 16 */
/* MOVHPD: 66 0F 16 */

void BX_CPU_C::MOVHPS_VpsMq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn());
  Bit64u v64;

  if (i->modC0()) /* MOVLHPS xmm1, xmm2 opcode */
  {
    BxPackedXmmRegister op2 = BX_READ_XMM_REG(i->rm());
    v64 = op2.xmm64u(0);
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &v64);
  }

  /* The low-order 64 bits of the destination 
                          XMM register are not modified. */
  op1.xmm64u(1) = v64;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("MOVHPS_VpsMq: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVHPS:    0F 17 */
/* MOVHPD: 66 0F 17 */

void BX_CPU_C::MOVHPS_MqVps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  if (i->modC0()) 
  {
    BX_INFO(("MOVHPS_MqVps: must be memory reference"));
    UndefinedOpcode(i);
  }

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());
  Bit64u v64 = op.xmm64u(1);
  write_virtual_qword(i->seg(), RMAddr(i), &v64);

#else
  BX_INFO(("MOVHPS_MqVps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MASKMOVQ_PqPRq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_PANIC(("MASKMOVQ_PqPRq: SSE instruction still not implemented"));
#else
  BX_INFO(("MASKMOVQ_PqPRq: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

void BX_CPU_C::MASKMOVDQU_VdqVRdq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BX_PANIC(("MASKMOVDQU_VdqVRdq: SSE2 instruction still not implemented"));
#else
  BX_INFO(("MASKMOVDQU_VdqVRdq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 0F 50 */
void BX_CPU_C::MOVMSKPS_GdVRps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());
  Bit32u val32 = 0;

  if(op.xmm32u(0) & 0x80000000) val32 |= 0x1;
  if(op.xmm32u(1) & 0x80000000) val32 |= 0x2;
  if(op.xmm32u(2) & 0x80000000) val32 |= 0x4;
  if(op.xmm32u(3) & 0x80000000) val32 |= 0x8;

  BX_WRITE_32BIT_REG(i->rm(), val32);
#else
  BX_INFO(("MOVMSKPS_GdVRps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 50 */
void BX_CPU_C::MOVMSKPD_EdVRpd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());
  Bit32u val32 = 0;

  if(op.xmm32u(1) & 0x80000000) val32 |= 0x1;
  if(op.xmm32u(3) & 0x80000000) val32 |= 0x2;
 
  BX_WRITE_32BIT_REG(i->rm(), val32);
#else
  BX_INFO(("MOVMSKPD_EdVRpd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 6E */
void BX_CPU_C::MOVD_VdqEd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1;
  op1.xmm64u(1) = 0;

#if BX_SUPPORT_X86_64
  if (i->os64L())  /* 64 bit operand size mode */
  {
    Bit64u op2;

    /* op2 is a register or memory reference */
    if (i->modC0()) {
      op2 = BX_READ_64BIT_REG(i->rm());
    }
    else {
      /* pointer, segment address pair */
      read_virtual_qword(i->seg(), RMAddr(i), &op2);
    }

    op1.xmm64u(0) = op2;
  }
  else
#endif
  {
    Bit32u op2;

    /* op2 is a register or memory reference */
    if (i->modC0()) {
      op2 = BX_READ_32BIT_REG(i->rm());
    }
    else {
      /* pointer, segment address pair */
      read_virtual_dword(i->seg(), RMAddr(i), &op2);
    }

    op1.xmm64u(0) = (Bit64u)(op2);
  }

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("MOVD_VdqEd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 7E */
void BX_CPU_C::MOVD_EdVd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn());

#if BX_SUPPORT_X86_64
  if (i->os64L())  /* 64 bit operand size mode */
  {
    Bit64u op2 = op1.xmm64u(0);

    /* destination is a register or memory reference */
    if (i->modC0()) {
      BX_WRITE_64BIT_REG(i->rm(), op2);
    }
    else {
      /* pointer, segment address pair */
      write_virtual_qword(i->seg(), RMAddr(i), &op2);
    }
  }
  else
#endif
  {
    Bit32u op2 = op1.xmm32u(0);

    /* destination is a register or memory reference */
    if (i->modC0()) {
      BX_WRITE_32BIT_REG(i->rm(), op2);
    }
    else {
      /* pointer, segment address pair */
      write_virtual_dword(i->seg(), RMAddr(i), &op2);
    }
  }

  BX_INFO(("MOVD_EdVd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F3 0F 7E */
void BX_CPU_C::MOVQ_VqWq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op;
  Bit64u val64;

  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    read_virtual_qword(i->seg(), RMAddr(i), &val64);
    op.xmm64u(0) = val64;
  }

  /* zero-extension to 128 bits */
  op.xmm64u(1) = 0;

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op);
#else
  BX_INFO(("MOVQ_VqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F D6 */
void BX_CPU_C::MOVQ_WqVq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());

  if (i->modC0()) 
  {
    op.xmm64u(1) = 0; /* zero-extension to 128 bits */
    BX_WRITE_XMM_REG(i->rm(), op);
  }
  else {
    write_virtual_qword(i->seg(), RMAddr(i), &(op.xmm64u(0)));
  }
#else
  BX_INFO(("MOVQ_WqVq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F2 0F D6 */
void BX_CPU_C::MOVDQ2Q_PqVRq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op = BX_READ_XMM_REG(i->nnn());
  BxPackedMmxRegister mm;
  MMXUQ(mm) = op.xmm64u(0);

  FPU_TWD  = 0;
  FPU_TOS  = 0;        /* Each time an MMX instruction is */
  FPU_SWD &= 0xc7ff;   /*      executed, the TOS value is set to 000b */

  BX_WRITE_MMX_REG(i->rm(), mm);
#else
  BX_INFO(("MOVDQ2Q_PqVRq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F3 0F D6 */
void BX_CPU_C::MOVQ2DQ_VdqQq(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op;
  BxPackedMmxRegister mm = BX_READ_MMX_REG(i->nnn());
  op.xmm64u(0) = MMXUQ(mm);
  op.xmm64u(1) = 0;

  FPU_TWD  = 0;
  FPU_TOS  = 0;        /* Each time an MMX instruction is */
  FPU_SWD &= 0xc7ff;   /*      executed, the TOS value is set to 000b */

  BX_WRITE_XMM_REG(i->rm(), op);
#else
  BX_INFO(("MOVQ2DQ_VdqQq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* ****************************** */
/* SSE: MEMORY SHUFFLE OPERATIONS */
/* ****************************** */

/* 0F C6 */
void BX_CPU_C::SHUFPS_VpsWpsIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;
  Bit8u order = i->Ib();

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm32u(0) = op1.xmm32u((order >> 0) & 0x3);
  result.xmm32u(1) = op1.xmm32u((order >> 2) & 0x3);
  result.xmm32u(2) = op2.xmm32u((order >> 4) & 0x3);
  result.xmm32u(3) = op2.xmm32u((order >> 6) & 0x3);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("SHUFPS_VpsWpsIb: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F C6 */
void BX_CPU_C::SHUFPD_VpdWpdIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->nnn()), op2, result;
  Bit8u order = i->Ib();

  /* op2 is a register or memory reference */
  if (i->modC0()) {
    op2 = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQwordAligned(i->seg(), RMAddr(i), (Bit8u *) &op2);
  }

  result.xmm64u(0) = op1.xmm64u((order >> 0) & 0x1);
  result.xmm64u(1) = op2.xmm64u((order >> 1) & 0x1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("SHUFPD_VpdWpdIb: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 60 */
void BX_CPU_C::PUNPCKLBW_VdqWq(bxInstruction_c *i)
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

  result.xmmubyte(0x0) = op1.xmmubyte(0);
  result.xmmubyte(0x1) = op2.xmmubyte(0);
  result.xmmubyte(0x2) = op1.xmmubyte(1);
  result.xmmubyte(0x3) = op2.xmmubyte(1);
  result.xmmubyte(0x4) = op1.xmmubyte(2);
  result.xmmubyte(0x5) = op2.xmmubyte(2);
  result.xmmubyte(0x6) = op1.xmmubyte(3);
  result.xmmubyte(0x7) = op2.xmmubyte(3);
  result.xmmubyte(0x8) = op1.xmmubyte(4);
  result.xmmubyte(0x9) = op2.xmmubyte(4);
  result.xmmubyte(0xA) = op1.xmmubyte(5);
  result.xmmubyte(0xB) = op2.xmmubyte(5);
  result.xmmubyte(0xC) = op1.xmmubyte(6);
  result.xmmubyte(0xD) = op2.xmmubyte(6);
  result.xmmubyte(0xE) = op1.xmmubyte(7);
  result.xmmubyte(0xF) = op2.xmmubyte(7);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKLBW_VdqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 61 */
void BX_CPU_C::PUNPCKLWD_VdqWq(bxInstruction_c *i)
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

  result.xmm16u(0) = op1.xmm16u(0);
  result.xmm16u(1) = op2.xmm16u(0);
  result.xmm16u(2) = op1.xmm16u(1);
  result.xmm16u(3) = op2.xmm16u(1);
  result.xmm16u(4) = op1.xmm16u(2);
  result.xmm16u(5) = op2.xmm16u(2);
  result.xmm16u(6) = op1.xmm16u(3);
  result.xmm16u(7) = op2.xmm16u(3);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKLWD_VdqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* UNPCKLPS:     0F 14 */
/* PUNPCKLDQ: 66 0F 62 */

void BX_CPU_C::PUNPCKLDQ_VdqWq(bxInstruction_c *i)
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

  result.xmm32u(0) = op1.xmm32u(0);
  result.xmm32u(1) = op2.xmm32u(0);
  result.xmm32u(2) = op1.xmm32u(1);
  result.xmm32u(3) = op2.xmm32u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKLDQ_VdqWq: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 68 */
void BX_CPU_C::PUNPCKHBW_VdqWq(bxInstruction_c *i)
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

  result.xmmubyte(0x0) = op1.xmmubyte(0x8);
  result.xmmubyte(0x1) = op2.xmmubyte(0x8);
  result.xmmubyte(0x2) = op1.xmmubyte(0x9);
  result.xmmubyte(0x3) = op2.xmmubyte(0x9);
  result.xmmubyte(0x4) = op1.xmmubyte(0xA);
  result.xmmubyte(0x5) = op2.xmmubyte(0xA);
  result.xmmubyte(0x6) = op1.xmmubyte(0xB);
  result.xmmubyte(0x7) = op2.xmmubyte(0xB);
  result.xmmubyte(0x8) = op1.xmmubyte(0xC);
  result.xmmubyte(0x9) = op2.xmmubyte(0xC);
  result.xmmubyte(0xA) = op1.xmmubyte(0xD);
  result.xmmubyte(0xB) = op2.xmmubyte(0xD);
  result.xmmubyte(0xC) = op1.xmmubyte(0xE);
  result.xmmubyte(0xD) = op2.xmmubyte(0xE);
  result.xmmubyte(0xE) = op1.xmmubyte(0xF);
  result.xmmubyte(0xF) = op2.xmmubyte(0xF);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKHBW_VdqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 69 */
void BX_CPU_C::PUNPCKHWD_VdqWq(bxInstruction_c *i)
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

  result.xmm16u(0) = op1.xmm16u(4);
  result.xmm16u(1) = op2.xmm16u(4);
  result.xmm16u(2) = op1.xmm16u(5);
  result.xmm16u(3) = op2.xmm16u(5);
  result.xmm16u(4) = op1.xmm16u(6);
  result.xmm16u(5) = op2.xmm16u(6);
  result.xmm16u(6) = op1.xmm16u(7);
  result.xmm16u(7) = op2.xmm16u(7);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKHWD_VdqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* UNPCKHPS:     0F 15 */
/* PUNPCKHDQ: 66 0F 6A */

void BX_CPU_C::PUNPCKHDQ_VdqWq(bxInstruction_c *i)
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

  result.xmm32u(0) = op1.xmm32u(2);
  result.xmm32u(1) = op2.xmm32u(2);
  result.xmm32u(2) = op1.xmm32u(3);
  result.xmm32u(3) = op2.xmm32u(3);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKHDQ_VdqWq: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* UNPCKLPD:   66 0F 14 */
/* PUNPCKLQDQ: 66 0F 6C */

void BX_CPU_C::PUNPCKLQDQ_VdqWq(bxInstruction_c *i)
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

  op1.xmm64u(1) = op2.xmm64u(0);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), op1);
#else
  BX_INFO(("PUNPCKLQDQ_VdqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* UNPCKHPD:   66 0F 15 */
/* PUNPCKHQDQ: 66 0F 6D */

void BX_CPU_C::PUNPCKHQDQ_VdqWq(bxInstruction_c *i)
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

  result.xmm64u(0) = op1.xmm64u(1);
  result.xmm64u(1) = op2.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PUNPCKHQDQ_VdqWq: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* 66 0F 70 */
void BX_CPU_C::PSHUFD_VdqWdqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op, result;
  Bit8u order = i->Ib();

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQword(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  result.xmm32u(0) = op.xmm32u((order >> 0) & 0x3);
  result.xmm32u(1) = op.xmm32u((order >> 2) & 0x3);
  result.xmm32u(2) = op.xmm32u((order >> 4) & 0x3);
  result.xmm32u(3) = op.xmm32u((order >> 6) & 0x3);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSHUFD_VdqWdqIb: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F2 0F 70 */
void BX_CPU_C::PSHUFHW_VqWqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op, result;
  Bit8u order = i->Ib();

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQword(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  result.xmm64u(0) = op.xmm64u(0);
  result.xmm16u(4) = op.xmm16u(4 + ((order >> 0) & 0x3));
  result.xmm16u(5) = op.xmm16u(4 + ((order >> 2) & 0x3));
  result.xmm16u(6) = op.xmm16u(4 + ((order >> 4) & 0x3));
  result.xmm16u(7) = op.xmm16u(4 + ((order >> 6) & 0x3));

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSHUFHW_VqWqIb: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* F3 0F 70 */
void BX_CPU_C::PSHUFLW_VqWqIb(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2
  BX_CPU_THIS_PTR prepareSSE();

  BxPackedXmmRegister op, result;
  Bit8u order = i->Ib();

  /* op is a register or memory reference */
  if (i->modC0()) {
    op = BX_READ_XMM_REG(i->rm());
  }
  else {
    /* pointer, segment address pair */
    readVirtualDQword(i->seg(), RMAddr(i), (Bit8u *) &op);
  }

  result.xmm16u(0) = op.xmm16u((order >> 0) & 0x3);
  result.xmm16u(1) = op.xmm16u((order >> 2) & 0x3);
  result.xmm16u(2) = op.xmm16u((order >> 4) & 0x3);
  result.xmm16u(3) = op.xmm16u((order >> 6) & 0x3);
  result.xmm64u(1) = op.xmm64u(1);

  /* now write result back to destination */
  BX_WRITE_XMM_REG(i->nnn(), result);
#else
  BX_INFO(("PSHUFLW_VqWqIb: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* **************************** */
/* SSE: STORE DATA NON-TEMPORAL */
/* **************************** */

/* 0F C3 */
void BX_CPU_C::MOVNTI_MdGd(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 2

  if (i->modC0()) {
    BX_INFO(("MOVNTI_MdGd: must be memory reference"));
    UndefinedOpcode(i);
  }

#if BX_SUPPORT_X86_64 
  if (i->os64L())   /* 64 bit operand size mode */
  {
    Bit64u val64 = BX_READ_64BIT_REG(i->nnn());
    write_virtual_qword(i->seg(), RMAddr(i), &val64);
  }
  else 
#endif
  {
    Bit32u val32 = BX_READ_32BIT_REG(i->nnn());
    write_virtual_dword(i->seg(), RMAddr(i), &val32);
  }

#else
  BX_INFO(("MOVNTI_MdGd: required SSE2, use --enable-sse option"));
  UndefinedOpcode(i);
#endif
}

/* MOVNTPS:    0F 2B */
/* MOVNTPD: 66 0F 2B */
/* MOVNTDQ: 66 0F E7 */

void BX_CPU_C::MOVNTPS_MdqVps(bxInstruction_c *i)
{
#if BX_SUPPORT_SSE >= 1
  BX_CPU_THIS_PTR prepareSSE();

  if (i->modC0()) {
    BX_INFO(("MOVNTPS_MdqVps: must be memory reference"));
    UndefinedOpcode(i);
  }

  BxPackedXmmRegister val128 = BX_READ_XMM_REG(i->nnn());
  writeVirtualDQword(i->seg(), RMAddr(i), (Bit8u *)(&val128));
#else
  BX_INFO(("MOVNTPS_MdqVps: required SSE, use --enable-sse option"));
  UndefinedOpcode(i);                      
#endif
}
