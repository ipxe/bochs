/*---------------------------------------------------------------------------+
 |  fpu_asm.h                                                                |
 |  $Id: fpu_asm.h,v 1.2.8.1 2002-03-17 08:51:21 bdenney Exp $
 |                                                                           |
 | Copyright (C) 1992,1995,1997                                              |
 |                       W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail billm@suburbia.net               |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#ifndef _FPU_ASM_H_
#define _FPU_ASM_H_

#include <linux/linkage.h>

#define	EXCEPTION	SYMBOL_NAME(FPU_exception)


#define PARAM1	8(%ebp)
#define	PARAM2	12(%ebp)
#define	PARAM3	16(%ebp)
#define	PARAM4	20(%ebp)
#define	PARAM5	24(%ebp)
#define	PARAM6	28(%ebp)
#define	PARAM7	32(%ebp)

#define SIGL_OFFSET 0
#define	EXP(x)	8(x)
#define SIG(x)	SIGL_OFFSET##(x)
#define	SIGL(x)	SIGL_OFFSET##(x)
#define	SIGH(x)	4(x)

#endif /* _FPU_ASM_H_ */
