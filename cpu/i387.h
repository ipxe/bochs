/////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2003 Stanislav Shwartsman
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


#ifndef BX_I387_RELATED_EXTENSIONS_H
#define BX_I387_RELATED_EXTENSIONS_H

#if BX_SUPPORT_FPU

// Endian  Host byte order         Guest (x86) byte order
// ======================================================
// Little  FFFFFFFFEEAAAAAA        FFFFFFFFEEAAAAAA
// Big     AAAAAAEEFFFFFFFF        FFFFFFFFEEAAAAAA
//
// Legend: F - fraction/mmx
//         E - exponent
//         A - alignment

#ifdef BX_BIG_ENDIAN
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=mac68k
#endif
struct bx_fpu_reg_t {
  Bit16u alignment1, alignment2, alignment3;
  Bit16s exp;   /* Signed quantity used in internal arithmetic. */
  Bit32u sigh;
  Bit32u sigl;
} GCC_ATTRIBUTE((aligned(16), packed));
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=reset
#endif
#else
struct bx_fpu_reg_t {
  Bit32u sigl;
  Bit32u sigh;
  Bit16s exp;   /* Signed quantity used in internal arithmetic. */
  Bit16u alignment1, alignment2, alignment3;
} GCC_ATTRIBUTE((aligned(16), packed));
#endif

typedef struct bx_fpu_reg_t FPU_REG;

/* Tag Word */
#define FPU_Tag_Valid   0x00
#define FPU_Tag_Zero    0x01
#define FPU_Tag_Special 0x02
#define FPU_Tag_Empty   0x03

#define BX_FPU_REG(index) \
    (BX_CPU_THIS_PTR the_i387.st_space[index])

#if defined(NEED_CPU_REG_SHORTCUTS)
#define FPU_PARTIAL_STATUS     (BX_CPU_THIS_PTR the_i387.swd)
#define FPU_CONTROL_WORD       (BX_CPU_THIS_PTR the_i387.cwd)
#define FPU_TAG_WORD           (BX_CPU_THIS_PTR the_i387.twd)
#define FPU_TOS                (BX_CPU_THIS_PTR the_i387.tos)
#endif

/* Status Word */
#define FPU_SW_Backward		(0x8000)  /* backward compatibility */
#define FPU_SW_C3	 	(0x4000)  /* condition bit 3 */
#define FPU_SW_Top		(0x3800)  /* top of stack */
#define FPU_SW_C2		(0x0400)  /* condition bit 2 */
#define FPU_SW_C1		(0x0200)  /* condition bit 1 */
#define FPU_SW_C0		(0x0100)  /* condition bit 0 */
#define FPU_SW_Summary   	(0x0080)  /* exception summary */
#define FPU_SW_Stack_Fault	(0x0040)  /* stack fault */
#define FPU_SW_Precision   	(0x0020)  /* loss of precision */
#define FPU_SW_Underflow   	(0x0010)  /* underflow */
#define FPU_SW_Overflow    	(0x0008)  /* overflow */
#define FPU_SW_Zero_Div    	(0x0004)  /* divide by zero */
#define FPU_SW_Denormal_Op   	(0x0002)  /* denormalized operand */
#define FPU_SW_Invalid    	(0x0001)  /* invalid operation */

#define FPU_SW_CC (FPU_SW_C0|FPU_SW_C1|FPU_SW_C2|FPU_SW_C3)

#define FPU_SW_Exceptions_Mask  (0x027f)  /* status word exceptions bit mask */

/* Special exceptions: */
#define FPU_EX_Stack_Overflow	(0x0041|FPU_SW_C1) 	/* stack overflow */
#define FPU_EX_Stack_Underflow	(0x0041)		/* stack underflow */

/* Exception flags: */
#define FPU_EX_Precision	(0x0020)  /* loss of precision */
#define FPU_EX_Underflow	(0x0010)  /* underflow */
#define FPU_EX_Overflow		(0x0008)  /* overflow */
#define FPU_EX_Zero_Div		(0x0004)  /* divide by zero */
#define FPU_EX_Denormal		(0x0002)  /* denormalized operand */
#define FPU_EX_Invalid		(0x0001)  /* invalid operation */

/* Control Word */
#define FPU_CW_RC		(0x0C00)  /* rounding control */
#define FPU_CW_PC		(0x0300)  /* precision control */

#define FPU_CW_Precision	(0x0020)  /* loss of precision mask */
#define FPU_CW_Underflow	(0x0010)  /* underflow mask */
#define FPU_CW_Overflow		(0x0008)  /* overflow mask */
#define FPU_CW_Zero_Div		(0x0004)  /* divide by zero mask */
#define FPU_CW_Denormal		(0x0002)  /* denormalized operand mask */
#define FPU_CW_Invalid		(0x0001)  /* invalid operation mask */

#define FPU_CW_Exceptions_Mask 	(0x003f)  /* all masks */

//
// Minimal i387 structure
//
struct i387_t 
{
    Bit32u cwd; 	// control word
    Bit32u swd; 	// status word
    Bit32u twd;		// tag word
    Bit32u fip;
    Bit32u fcs;
    Bit32u foo; 	// last instruction opcode
    Bit32u fos;

    bx_address fip_;
    Bit16u fcs_;
    Bit16u fds_;
    bx_address fos_;

    unsigned char tos;
    unsigned char no_update;
    unsigned char rm;
    unsigned char align8;

    FPU_REG st_space[8];
};

// for now solution, will be merged with i387_t when FPU 
// replacement will be done
#ifdef __cplusplus

#define clear_C1() { FPU_PARTIAL_STATUS &= ~FPU_SW_C1; }

#define SETCC(cc) do { 				\
  FPU_PARTIAL_STATUS &= ~(FPU_SW_CC); 		\
  FPU_PARTIAL_STATUS |= (cc) & FPU_SW_CC; 	\
} while(0);

extern "C" int FPU_tagof(FPU_REG *reg) BX_CPP_AttrRegparmN(1);
extern softfloat_status_word_t FPU_pre_exception_handling(Bit16u control_word);

struct i387_structure_t : public i387_t
{
    i387_structure_t() {}

    void	init();	// initalize fpu stuff

public:
    int    	get_tos() const { return tos; }

    int 	is_IA_masked() const { return (cwd & FPU_CW_Invalid); }

    Bit16u 	get_control_word() const { return cwd & 0xFFFF; }
    Bit16u 	get_tag_word() const { return twd & 0xFFFF; }
    Bit16u 	get_status_word() const { return (swd & ~FPU_SW_Top & 0xFFFF) | ((tos << 11) & FPU_SW_Top); }
    Bit16u 	get_partial_status() const { return swd & 0xFFFF; }

    void   	FPU_pop ();
    void   	FPU_push();

    void   	FPU_settag (int tag, int regnr);
    int    	FPU_gettag (int regnr);

    void   	FPU_settagi(int tag, int stnr) { FPU_settag(tag, tos+stnr); }
    int    	FPU_gettagi(int stnr) { return FPU_gettag(tos+stnr); }

    void	FPU_save_reg (floatx80 reg, int regnr);
    floatx80 	FPU_read_reg (int regnr);
    void	FPU_save_reg (floatx80 reg, int tag, int regnr);

    void  	FPU_save_regi(floatx80 reg, int stnr) { FPU_save_reg(reg, (tos+stnr) & 0x07); }
    floatx80 	FPU_read_regi(int stnr) { return FPU_read_reg((tos+stnr) & 0x07); }
    void  	FPU_save_regi(floatx80 reg, int tag, int stnr) { FPU_save_reg(reg, tag, (tos+stnr) & 0x07); }
};

#define IS_TAG_EMPTY(i) 		\
  ((BX_CPU_THIS_PTR the_i387.FPU_gettagi(i)) == FPU_Tag_Empty)

#define IS_IA_MASKED()			\
  (BX_CPU_THIS_PTR the_i387.get_control_word() & FPU_CW_Invalid)

#define BX_READ_FPU_REG(i)		\
  (BX_CPU_THIS_PTR the_i387.FPU_read_regi(i))

#define BX_WRITE_FPU_REGISTER_AND_TAG(value, tag, i)			\
{                                                               	\
    BX_CPU_THIS_PTR the_i387.FPU_save_regi((value), (tag), (i));      	\
}                                                               	

#define BX_WRITE_FPU_REG(value, i)					\
{                                                               	\
    BX_CPU_THIS_PTR the_i387.FPU_save_regi((value), (i));      		\
}                                                               	

BX_CPP_INLINE int i387_structure_t::FPU_gettag(int regnr)
{
  return (get_tag_word() >> ((regnr & 7)*2)) & 3;
}

BX_CPP_INLINE void i387_structure_t::FPU_settag (int tag, int regnr)
{
  regnr &= 7;
  twd &= ~(3 << (regnr*2));
  twd |= (tag & 3) << (regnr*2);
}

BX_CPP_INLINE void i387_structure_t::FPU_push(void)
{
  tos--;
}

BX_CPP_INLINE void i387_structure_t::FPU_pop(void)
{
  twd |= 3 << ((tos & 7)*2);
  tos++;
}

BX_CPP_INLINE floatx80 i387_structure_t::FPU_read_reg(int regnr)
{
  FPU_REG reg;
  memcpy(&reg, &st_space[regnr], sizeof(FPU_REG));

  floatx80 result;
  result.exp = reg.exp;
  result.fraction = (((Bit64u)(reg.sigh)) << 32) | ((Bit64u)(reg.sigl));

//printf("load x80 register: %08lx.%08lx%08lx\n", (Bit32u)result.exp, (Bit32u)(result.fraction >> 32), (Bit32u)(result.fraction & 0xFFFFFFFF));

  return result;
}

BX_CPP_INLINE void i387_structure_t::FPU_save_reg (floatx80 reg, int regnr)
{
  FPU_REG result;

  result.exp  = reg.exp;
  result.sigl = reg.fraction & 0xFFFFFFFF;
  result.sigh = reg.fraction >> 32;

//printf("save FPU register: %08lx.%08lx%08lx\n", (Bit32u)result.exp, result.sigh, result.sigl);

  memcpy(&st_space[regnr], &result, sizeof(FPU_REG));
  FPU_settag(FPU_tagof(&result), regnr);
}

BX_CPP_INLINE void i387_structure_t::FPU_save_reg (floatx80 reg, int tag, int regnr)
{
  FPU_REG result;

  result.exp  = reg.exp;
  result.sigl = reg.fraction & 0xFFFFFFFF;
  result.sigh = reg.fraction >> 32;

//printf("save FPU register: %08lx.%08lx%08lx\n", (Bit32u)result.exp, result.sigh, result.sigl);

  memcpy(&st_space[regnr], &result, sizeof(FPU_REG));
  FPU_settag(tag, regnr);
}

BX_CPP_INLINE void i387_structure_t::init()
{
  cwd = 0x037F;
  swd = 0;
  tos = 0;
  twd = 0xFFFF;

  fip_ = fcs_ = fds_ = fos_ = foo = 0;
  fip  = fcs  = fos  = 0;
}

extern const floatx80 Const_QNaN;
extern const floatx80 Const_Z;
extern const floatx80 Const_1;
extern const floatx80 Const_2;
extern const floatx80 Const_HALF;
extern const floatx80 Const_L2T;
extern const floatx80 Const_L2E;
extern const floatx80 Const_PI;
extern const floatx80 Const_PI2;
extern const floatx80 Const_PI4;
extern const floatx80 Const_LG2;
extern const floatx80 Const_LN2;

#endif

#if BX_SUPPORT_MMX

typedef union bx_packed_mmx_reg_t {
   Bit8s   _sbyte[8];
   Bit16s  _s16[4];
   Bit32s  _s32[2];
   Bit64s  _s64;
   Bit8u   _ubyte[8];
   Bit16u  _u16[4];
   Bit32u  _u32[2];
   Bit64u  _u64;
} BxPackedMmxRegister;

#ifdef BX_BIG_ENDIAN
#define mmx64s(i)   _s64
#define mmx32s(i)   _s32[1 - (i)]
#define mmx16s(i)   _s16[3 - (i)]
#define mmxsbyte(i) _sbyte[7 - (i)]
#define mmxubyte(i) _ubyte[7 - (i)]
#define mmx16u(i)   _u16[3 - (i)]
#define mmx32u(i)   _u32[1 - (i)]
#define mmx64u      _u64
#else
#define mmx64s(i)   _s64
#define mmx32s(i)   _s32[(i)]
#define mmx16s(i)   _s16[(i)]
#define mmxsbyte(i) _sbyte[(i)]
#define mmxubyte(i) _ubyte[(i)]
#define mmx16u(i)   _u16[(i)]
#define mmx32u(i)   _u32[(i)]
#define mmx64u      _u64
#endif

/* for compatability with already written code */
#define MMXSB0(reg) (reg.mmxsbyte(0))
#define MMXSB1(reg) (reg.mmxsbyte(1))
#define MMXSB2(reg) (reg.mmxsbyte(2))
#define MMXSB3(reg) (reg.mmxsbyte(3))
#define MMXSB4(reg) (reg.mmxsbyte(4))
#define MMXSB5(reg) (reg.mmxsbyte(5))
#define MMXSB6(reg) (reg.mmxsbyte(6))
#define MMXSB7(reg) (reg.mmxsbyte(7))

#define MMXSW0(reg) (reg.mmx16s(0))
#define MMXSW1(reg) (reg.mmx16s(1))
#define MMXSW2(reg) (reg.mmx16s(2))
#define MMXSW3(reg) (reg.mmx16s(3))

#define MMXSD0(reg) (reg.mmx32s(0))
#define MMXSD1(reg) (reg.mmx32s(1))

#define MMXSQ(reg)  (reg.mmx64s)
#define MMXUQ(reg)  (reg.mmx64u)
                                
#define MMXUD0(reg) (reg.mmx32u(0))
#define MMXUD1(reg) (reg.mmx32u(1))

#define MMXUW0(reg) (reg.mmx16u(0))
#define MMXUW1(reg) (reg.mmx16u(1))
#define MMXUW2(reg) (reg.mmx16u(2))
#define MMXUW3(reg) (reg.mmx16u(3))

#define MMXUB0(reg) (reg.mmxubyte(0))
#define MMXUB1(reg) (reg.mmxubyte(1))
#define MMXUB2(reg) (reg.mmxubyte(2))
#define MMXUB3(reg) (reg.mmxubyte(3))
#define MMXUB4(reg) (reg.mmxubyte(4))
#define MMXUB5(reg) (reg.mmxubyte(5))
#define MMXUB6(reg) (reg.mmxubyte(6))
#define MMXUB7(reg) (reg.mmxubyte(7))

// Endian  Host byte order         Guest (x86) byte order
// ======================================================
// Little  FFFFFFFFEEAAAAAA        FFFFFFFFEEAAAAAA
// Big     AAAAAAEEFFFFFFFF        FFFFFFFFEEAAAAAA
//
// Legend: F - fraction/mmx
//         E - exponent
//         A - alignment

#ifdef BX_BIG_ENDIAN
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=mac68k
#endif
struct bx_mmx_reg_t {
   Bit16u alignment1, alignment2, alignment3; 
   Bit16u exp; /* 2 byte FP-reg exponent */
   BxPackedMmxRegister packed_mmx_register;
} GCC_ATTRIBUTE((aligned(16), packed));
#if defined(__MWERKS__) && defined(macintosh)
#pragma options align=reset
#endif
#else
struct bx_mmx_reg_t {
   BxPackedMmxRegister packed_mmx_register;
   Bit16u exp; /* 2 byte FP reg exponent */
   Bit16u alignment1, alignment2, alignment3; 
} GCC_ATTRIBUTE((aligned(16), packed));
#endif

#define BX_MMX_REG(index) 						\
    (((bx_mmx_reg_t*)(BX_CPU_THIS_PTR the_i387.st_space))[index])

#define BX_READ_MMX_REG(index) 						\
    ((BX_MMX_REG(index)).packed_mmx_register)

#define BX_WRITE_MMX_REG(index, value)            			\
{                                 					\
   (BX_MMX_REG(index)).packed_mmx_register = value;			\
   (BX_MMX_REG(index)).exp = 0xffff;       				\
}                                                      

#endif 		/* BX_SUPPORT_MMX */

#endif		/* BX_SUPPORT_FPU */

#endif
