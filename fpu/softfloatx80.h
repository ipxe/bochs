/*============================================================================
This source file is an extension to the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b, written for Bochs (x86 achitecture simulator)
floating point emulation.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.
=============================================================================*/

/*============================================================================
 * Adapted for Bochs (x86 achitecture simulator) by
 *            Stanislav Shwartsman (gate@fidonet.org.il)
 * ==========================================================================*/ 

/*============================================================================
 * Unlike original softfloat library code, the functions included in this
 * source package  fully handle unsupported encodings for extended double
 * precision floating point values.
 * ==========================================================================*/ 

#ifndef SOFTFLOATX80_EXTENSIONS_H
#define SOFTFLOATX80_EXTENSIONS_H

#include "softfloat.h"
#include "softfloat-specialize.h"

/*----------------------------------------------------------------------------
| Software IEC/IEEE integer-to-floating-point conversion routines.
*----------------------------------------------------------------------------*/

Bit16s floatx80_to_int16(floatx80, float_status_t &status);
Bit16s floatx80_to_int16_round_to_zero(floatx80, float_status_t &status);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision operations.
*----------------------------------------------------------------------------*/

floatx80 floatx80_extract(floatx80 &a, float_status_t &status);
float_class_t floatx80_class(floatx80);

/*----------------------------------------------------------------------------
| Software IEC/IEEE extended double-precision compare.
*----------------------------------------------------------------------------*/

int floatx80_compare(floatx80, floatx80, float_status_t &status);
int floatx80_compare_quiet(floatx80, floatx80, float_status_t &status);

/*-----------------------------------------------------------------------------
| Calculates the absolute value of the extended double-precision floating-point
| value `a'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80& floatx80_abs(floatx80 &reg)
{
    reg.exp &= 0x7FFF;
    return reg;   
}

/*-----------------------------------------------------------------------------
| Changes the sign of the extended double-precision floating-point value 'a'.
| The operation is performed according to the IEC/IEEE Standard for Binary 
| Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE floatx80& floatx80_chs(floatx80 &reg)
{
    reg.exp ^= 0x8000;
    return reg;   
}

/*-----------------------------------------------------------------------------
| Commonly used extended double-precision floating-point constants.
*----------------------------------------------------------------------------*/

extern const floatx80 Const_QNaN;
extern const floatx80 Const_Z;
extern const floatx80 Const_1;
extern const floatx80 Const_L2T;
extern const floatx80 Const_L2E;
extern const floatx80 Const_PI;
extern const floatx80 Const_PI2;
extern const floatx80 Const_PI4;
extern const floatx80 Const_LG2;
extern const floatx80 Const_LN2;
extern const floatx80 Const_INF;

#endif
