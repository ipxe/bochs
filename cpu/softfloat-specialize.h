/*============================================================================
This C source fragment is part of the SoftFloat IEC/IEEE Floating-point
Arithmetic Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

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

typedef int flag;

/*----------------------------------------------------------------------------
| Raises the exceptions specified by `flags'.  Floating-point traps can be
| defined here if desired.  It is currently not possible for such a trap
| to substitute a result value.  If traps are not implemented, this routine
| should be simply `float_exception_flags |= flags;'
*----------------------------------------------------------------------------*/

BX_CPP_INLINE void float_raise(float_status_t &status, int flags)
{
    status.float_exception_flags |= flags;
}

/*----------------------------------------------------------------------------
| Returns current floating point rounding mode specified by status word.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int get_float_rounding_mode(float_status_t &status)
{
    return status.float_rounding_mode;
}

/*----------------------------------------------------------------------------
| Returns current floating point NaN operands handling mode specified 
| by status word.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int get_float_nan_handling_mode(float_status_t &status)
{
    return status.float_nan_handling_mode;
}

/*----------------------------------------------------------------------------
| Returns 1 if the <denormals-are-zeroes> feature is supported;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

BX_CPP_INLINE int get_DAZ(float_status_t &status)
{
    return status.denormals_are_zeroes;
}

/*----------------------------------------------------------------------------
| Internal canonical NaN format.
*----------------------------------------------------------------------------*/

typedef struct {
    flag sign;
    Bit64u hi, lo;
} commonNaNT;

/*----------------------------------------------------------------------------
| The pattern for a default generated single-precision NaN.
*----------------------------------------------------------------------------*/
#define float32_default_nan 0xFFC00000
/*        in another version
#define float32_default_nan 0x7FFFFFFF
*/

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a zero;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

flag float32_is_zero(float32 a)
{
    return ! ((Bit32u)(a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

flag float32_is_nan(float32 a)
{
    return (0xFF000000 < (Bit32u) (a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the single-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

flag float32_is_signaling_nan(float32 a)
{
    return (((a>>22) & 0x1FF) == 0x1FE) && (a & 0x003FFFFF);
}

/*----------------------------------------------------------------------------
| Returns the result of converting the single-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

static commonNaNT float32ToCommonNaN(float32 a, float_status_t &status)
{
    commonNaNT z;

    if (float32_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = a>>31;
    z.lo = 0;
    z.hi = ((Bit64u) a)<<41;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the single-
| precision floating-point format.
*----------------------------------------------------------------------------*/

static float32 commonNaNToFloat32(commonNaNT a)
{
    return (((Bit32u) a.sign)<<31) | 0x7FC00000 | (a.hi>>41);
}

/*----------------------------------------------------------------------------
| Takes two single-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

static float32 propagateFloat32NaN(float32 a, float32 b, float_status_t &status)
{
    flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

    aIsNaN = float32_is_nan(a);
    aIsSignalingNaN = float32_is_signaling_nan(a);
    bIsNaN = float32_is_nan(b);
    bIsSignalingNaN = float32_is_signaling_nan(b);
    a |= 0x00400000;
    b |= 0x00400000;
    if (aIsSignalingNaN | bIsSignalingNaN) float_raise(status, float_flag_invalid);
    if (get_float_nan_handling_mode(status) == float_larger_significand_nan) {
        if (aIsSignalingNaN) {
            if (bIsSignalingNaN) goto returnLargerSignificand;
            return bIsNaN ? b : a;
        }
        else if (aIsNaN) {
            if (bIsSignalingNaN | ! bIsNaN) return a;
      returnLargerSignificand:
            if ((Bit32u) (a<<1) < (Bit32u) (b<<1)) return b;
            if ((Bit32u) (b<<1) < (Bit32u) (a<<1)) return a;
            return (a < b) ? a : b;
        }
        else {
            return b;
        }
    } else {
        return (aIsSignalingNaN | aIsNaN) ? a : b;
    }
}

/*----------------------------------------------------------------------------
| The pattern for a default generated double-precision NaN.
*----------------------------------------------------------------------------*/
#define float64_default_nan BX_CONST64(0xFFF8000000000000)
/*                in another version
#define float64_default_nan BX_CONST64(0x7FFFFFFFFFFFFFFF)
*/

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a zero;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

flag float64_is_zero(float64 a)
{
    return ! ((Bit64u)(a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a NaN;
| otherwise returns 0.
*----------------------------------------------------------------------------*/

flag float64_is_nan(float64 a)
{
    return (BX_CONST64(0xFFE0000000000000) < (Bit64u) (a<<1));
}

/*----------------------------------------------------------------------------
| Returns 1 if the double-precision floating-point value `a' is a signaling
| NaN; otherwise returns 0.
*----------------------------------------------------------------------------*/

flag float64_is_signaling_nan(float64 a)
{
    return (((a>>51) & 0xFFF) == 0xFFE) && (a & BX_CONST64(0x0007FFFFFFFFFFFF));
}

/*----------------------------------------------------------------------------
| Returns the result of converting the double-precision floating-point NaN
| `a' to the canonical NaN format.  If `a' is a signaling NaN, the invalid
| exception is raised.
*----------------------------------------------------------------------------*/

static commonNaNT float64ToCommonNaN(float64 a, float_status_t &status)
{
    commonNaNT z;

    if (float64_is_signaling_nan(a)) float_raise(status, float_flag_invalid);
    z.sign = a>>63;
    z.lo = 0;
    z.hi = a<<12;
    return z;
}

/*----------------------------------------------------------------------------
| Returns the result of converting the canonical NaN `a' to the double-
| precision floating-point format.
*----------------------------------------------------------------------------*/

static float64 commonNaNToFloat64(commonNaNT a)
{
    return (((Bit64u) a.sign)<<63) | BX_CONST64(0x7FF8000000000000) | (a.hi>>12);
}

/*----------------------------------------------------------------------------
| Takes two double-precision floating-point values `a' and `b', one of which
| is a NaN, and returns the appropriate NaN result.  If either `a' or `b' is a
| signaling NaN, the invalid exception is raised.
*----------------------------------------------------------------------------*/

static float64 propagateFloat64NaN(float64 a, float64 b, float_status_t &status)
{
    flag aIsNaN, aIsSignalingNaN, bIsNaN, bIsSignalingNaN;

    aIsNaN = float64_is_nan(a);
    aIsSignalingNaN = float64_is_signaling_nan(a);
    bIsNaN = float64_is_nan(b);
    bIsSignalingNaN = float64_is_signaling_nan(b);
    a |= BX_CONST64(0x0008000000000000);
    b |= BX_CONST64(0x0008000000000000);
    if (aIsSignalingNaN | bIsSignalingNaN) float_raise(status, float_flag_invalid);
    if (get_float_nan_handling_mode(status) == float_larger_significand_nan) {
        if (aIsSignalingNaN) {
            if (bIsSignalingNaN) goto returnLargerSignificand;
            return bIsNaN ? b : a;
        }
        else if (aIsNaN) {
            if (bIsSignalingNaN | ! bIsNaN) return a;
      returnLargerSignificand:
            if ((Bit64u) (a<<1) < (Bit64u) (b<<1)) return b;
            if ((Bit64u) (b<<1) < (Bit64u) (a<<1)) return a;
            return (a < b) ? a : b;
        }
        else {
            return b;
        }
    } else {
        return (aIsSignalingNaN | aIsNaN) ? a : b;
    }
}
