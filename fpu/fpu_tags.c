/*---------------------------------------------------------------------------+
 |  fpu_tags.c                                                               |
 |  $Id: fpu_tags.c,v 1.7.6.2 2004-03-27 20:09:52 sshwarts Exp $
 |                                                                           |
 |  Set FPU register tags.                                                   |
 |                                                                           |
 | Copyright (C) 1997                                                        |
 |                  W. Metzenthen, 22 Parker St, Ormond, Vic 3163, Australia |
 |                  E-mail   billm@jacobi.maths.monash.edu.au                |
 |                                                                           |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#include "fpu_emu.h"
#include "fpu_system.h"
#include "exception.h"

void FPU_pop(void)
{
  FPU_tag_word |= 3 << ((FPU_tos & 7)*2);
  FPU_tos++;
}

int FPU_gettag0(void)
{
  return FPU_gettag(FPU_tos);
}

int BX_CPP_AttrRegparmN(1) FPU_gettagi(int stnr)
{
  return FPU_gettag(FPU_tos+stnr);
}

int BX_CPP_AttrRegparmN(1) FPU_gettag(int regnr)
{
  return (FPU_tag_word >> ((regnr & 7)*2)) & 3;
}

void BX_CPP_AttrRegparmN(1) FPU_settag0(int tag)
{
  FPU_settag(FPU_tos, tag);
}

void BX_CPP_AttrRegparmN(2) FPU_settagi(int stnr, int tag)
{
  FPU_settag(stnr+FPU_tos, tag);
}

void BX_CPP_AttrRegparmN(2) FPU_settag(int regnr, int tag)
{
  regnr &= 7;
  FPU_tag_word &= ~(3 << (regnr*2));
  FPU_tag_word |= (tag & 3) << (regnr*2);
}

int BX_CPP_AttrRegparmN(1) FPU_Special(FPU_REG const *ptr)
{
  int exp = exponent(ptr);

  if ( exp == EXP_BIAS+EXP_UNDER )
    return TW_Denormal;
  else if ( exp != EXP_BIAS+EXP_OVER )
    return TW_NaN;
  else if ( (ptr->sigh == 0x80000000) && (ptr->sigl == 0) )
    return TW_Infinity;
  return TW_NaN;
}

int BX_CPP_AttrRegparmN(1) isNaN(FPU_REG const *ptr)
{
  return ((exponent(ptr) == EXP_BIAS+EXP_OVER)
	   && !((ptr->sigh == 0x80000000) && (ptr->sigl == 0)));
}

int FPU_stackoverflow(FPU_REG **st_new_ptr)
{
  *st_new_ptr = &st(-1);
  return ((FPU_tag_word >> (((FPU_tos - 1) & 7)*2)) & 3) != TAG_Empty;
}

void  BX_CPP_AttrRegparmN(3) FPU_copy_to_regi(FPU_REG const *r, u_char tag, int stnr)
{
  reg_copy(r, &st(stnr));
  FPU_settagi(stnr, tag);
}

void  BX_CPP_AttrRegparmN(2) FPU_copy_to_reg1(FPU_REG const *r, u_char tag)
{
  reg_copy(r, &st(1));
  FPU_settagi(1, tag);
}

void  BX_CPP_AttrRegparmN(2) FPU_copy_to_reg0(FPU_REG const *r, u_char tag)
{
  reg_copy(r, &st(0));
  FPU_settagi(0, tag);
}

int BX_CPP_AttrRegparmN(1) FPU_tagof(FPU_REG *reg)
{
  int exp = exponent16(reg) & 0x7fff;
  if (exp == 0)
    {
      if (!(reg->sigh | reg->sigl))
	{
	  return TAG_Zero;
	}
      /* The number is a de-normal or pseudodenormal. */
      return TAG_Special;
    }

  if (exp == 0x7fff)
    {
      /* Is an Infinity, a NaN, or an unsupported data type. */
      return TAG_Special;
    }

  if (!(reg->sigh & 0x80000000))
    {
      /* Unsupported data type. */
      /* Valid numbers have the ms bit set to 1. */
      /* Unnormal. */
      return TAG_Special;
    }

  return TAG_Valid;
}
