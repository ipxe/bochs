/*---------------------------------------------------------------------------+
 |  reg_constant.h                                                           |
 |  $Id: reg_constant.h,v 1.6.2.2 2004/03/27 20:09:53 sshwarts Exp $
 |                                                                           |
 | Copyright (C) 1992    W. Metzenthen, 22 Parker St, Ormond, Vic 3163,      |
 |                       Australia.  E-mail   billm@vaxc.cc.monash.edu.au    |
 |                                                                           |
 +---------------------------------------------------------------------------*/

#ifndef _REG_CONSTANT_H_
#define _REG_CONSTANT_H_

#include "fpu_emu.h"

extern FPU_REG const CONST_1;
extern FPU_REG const CONST_PI;

/*
 * make CONST_PI2 non-const so that you can write "&CONST_PI2" when
 * calling a function.  Otherwise you get const warnings.  Surely there's
 * a better way.
 */
extern FPU_REG CONST_PI2;
extern FPU_REG const CONST_PI4;
extern FPU_REG const CONST_Z;
extern FPU_REG const CONST_INF;
extern FPU_REG const CONST_QNaN;

#endif /* _REG_CONSTANT_H_ */
