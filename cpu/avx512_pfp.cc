/////////////////////////////////////////////////////////////////////////
// $Id$
/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2013 Stanislav Shwartsman
//          Written by Stanislav Shwartsman [sshwarts at sourceforge net]
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
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA B 02110-1301 USA
//
/////////////////////////////////////////////////////////////////////////

#define NEED_CPU_REG_SHORTCUTS 1
#include "bochs.h"
#include "cpu.h"
#define LOG_THIS BX_CPU_THIS_PTR

#if BX_SUPPORT_EVEX

extern void mxcsr_to_softfloat_status_word(float_status_t &status, bx_mxcsr_t mxcsr);

#include "fpu/softfloat-compare.h"
#include "simd_int.h"
#include "simd_pfp.h"

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VADDPS_MASK_VpsHpsWpsR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_16BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 4) {
    xmm_addps_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 4)
      xmm_blendps(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VADDPD_MASK_VpdHpdWpdR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 2) {
    xmm_addpd_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 2)
      xmm_blendpd(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VADDSS_MASK_VssHpsWssR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float32 op2 = BX_READ_XMM_REG_LO_DWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm32u(0) = float32_add(op1.xmm32u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm32u(0) = 0;
    else
      op1.xmm32u(0) = BX_READ_XMM_REG_LO_DWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VADDSD_MASK_VsdHpdWsdR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float64 op2 = BX_READ_XMM_REG_LO_QWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm64u(0) = float64_add(op1.xmm64u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm64u(0) = 0;
    else
      op1.xmm64u(0) = BX_READ_XMM_REG_LO_QWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VSUBPS_MASK_VpsHpsWpsR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_16BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 4) {
    xmm_subps_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 4)
      xmm_blendps(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VSUBPD_MASK_VpdHpdWpdR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 2) {
    xmm_subpd_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 2)
      xmm_blendpd(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VSUBSS_MASK_VssHpsWssR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float32 op2 = BX_READ_XMM_REG_LO_DWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm32u(0) = float32_sub(op1.xmm32u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm32u(0) = 0;
    else
      op1.xmm32u(0) = BX_READ_XMM_REG_LO_DWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VSUBSD_MASK_VsdHpdWsdR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float64 op2 = BX_READ_XMM_REG_LO_QWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm64u(0) = float64_sub(op1.xmm64u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm64u(0) = 0;
    else
      op1.xmm64u(0) = BX_READ_XMM_REG_LO_QWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMULPS_MASK_VpsHpsWpsR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_16BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 4) {
    xmm_mulps_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 4)
      xmm_blendps(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMULPD_MASK_VpdHpdWpdR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 2) {
    xmm_mulpd_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 2)
      xmm_blendpd(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMULSS_MASK_VssHpsWssR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float32 op2 = BX_READ_XMM_REG_LO_DWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm32u(0) = float32_mul(op1.xmm32u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm32u(0) = 0;
    else
      op1.xmm32u(0) = BX_READ_XMM_REG_LO_DWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMULSD_MASK_VsdHpdWsdR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float64 op2 = BX_READ_XMM_REG_LO_QWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm64u(0) = float64_mul(op1.xmm64u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm64u(0) = 0;
    else
      op1.xmm64u(0) = BX_READ_XMM_REG_LO_QWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VDIVPS_MASK_VpsHpsWpsR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_16BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 4) {
    xmm_divps_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 4)
      xmm_blendps(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VDIVPD_MASK_VpdHpdWpdR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 2) {
    xmm_divpd_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 2)
      xmm_blendpd(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VDIVSS_MASK_VssHpsWssR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float32 op2 = BX_READ_XMM_REG_LO_DWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm32u(0) = float32_div(op1.xmm32u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm32u(0) = 0;
    else
      op1.xmm32u(0) = BX_READ_XMM_REG_LO_DWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VDIVSD_MASK_VsdHpdWsdR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float64 op2 = BX_READ_XMM_REG_LO_QWORD(i->src2());

    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);

    op1.xmm64u(0) = float64_div(op1.xmm64u(0), op2, status);

    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm64u(0) = 0;
    else
      op1.xmm64u(0) = BX_READ_XMM_REG_LO_QWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMINPS_MASK_VpsHpsWpsR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_16BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 4) {
    xmm_minps_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 4)
      xmm_blendps(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMINPD_MASK_VpdHpdWpdR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 2) {
    xmm_minpd_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 2)
      xmm_blendpd(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMINSS_MASK_VssHpsWssR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float32 op2 = BX_READ_XMM_REG_LO_DWORD(i->src2());
    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);
    op1.xmm32u(0) = float32_min(op1.xmm32u(0), op2, status);
    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm32u(0) = 0;
    else
      op1.xmm32u(0) = BX_READ_XMM_REG_LO_DWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMINSD_MASK_VsdHpdWsdR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float64 op2 = BX_READ_XMM_REG_LO_QWORD(i->src2());
    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);
    op1.xmm64u(0) = float64_min(op1.xmm64u(0), op2, status);
    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm64u(0) = 0;
    else
      op1.xmm64u(0) = BX_READ_XMM_REG_LO_QWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMAXPS_MASK_VpsHpsWpsR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_16BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 4) {
    xmm_maxps_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 4)
      xmm_blendps(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMAXPD_MASK_VpdHpdWpdR(bxInstruction_c *i)
{
  BxPackedAvxRegister op1 = BX_READ_AVX_REG(i->src1()), op2 = BX_READ_AVX_REG(i->src2());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());
  unsigned len = i->getVL();

  float_status_t status;
  mxcsr_to_softfloat_status_word(status, MXCSR);
  evex_softfloat_status_word_override(status, i, len);

  for (unsigned n=0, tmp_mask = mask; n < len; n++, tmp_mask >>= 2) {
    xmm_maxpd_mask(&op1.vmm128(n), &op2.vmm128(n), status, tmp_mask);
  }

  check_exceptionsSSE(status.float_exception_flags);

  if (! i->isZeroMasking()) {
    for (unsigned n=0; n < len; n++, mask >>= 2)
      xmm_blendpd(&op1.vmm128(n), &BX_READ_AVX_REG_LANE(i->dst(), n), ~mask);
  }

  BX_WRITE_AVX_REGZ(i->dst(), op1, len);
  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMAXSS_MASK_VssHpsWssR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float32 op2 = BX_READ_XMM_REG_LO_DWORD(i->src2());
    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);
    op1.xmm32u(0) = float32_max(op1.xmm32u(0), op2, status);
    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm32u(0) = 0;
    else
      op1.xmm32u(0) = BX_READ_XMM_REG_LO_DWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

BX_INSF_TYPE BX_CPP_AttrRegparmN(1) BX_CPU_C::VMAXSD_MASK_VsdHpdWsdR(bxInstruction_c *i)
{
  BxPackedXmmRegister op1 = BX_READ_XMM_REG(i->src1());
  unsigned mask = BX_READ_8BIT_OPMASK(i->opmask());

  if (mask & 0x1) {
    float64 op2 = BX_READ_XMM_REG_LO_QWORD(i->src2());
    float_status_t status;
    mxcsr_to_softfloat_status_word(status, MXCSR);
    evex_softfloat_status_word_override(status, i, BX_VL512);
    op1.xmm64u(0) = float64_max(op1.xmm64u(0), op2, status);
    check_exceptionsSSE(status.float_exception_flags);
  }
  else {
    if (i->isZeroMasking())
      op1.xmm64u(0) = 0;
    else
      op1.xmm64u(0) = BX_READ_XMM_REG_LO_QWORD(i->dst());
  }

  BX_WRITE_XMM_REG_CLEAR_HIGH(i->dst(), op1);

  BX_NEXT_INSTR(i);
}

#endif
