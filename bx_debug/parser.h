/* A Bison parser, made by GNU Bison 1.875b.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     BX_TOKEN_REG_AL = 258,
     BX_TOKEN_REG_BL = 259,
     BX_TOKEN_REG_CL = 260,
     BX_TOKEN_REG_DL = 261,
     BX_TOKEN_REG_AH = 262,
     BX_TOKEN_REG_BH = 263,
     BX_TOKEN_REG_CH = 264,
     BX_TOKEN_REG_DH = 265,
     BX_TOKEN_REG_AX = 266,
     BX_TOKEN_REG_BX = 267,
     BX_TOKEN_REG_CX = 268,
     BX_TOKEN_REG_DX = 269,
     BX_TOKEN_REG_EAX = 270,
     BX_TOKEN_REG_EBX = 271,
     BX_TOKEN_REG_ECX = 272,
     BX_TOKEN_REG_EDX = 273,
     BX_TOKEN_REG_SI = 274,
     BX_TOKEN_REG_DI = 275,
     BX_TOKEN_REG_BP = 276,
     BX_TOKEN_REG_SP = 277,
     BX_TOKEN_REG_IP = 278,
     BX_TOKEN_REG_ESI = 279,
     BX_TOKEN_REG_EDI = 280,
     BX_TOKEN_REG_EBP = 281,
     BX_TOKEN_REG_ESP = 282,
     BX_TOKEN_REG_EIP = 283,
     BX_TOKEN_CONTINUE = 284,
     BX_TOKEN_STEPN = 285,
     BX_TOKEN_STEP_OVER = 286,
     BX_TOKEN_NEXT_STEP = 287,
     BX_TOKEN_SET = 288,
     BX_TOKEN_DEBUGGER = 289,
     BX_TOKEN_LIST_BREAK = 290,
     BX_TOKEN_VBREAKPOINT = 291,
     BX_TOKEN_LBREAKPOINT = 292,
     BX_TOKEN_PBREAKPOINT = 293,
     BX_TOKEN_DEL_BREAKPOINT = 294,
     BX_TOKEN_ENABLE_BREAKPOINT = 295,
     BX_TOKEN_DISABLE_BREAKPOINT = 296,
     BX_TOKEN_INFO = 297,
     BX_TOKEN_QUIT = 298,
     BX_TOKEN_REGISTERS = 299,
     BX_TOKEN_CPU = 300,
     BX_TOKEN_FPU = 301,
     BX_TOKEN_SSE = 302,
     BX_TOKEN_ALL = 303,
     BX_TOKEN_IDT = 304,
     BX_TOKEN_GDT = 305,
     BX_TOKEN_LDT = 306,
     BX_TOKEN_TSS = 307,
     BX_TOKEN_DIRTY = 308,
     BX_TOKEN_LINUX = 309,
     BX_TOKEN_CONTROL_REGS = 310,
     BX_TOKEN_EXAMINE = 311,
     BX_TOKEN_XFORMAT = 312,
     BX_TOKEN_DISFORMAT = 313,
     BX_TOKEN_SETPMEM = 314,
     BX_TOKEN_SYMBOLNAME = 315,
     BX_TOKEN_QUERY = 316,
     BX_TOKEN_PENDING = 317,
     BX_TOKEN_TAKE = 318,
     BX_TOKEN_DMA = 319,
     BX_TOKEN_IRQ = 320,
     BX_TOKEN_DUMP_CPU = 321,
     BX_TOKEN_SET_CPU = 322,
     BX_TOKEN_ON = 323,
     BX_TOKEN_OFF = 324,
     BX_TOKEN_DISASSEMBLE = 325,
     BX_TOKEN_INSTRUMENT = 326,
     BX_TOKEN_START = 327,
     BX_TOKEN_STOP = 328,
     BX_TOKEN_RESET = 329,
     BX_TOKEN_PRINT = 330,
     BX_TOKEN_LOADER = 331,
     BX_TOKEN_STRING = 332,
     BX_TOKEN_DOIT = 333,
     BX_TOKEN_CRC = 334,
     BX_TOKEN_TRACEON = 335,
     BX_TOKEN_TRACEOFF = 336,
     BX_TOKEN_SWITCH_MODE = 337,
     BX_TOKEN_SIZE = 338,
     BX_TOKEN_PTIME = 339,
     BX_TOKEN_TIMEBP_ABSOLUTE = 340,
     BX_TOKEN_TIMEBP = 341,
     BX_TOKEN_RECORD = 342,
     BX_TOKEN_PLAYBACK = 343,
     BX_TOKEN_MODEBP = 344,
     BX_TOKEN_PRINT_STACK = 345,
     BX_TOKEN_WATCH = 346,
     BX_TOKEN_UNWATCH = 347,
     BX_TOKEN_READ = 348,
     BX_TOKEN_WRITE = 349,
     BX_TOKEN_SHOW = 350,
     BX_TOKEN_LOAD_SYMBOLS = 351,
     BX_TOKEN_SYMBOLS = 352,
     BX_TOKEN_LIST_SYMBOLS = 353,
     BX_TOKEN_GLOBAL = 354,
     BX_TOKEN_WHERE = 355,
     BX_TOKEN_PRINT_STRING = 356,
     BX_TOKEN_DIFF_MEMORY = 357,
     BX_TOKEN_SYNC_MEMORY = 358,
     BX_TOKEN_SYNC_CPU = 359,
     BX_TOKEN_FAST_FORWARD = 360,
     BX_TOKEN_PHY_2_LOG = 361,
     BX_TOKEN_NUMERIC = 362,
     BX_TOKEN_LONG_NUMERIC = 363,
     BX_TOKEN_INFO_ADDRESS = 364,
     BX_TOKEN_NE2000 = 365,
     BX_TOKEN_PIC = 366,
     BX_TOKEN_PAGE = 367,
     BX_TOKEN_CS = 368,
     BX_TOKEN_ES = 369,
     BX_TOKEN_SS = 370,
     BX_TOKEN_DS = 371,
     BX_TOKEN_FS = 372,
     BX_TOKEN_GS = 373,
     BX_TOKEN_FLAGS = 374,
     BX_TOKEN_ALWAYS_CHECK = 375,
     BX_TOKEN_TRACEREGON = 376,
     BX_TOKEN_TRACEREGOFF = 377,
     BX_TOKEN_HELP = 378,
     BX_TOKEN_CALC = 379,
     BX_TOKEN_VGA = 380,
     BX_TOKEN_RSHIFT = 381,
     BX_TOKEN_LSHIFT = 382,
     BX_TOKEN_IVT = 383,
     NEG = 384,
     NOT = 385
   };
#endif
#define BX_TOKEN_REG_AL 258
#define BX_TOKEN_REG_BL 259
#define BX_TOKEN_REG_CL 260
#define BX_TOKEN_REG_DL 261
#define BX_TOKEN_REG_AH 262
#define BX_TOKEN_REG_BH 263
#define BX_TOKEN_REG_CH 264
#define BX_TOKEN_REG_DH 265
#define BX_TOKEN_REG_AX 266
#define BX_TOKEN_REG_BX 267
#define BX_TOKEN_REG_CX 268
#define BX_TOKEN_REG_DX 269
#define BX_TOKEN_REG_EAX 270
#define BX_TOKEN_REG_EBX 271
#define BX_TOKEN_REG_ECX 272
#define BX_TOKEN_REG_EDX 273
#define BX_TOKEN_REG_SI 274
#define BX_TOKEN_REG_DI 275
#define BX_TOKEN_REG_BP 276
#define BX_TOKEN_REG_SP 277
#define BX_TOKEN_REG_IP 278
#define BX_TOKEN_REG_ESI 279
#define BX_TOKEN_REG_EDI 280
#define BX_TOKEN_REG_EBP 281
#define BX_TOKEN_REG_ESP 282
#define BX_TOKEN_REG_EIP 283
#define BX_TOKEN_CONTINUE 284
#define BX_TOKEN_STEPN 285
#define BX_TOKEN_STEP_OVER 286
#define BX_TOKEN_NEXT_STEP 287
#define BX_TOKEN_SET 288
#define BX_TOKEN_DEBUGGER 289
#define BX_TOKEN_LIST_BREAK 290
#define BX_TOKEN_VBREAKPOINT 291
#define BX_TOKEN_LBREAKPOINT 292
#define BX_TOKEN_PBREAKPOINT 293
#define BX_TOKEN_DEL_BREAKPOINT 294
#define BX_TOKEN_ENABLE_BREAKPOINT 295
#define BX_TOKEN_DISABLE_BREAKPOINT 296
#define BX_TOKEN_INFO 297
#define BX_TOKEN_QUIT 298
#define BX_TOKEN_REGISTERS 299
#define BX_TOKEN_CPU 300
#define BX_TOKEN_FPU 301
#define BX_TOKEN_SSE 302
#define BX_TOKEN_ALL 303
#define BX_TOKEN_IDT 304
#define BX_TOKEN_GDT 305
#define BX_TOKEN_LDT 306
#define BX_TOKEN_TSS 307
#define BX_TOKEN_DIRTY 308
#define BX_TOKEN_LINUX 309
#define BX_TOKEN_CONTROL_REGS 310
#define BX_TOKEN_EXAMINE 311
#define BX_TOKEN_XFORMAT 312
#define BX_TOKEN_DISFORMAT 313
#define BX_TOKEN_SETPMEM 314
#define BX_TOKEN_SYMBOLNAME 315
#define BX_TOKEN_QUERY 316
#define BX_TOKEN_PENDING 317
#define BX_TOKEN_TAKE 318
#define BX_TOKEN_DMA 319
#define BX_TOKEN_IRQ 320
#define BX_TOKEN_DUMP_CPU 321
#define BX_TOKEN_SET_CPU 322
#define BX_TOKEN_ON 323
#define BX_TOKEN_OFF 324
#define BX_TOKEN_DISASSEMBLE 325
#define BX_TOKEN_INSTRUMENT 326
#define BX_TOKEN_START 327
#define BX_TOKEN_STOP 328
#define BX_TOKEN_RESET 329
#define BX_TOKEN_PRINT 330
#define BX_TOKEN_LOADER 331
#define BX_TOKEN_STRING 332
#define BX_TOKEN_DOIT 333
#define BX_TOKEN_CRC 334
#define BX_TOKEN_TRACEON 335
#define BX_TOKEN_TRACEOFF 336
#define BX_TOKEN_SWITCH_MODE 337
#define BX_TOKEN_SIZE 338
#define BX_TOKEN_PTIME 339
#define BX_TOKEN_TIMEBP_ABSOLUTE 340
#define BX_TOKEN_TIMEBP 341
#define BX_TOKEN_RECORD 342
#define BX_TOKEN_PLAYBACK 343
#define BX_TOKEN_MODEBP 344
#define BX_TOKEN_PRINT_STACK 345
#define BX_TOKEN_WATCH 346
#define BX_TOKEN_UNWATCH 347
#define BX_TOKEN_READ 348
#define BX_TOKEN_WRITE 349
#define BX_TOKEN_SHOW 350
#define BX_TOKEN_LOAD_SYMBOLS 351
#define BX_TOKEN_SYMBOLS 352
#define BX_TOKEN_LIST_SYMBOLS 353
#define BX_TOKEN_GLOBAL 354
#define BX_TOKEN_WHERE 355
#define BX_TOKEN_PRINT_STRING 356
#define BX_TOKEN_DIFF_MEMORY 357
#define BX_TOKEN_SYNC_MEMORY 358
#define BX_TOKEN_SYNC_CPU 359
#define BX_TOKEN_FAST_FORWARD 360
#define BX_TOKEN_PHY_2_LOG 361
#define BX_TOKEN_NUMERIC 362
#define BX_TOKEN_LONG_NUMERIC 363
#define BX_TOKEN_INFO_ADDRESS 364
#define BX_TOKEN_NE2000 365
#define BX_TOKEN_PIC 366
#define BX_TOKEN_PAGE 367
#define BX_TOKEN_CS 368
#define BX_TOKEN_ES 369
#define BX_TOKEN_SS 370
#define BX_TOKEN_DS 371
#define BX_TOKEN_FS 372
#define BX_TOKEN_GS 373
#define BX_TOKEN_FLAGS 374
#define BX_TOKEN_ALWAYS_CHECK 375
#define BX_TOKEN_TRACEREGON 376
#define BX_TOKEN_TRACEREGOFF 377
#define BX_TOKEN_HELP 378
#define BX_TOKEN_CALC 379
#define BX_TOKEN_VGA 380
#define BX_TOKEN_RSHIFT 381
#define BX_TOKEN_LSHIFT 382
#define BX_TOKEN_IVT 383
#define NEG 384
#define NOT 385




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 19 "parser.y"
typedef union YYSTYPE {
  char    *sval;
  Bit32u   uval;
  Bit64u   ulval;
  bx_num_range   uval_range;
  Regs     reg;
  } YYSTYPE;
/* Line 1252 of yacc.c.  */
#line 305 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE bxlval;



