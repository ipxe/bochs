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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

#ifndef FPU_TAG_WORD_H
#define FPU_TAG_WORD_H

/* Tag Word */
#define FPU_Tag_Valid   0x00
#define FPU_Tag_Zero    0x01
#define FPU_Tag_Special 0x02
#define FPU_Tag_Empty   0x03

#ifdef __cplusplus
extern "C" 
#endif
int FPU_tagof(FPU_REG *reg) BX_CPP_AttrRegparmN(1);

#endif
