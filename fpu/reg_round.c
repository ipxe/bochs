/*---------------------------------------------------------------------------+
 |  reg_round.c                                                              |
 |  $Id: reg_round.c,v 1.8.8.3 2004/03/26 21:36:09 sshwarts Exp $
 |                                                                           |
 | Rounding/truncation/etc for FPU basic arithmetic functions.               |
 |                                                                           |
 | Copyright (C) 1993,1995,1997,1999                                         |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail billm@melbpc.org.au              |
 |                                                                           |
 | This code has four possible entry points.                                 |
 | The following must be entered by a jmp instruction:                       |
 |   fpu_reg_round, fpu_reg_round_sqrt, and fpu_Arith_exit.                  |
 |                                                                           |
 | The FPU_round entry point is intended to be used by C code.               |
 |                                                                           |
 |    Return value is the tag of the answer, or-ed with FPU_Exception if     |
 |    one was raised, or -1 on internal error.                               |
 |                                                                           |
 | For correct "up" and "down" rounding, the argument must have the correct  |
 | sign.                                                                     |
 |                                                                           |
 +---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------+
 |                                                                           |
 | The significand and its extension are assumed to be exact in the          |
 | following sense:                                                          |
 |   If the significand by itself is the exact result then the significand   |
 |   extension (%edx) must contain 0, otherwise the significand extension    |
 |   must be non-zero.                                                       |
 |   If the significand extension is non-zero then the significand is        |
 |   smaller than the magnitude of the correct exact result by an amount     |
 |   greater than zero and less than one ls bit of the significand.          |
 |   The significand extension is only required to have three possible       |
 |   non-zero values:                                                        |
 |       less than 0x80000000  <=> the significand is less than 1/2 an ls    |
 |                                 bit smaller than the magnitude of the     |
 |                                 true exact result.                        |
 |         exactly 0x80000000  <=> the significand is exactly 1/2 an ls bit  |
 |                                 smaller than the magnitude of the true    |
 |                                 exact result.                             |
 |    greater than 0x80000000  <=> the significand is more than 1/2 an ls    |
 |                                 bit smaller than the magnitude of the     |
 |                                 true exact result.                        |
 |                                                                           |
 +---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------+
 |  The code in this module has become quite complex, but it should handle   |
 |  all of the FPU flags which are set at this stage of the basic arithmetic |
 |  computations.                                                            |
 |  There are a few rare cases where the results are not set identically to  |
 |  a real FPU. These require a bit more thought because at this stage the   |
 |  results of the code here appear to be more consistent...                 |
 |  This may be changed in a future version.                                 |
 +---------------------------------------------------------------------------*/


#include "fpu_emu.h"
#include "status_w.h"
#include "exception.h"
#include "control_w.h"

/* Flags for FPU_bits_lost */
#define	LOST_DOWN	1
#define	LOST_UP		2

/* Flags for FPU_denormal */
#define	DENORMAL	1
#define	UNMASKED_UNDERFLOW 2


static int round_up_64(FPU_REG *x)
{
  x->sigl ++;
  if (x->sigl == 0)
    {
      x->sigh ++;
      if (x->sigh == 0)
	{
	  x->sigh = 0x80000000;
	  x->exp ++;
	}
    }
  return LOST_UP;
}

static int truncate_64(FPU_REG *x)
{
  return LOST_DOWN;
}

static int round_up_53(FPU_REG *x)
{
  x->sigl &= 0xfffff800;
  x->sigl += 0x800;
  if (x->sigl == 0)
    {
      x->sigh ++;
      if (x->sigh == 0)
	{
      x->sigh = 0x80000000;
      x->exp ++;
      /* if the above increment of a signed 16-bit value overflowed, substitute
      the maximum positive exponent to force FPU_round to produce overflow */
      if (x->exp == 0xFFFF8000) 
          x->exp = 0x7FFF;
	}
    }
  return LOST_UP;
}

static int truncate_53(FPU_REG *x)
{
  x->sigl &= 0xfffff800;
  return LOST_DOWN;
}

static int round_up_24(FPU_REG *x)
{
  x->sigl = 0;
  x->sigh &= 0xffffff00;
  x->sigh += 0x100;
  if (x->sigh == 0)
    {
      x->sigh = 0x80000000;
      x->exp ++;
      /* if the above increment of a signed 16-bit value overflowed, substitute
      the maximum positive exponent to force FPU_round to produce overflow */
      if (x->exp == 0xFFFF8000) 
          x->exp = 0x7FFF;
    }
  return LOST_UP;
}

static int truncate_24(FPU_REG *x)
{
  x->sigl = 0;
  x->sigh &= 0xffffff00;
  return LOST_DOWN;
}

int FPU_round(FPU_REG *x, u32 extent, u16 control_w, u8 sign)
{
  u64 work;
  u32 leading;
  s16 expon = x->exp;
  int FPU_bits_lost = 0, FPU_denormal, shift, tag, inexact = 0;

  if (expon <= EXP_UNDER)
    {
      /* A denormal or zero */
      if (control_w & FPU_CW_Underflow)
	{
	  /* Underflow is masked. */
	  FPU_denormal = DENORMAL;
	  shift = EXP_UNDER+1 - expon;
	  if (shift >= 64)
	    {
	      if (shift == 64)
		{
		  x->exp += 64;
		  if (extent | x->sigl)
		    extent = x->sigh | 1;
		  else
		    extent = x->sigh;
		}
	      else
		{
		  x->exp = EXP_UNDER+1;
		  extent = 1;
		}
	      significand(x) = 0;
	    }
	  else
	    {
	      x->exp += shift;
	      if (shift >= 32)
		{
		  shift -= 32;
		  if (shift)
		    {
		      extent |= x->sigl;
		      work = significand(x) >> shift;
		      if (extent)
			extent = work | 1;
		      else
			extent = work;
		      x->sigl = x->sigh >>= shift;
		    }
		  else
		    {
		      if (extent)
			extent = x->sigl | 1;
		      else
			extent = x->sigl;
		      x->sigl = x->sigh;
		    }
		  x->sigh = 0;
		}
	      else
		{
		  /* Shift by 1 to 32 places. */
		  work = x->sigl;
		  work <<= 32;
		  work |= extent;
		  work >>= shift;
		  if (extent)
		    extent = 1;
		  extent |= work;
		  significand(x) >>= shift;
		}
	    }
	}
      else
	{
	  /* Unmasked underflow. */
	  FPU_denormal = UNMASKED_UNDERFLOW;
	}
    }
  else
    FPU_denormal = 0;

  switch (control_w & FPU_CW_PC)
    {
    case 01:
#ifndef PECULIAR_486
      /* With the precision control bits set to 01 "(reserved)", a real 80486
	 behaves as if the precision control bits were set to 11 "64 bits" */
#ifdef PARANOID
	INTERNAL(0x236);
	return -1;
#endif
#endif
	/* Fall through to the 64 bit case. */
    case FPU_PR_80_BITS:
      if (extent)
	{
	  switch (control_w & FPU_CW_RC)
	    {
	    case FPU_RC_RND:		/* Nearest or even */
	      /* See if there is exactly half a ulp. */
	      if (extent == 0x80000000)
		{
		  /* Round to even. */
		  if (x->sigl & 0x1)
		    /* Odd */
		    FPU_bits_lost = round_up_64(x);
		  else
		    /* Even */
		    FPU_bits_lost = truncate_64(x);
		}
	      else if (extent > 0x80000000)
		{
		  /* Greater than half */
		  FPU_bits_lost = round_up_64(x);
		}
	      else
		{
		  /* Less than half */
		  FPU_bits_lost = truncate_64(x);
		}
	      break;

	    case FPU_RC_CHOP:		/* Truncate */
	      FPU_bits_lost = truncate_64(x);
	      break;
	      
	    case FPU_RC_UP:		/* Towards +infinity */
	      if (sign == SIGN_POS)
		{
		  FPU_bits_lost = round_up_64(x);
		}
	      else
		{
		  FPU_bits_lost = truncate_64(x);
		}
	      break;

	    case FPU_RC_DOWN:		/* Towards -infinity */
	      if (sign != SIGN_POS)
		{
		  FPU_bits_lost = round_up_64(x);
		}
	      else
		{
		  FPU_bits_lost = truncate_64(x);
		}
	      break;

	    default:
	      INTERNAL(0x231);
	      return -1;
	    }
	}
      break;

    case FPU_PR_64_BITS:
      leading = x->sigl & 0x7ff;
      if (extent || leading)
	{
	  switch (control_w & FPU_CW_RC)
	    {
	    case FPU_RC_RND:		/* Nearest or even */
	      /* See if there is exactly half a ulp. */
	      if (leading == 0x400)
		{
		  if (extent == 0)
		    {
		      /* Round to even. */
		      if (x->sigl & 0x800)
			/* Odd */
			FPU_bits_lost = round_up_53(x);
		      else
			/* Even */
			FPU_bits_lost = truncate_53(x);
		    }
		  else
		    {
		      /* Greater than half */
		      FPU_bits_lost = round_up_53(x);
		    }
		}
	      else if (leading > 0x400)
		{
		  /* Greater than half */
		  FPU_bits_lost = round_up_53(x);
		}
	      else
		{
		  /* Less than half */
		  FPU_bits_lost = truncate_53(x);
		}
	      break;

	    case FPU_RC_CHOP:		/* Truncate */
	      FPU_bits_lost = truncate_53(x);
	      break;

	    case FPU_RC_UP:		/* Towards +infinity */
	      if (sign == SIGN_POS)
		{
		  FPU_bits_lost = round_up_53(x);
		}
	      else
		{
		  FPU_bits_lost = truncate_53(x);
		}
	      break;

	    case FPU_RC_DOWN:		/* Towards -infinity */
	      if (sign != SIGN_POS)
		{
		  FPU_bits_lost = round_up_53(x);
		}
	      else
		{
		  FPU_bits_lost = truncate_53(x);
		}
	      break;

	    default:
	      INTERNAL(0x231);
	      return -1;
	    }
	}
      inexact = FPU_bits_lost;
      FPU_bits_lost = 0;
      break;

    case FPU_PR_32_BITS:
      leading = x->sigh & 0xff;
      if (leading || x->sigl || extent)
	{
	  switch (control_w & FPU_CW_RC)
	    {
	    case FPU_RC_RND:		/* Nearest or even */
	      /* See if there is exactly half a ulp. */
	      if (leading == 0x80)
		{
		  if ((x->sigl == 0) && (extent == 0))
		    {
		      /* Round to even. */
		      if (x->sigh & 0x100)
			/* Odd */
			FPU_bits_lost = round_up_24(x);
		      else
			/* Even */
			FPU_bits_lost = truncate_24(x);
		    }
		  else
		    {
		      /* Greater than half */
		      FPU_bits_lost = round_up_24(x);
		    }
		}
	      else if (leading > 0x80)
		{
		  /* Greater than half */
		  FPU_bits_lost = round_up_24(x);
		}
	      else
		{
		  /* Less than half */
		  FPU_bits_lost = truncate_24(x);
		}
	      break;

	    case FPU_RC_CHOP:		/* Truncate */
	      FPU_bits_lost = truncate_24(x);
	      break;

	    case FPU_RC_UP:		/* Towards +infinity */
	      if (sign == SIGN_POS)
		{
		  FPU_bits_lost = round_up_24(x);
		}
	      else
		{
		  FPU_bits_lost = truncate_24(x);
		}
	      break;

	    case FPU_RC_DOWN:		/* Towards -infinity */
	      if (sign != SIGN_POS)
		{
		  FPU_bits_lost = round_up_24(x);
		}
	      else
		{
		  FPU_bits_lost = truncate_24(x);
		}
	      break;

	    default:
	      INTERNAL(0x231);
	      return -1;
	    }
	}
      inexact = FPU_bits_lost;
      FPU_bits_lost = 0;
      break;

    default:
#ifdef PARANOID
	INTERNAL(0x230);
	return -1;
#endif
      break;
    }

  tag = TAG_Valid;

  if (FPU_denormal)
    {
      /* Undo the de-normalisation. */
      if (FPU_denormal == UNMASKED_UNDERFLOW)
	{
	  if (x->exp <= EXP_UNDER)
	    {
	      /* Increase the exponent by the magic number */
	      x->exp += 3 * (1 << 13);
	      EXCEPTION(EX_Underflow);
	    }
	}
      else
	{
	  if (x->exp != EXP_UNDER+1)
	    {
	      INTERNAL(0x234);
	    }
	  if ((x->sigh == 0) && (x->sigl == 0))
	    {
	      /* Underflow to zero */
	      set_precision_flag_down();
	      EXCEPTION(EX_Underflow);
	      x->exp = EXP_UNDER;
	      tag = TAG_Zero;
	      FPU_bits_lost = 0;  /* Stop another call to
				     set_precision_flag_down() */
	    }
	  else
	    {
	      if (x->sigh & 0x80000000)
		{
#ifdef PECULIAR_486
	/*
	 * This implements a special feature of 80486 behaviour.
	 * Underflow will be signalled even if the number is
	 * not a denormal after rounding.
	 * This difference occurs only for masked underflow, and not
	 * in the unmasked case.
	 * Actual 80486 behaviour differs from this in some circumstances.
	 */
	      /* Will be masked underflow */
#else
	      /* No longer a denormal */
#endif
		}
	      else
#ifndef PECULIAR_486
		{
#endif
		x->exp --;

	      if (FPU_bits_lost  && (x->sigh & 0x80000000) == 0)
		{
		  /* There must be a masked underflow */
		  EXCEPTION(EX_Underflow);
		}

              if (inexact && x->exp < -16382)
                {
                  /* There must be a masked underflow */
                  EXCEPTION(EX_Underflow);
                }
        
	      tag = TAG_Special;
#ifndef PECULIAR_486
		}
#endif
	    }
	}
    }


  if (FPU_bits_lost == LOST_UP || inexact == LOST_UP)
     set_precision_flag_up();
       else   if (FPU_bits_lost == LOST_DOWN || inexact == LOST_DOWN)
     set_precision_flag_down();

  if (x->exp >= EXP_OVER)
    {
      x->exp += EXTENDED_Ebias;
      tag = arith_round_overflow(x, sign);
    }
  else
    {
      x->exp += EXTENDED_Ebias;
      x->exp &= 0x7fff;
    }

  if (sign != SIGN_POS)
    x->exp |= 0x8000;

  return tag;
}

/*===========================================================================*/

/* r gets mangled such that sig is int, sign: 
   it is NOT normalized */
/* The return value (in eax) is zero if the result is exact,
   if bits are changed due to rounding, truncation, etc, then
   a non-zero value is returned */
/* Overflow is signalled by a non-zero return value (in eax).
   In the case of overflow, the returned significand always has the
   largest possible value */
int  BX_CPP_AttrRegparmN(2)
FPU_round_to_int(FPU_REG *r, u_char tag)
{
  u_char     very_big;
  unsigned eax;

  if (tag == TAG_Zero)
    {
      /* Make sure that zero is returned */
      significand(r) = 0;
      return 0;        /* o.k. */
    }

  if (exponent(r) > 63)
    {
      r->sigl = r->sigh = ~0;      /* The largest representable number */
      return 1;        /* overflow */
    }

#ifndef EMU_BIG_ENDIAN
  eax = FPU_shrxs(&r->sigl, 63 - exponent(r));
#else
  eax = FPU_shrxs(&r->sigh, 63 - exponent(r));
#endif
  very_big = !(~(r->sigh) | ~(r->sigl));  /* test for 0xfff...fff */
#define	half_or_more	(eax & 0x80000000)
#define	frac_part	(eax)
#define more_than_half  (eax > 0x80000000)
  switch (FPU_control_word & FPU_CW_RC)
    {
    case FPU_RC_RND:
      if (more_than_half               	/* nearest */
	  || (half_or_more && (r->sigl & 1)))	/* odd -> even */
	{
	  if (very_big) return 1;        /* overflow */
	  significand(r) ++;
	  return PRECISION_LOST_UP;
	}
      break;
    case FPU_RC_DOWN:
      if (frac_part && getsign(r))
	{
	  if (very_big) return 1;        /* overflow */
	  significand(r) ++;
	  return PRECISION_LOST_UP;
	}
      break;
    case FPU_RC_UP:
      if (frac_part && !getsign(r))
	{
	  if (very_big) return 1;        /* overflow */
	  significand(r) ++;
	  return PRECISION_LOST_UP;
	}
      break;
    case FPU_RC_CHOP:
      break;
    }

  return eax ? PRECISION_LOST_DOWN : 0;
}
