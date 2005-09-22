#ifndef _BX_DISASM_TABLES_
#define _BX_DISASM_TABLES_

// opcode table attributes
#define _PREFIX        1
#define _GROUPN        2
#define _SPLIT11B      3
#define _GRPFP         4
#define _GRP3DNOW      5
#define _GRPSSE        6

/* ************************************************************************ */
#define GRPSSE(n)       "(error)",  _GRPSSE, NULL, 0, NULL, 0, NULL, 0, BxDisasmGroupSSE_##n
#define GRPN(n)         "(error)",  _GROUPN, NULL, 0, NULL, 0, NULL, 0, BxDisasmGroup##n
#define GRPFP(n)        "(error)",  _GRPFP,  NULL, 0, NULL, 0, NULL, 0, BxDisasmFPGroup##n

#define GRP3DNOW        "(error)",  _GRP3DNOW, NULL, 0, NULL, 0, NULL, 0, BxDisasm3DNowGroup
#define INVALID         "(invalid)",        0, NULL, 0, NULL, 0, NULL, 0, NULL

#define PREFIX_CS       "cs",       _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_DS       "ds",       _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_ES       "es",       _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_SS       "ss",       _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_FS       "fs",       _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_GS       "gs",       _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_OPSIZE   "opsize",   _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_ADDRSIZE "addrsize", _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_LOCK     "lock",     _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_REP      "rep",      _PREFIX, NULL, 0, NULL, 0, NULL, 0
#define PREFIX_REPNE    "repne",    _PREFIX, NULL, 0, NULL, 0, NULL, 0

#if BX_SUPPORT_X86_64
#define PREFIX_REX      "rex",      _PREFIX, NULL, 0, NULL, 0, NULL, 0
#endif
/* ************************************************************************ */

 // fpu
#define ST0 &disassembler::ST0, 0
#define STj &disassembler::STj, 0

 // control/debug register
#define Cd  &disassembler::Cd,  0
#define Dd  &disassembler::Dd,  0
#define Td  &disassembler::Td,  0

#define Sw  &disassembler::Sw,  0
#define Rw  &disassembler::Rw,  0
#define Rd  &disassembler::Rd,  0

 // segment register
#define CS  &disassembler::OP_SEG, CS_REG
#define DS  &disassembler::OP_SEG, DS_REG
#define ES  &disassembler::OP_SEG, ES_REG
#define SS  &disassembler::OP_SEG, SS_REG
#define GS  &disassembler::OP_SEG, GS_REG
#define FS  &disassembler::OP_SEG, FS_REG

 // 16/32-bit general purpose register 
#define eAX &disassembler::REG32, eAX_REG
#define eCX &disassembler::REG32, eCX_REG
#define eDX &disassembler::REG32, eDX_REG
#define eBX &disassembler::REG32, eBX_REG
#define eSI &disassembler::REG32, eSI_REG
#define eDI &disassembler::REG32, eDI_REG
#define eBP &disassembler::REG32, eBP_REG
#define eSP &disassembler::REG32, eSP_REG

 // 16-bit general purpose register 
#define AX  &disassembler::REG16, AX_REG
#define DX  &disassembler::REG16, DX_REG

 // 8-bit general purpose register 
#define AL  &disassembler::REG8,  AL_REG
#define AH  &disassembler::REG8,  AH_REG
#define BL  &disassembler::REG8,  BL_REG
#define BH  &disassembler::REG8,  BH_REG
#define CL  &disassembler::REG8,  CL_REG
#define CH  &disassembler::REG8,  CH_REG
#define DL  &disassembler::REG8,  DL_REG
#define DH  &disassembler::REG8,  DH_REG

 // mmx
#define Pd  &disassembler::OP_P,  D_SIZE
#define Pq  &disassembler::OP_P,  Q_SIZE
#define Qd  &disassembler::OP_Q,  D_SIZE
#define Qq  &disassembler::OP_Q,  Q_SIZE

 // xmm
#define Vd  &disassembler::OP_V,  D_SIZE
#define Vq  &disassembler::OP_V,  Q_SIZE
#define Vdq &disassembler::OP_V,  O_SIZE
#define Vss &disassembler::OP_V,  D_SIZE
#define Vsd &disassembler::OP_V,  Q_SIZE
#define Vps &disassembler::OP_V,  O_SIZE
#define Vpd &disassembler::OP_V,  O_SIZE

#define Wq  &disassembler::OP_W,  Q_SIZE
#define Wdq &disassembler::OP_W,  O_SIZE
#define Wss &disassembler::OP_W,  D_SIZE
#define Wsd &disassembler::OP_W,  Q_SIZE
#define Wps &disassembler::OP_W,  O_SIZE
#define Wpd &disassembler::OP_W,  O_SIZE

 // string instructions
#define Xb  &disassembler::OP_X,  B_SIZE
#define Yb  &disassembler::OP_Y,  B_SIZE
#define Xv  &disassembler::OP_X,  V_SIZE
#define Yv  &disassembler::OP_Y,  V_SIZE

 // mov
#define Ob  &disassembler::OP_O,  B_SIZE
#define Ov  &disassembler::OP_O,  V_SIZE

 // immediate
#define I1 &disassembler::I1, 0
#define Ib &disassembler::Ib, B_SIZE
#define Iw &disassembler::Iw, W_SIZE
#define Id &disassembler::Id, D_SIZE
#define Iv &disassembler::Iv, V_SIZE

 // sign-extended immediate
#define sIb &disassembler::sIb, 0

 // memory only
#define Mb  &disassembler::OP_MEM, B_SIZE
#define Mw  &disassembler::OP_MEM, W_SIZE
#define Mv  &disassembler::OP_MEM, V_SIZE
#define Md  &disassembler::OP_MEM, D_SIZE
#define Mp  &disassembler::OP_MEM, P_SIZE
#define Mq  &disassembler::OP_MEM, Q_SIZE
#define Mt  &disassembler::OP_MEM, T_SIZE
#define Ms  &disassembler::OP_MEM, S_SIZE
#define Mx  &disassembler::OP_MEM, X_SIZE
#define Mdq &disassembler::OP_MEM, O_SIZE
#define Mps &disassembler::OP_MEM, O_SIZE
#define Mpd &disassembler::OP_MEM, O_SIZE

 // general purpose register
#define Gb  &disassembler::Gb, B_SIZE
#define Gv  &disassembler::Gv, V_SIZE
#define Gd  &disassembler::Gd, D_SIZE

 // general purpose register or memory operand
#define Eb  &disassembler::Eb, B_SIZE
#define Ew  &disassembler::Ew, W_SIZE
#define Ed  &disassembler::Ed, D_SIZE
#define Ev  &disassembler::Ev, V_SIZE

 // jump/call
#define Jb  &disassembler::Jb, 0
#define Jv  &disassembler::Jv, 0

 // branch hint
#define Cond_Jump NULL, BRANCH_HINT

 // jump/call far
#define Ap  &disassembler::Ap, 0

 // no operand
#define XX  NULL, 0

//
// Capital letters in opcode name suffix are macroses:
//
// B - print 'b' suffix for this instruction (in AT&T syntax mode)
// W - print 'w' suffix for this instruction (in AT&T syntax mode)
// L - print 'l' suffix for this instruction (in AT&T syntax mode)
// V - print 'w' or 'l' suffix for this instruction depending on
//     operands size (in AT&T syntax mode)
// Q - print 'q' suffix for this instruction (in AT&T syntax mode)
// S - print 'w' or 'd' suffix for this instruction depending on
//     its operands size (for string instructions)
// T - print 't' suffix for this instruction (in AT&T syntax mode)
// X - print 'bl', 'bw', "wl", "bq", "wq" or 'lq' suffix for this
//     instruction depending on its operands sizes in AT&T syntax 
//     mode and 'x' suffix in Intel syntax mode (for movsx and 
//     movzx instructions)
// D - print 'd' for 32-bit operand size in Intel syntax mode or
//     'l' AT&T syntax mode and 'q' for 64-bit operand size.
//

/* ************************************************************************ */
/* SSE opcodes */

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f10[4] = {
  /* -- */  { "movups",     0, Vps, Wps, XX },
  /* 66 */  { "movupd",     0, Vpd, Wpd, XX },
  /* F2 */  { "movsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "movss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f11[4] = {
  /* -- */  { "movups",     0, Wps, Vps, XX },
  /* 66 */  { "movupd",     0, Wpd, Vpd, XX },
  /* F2 */  { "movsd",      0, Wsd, Vsd, XX },
  /* F3 */  { "movss",      0, Wss, Vss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f12[4] = {
  /* -- */  { "movlps",     0, Vps,  Mq, XX },
  /* 66 */  { "movlpd",     0, Vpd,  Mq, XX },
  /* F2 */  { "movddup",    0, Vdq,  Wq, XX },
  /* F3 */  { "movsldup",   0, Vdq, Wdq, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f13[4] = {
  /* -- */  { "movlps",     0,  Mq, Vps, XX },
  /* 66 */  { "movlpd",     0,  Mq, Vpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};                        

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f14[4] = {
  /* -- */  { "unpcklps",   0, Vps, Wq, XX },
  /* 66 */  { "unpcklpd",   0, Vpd, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f15[4] = {
  /* -- */  { "unpckhps",   0, Vps, Wq, XX },
  /* 66 */  { "unpckhpd",   0, Vpd, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f16[4] = {
  /* -- */  { "movhps",     0, Vps,  Mq, XX  },
  /* 66 */  { "movhpd",     0, Vpd,  Mq, XX  },
  /* F2 */  { INVALID },
  /* F3 */  { "movshdup",   0, Vdq, Wdq, XX },
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f17[4] = {
  /* -- */  { "movhps",     0, Mq, Vps, XX },
  /* 66 */  { "movhpd",     0, Mq, Vpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f28[4] = {
  /* -- */  { "movaps",     0, Vps, Wps, XX },
  /* 66 */  { "movapd",     0, Vpd, Wpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f29[4] = {
  /* -- */  { "movaps",     0, Wps, Vps, XX },
  /* 66 */  { "movapd",     0, Wpd, Vpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f2a[4] = {
  /* -- */  { "cvtpi2ps",   0, Vps,  Qq, XX },
  /* 66 */  { "cvtpi2pd",   0, Vpd,  Qd, XX },
  /* F2 */  { "cvtsi2sd",   0, Vsd,  Ed, XX },
  /* F3 */  { "cvtsi2ss",   0, Vss,  Ed, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f2b[4] = {
  /* -- */  { "movntps",    0, Mps, Vps, XX },
  /* 66 */  { "movntpd",    0, Mpd, Vpd, XX },                  
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f2c[4] = {
  /* -- */  { "cvtps2pi",   0,  Pq, Wps, XX },
  /* 66 */  { "cvtpd2pi",   0,  Pq, Wpd, XX },
  /* F2 */  { "cvtsd2si",   0,  Gd, Wsd, XX },
  /* F3 */  { "cvtss2ss",   0,  Gd, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f2d[4] = {
  /* -- */  { "cvtps2pi",   0,  Pq, Wps, XX },
  /* 66 */  { "cvtpd2pi",   0,  Pq, Wpd, XX },
  /* F2 */  { "cvtsd2si",   0,  Gd, Wsd, XX },
  /* F3 */  { "cvtss2si",   0,  Gd, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f2e[4] = {
  /* -- */  { "ucomiss",    0, Vss, Wss, XX },
  /* 66 */  { "ucomisd",    0, Vsd, Wss, XX },                  
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f2f[4] = {
  /* -- */  { "comiss",     0, Vss, Wss, XX },
  /* 66 */  { "comisd",     0, Vsd, Wsd, XX },   
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f50[4] = {
  /* -- */  { "movmskps",   0,  Gd, Vps, XX },
  /* 66 */  { "movmskpd",   0,  Ed, Vpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f51[4] = {
  /* -- */  { "sqrtps",     0, Vps, Wps, XX },
  /* 66 */  { "sqrtpd",     0, Vpd, Wpd, XX },
  /* F2 */  { "sqrtsd",     0, Vsd, Wsd, XX },
  /* F3 */  { "sqrtss",     0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f52[4] = {
  /* -- */  { "rsqrtps",    0, Vps, Wps, XX },
  /* 66 */  { INVALID },
  /* F2 */  { INVALID },
  /* F3 */  { "rsqrtss",    0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f53[4] = {
  /* -- */  { "rcpps",      0, Vps, Wps, XX },
  /* 66 */  { INVALID },
  /* F2 */  { INVALID },
  /* F3 */  { "rcpss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f54[4] = {
  /* -- */  { "andps",      0, Vps, Wps, XX },
  /* 66 */  { "andpd",      0, Vpd, Wpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f55[4] = {
  /* -- */  { "andnps",     0, Vps, Wps, XX },
  /* 66 */  { "andnpd",     0, Vpd, Wpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f56[4] = {
  /* -- */  { "orps",       0, Vps, Wps, XX },
  /* 66 */  { "orpd",       0, Vpd, Wpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f57[4] = {
  /* -- */  { "xorps",      0, Vps, Wps, XX },
  /* 66 */  { "xorpd",      0, Vpd, Wpd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f58[4] = {
  /* -- */  { "addps",      0, Vps, Wps, XX },
  /* 66 */  { "addpd",      0, Vpd, Wpd, XX },
  /* F2 */  { "addsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "addss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f59[4] = {
  /* -- */  { "mulps",      0, Vps, Wps, XX },
  /* 66 */  { "mulpd",      0, Vpd, Wpd, XX },
  /* F2 */  { "mulsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "mulss",      0, Vss, Wss, XX }
};                   

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f5a[4] = {
  /* -- */  { "cvtps2pd",   0, Vpd, Wps, XX },
  /* 66 */  { "cvtpd2ps",   0, Vps, Wpd, XX },
  /* F2 */  { "cvtsd2ss",   0, Vss, Wss, XX },
  /* F3 */  { "cvtss2sd",   0, Vsd, Wsd, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f5b[4] = {
  /* -- */  { "cvtdq2ps",   0, Vps, Wdq, XX },
  /* 66 */  { "cvtps2dq",   0, Vdq, Wps, XX },
  /* F2 */  { INVALID },
  /* F3 */  { "cvttps2dq",  0, Vdq, Wps, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f5c[4] = {
  /* -- */  { "subps",      0, Vps, Wps, XX },
  /* 66 */  { "subpd",      0, Vpd, Wpd, XX },
  /* F2 */  { "subsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "subss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f5d[4] = {
  /* -- */  { "minps",      0, Vps, Wps, XX },
  /* 66 */  { "minpd",      0, Vpd, Wpd, XX },
  /* F2 */  { "minsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "minss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f5e[4] = {
  /* -- */  { "divps",      0, Vps, Wps, XX },
  /* 66 */  { "divpd",      0, Vpd, Wpd, XX },
  /* F2 */  { "divsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "divss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f5f[4] = {
  /* -- */  { "maxps",      0, Vps, Wps, XX },
  /* 66 */  { "maxpd",      0, Vpd, Wpd, XX },
  /* F2 */  { "maxsd",      0, Vsd, Wsd, XX },
  /* F3 */  { "maxss",      0, Vss, Wss, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f60[4] = {
  /* -- */  { "punpcklbw",  0,  Pq, Qd, XX },
  /* 66 */  { "punpcklbw",  0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f61[4] = {
  /* -- */  { "punpcklwd",  0,  Pq, Qd, XX },
  /* 66 */  { "punpcklwd",  0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f62[4] = {
  /* -- */  { "punpckldq",  0,  Pq, Qd, XX },
  /* 66 */  { "punpckldq",  0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f63[4] = {
  /* -- */  { "packsswb",   0,  Pq, Qq, XX },
  /* 66 */  { "packsswb",   0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f64[4] = {
  /* -- */  { "pcmpgtb",    0,  Pq, Qq, XX },
  /* 66 */  { "pcmpgtb",    0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f65[4] = {
  /* -- */  { "pcmpgtw",    0,  Pq, Qq, XX },
  /* 66 */  { "pcmpgtw",    0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f66[4] = {
  /* -- */  { "pcmpgtd",    0,  Pq,  Qq, XX },
  /* 66 */  { "pcmpgtd",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f67[4] = {
  /* -- */  { "packuswb",   0,  Pq,  Qq, XX },
  /* 66 */  { "packuswb",   0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f68[4] = {
  /* -- */  { "punpckhbw",  0,  Pq, Qq, XX },
  /* 66 */  { "punpckhbw",  0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f69[4] = {
  /* -- */  { "punpckhwd",  0,  Pq, Qq, XX },
  /* 66 */  { "punpckhwd",  0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f6a[4] = {
  /* -- */  { "punpckhdq",  0,  Pq, Qq, XX },
  /* 66 */  { "punpckhdq",  0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f6b[4] = {
  /* -- */  { "packssdw",   0,  Pq,  Qq, XX },
  /* 66 */  { "packssdw",   0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f6c[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "punpcklqdq", 0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f6d[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "punpckhqdq", 0, Vdq, Wq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f6e[4] = {
  /* -- */  { "movd",       0,  Pq, Ed, XX },
  /* 66 */  { "movd",       0, Vdq, Ed, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f6f[4] = {
  /* -- */  { "movq",       0,  Pq,  Qq, XX },
  /* 66 */  { "movdqa",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { "movdqu",     0, Vdq, Wdq, XX },
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f70[4] = {
  /* -- */  { "pshufw",     0,  Pq,  Qq, Ib },
  /* 66 */  { "pshufd",     0, Vdq, Wdq, Ib },
  /* F2 */  { "pshufhw",    0,  Vq,  Wq, Ib },
  /* F3 */  { "pshuflw",    0,  Vq,  Wq, Ib }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f74[4] = {
  /* -- */  { "pcmpeqb",    0,  Pq,  Qq, XX },
  /* 66 */  { "pcmpeqb",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f75[4] = {
  /* -- */  { "pcmpeqw",    0,  Pq,  Qq, XX },
  /* 66 */  { "pcmpeqw",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f76[4] = {
  /* -- */  { "pcmpeqd",    0,  Pq,  Qq, XX },
  /* 66 */  { "pcmpeqd",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f7c[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "haddpd",     0, Vpd, Wpd, XX },
  /* F2 */  { "haddps",     0, Vps, Wps, XX },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f7d[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "hsubpd",     0, Vpd, Wpd, XX },
  /* F2 */  { "hsubps",     0, Vps, Wps, XX },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f7e[4] = {
  /* -- */  { "movd",       0,  Ed, Pq, XX },
  /* 66 */  { "movd",       0,  Ed, Vd, XX },
  /* F2 */  { INVALID },
  /* F3 */  { "movq",       0,  Vq, Wq, XX },
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0f7f[4] = {
  /* -- */  { "movq",       0,  Qq,  Pq, XX },
  /* 66 */  { "movdqa",     0, Wdq, Vdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { "movdqu",     0, Wdq, Vdq, XX },
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fc2[4] = {
  /* -- */  { "cmpps",      0, Vps, Wps, Ib },
  /* 66 */  { "cmppd",      0, Vpd, Wpd, Ib },
  /* F2 */  { "cmpsd",      0, Vsd, Wsd, Ib },
  /* F3 */  { "cmpss",      0, Vss, Wss, Ib }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fc3[4] = {
  /* -- */  { "movnti",     0, Md, Gd, XX },
  /* 66 */  { INVALID },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fc4[4] = {
  /* -- */  { "pinsrw",     0,  Pq, Ed, Ib },
  /* 66 */  { "pinsrw",     0, Vdq, Ed, Ib },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fc5[4] = {
  /* -- */  { "pextrw",     0,  Pq, Ed, Ib },
  /* 66 */  { "pextrw",     0, Vdq, Ed, Ib },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fc6[4] = {
  /* -- */  { "shufps",     0, Vps, Wps, Ib },
  /* 66 */  { "shufpd",     0, Vpd, Wpd, Ib },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd0[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "addsubpd",   0, Vpd, Wpd, XX },
  /* F2 */  { "addsubps",   0, Vps, Wps, XX },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd1[4] = {
  /* -- */  { "psrlw",      0,  Pq,  Qq, XX },
  /* 66 */  { "psrlw",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd2[4] = {
  /* -- */  { "psrld",      0,  Pq,  Qq, XX },
  /* 66 */  { "psrld",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd3[4] = {
  /* -- */  { "psrlq",      0,  Pq,  Qq, XX },
  /* 66 */  { "psrlq",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd4[4] = {
  /* -- */  { "paddq",      0,  Pq,  Qq, XX },
  /* 66 */  { "paddq",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd5[4] = {
  /* -- */  { "pmullw",     0,  Pq,  Qq, XX },
  /* 66 */  { "pmullw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd6[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "movq",       0,  Wq, Vq, XX },
  /* F2 */  { "movdq2q",    0,  Pq, Vq, XX },
  /* F3 */  { "movq2dq",    0, Vdq, Qq, XX },
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd7[4] = {
  /* -- */  { "pmovmskb",   0, Gd,  Pq, XX },
  /* 66 */  { "pmovmskb",   0, Gd, Vdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd8[4] = {
  /* -- */  { "psubusb",    0,  Pq,  Qq, XX },
  /* 66 */  { "psubusb",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fd9[4] = {
  /* -- */  { "psubusw",    0, Pq, Qq, XX   },
  /* 66 */  { "psubusw",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fda[4] = {
  /* -- */  { "pminub",     0,  Pq,  Qq, XX },
  /* 66 */  { "pminub",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fdb[4] = {
  /* -- */  { "pand",       0,  Pq,  Qq, XX },
  /* 66 */  { "pand",       0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fdc[4] = {
  /* -- */  { "paddusb",    0,  Pq,  Qq, XX },
  /* 66 */  { "paddusb",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fdd[4] = {
  /* -- */  { "paddusw",    0,  Pq,  Qq, XX },
  /* 66 */  { "paddusw",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fde[4] = {
  /* -- */  { "pmaxub",     0,  Pq,  Qq, XX },
  /* 66 */  { "pmaxub",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fdf[4] = {
  /* -- */  { "pandn",      0,  Pq,  Qq, XX },
  /* 66 */  { "pandn",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe0[4] = {
  /* -- */  { "pavgb",      0,  Pq,  Qq, XX },
  /* 66 */  { "pavgb",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe1[4] = {
  /* -- */  { "psraw",      0,  Pq,  Qq, XX },
  /* 66 */  { "psraw",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe2[4] = {
  /* -- */  { "psrad",      0,  Pq,  Qq, XX },
  /* 66 */  { "psrad",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe3[4] = {
  /* -- */  { "pavgw",      0,  Pq,  Qq, XX },
  /* 66 */  { "pavgw",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe4[4] = {
  /* -- */  { "pmulhuw",    0,  Pq,  Qq, XX },
  /* 66 */  { "pmulhuw",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe5[4] = {
  /* -- */  { "pmulhw",     0,  Pq,  Qq, XX },
  /* 66 */  { "pmulhw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe6[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "cvttpd2dq",  0,  Vq, Wpd, XX },
  /* F2 */  { "cvtpd2dq",   0,  Vq, Wpd, XX },
  /* F3 */  { "cvtdq2pd",   0, Vpd,  Wq, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe7[4] = {
  /* -- */  { "movntq",     0,  Mq,  Pq, XX },
  /* 66 */  { "movntdq",    0, Mdq, Vdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe8[4] = {
  /* -- */  { "psubsb",     0,  Pq,  Qq, XX },
  /* 66 */  { "psubsb",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fe9[4] = {
  /* -- */  { "psubsw",     0,  Pq,  Qq, XX },
  /* 66 */  { "psubsw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fea[4] = {
  /* -- */  { "pminsw",     0,  Pq,  Qq, XX },
  /* 66 */  { "pminsw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0feb[4] = {
  /* -- */  { "por",        0,  Pq,  Qq, XX },
  /* 66 */  { "por",        0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fec[4] = {
  /* -- */  { "paddsb",     0,  Pq,  Qq, XX },
  /* 66 */  { "paddsb",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fed[4] = {
  /* -- */  { "paddsw",     0,  Pq,  Qq, XX },
  /* 66 */  { "paddsw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fee[4] = {
  /* -- */  { "pmaxuw",     0,  Pq,  Qq, XX },
  /* 66 */  { "pmaxuw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0fef[4] = {
  /* -- */  { "pxor",       0,  Pq,  Qq, XX },
  /* 66 */  { "pxor",       0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff0[4] = {
  /* -- */  { INVALID },
  /* 66 */  { INVALID },
  /* F3 */  { "lddqu",      0, Vdq, Mdq, XX },
  /* F3 */  { INVALID }
};                        

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff1[4] = {
  /* -- */  { "psllw",      0,  Pq,  Qq, XX },
  /* 66 */  { "psllw",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff2[4] = {
  /* -- */  { "pslld",      0,  Pq,  Qq, XX },
  /* 66 */  { "pslld",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff3[4] = {
  /* -- */  { "psllq",      0,  Pq,  Qq, XX },
  /* 66 */  { "psllq",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff4[4] = {
  /* -- */  { "pmuludq",    0,  Pq,  Qq, XX },
  /* 66 */  { "pmuludq",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff5[4] = {
  /* -- */  { "pmaddwd",    0,  Pq,  Qq, XX },
  /* 66 */  { "pmaddwd",    0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff6[4] = {
  /* -- */  { "psadbw",     0,  Pq,  Qq, XX },
  /* 66 */  { "psadbw",     0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff7[4] = {
  /* -- */  { "maskmovq",   0,  Pq,  Qq, XX },
  /* 66 */  { "maskmovdqu", 0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff8[4] = {
  /* -- */  { "psubb",      0,  Pq,  Qq, XX },
  /* 66 */  { "psubb",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ff9[4] = {
  /* -- */  { "psubw",      0,  Pq,  Qq, XX },
  /* 66 */  { "psubw",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ffa[4] = {
  /* -- */  { "psubd",      0,  Pq,  Qq, XX },
  /* 66 */  { "psubd",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ffb[4] = {
  /* -- */  { "psubq",      0,  Pq,  Qq, XX },
  /* 66 */  { "psubq",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ffc[4] = {
  /* -- */  { "paddb",      0,  Pq,  Qq, XX },
  /* 66 */  { "paddb",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ffd[4] = {
  /* -- */  { "paddw",      0,  Pq,  Qq, XX },
  /* 66 */  { "paddw",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_0ffe[4] = {
  /* -- */  { "paddd",      0,  Pq,  Qq, XX },
  /* 66 */  { "paddd",      0, Vdq, Wdq, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1202[4] = {
  /* -- */  { "psrlw",      0,  Pq, Ib, XX },
  /* 66 */  { "psrlw",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1204[4] = {
  /* -- */  { "psraw",      0,  Pq, Ib, XX },
  /* 66 */  { "psraw",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1206[4] = {
  /* -- */  { "psllw",      0,  Pq, Ib, XX },
  /* 66 */  { "psllw",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1302[4] = {
  /* -- */  { "psrld",      0,  Pq, Ib, XX },
  /* 66 */  { "psrld",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1304[4] = {
  /* -- */  { "psrad",      0,  Pq, Ib, XX },
  /* 66 */  { "psrad",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1306[4] = {
  /* -- */  { "pslld",      0,  Pq, Ib, XX },
  /* 66 */  { "pslld",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1402[4] = {
  /* -- */  { "psrlq",      0,  Pq, Ib, XX },
  /* 66 */  { "psrlq",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1403[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "psrldq",     0, Wdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1406[4] = {
  /* -- */  { "psllq",      0,  Pq, Ib, XX },
  /* 66 */  { "psllq",      0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupSSE_G1407[4] = {
  /* -- */  { INVALID },
  /* 66 */  { "pslldq",     0, Vdq, Ib, XX },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID }
};


/* ************************************************************************ */
/* Opcode GroupN */

static BxDisasmOpcodeInfo_t BxDisasmGroupG1EbIb[8] = {
  /* 0 */  { "addB",        0, Eb, Ib, XX },
  /* 1 */  { "orB",         0, Eb, Ib, XX },
  /* 2 */  { "adcB",        0, Eb, Ib, XX },
  /* 3 */  { "sbbB",        0, Eb, Ib, XX },
  /* 4 */  { "andB",        0, Eb, Ib, XX },
  /* 5 */  { "subB",        0, Eb, Ib, XX },
  /* 6 */  { "xorB",        0, Eb, Ib, XX },
  /* 7 */  { "cmpB",        0, Eb, Ib, XX }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupG1EvIv[8] = {
  /* 0 */  { "addV",        0, Ev, Iv, XX },
  /* 1 */  { "orV",         0, Ev, Iv, XX },
  /* 2 */  { "adcV",        0, Ev, Iv, XX },
  /* 3 */  { "sbbV",        0, Ev, Iv, XX },
  /* 4 */  { "andV",        0, Ev, Iv, XX },
  /* 5 */  { "subV",        0, Ev, Iv, XX },
  /* 6 */  { "xorV",        0, Ev, Iv, XX },
  /* 7 */  { "cmpV",        0, Ev, Iv, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG1EvIb[8] = {
  /* 0 */  { "addV",        0, Ev, sIb, XX },	// sign-extend byte 
  /* 1 */  { "orV",         0, Ev, sIb, XX },
  /* 2 */  { "adcV",        0, Ev, sIb, XX },
  /* 3 */  { "sbbV",        0, Ev, sIb, XX },
  /* 4 */  { "andV",        0, Ev, sIb, XX },
  /* 5 */  { "subV",        0, Ev, sIb, XX },
  /* 6 */  { "xorV",        0, Ev, sIb, XX },
  /* 7 */  { "cmpV",        0, Ev, sIb, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG2Eb[8] = {
  /* 0 */  { "rolB",        0, Eb, Ib, XX },
  /* 1 */  { "rorB",        0, Eb, Ib, XX },
  /* 2 */  { "rclB",        0, Eb, Ib, XX },
  /* 3 */  { "rcrB",        0, Eb, Ib, XX },
  /* 4 */  { "shlB",        0, Eb, Ib, XX },
  /* 5 */  { "shrB",        0, Eb, Ib, XX },
  /* 6 */  { "shlB",        0, Eb, Ib, XX },
  /* 7 */  { "sarB",        0, Eb, Ib, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG2Eb1[8] = {
  /* 0 */  { "rolB",        0, Eb, I1, XX },
  /* 1 */  { "rorB",        0, Eb, I1, XX },
  /* 2 */  { "rclB",        0, Eb, I1, XX },
  /* 3 */  { "rcrB",        0, Eb, I1, XX },
  /* 4 */  { "shlB",        0, Eb, I1, XX },
  /* 5 */  { "shrB",        0, Eb, I1, XX },
  /* 6 */  { "shlB",        0, Eb, I1, XX },
  /* 7 */  { "sarB",        0, Eb, I1, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG2EbCL[8] = {
  /* 0 */  { "rolB",        0, Eb, CL, XX },
  /* 1 */  { "rorB",        0, Eb, CL, XX },
  /* 2 */  { "rclB",        0, Eb, CL, XX },
  /* 3 */  { "rcrB",        0, Eb, CL, XX },
  /* 4 */  { "shlB",        0, Eb, CL, XX },
  /* 5 */  { "shrB",        0, Eb, CL, XX },
  /* 6 */  { "shlB",        0, Eb, CL, XX },
  /* 7 */  { "sarB",        0, Eb, CL, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG2Ev[8] = {
  /* 0 */  { "rolV",        0, Ev, Ib, XX },
  /* 1 */  { "rorV",        0, Ev, Ib, XX },
  /* 2 */  { "rclV",        0, Ev, Ib, XX },
  /* 3 */  { "rcrV",        0, Ev, Ib, XX },
  /* 4 */  { "shlV",        0, Ev, Ib, XX },
  /* 5 */  { "shrV",        0, Ev, Ib, XX },
  /* 6 */  { "shlV",        0, Ev, Ib, XX },
  /* 7 */  { "sarV",        0, Ev, Ib, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG2Ev1[8] = {
  /* 0 */  { "rolV",        0, Ev, I1, XX },
  /* 1 */  { "rorV",        0, Ev, I1, XX },
  /* 2 */  { "rclV",        0, Ev, I1, XX },
  /* 3 */  { "rcrV",        0, Ev, I1, XX },
  /* 4 */  { "shlV",        0, Ev, I1, XX },
  /* 5 */  { "shrV",        0, Ev, I1, XX },
  /* 6 */  { "shlV",        0, Ev, I1, XX },
  /* 7 */  { "sarV",        0, Ev, I1, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG2EvCL[8] = {
  /* 0 */  { "rolV",        0, Ev, CL, XX },
  /* 1 */  { "rorV",        0, Ev, CL, XX },
  /* 2 */  { "rclV",        0, Ev, CL, XX },
  /* 3 */  { "rcrV",        0, Ev, CL, XX },
  /* 4 */  { "shlV",        0, Ev, CL, XX },
  /* 5 */  { "shrV",        0, Ev, CL, XX },
  /* 6 */  { "shlV",        0, Ev, CL, XX },
  /* 7 */  { "sarV",        0, Ev, CL, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG3Eb[8] = {
  /* 0 */  { "testB",       0, Eb, Ib, XX },
  /* 1 */  { "testB",       0, Eb, Ib, XX },
  /* 2 */  { "notB",        0, Eb, XX, XX },
  /* 3 */  { "negB",        0, Eb, XX, XX },
  /* 4 */  { "mulB",        0, AL, Eb, XX },
  /* 5 */  { "imulB",       0, AL, Eb, XX },
  /* 6 */  { "divB",        0, AL, Eb, XX },
  /* 7 */  { "idivB",       0, AL, Eb, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG3Ev[8] = {
  /* 0 */  { "testV",       0,  Ev, Iv, XX },
  /* 1 */  { "testV",       0,  Ev, Iv, XX },
  /* 2 */  { "notV",        0,  Ev, XX, XX },
  /* 3 */  { "negV",        0,  Ev, XX, XX },    
  /* 4 */  { "mulV",        0, eAX, Ev, XX },
  /* 5 */  { "imulV",       0, eAX, Ev, XX },
  /* 6 */  { "divV",        0, eAX, Ev, XX },
  /* 7 */  { "idivV",       0, eAX, Ev, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG4[8] = {
  /* 0 */  { "incB",        0, Eb, XX, XX },
  /* 1 */  { "decB",        0, Eb, XX, XX },
  /* 2 */  { INVALID },
  /* 3 */  { INVALID },
  /* 4 */  { INVALID },
  /* 5 */  { INVALID },
  /* 6 */  { INVALID },
  /* 7 */  { INVALID }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG5[8] = {
  /* 0 */  { "incV",        0, Ev, XX, XX },
  /* 1 */  { "decV",        0, Ev, XX, XX },
  /* 2 */  { "callV",       0, Ev, XX, XX },
  /* 3 */  { "call far",    0, Mp, XX, XX },
  /* 4 */  { "jmpV",        0, Ev, XX, XX },
  /* 5 */  { "jmp far",     0, Mp, XX, XX },
  /* 6 */  { "pushV",       0, Ev, XX, XX },
  /* 7 */  { INVALID }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG6[8] = {
  /* 0 */  { "sldtW",       0, Ew, XX, XX },
  /* 1 */  { "strW",        0, Ew, XX, XX },
  /* 2 */  { "lldtW",       0, Ew, XX, XX },
  /* 3 */  { "ltrW",        0, Ew, XX, XX },
  /* 4 */  { "verrW",       0, Ew, XX, XX },
  /* 5 */  { "verwW",       0, Ew, XX, XX },
  /* 6 */  { INVALID },
  /* 7 */  { INVALID }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG7[8] = {
  /* 0 */  { "sgdt",        0, Ms, XX, XX },
  /* 1 */  { "sidt",        0, Ms, XX, XX },
  /* 2 */  { "lgdt",        0, Ms, XX, XX },
  /* 3 */  { "lidt",        0, Ms, XX, XX },
  /* 4 */  { "smsw",        0, Ew, XX, XX },
  /* 5 */  { INVALID },
  /* 6 */  { "lmsw",        0, Ew, XX, XX },
  /* 7 */  { "invlpg",      0, XX, XX, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG8EvIb[8] = {
  /* 0 */  { INVALID },
  /* 1 */  { INVALID },
  /* 2 */  { INVALID },
  /* 3 */  { INVALID },
  /* 4 */  { "btV",         0, Ev, Ib, XX },
  /* 5 */  { "btsV",        0, Ev, Ib, XX },
  /* 6 */  { "btrV",        0, Ev, Ib, XX },
  /* 7 */  { "btcV",        0, Ev, Ib, XX }
}; 

static BxDisasmOpcodeInfo_t BxDisasmGroupG9[8] = {
  /* 0 */  { INVALID },
  /* 1 */  { "cmpxchg8b",   0, Mq, XX, XX },
  /* 2 */  { INVALID },
  /* 3 */  { INVALID },
  /* 4 */  { INVALID },
  /* 5 */  { INVALID },
  /* 6 */  { INVALID },
  /* 7 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupG12[8] = {
  /* 0 */  { INVALID },
  /* 1 */  { INVALID },
  /* 2 */  { GRPSSE(G1202) },
  /* 3 */  { INVALID },
  /* 4 */  { GRPSSE(G1204) },
  /* 5 */  { INVALID },
  /* 6 */  { GRPSSE(G1206) },
  /* 7 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupG13[8] = {
  /* 0 */  { INVALID },
  /* 1 */  { INVALID },
  /* 2 */  { GRPSSE(G1302) },
  /* 3 */  { INVALID },
  /* 4 */  { GRPSSE(G1304) },
  /* 5 */  { INVALID },
  /* 6 */  { GRPSSE(G1306) },
  /* 7 */  { INVALID }
};

static BxDisasmOpcodeInfo_t BxDisasmGroupG14[8] = {
  /* 0 */  { INVALID },
  /* 1 */  { INVALID },
  /* 2 */  { GRPSSE(G1402) },
  /* 3 */  { GRPSSE(G1403) },
  /* 4 */  { INVALID },
  /* 5 */  { INVALID },
  /* 6 */  { GRPSSE(G1406) },
  /* 7 */  { GRPSSE(G1407) } 
};

static BxDisasmOpcodeInfo_t BxDisasmGroupG15[8] = {
  /* 0 */  { "fxsave",      0, Mx, XX, XX },
  /* 1 */  { "fxrstor",     0, Mx, XX, XX },
  /* 2 */  { "ldmxcsr",     0, Md, XX, XX },
  /* 3 */  { "stmxcsr",     0, Md, XX, XX },
  /* 4 */  { INVALID },
  /* 5 */  { "lfence",      0, XX, XX, XX },
  /* 6 */  { "mfence",      0, XX, XX, XX },
  /* 7 */  { "sfence",      0, XX, XX, XX }       /* SFENCE/CFLUSH */
};

static BxDisasmOpcodeInfo_t BxDisasmGroupG16[8] =
{
  /* 0 */  { "prefetchnta", 0, Mb, XX, XX },
  /* 1 */  { "prefetcht0",  0, Mb, XX, XX },
  /* 2 */  { "prefetcht1",  0, Mb, XX, XX },
  /* 3 */  { "prefetcht2",  0, Mb, XX, XX },
  /* 4 */  { INVALID },
  /* 5 */  { INVALID },
  /* 6 */  { INVALID },
  /* 7 */  { INVALID }
};

/* ************************************************************************ */
/* 3DNow! opcodes */

static BxDisasmOpcodeInfo_t BxDisasm3DNowGroup[256] = {
  // 256 entries for 3DNow opcodes, by suffix
  /* 00 */  { INVALID },
  /* 01 */  { INVALID },
  /* 02 */  { INVALID },
  /* 03 */  { INVALID },
  /* 04 */  { INVALID },
  /* 05 */  { INVALID },
  /* 06 */  { INVALID },
  /* 07 */  { INVALID },
  /* 08 */  { INVALID },
  /* 09 */  { INVALID },
  /* 0A */  { INVALID },
  /* 0B */  { INVALID },
  /* 0C */  { "pi2fw",    0, Pq, Qq, XX },
  /* 0D */  { "pi2fd",    0, Pq, Qq, XX },
  /* 0E */  { INVALID },
  /* 0F */  { INVALID },
  /* 10 */  { INVALID },
  /* 11 */  { INVALID },
  /* 12 */  { INVALID },
  /* 13 */  { INVALID },
  /* 14 */  { INVALID },
  /* 15 */  { INVALID },
  /* 16 */  { INVALID },
  /* 17 */  { INVALID },
  /* 18 */  { INVALID },
  /* 19 */  { INVALID },
  /* 1A */  { INVALID },
  /* 1B */  { INVALID },
  /* 1C */  { "pf2iw",    0, Pq, Qq, XX },
  /* 1D */  { "pf2id",    0, Pq, Qq, XX },
  /* 1E */  { INVALID },
  /* 1F */  { INVALID },
  /* 20 */  { INVALID },
  /* 21 */  { INVALID },
  /* 22 */  { INVALID },
  /* 23 */  { INVALID },
  /* 24 */  { INVALID },
  /* 25 */  { INVALID },
  /* 26 */  { INVALID },
  /* 27 */  { INVALID },
  /* 28 */  { INVALID },
  /* 29 */  { INVALID },
  /* 2A */  { INVALID },
  /* 2B */  { INVALID },
  /* 2C */  { INVALID },
  /* 2D */  { INVALID },
  /* 2E */  { INVALID },
  /* 2F */  { INVALID },
  /* 30 */  { INVALID },
  /* 31 */  { INVALID },
  /* 32 */  { INVALID },
  /* 33 */  { INVALID },
  /* 34 */  { INVALID },
  /* 35 */  { INVALID },
  /* 36 */  { INVALID },
  /* 37 */  { INVALID },
  /* 38 */  { INVALID },
  /* 39 */  { INVALID },
  /* 3A */  { INVALID },
  /* 3B */  { INVALID },
  /* 3C */  { INVALID },
  /* 3D */  { INVALID },
  /* 3E */  { INVALID },
  /* 3F */  { INVALID },
  /* 40 */  { INVALID },
  /* 41 */  { INVALID },
  /* 42 */  { INVALID },
  /* 43 */  { INVALID },
  /* 44 */  { INVALID },
  /* 45 */  { INVALID },
  /* 46 */  { INVALID },
  /* 47 */  { INVALID },
  /* 48 */  { INVALID },
  /* 49 */  { INVALID },
  /* 4A */  { INVALID },
  /* 4B */  { INVALID },
  /* 4C */  { INVALID },
  /* 4D */  { INVALID },
  /* 4E */  { INVALID },
  /* 4F */  { INVALID },
  /* 50 */  { INVALID },
  /* 51 */  { INVALID },
  /* 52 */  { INVALID },
  /* 53 */  { INVALID },
  /* 54 */  { INVALID },
  /* 55 */  { INVALID },
  /* 56 */  { INVALID },
  /* 57 */  { INVALID },
  /* 58 */  { INVALID },
  /* 59 */  { INVALID },
  /* 5A */  { INVALID },
  /* 5B */  { INVALID },
  /* 5C */  { INVALID },
  /* 5D */  { INVALID },
  /* 5E */  { INVALID },
  /* 5F */  { INVALID },
  /* 60 */  { INVALID },
  /* 61 */  { INVALID },
  /* 62 */  { INVALID },
  /* 63 */  { INVALID },
  /* 64 */  { INVALID },
  /* 65 */  { INVALID },
  /* 66 */  { INVALID },
  /* 67 */  { INVALID },
  /* 68 */  { INVALID },
  /* 69 */  { INVALID },
  /* 6A */  { INVALID },
  /* 6B */  { INVALID },
  /* 6C */  { INVALID },
  /* 6D */  { INVALID },
  /* 6E */  { INVALID },
  /* 6F */  { INVALID },
  /* 70 */  { INVALID },
  /* 71 */  { INVALID },
  /* 72 */  { INVALID },
  /* 73 */  { INVALID },
  /* 74 */  { INVALID },
  /* 75 */  { INVALID },
  /* 76 */  { INVALID },
  /* 77 */  { INVALID },
  /* 78 */  { INVALID },
  /* 79 */  { INVALID },
  /* 7A */  { INVALID },
  /* 7B */  { INVALID },
  /* 7C */  { INVALID },
  /* 7D */  { INVALID },
  /* 7E */  { INVALID },
  /* 7F */  { INVALID },
  /* 80 */  { INVALID },
  /* 81 */  { INVALID },
  /* 82 */  { INVALID },
  /* 83 */  { INVALID },
  /* 84 */  { INVALID },
  /* 85 */  { INVALID },
  /* 86 */  { INVALID },
  /* 87 */  { INVALID },
  /* 88 */  { INVALID },
  /* 89 */  { INVALID },
  /* 8A */  { "pfnacc",   0, Pq, Qq, XX },
  /* 8B */  { INVALID },
  /* 8C */  { INVALID },
  /* 8D */  { INVALID },
  /* 8E */  { "pfpnacc",  0, Pq, Qq, XX },
  /* 8F */  { INVALID },
  /* 90 */  { "pfcmpge",  0, Pq, Qq, XX },
  /* 91 */  { INVALID },
  /* 92 */  { INVALID },
  /* 93 */  { INVALID },
  /* 94 */  { "pfmin",    0, Pq, Qq, XX },
  /* 95 */  { INVALID },
  /* 96 */  { "pfrcp",    0, Pq, Qq, XX },
  /* 97 */  { "pfrsqrt",  0, Pq, Qq, XX },
  /* 98 */  { INVALID },
  /* 99 */  { INVALID },
  /* 9A */  { "pfsub",    0, Pq, Qq, XX },
  /* 9B */  { INVALID },
  /* 9C */  { INVALID },
  /* 9D */  { INVALID },
  /* 9E */  { "pfadd",    0, Pq, Qq, XX },
  /* 9F */  { INVALID },
  /* A0 */  { "pfcmpgt",  0, Pq, Qq, XX },
  /* A1 */  { INVALID },
  /* A2 */  { INVALID },
  /* A3 */  { INVALID },
  /* A4 */  { "pfmax",    0, Pq, Qq, XX },
  /* A5 */  { INVALID },
  /* A6 */  { "pfrcpit1", 0, Pq, Qq, XX },
  /* A7 */  { "pfrsqit1", 0, Pq, Qq, XX },
  /* A8 */  { INVALID },
  /* A9 */  { INVALID },
  /* AA */  { "pfsubr",   0, Pq, Qq, XX },
  /* AB */  { INVALID },
  /* AC */  { INVALID },
  /* AD */  { INVALID },
  /* AE */  { "pfacc",    0, Pq, Qq, XX },
  /* AF */  { INVALID },
  /* B0 */  { "pfcmpeq",  0, Pq, Qq, XX },
  /* B1 */  { INVALID },
  /* B2 */  { INVALID },
  /* B3 */  { INVALID },
  /* B4 */  { "pfmul",    0, Pq, Qq, XX },
  /* B5 */  { INVALID },
  /* B6 */  { "pfrcpit2", 0, Pq, Qq, XX },
  /* B7 */  { "pmulhrw",  0, Pq, Qq, XX },
  /* B8 */  { INVALID },    
  /* B9 */  { INVALID },
  /* BA */  { INVALID },
  /* BB */  { "pswapd",   0, Pq, Qq, XX },
  /* BC */  { INVALID },
  /* BD */  { INVALID },
  /* BE */  { INVALID },
  /* BF */  { "pavgb",    0, Pq, Qq, XX },
  /* C0 */  { INVALID },
  /* C1 */  { INVALID },
  /* C2 */  { INVALID },
  /* C3 */  { INVALID },
  /* C4 */  { INVALID },
  /* C5 */  { INVALID },
  /* C6 */  { INVALID },
  /* C7 */  { INVALID },
  /* C8 */  { INVALID },
  /* C9 */  { INVALID },
  /* CA */  { INVALID },
  /* CB */  { INVALID },
  /* CC */  { INVALID },
  /* CD */  { INVALID },
  /* CE */  { INVALID },
  /* CF */  { INVALID },
  /* D0 */  { INVALID },
  /* D1 */  { INVALID },
  /* D2 */  { INVALID },
  /* D3 */  { INVALID },
  /* D4 */  { INVALID },
  /* D5 */  { INVALID },
  /* D6 */  { INVALID },
  /* D7 */  { INVALID },
  /* D8 */  { INVALID },
  /* D9 */  { INVALID },
  /* DA */  { INVALID },
  /* DB */  { INVALID },
  /* DC */  { INVALID },
  /* DD */  { INVALID },
  /* DE */  { INVALID },
  /* DF */  { INVALID },
  /* E0 */  { INVALID },
  /* E1 */  { INVALID },
  /* E2 */  { INVALID },
  /* E3 */  { INVALID },
  /* E4 */  { INVALID },
  /* E5 */  { INVALID },
  /* E6 */  { INVALID },
  /* E7 */  { INVALID },
  /* E8 */  { INVALID },
  /* E9 */  { INVALID },
  /* EA */  { INVALID },
  /* EB */  { INVALID },
  /* EC */  { INVALID },
  /* ED */  { INVALID },
  /* EE */  { INVALID },
  /* EF */  { INVALID },
  /* F0 */  { INVALID },
  /* F1 */  { INVALID },
  /* F2 */  { INVALID },
  /* F3 */  { INVALID },
  /* F4 */  { INVALID },
  /* F5 */  { INVALID },
  /* F6 */  { INVALID },
  /* F7 */  { INVALID },
  /* F8 */  { INVALID },
  /* F9 */  { INVALID },
  /* FA */  { INVALID },
  /* FB */  { INVALID },
  /* FC */  { INVALID },
  /* FD */  { INVALID },
  /* FE */  { INVALID },
  /* FF */  { INVALID }
};                                

/* ************************************************************************ */
/* FPU Opcodes */

// floating point instructions when mod!=11b.
// the following tables will be accessed like groups using the nnn (reg) field of
// the modrm byte. (the first byte is D8-DF)

  // D8 (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupD8[8] = { 
  /* 0 */  { "faddL",       0, Md, XX, XX },
  /* 1 */  { "fmulL",       0, Md, XX, XX },
  /* 2 */  { "fcomL",       0, Md, XX, XX },
  /* 3 */  { "fcompL",      0, Md, XX, XX },
  /* 4 */  { "fsubL",       0, Md, XX, XX },
  /* 5 */  { "fsubrL",      0, Md, XX, XX },
  /* 6 */  { "fdivL",       0, Md, XX, XX },
  /* 7 */  { "fdivprL",     0, Md, XX, XX }
};

  // D9 (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupD9[8] = { 
  /* 0 */  { "fldL",        0, Md, XX, XX },
  /* 1 */  { INVALID },
  /* 2 */  { "fstL",        0, Md, XX, XX },
  /* 3 */  { "fstpL",       0, Md, XX, XX },
  /* 4 */  { "fldenv",      0, Mx, XX, XX },
  /* 5 */  { "fldcwW",      0, Ew, XX, XX },
  /* 6 */  { "fnstenv",     0, Mx, XX, XX },
  /* 7 */  { "fnstcwW",     0, Mw, XX, XX }
};

  // DA (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupDA[8] = { 
  /* 0 */  { "fiaddL",      0, Md, XX, XX },
  /* 1 */  { "fimulL",      0, Md, XX, XX },
  /* 2 */  { "ficomL",      0, Md, XX, XX },
  /* 3 */  { "ficompL",     0, Md, XX, XX },
  /* 4 */  { "fisubL",      0, Md, XX, XX },
  /* 5 */  { "fisubrL",     0, Md, XX, XX },
  /* 6 */  { "fidivL",      0, Md, XX, XX },
  /* 7 */  { "fidivrL",     0, Md, XX, XX }
};

  // DB (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupDB[8] = { 
  /* 0 */  { "fildL",       0, Md, XX, XX },
  /* 1 */  { "fisttpL",     0, Md, XX, XX },
  /* 2 */  { "fistL",       0, Md, XX, XX },
  /* 3 */  { "fistpL",      0, Md, XX, XX },
  /* 4 */  { INVALID },
  /* 5 */  { "fldT",        0, Mt, XX, XX },
  /* 6 */  { INVALID },
  /* 7 */  { "fstpT",       0, Mt, XX, XX }
};

  // DC (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupDC[8] = { 
  /* 0 */  { "faddQ",       0, Mq, XX, XX },
  /* 1 */  { "fmulQ",       0, Mq, XX, XX },
  /* 2 */  { "fcomQ",       0, Mq, XX, XX },
  /* 3 */  { "fcompQ",      0, Mq, XX, XX },
  /* 4 */  { "fsubQ",       0, Mq, XX, XX },
  /* 5 */  { "fsubrQ",      0, Mq, XX, XX },
  /* 6 */  { "fdivQ",       0, Mq, XX, XX },
  /* 7 */  { "fdivrQ",      0, Mq, XX, XX }
};

  // DD (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupDD[8] = { 
  /* 0 */  { "fldQ",        0, Mq, XX, XX },
  /* 1 */  { "fisttpQ",     0, Mq, XX, XX },
  /* 2 */  { "fstQ",        0, Mq, XX, XX },
  /* 3 */  { "fstpQ",       0, Mq, XX, XX },
  /* 4 */  { "frstor",      0, Mx, XX, XX },
  /* 5 */  { INVALID },
  /* 6 */  { "fnsave",      0, Mx, XX, XX },
  /* 7 */  { "fnstswW",     0, Mw, XX, XX }
};

  // DE (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupDE[8] = { 
  /* 0 */  { "fiaddW",      0, Mw, XX, XX },
  /* 1 */  { "fimulW",      0, Mw, XX, XX },
  /* 2 */  { "ficomW",      0, Mw, XX, XX },
  /* 3 */  { "ficompW",     0, Mw, XX, XX },
  /* 4 */  { "fisubW",      0, Mw, XX, XX },
  /* 5 */  { "fisubrW",     0, Mw, XX, XX },
  /* 6 */  { "fidivW",      0, Mw, XX, XX },
  /* 7 */  { "fidivrW",     0, Mw, XX, XX }
};

  // DF (modrm is outside 00h - BFh) (mod != 11)
static BxDisasmOpcodeInfo_t BxDisasmFPGroupDF[8] = { 
  /* 0 */  { "fildW",       0, Mw, XX, XX },
  /* 1 */  { "fisttpW",     0, Mw, XX, XX },
  /* 2 */  { "fistW",       0, Mw, XX, XX },
  /* 3 */  { "fistpW",      0, Mw, XX, XX },
  /* 4 */  { "fbldT",       0, Mt, XX, XX },
  /* 5 */  { "fildQ",       0, Mq, XX, XX },
  /* 6 */  { "fbstpT",      0, Mt, XX, XX },
  /* 7 */  { "fistpQ",      0, Mq, XX, XX }
};

// 512 entries for second byte of floating point instructions. (when mod==11b) 
static BxDisasmOpcodeInfo_t BxDisasmOpcodeInfoFP[512] = { 
  // D8 (modrm is outside 00h - BFh) (mod == 11)
  /* D8 C0 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C1 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C2 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C3 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C4 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C5 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C6 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C7 */  { "fadd",      0, ST0, STj, XX },
  /* D8 C8 */  { "fmul",      0, ST0, STj, XX },
  /* D8 C9 */  { "fmul",      0, ST0, STj, XX },
  /* D8 CA */  { "fmul",      0, ST0, STj, XX },
  /* D8 CB */  { "fmul",      0, ST0, STj, XX },
  /* D8 CC */  { "fmul",      0, ST0, STj, XX },
  /* D8 CD */  { "fmul",      0, ST0, STj, XX },
  /* D8 CE */  { "fmul",      0, ST0, STj, XX },
  /* D8 CF */  { "fmul",      0, ST0, STj, XX },
  /* D8 D0 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D1 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D2 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D3 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D4 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D5 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D6 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D7 */  { "fcom",      0, STj,  XX, XX },
  /* D8 D8 */  { "fcomp",     0, STj,  XX, XX },
  /* D8 D9 */  { "fcomp",     0, STj,  XX, XX },
  /* D8 DA */  { "fcomp",     0, STj,  XX, XX },
  /* D8 DB */  { "fcomp",     0, STj,  XX, XX },
  /* D8 DC */  { "fcomp",     0, STj,  XX, XX },
  /* D8 DD */  { "fcomp",     0, STj,  XX, XX },
  /* D8 DE */  { "fcomp",     0, STj,  XX, XX },
  /* D8 DF */  { "fcomp",     0, STj,  XX, XX },
  /* D8 E0 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E1 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E2 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E3 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E4 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E5 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E6 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E7 */  { "fsub",      0, ST0, STj, XX },
  /* D8 E8 */  { "fsubr",     0, ST0, STj, XX },
  /* D8 E9 */  { "fsubr",     0, ST0, STj, XX },
  /* D8 EA */  { "fsubr",     0, ST0, STj, XX },
  /* D8 EB */  { "fsubr",     0, ST0, STj, XX },
  /* D8 EC */  { "fsubr",     0, ST0, STj, XX },
  /* D8 ED */  { "fsubr",     0, ST0, STj, XX },
  /* D8 EE */  { "fsubr",     0, ST0, STj, XX },
  /* D8 EF */  { "fsubr",     0, ST0, STj, XX },
  /* D8 F0 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F1 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F2 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F3 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F4 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F5 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F6 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F7 */  { "fdiv",      0, ST0, STj, XX },
  /* D8 F8 */  { "fsubr",     0, ST0, STj, XX },
  /* D8 F9 */  { "fsubr",     0, ST0, STj, XX },
  /* D8 FA */  { "fsubr",     0, ST0, STj, XX },
  /* D8 FB */  { "fsubr",     0, ST0, STj, XX },
  /* D8 FC */  { "fsubr",     0, ST0, STj, XX },
  /* D8 FD */  { "fsubr",     0, ST0, STj, XX },
  /* D8 FE */  { "fsubr",     0, ST0, STj, XX },
  /* D8 FF */  { "fsubr",     0, ST0, STj, XX },
  
  // D9 (modrm is outside 00h - BFh) (mod == 11)
  /* D9 C0 */  { "fld",       0, STj,  XX, XX },
  /* D9 C1 */  { "fld",       0, STj,  XX, XX },
  /* D9 C2 */  { "fld",       0, STj,  XX, XX },
  /* D9 C3 */  { "fld",       0, STj,  XX, XX },
  /* D9 C4 */  { "fld",       0, STj,  XX, XX },
  /* D9 C5 */  { "fld",       0, STj,  XX, XX },
  /* D9 C6 */  { "fld",       0, STj,  XX, XX },
  /* D9 C7 */  { "fld",       0, STj,  XX, XX },
  /* D9 C8 */  { "fxch",      0, STj,  XX, XX },
  /* D9 C9 */  { "fxch",      0, STj,  XX, XX },
  /* D9 CA */  { "fxch",      0, STj,  XX, XX },
  /* D9 CB */  { "fxch",      0, STj,  XX, XX },
  /* D9 CC */  { "fxch",      0, STj,  XX, XX },
  /* D9 CD */  { "fxch",      0, STj,  XX, XX },
  /* D9 CE */  { "fxch",      0, STj,  XX, XX },
  /* D9 CF */  { "fxch",      0, STj,  XX, XX },
  /* D9 D0 */  { "fnop",      0,  XX,  XX, XX },
  /* D9 D1 */  { INVALID },
  /* D9 D2 */  { INVALID },
  /* D9 D3 */  { INVALID },
  /* D9 D4 */  { INVALID },
  /* D9 D5 */  { INVALID },
  /* D9 D6 */  { INVALID },
  /* D9 D7 */  { INVALID },
  /* D9 D8 */  { INVALID },
  /* D9 D9 */  { INVALID },
  /* D9 DA */  { INVALID },
  /* D9 DB */  { INVALID },
  /* D9 DC */  { INVALID },
  /* D9 DD */  { INVALID },
  /* D9 DE */  { INVALID },
  /* D9 DF */  { INVALID },
  /* D9 E0 */  { "fchs",      0,  XX,  XX, XX },
  /* D9 E1 */  { "fabs",      0,  XX,  XX, XX },
  /* D9 E2 */  { INVALID },
  /* D9 E3 */  { INVALID },
  /* D9 E4 */  { "ftst",      0,  XX,  XX, XX },
  /* D9 E5 */  { "fxam",      0,  XX,  XX, XX },
  /* D9 E6 */  { INVALID },
  /* D9 E7 */  { INVALID },
  /* D9 E8 */  { "fld1",      0,  XX,  XX, XX },
  /* D9 E9 */  { "fldl2t",    0,  XX,  XX, XX },
  /* D9 EA */  { "fldl2e",    0,  XX,  XX, XX },
  /* D9 EB */  { "fldpi",     0,  XX,  XX, XX },
  /* D9 EC */  { "fldlg2",    0,  XX,  XX, XX },
  /* D9 ED */  { "fldln2",    0,  XX,  XX, XX },
  /* D9 EE */  { "fldz",      0,  XX,  XX, XX },
  /* D9 EF */  { INVALID },
  /* D9 F0 */  { "f2xm1",     0,  XX,  XX, XX },
  /* D9 F1 */  { "fyl2x",     0,  XX,  XX, XX },
  /* D9 F2 */  { "fptan",     0,  XX,  XX, XX },
  /* D9 F3 */  { "fpatan",    0,  XX,  XX, XX },
  /* D9 F4 */  { "fxtract",   0,  XX,  XX, XX },
  /* D9 F5 */  { "fprem1",    0,  XX,  XX, XX },
  /* D9 F6 */  { "fdecstp",   0,  XX,  XX, XX },
  /* D9 F7 */  { "fincstp",   0,  XX,  XX, XX },
  /* D9 F8 */  { "fprem",     0,  XX,  XX, XX },
  /* D9 F9 */  { "fyl2xp1",   0,  XX,  XX, XX },
  /* D9 FA */  { "fsqrt",     0,  XX,  XX, XX },
  /* D9 FB */  { "fsincos",   0,  XX,  XX, XX },
  /* D9 FC */  { "frndint",   0,  XX,  XX, XX },
  /* D9 FD */  { "fscale",    0,  XX,  XX, XX },
  /* D9 FE */  { "fsin",      0,  XX,  XX, XX },
  /* D9 FF */  { "fcos",      0,  XX,  XX, XX },
                                  
  // DA (modrm is outside 00h - BFh) (mod == 11)
  /* DA C0 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C1 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C2 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C3 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C4 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C5 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C6 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C7 */  { "fcmovb",    0, ST0, STj, XX },
  /* DA C8 */  { "fcmove",    0, ST0, STj, XX },
  /* DA C9 */  { "fcmove",    0, ST0, STj, XX },
  /* DA CA */  { "fcmove",    0, ST0, STj, XX },
  /* DA CB */  { "fcmove",    0, ST0, STj, XX },
  /* DA CC */  { "fcmove",    0, ST0, STj, XX },
  /* DA CD */  { "fcmove",    0, ST0, STj, XX },
  /* DA CE */  { "fcmove",    0, ST0, STj, XX },
  /* DA CF */  { "fcmove",    0, ST0, STj, XX },
  /* DA D0 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D1 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D2 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D3 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D4 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D5 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D6 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D7 */  { "fcmovbe",   0, ST0, STj, XX },
  /* DA D8 */  { "fcmovu",    0, ST0, STj, XX },
  /* DA D9 */  { "fcmovu",    0, ST0, STj, XX },
  /* DA DA */  { "fcmovu",    0, ST0, STj, XX },
  /* DA DB */  { "fcmovu",    0, ST0, STj, XX },
  /* DA DC */  { "fcmovu",    0, ST0, STj, XX },
  /* DA DD */  { "fcmovu",    0, ST0, STj, XX },
  /* DA DE */  { "fcmovu",    0, ST0, STj, XX },
  /* DA DF */  { "fcmovu",    0, ST0, STj, XX },
  /* DA E0 */  { INVALID },
  /* DA E1 */  { INVALID },
  /* DA E2 */  { INVALID },
  /* DA E3 */  { INVALID },
  /* DA E4 */  { INVALID },
  /* DA E5 */  { INVALID },
  /* DA E6 */  { INVALID },
  /* DA E7 */  { INVALID },
  /* DA E8 */  { INVALID },
  /* DA E9 */  { "fucompp",   0,  XX,  XX, XX },
  /* DA EA */  { INVALID },
  /* DA EB */  { INVALID },
  /* DA EC */  { INVALID },
  /* DA ED */  { INVALID },
  /* DA EE */  { INVALID },
  /* DA EF */  { INVALID },
  /* DA F0 */  { INVALID },
  /* DA F1 */  { INVALID },
  /* DA F2 */  { INVALID },
  /* DA F3 */  { INVALID },
  /* DA F4 */  { INVALID },
  /* DA F5 */  { INVALID },
  /* DA F6 */  { INVALID },
  /* DA F7 */  { INVALID },
  /* DA F8 */  { INVALID },
  /* DA F9 */  { INVALID },
  /* DA FA */  { INVALID },
  /* DA FB */  { INVALID },
  /* DA FC */  { INVALID },
  /* DA FD */  { INVALID },
  /* DA FE */  { INVALID },
  /* DA FF */  { INVALID },

  // DB (modrm is outside 00h - BFh) (mod == 11)
  /* DB C0 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C1 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C2 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C3 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C4 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C5 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C6 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C7 */  { "fcmovnb",   0, ST0, STj, XX },
  /* DB C8 */  { "fcmovne",   0, ST0, STj, XX },
  /* DB C9 */  { "fcmovne",   0, ST0, STj, XX },
  /* DB CA */  { "fcmovne",   0, ST0, STj, XX },
  /* DB CB */  { "fcmovne",   0, ST0, STj, XX },
  /* DB CC */  { "fcmovne",   0, ST0, STj, XX },
  /* DB CD */  { "fcmovne",   0, ST0, STj, XX },
  /* DB CE */  { "fcmovne",   0, ST0, STj, XX },
  /* DB CF */  { "fcmovne",   0, ST0, STj, XX },
  /* DB D0 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D1 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D2 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D3 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D4 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D5 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D6 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D7 */  { "fcmovnbe",  0, ST0, STj, XX },
  /* DB D8 */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB D9 */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB DA */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB DB */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB DC */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB DD */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB DE */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB DF */  { "fcmovnu",   0, ST0, STj, XX },
  /* DB E0 */  { "feni   (287 only)", 0, XX, XX, XX },
  /* DB E1 */  { "fdisi  (287 only)", 0, XX, XX, XX },
  /* DB E2 */  { "fnclex",    0,  XX,  XX, XX },
  /* DB E3 */  { "fninit",    0,  XX,  XX, XX },
  /* DB E4 */  { "fsetpm (287 only)", 0, XX, XX, XX },
  /* DB E5 */  { INVALID },
  /* DB E6 */  { INVALID },
  /* DB E7 */  { INVALID },
  /* DB E8 */  { "fucomi",    0, STj, STj, XX },
  /* DB E9 */  { "fucomi",    0, STj, STj, XX },
  /* DB EA */  { "fucomi",    0, STj, STj, XX },
  /* DB EB */  { "fucomi",    0, STj, STj, XX },
  /* DB EC */  { "fucomi",    0, STj, STj, XX },
  /* DB ED */  { "fucomi",    0, STj, STj, XX },
  /* DB EE */  { "fucomi",    0, STj, STj, XX },
  /* DB EF */  { "fucomi",    0, STj, STj, XX },
  /* DB F0 */  { "fcomi",     0, STj, STj, XX },
  /* DB F1 */  { "fcomi",     0, STj, STj, XX },
  /* DB F2 */  { "fcomi",     0, STj, STj, XX },
  /* DB F3 */  { "fcomi",     0, STj, STj, XX },
  /* DB F4 */  { "fcomi",     0, STj, STj, XX },
  /* DB F5 */  { "fcomi",     0, STj, STj, XX },
  /* DB F6 */  { "fcomi",     0, STj, STj, XX },
  /* DB F7 */  { "fcomi",     0, STj, STj, XX },
  /* DB F8 */  { INVALID },
  /* DB F9 */  { INVALID },
  /* DB FA */  { INVALID },
  /* DB FB */  { INVALID },
  /* DB FC */  { INVALID },
  /* DB FD */  { INVALID },
  /* DB FE */  { INVALID },
  /* DB FF */  { INVALID },

  // DC (modrm is outside 00h - BFh) (mod == 11)
  /* DC C0 */  { "fadd",      0, STj, ST0, XX },
  /* DC C1 */  { "fadd",      0, STj, ST0, XX },
  /* DC C2 */  { "fadd",      0, STj, ST0, XX },
  /* DC C3 */  { "fadd",      0, STj, ST0, XX },
  /* DC C4 */  { "fadd",      0, STj, ST0, XX },
  /* DC C5 */  { "fadd",      0, STj, ST0, XX },
  /* DC C6 */  { "fadd",      0, STj, ST0, XX },
  /* DC C7 */  { "fadd",      0, STj, ST0, XX },
  /* DC C8 */  { "fmul",      0, STj, ST0, XX },
  /* DC C9 */  { "fmul",      0, STj, ST0, XX },
  /* DC CA */  { "fmul",      0, STj, ST0, XX },
  /* DC CB */  { "fmul",      0, STj, ST0, XX },
  /* DC CC */  { "fmul",      0, STj, ST0, XX },
  /* DC CD */  { "fmul",      0, STj, ST0, XX },
  /* DC CE */  { "fmul",      0, STj, ST0, XX },
  /* DC CF */  { "fmul",      0, STj, ST0, XX },
  /* DC D0 */  { INVALID },
  /* DC D1 */  { INVALID },
  /* DC D2 */  { INVALID },
  /* DC D3 */  { INVALID },
  /* DC D4 */  { INVALID },
  /* DC D5 */  { INVALID },
  /* DC D6 */  { INVALID },
  /* DC D7 */  { INVALID },
  /* DC D8 */  { INVALID },
  /* DC D9 */  { INVALID },
  /* DC DA */  { INVALID },
  /* DC DB */  { INVALID },
  /* DC DC */  { INVALID },
  /* DC DD */  { INVALID },
  /* DC DE */  { INVALID },
  /* DC DF */  { INVALID },
  /* DC E0 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E1 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E2 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E3 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E4 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E5 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E6 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E7 */  { "fsubr",     0, STj, ST0, XX },
  /* DC E8 */  { "fsub",      0, STj, ST0, XX },
  /* DC E9 */  { "fsub",      0, STj, ST0, XX },
  /* DC EA */  { "fsub",      0, STj, ST0, XX },
  /* DC EB */  { "fsub",      0, STj, ST0, XX },
  /* DC EC */  { "fsub",      0, STj, ST0, XX },
  /* DC ED */  { "fsub",      0, STj, ST0, XX },
  /* DC EE */  { "fsub",      0, STj, ST0, XX },
  /* DC EF */  { "fsub",      0, STj, ST0, XX },
  /* DC F0 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F1 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F2 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F3 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F4 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F5 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F6 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F7 */  { "fdivr",     0, STj, ST0, XX },
  /* DC F8 */  { "fdiv",      0, STj, ST0, XX },
  /* DC F9 */  { "fdiv",      0, STj, ST0, XX },
  /* DC FA */  { "fdiv",      0, STj, ST0, XX },
  /* DC FB */  { "fdiv",      0, STj, ST0, XX },
  /* DC FC */  { "fdiv",      0, STj, ST0, XX },
  /* DC FD */  { "fdiv",      0, STj, ST0, XX },
  /* DC FE */  { "fdiv",      0, STj, ST0, XX },
  /* DC FF */  { "fdiv",      0, STj, ST0, XX },

  // DD (modrm is outside 00h - BFh) (mod == 11)
  /* DD C0 */  { "ffree",     0, STj,  XX, XX },
  /* DD C1 */  { "ffree",     0, STj,  XX, XX },
  /* DD C2 */  { "ffree",     0, STj,  XX, XX },
  /* DD C3 */  { "ffree",     0, STj,  XX, XX },
  /* DD C4 */  { "ffree",     0, STj,  XX, XX },
  /* DD C5 */  { "ffree",     0, STj,  XX, XX },
  /* DD C6 */  { "ffree",     0, STj,  XX, XX },
  /* DD C7 */  { "ffree",     0, STj,  XX, XX },
  /* DD C8 */  { INVALID },
  /* DD C9 */  { INVALID },
  /* DD CA */  { INVALID },
  /* DD CB */  { INVALID },
  /* DD CC */  { INVALID },
  /* DD CD */  { INVALID },
  /* DD CE */  { INVALID },
  /* DD CF */  { INVALID },
  /* DD D0 */  { "fst",       0, STj,  XX, XX },
  /* DD D1 */  { "fst",       0, STj,  XX, XX },
  /* DD D2 */  { "fst",       0, STj,  XX, XX },
  /* DD D3 */  { "fst",       0, STj,  XX, XX },
  /* DD D4 */  { "fst",       0, STj,  XX, XX },
  /* DD D5 */  { "fst",       0, STj,  XX, XX },
  /* DD D6 */  { "fst",       0, STj,  XX, XX },
  /* DD D7 */  { "fst",       0, STj,  XX, XX },
  /* DD D8 */  { "fstp",      0, STj,  XX, XX },
  /* DD D9 */  { "fstp",      0, STj,  XX, XX },
  /* DD DA */  { "fstp",      0, STj,  XX, XX },
  /* DD DB */  { "fstp",      0, STj,  XX, XX },
  /* DD DC */  { "fstp",      0, STj,  XX, XX },
  /* DD DD */  { "fstp",      0, STj,  XX, XX },
  /* DD DE */  { "fstp",      0, STj,  XX, XX },
  /* DD DF */  { "fstp",      0, STj,  XX, XX },
  /* DD E0 */  { "fucom",     0, STj,  XX, XX },
  /* DD E1 */  { "fucom",     0, STj,  XX, XX },
  /* DD E2 */  { "fucom",     0, STj,  XX, XX },
  /* DD E3 */  { "fucom",     0, STj,  XX, XX },
  /* DD E4 */  { "fucom",     0, STj,  XX, XX },
  /* DD E5 */  { "fucom",     0, STj,  XX, XX },
  /* DD E6 */  { "fucom",     0, STj,  XX, XX },
  /* DD E7 */  { "fucom",     0, STj,  XX, XX },
  /* DD E8 */  { "fucomp",    0, STj,  XX, XX },
  /* DD E9 */  { "fucomp",    0, STj,  XX, XX },
  /* DD EA */  { "fucomp",    0, STj,  XX, XX },
  /* DD EB */  { "fucomp",    0, STj,  XX, XX },
  /* DD EC */  { "fucomp",    0, STj,  XX, XX },
  /* DD ED */  { "fucomp",    0, STj,  XX, XX },
  /* DD EE */  { "fucomp",    0, STj,  XX, XX },
  /* DD EF */  { "fucomp",    0, STj,  XX, XX },
  /* DD F0 */  { INVALID },
  /* DD F1 */  { INVALID },
  /* DD F2 */  { INVALID },
  /* DD F3 */  { INVALID },
  /* DD F4 */  { INVALID },
  /* DD F5 */  { INVALID },
  /* DD F6 */  { INVALID },
  /* DD F7 */  { INVALID },
  /* DD F8 */  { INVALID },
  /* DD F9 */  { INVALID },
  /* DD FA */  { INVALID },
  /* DD FB */  { INVALID },
  /* DD FC */  { INVALID },
  /* DD FD */  { INVALID },
  /* DD FE */  { INVALID },
  /* DD FF */  { INVALID },

  // DE (modrm is outside 00h - BFh) (mod == 11)
  /* DE C0 */  { "faddp",     0, STj, ST0, XX },
  /* DE C1 */  { "faddp",     0, STj, ST0, XX },
  /* DE C2 */  { "faddp",     0, STj, ST0, XX },
  /* DE C3 */  { "faddp",     0, STj, ST0, XX },
  /* DE C4 */  { "faddp",     0, STj, ST0, XX },
  /* DE C5 */  { "faddp",     0, STj, ST0, XX },
  /* DE C6 */  { "faddp",     0, STj, ST0, XX },
  /* DE C7 */  { "faddp",     0, STj, ST0, XX },
  /* DE C8 */  { "fmulp",     0, STj, ST0, XX },
  /* DE C9 */  { "fmulp",     0, STj, ST0, XX },
  /* DE CA */  { "fmulp",     0, STj, ST0, XX },
  /* DE CB */  { "fmulp",     0, STj, ST0, XX },
  /* DE CC */  { "fmulp",     0, STj, ST0, XX },
  /* DE CD */  { "fmulp",     0, STj, ST0, XX },
  /* DE CE */  { "fmulp",     0, STj, ST0, XX },
  /* DE CF */  { "fmulp",     0, STj, ST0, XX },
  /* DE D0 */  { INVALID },
  /* DE D1 */  { INVALID },
  /* DE D2 */  { INVALID },
  /* DE D3 */  { INVALID },
  /* DE D4 */  { INVALID },
  /* DE D5 */  { INVALID },
  /* DE D6 */  { INVALID },
  /* DE D7 */  { INVALID },
  /* DE D8 */  { INVALID },
  /* DE D9 */  { "fcompp",    0,  XX,  XX, XX },
  /* DE DA */  { INVALID },
  /* DE DB */  { INVALID },
  /* DE DC */  { INVALID },
  /* DE DD */  { INVALID },
  /* DE DE */  { INVALID },
  /* DE DF */  { INVALID },
  /* DE E0 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E1 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E2 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E3 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E4 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E5 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E6 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E7 */  { "fsubrp",    0, STj, ST0, XX },
  /* DE E8 */  { "fsubp",     0, STj, ST0, XX },
  /* DE E9 */  { "fsubp",     0, STj, ST0, XX },
  /* DE EA */  { "fsubp",     0, STj, ST0, XX },
  /* DE EB */  { "fsubp",     0, STj, ST0, XX },
  /* DE EC */  { "fsubp",     0, STj, ST0, XX },
  /* DE ED */  { "fsubp",     0, STj, ST0, XX },
  /* DE EE */  { "fsubp",     0, STj, ST0, XX },
  /* DE EF */  { "fsubp",     0, STj, ST0, XX },
  /* DE F0 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F1 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F2 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F3 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F4 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F5 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F6 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F7 */  { "fdivrp",    0, STj, ST0, XX },
  /* DE F8 */  { "fdivp",     0, STj, ST0, XX },
  /* DE F9 */  { "fdivp",     0, STj, ST0, XX },
  /* DE FA */  { "fdivp",     0, STj, ST0, XX },
  /* DE FB */  { "fdivp",     0, STj, ST0, XX },
  /* DE FC */  { "fdivp",     0, STj, ST0, XX },
  /* DE FD */  { "fdivp",     0, STj, ST0, XX },
  /* DE FE */  { "fdivp",     0, STj, ST0, XX },
  /* DE FF */  { "fdivp",     0, STj, ST0, XX },

  // DF (modrm is outside 00h - BFh) (mod == 11)
  /* DF C0 */  { "ffreep",    0, STj,  XX, XX }, // 287+ compatibility opcode
  /* DF C1 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C2 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C3 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C4 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C5 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C6 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C7 */  { "ffreep",    0, STj,  XX, XX },
  /* DF C8 */  { INVALID },
  /* DF C9 */  { INVALID },
  /* DF CA */  { INVALID },
  /* DF CB */  { INVALID },
  /* DF CC */  { INVALID },
  /* DF CD */  { INVALID },
  /* DF CE */  { INVALID },
  /* DF CF */  { INVALID },
  /* DF D0 */  { INVALID },
  /* DF D1 */  { INVALID },
  /* DF D2 */  { INVALID },
  /* DF D3 */  { INVALID },
  /* DF D4 */  { INVALID },
  /* DF D5 */  { INVALID },
  /* DF D6 */  { INVALID },
  /* DF D7 */  { INVALID },
  /* DF D8 */  { INVALID },
  /* DF D9 */  { INVALID },
  /* DF DA */  { INVALID },
  /* DF DB */  { INVALID },
  /* DF DC */  { INVALID },
  /* DF DD */  { INVALID },
  /* DF DE */  { INVALID },
  /* DF DF */  { INVALID },
  /* DF E0 */  { "fnstsw",    0,  AX,  XX, XX },
  /* DF E1 */  { INVALID },
  /* DF E2 */  { INVALID },
  /* DF E3 */  { INVALID },
  /* DF E4 */  { INVALID },
  /* DF E5 */  { INVALID },
  /* DF E6 */  { INVALID },
  /* DF E7 */  { INVALID },
  /* DF E8 */  { "fucomip",   0, ST0, STj, XX },
  /* DF E9 */  { "fucomip",   0, ST0, STj, XX },
  /* DF EA */  { "fucomip",   0, ST0, STj, XX },
  /* DF EB */  { "fucomip",   0, ST0, STj, XX },
  /* DF EC */  { "fucomip",   0, ST0, STj, XX },
  /* DF ED */  { "fucomip",   0, ST0, STj, XX },
  /* DF EE */  { "fucomip",   0, ST0, STj, XX },
  /* DF EF */  { "fucomip",   0, ST0, STj, XX },
  /* DF F0 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F1 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F2 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F3 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F4 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F5 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F6 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F7 */  { "fcomip",    0, ST0, STj, XX },
  /* DF F8 */  { INVALID },
  /* DF F9 */  { INVALID },
  /* DF FA */  { INVALID },
  /* DF FB */  { INVALID },
  /* DF FC */  { INVALID },
  /* DF FD */  { INVALID },
  /* DF FE */  { INVALID },
  /* DF FF */  { INVALID },
};

static BxDisasmOpcodeInfo_t BxDisasmOpcodes[256*2] = {
  // 256 entries for single byte opcodes
  /* 00 */  { "addB",      0,  Eb,  Gb, XX },
  /* 01 */  { "addV",      0,  Ev,  Gv, XX },
  /* 02 */  { "addB",      0,  Gb,  Eb, XX },
  /* 03 */  { "addV",      0,  Gv,  Ev, XX },
  /* 04 */  { "addB",      0,  AL,  Ib, XX },
  /* 05 */  { "addV",      0, eAX,  Iv, XX },
  /* 06 */  { "push",      0,  ES,  XX, XX },
  /* 07 */  { "pop",       0,  ES,  XX, XX },
  /* 08 */  { "orB",       0,  Eb,  Gb, XX },
  /* 09 */  { "orV",       0,  Ev,  Gv, XX },
  /* 0A */  { "orB",       0,  Gb,  Eb, XX },
  /* 0B */  { "orV",       0,  Gv,  Ev, XX },
  /* 0C */  { "orB",       0,  AL,  Ib, XX },
  /* 0D */  { "orV",       0, eAX,  Iv, XX },
  /* 0E */  { "push",      0,  CS,  XX, XX },
  /* 0F */  { "(error)",   0,  XX,  XX, XX },   // 2 byte escape
  /* 10 */  { "adcB",      0,  Eb,  Gb, XX },
  /* 11 */  { "adcV",      0,  Ev,  Gv, XX },
  /* 12 */  { "adcB",      0,  Gb,  Eb, XX },
  /* 13 */  { "adcV",      0,  Gv,  Ev, XX },
  /* 14 */  { "adcB",      0,  AL,  Ib, XX },
  /* 15 */  { "adcV",      0, eAX,  Iv, XX },
  /* 16 */  { "push",      0,  SS,  XX, XX },
  /* 17 */  { "pop",       0,  SS,  XX, XX },
  /* 18 */  { "sbbB",      0,  Eb,  Gb, XX },
  /* 19 */  { "sbbV",      0,  Ev,  Gv, XX },
  /* 1A */  { "sbbB",      0,  Gb,  Eb, XX },
  /* 1B */  { "sbbV",      0,  Gv,  Ev, XX },
  /* 1C */  { "sbbB",      0,  AL,  Ib, XX },
  /* 1D */  { "sbbV",      0, eAX,  Iv, XX },
  /* 1E */  { "push",      0,  DS,  XX, XX },
  /* 1F */  { "pop",       0,  DS,  XX, XX },
  /* 20 */  { "andB",      0,  Eb,  Gb, XX },
  /* 21 */  { "andV",      0,  Ev,  Gv, XX },
  /* 22 */  { "andB",      0,  Gb,  Eb, XX },
  /* 23 */  { "andV",      0,  Gv,  Ev, XX },
  /* 24 */  { "andB",      0,  AL,  Ib, XX },
  /* 25 */  { "andV",      0, eAX,  Iv, XX },
  /* 26 */  { PREFIX_ES },      // ES:
  /* 27 */  { "daa",       0,  XX,  XX, XX },
  /* 28 */  { "subB",      0,  Eb,  Gb, XX },
  /* 29 */  { "subV",      0,  Ev,  Gv, XX },
  /* 2A */  { "subB",      0,  Gb,  Eb, XX },
  /* 2B */  { "subV",      0,  Gv,  Ev, XX },
  /* 2C */  { "subB",      0,  AL,  Ib, XX },
  /* 2D */  { "subV",      0, eAX,  Iv, XX },
  /* 2E */  { PREFIX_CS },      // CS:
  /* 2F */  { "das",       0,  XX,  XX, XX },
  /* 30 */  { "xorB",      0,  Eb,  Gb, XX },
  /* 31 */  { "xorV",      0,  Ev,  Gv, XX },
  /* 32 */  { "xorB",      0,  Gb,  Eb, XX },
  /* 33 */  { "xorV",      0,  Gv,  Ev, XX },
  /* 34 */  { "xorB",      0,  AL,  Ib, XX },
  /* 35 */  { "xorV",      0, eAX,  Iv, XX },
  /* 36 */  { PREFIX_SS },      // SS:
  /* 37 */  { "aaa",       0,  XX,  XX, XX },
  /* 38 */  { "cmpB",      0,  Eb,  Gb, XX },
  /* 39 */  { "cmpV",      0,  Ev,  Gv, XX },
  /* 3A */  { "cmpB",      0,  Gb,  Eb, XX },
  /* 3B */  { "cmpV",      0,  Gv,  Ev, XX },
  /* 3C */  { "cmpB",      0,  AL,  Ib, XX },
  /* 3D */  { "cmpV",      0, eAX,  Iv, XX },
  /* 3E */  { PREFIX_DS },      // DS:
  /* 3F */  { "aas",       0,  XX,  XX, XX },
  /* 40 */  { "incV",      0, eAX,  XX, XX },
  /* 41 */  { "incV",      0, eCX,  XX, XX },
  /* 42 */  { "incV",      0, eDX,  XX, XX },
  /* 43 */  { "incV",      0, eBX,  XX, XX },
  /* 44 */  { "incV",      0, eSP,  XX, XX },
  /* 45 */  { "incV",      0, eBP,  XX, XX },
  /* 46 */  { "incV",      0, eSI,  XX, XX },
  /* 47 */  { "incV",      0, eDI,  XX, XX },
  /* 48 */  { "decV",      0, eAX,  XX, XX },
  /* 49 */  { "decV",      0, eCX,  XX, XX },
  /* 4A */  { "decV",      0, eDX,  XX, XX },
  /* 4B */  { "decV",      0, eBX,  XX, XX },
  /* 4C */  { "decV",      0, eSP,  XX, XX },
  /* 4D */  { "decV",      0, eBP,  XX, XX },
  /* 4E */  { "decV",      0, eSI,  XX, XX },
  /* 4F */  { "decV",      0, eDI,  XX, XX },
  /* 50 */  { "pushV",     0, eAX,  XX, XX },
  /* 51 */  { "pushV",     0, eCX,  XX, XX },
  /* 52 */  { "pushV",     0, eDX,  XX, XX },
  /* 53 */  { "pushV",     0, eBX,  XX, XX },
  /* 54 */  { "pushV",     0, eSP,  XX, XX },
  /* 55 */  { "pushV",     0, eBP,  XX, XX },
  /* 56 */  { "pushV",     0, eSI,  XX, XX },
  /* 57 */  { "pushV",     0, eDI,  XX, XX },
  /* 58 */  { "popV",      0, eAX,  XX, XX },
  /* 59 */  { "popV",      0, eCX,  XX, XX },
  /* 5A */  { "popV",      0, eDX,  XX, XX },
  /* 5B */  { "popV",      0, eBX,  XX, XX },
  /* 5C */  { "popV",      0, eSP,  XX, XX },
  /* 5D */  { "popV",      0, eBP,  XX, XX },
  /* 5E */  { "popV",      0, eSI,  XX, XX },
  /* 5F */  { "popV",      0, eDI,  XX, XX },
  /* 60 */  { "pushaD",    0,  XX,  XX, XX },
  /* 61 */  { "popaD",     0,  XX,  XX, XX },
  /* 62 */  { "boundW",    0,  Gv,  Mx, XX },
  /* 63 */  { "arplW",     0,  Ew,  Rw, XX },
  /* 64 */  { PREFIX_FS },      // FS:
  /* 65 */  { PREFIX_GS },      // GS:
  /* 66 */  { PREFIX_OPSIZE },
  /* 67 */  { PREFIX_ADDRSIZE },
  /* 68 */  { "pushV",     0,  Iv,  XX, XX },
  /* 69 */  { "imulV",     0,  Gv,  Ev, Iv },
  /* 6A */  { "pushV",     0, sIb,  XX, XX },   // sign extended immediate
  /* 6B */  { "imulV",     0,  Gv,  Ev, sIb },   
  /* 6C */  { "insb",      0,  Yb,  DX, XX },
  /* 6D */  { "insS",      0,  Yv,  DX, XX },
  /* 6E */  { "outsb",     0,  DX,  Xb, XX },
  /* 6F */  { "outsS",     0,  DX,  Xv, XX },
  /* 70 */  { "jo",        0,  Jb,  XX, Cond_Jump },
  /* 71 */  { "jno",       0,  Jb,  XX, Cond_Jump },
  /* 72 */  { "jb",        0,  Jb,  XX, Cond_Jump },
  /* 73 */  { "jnb",       0,  Jb,  XX, Cond_Jump },
  /* 74 */  { "jz",        0,  Jb,  XX, Cond_Jump },
  /* 75 */  { "jnz",       0,  Jb,  XX, Cond_Jump },
  /* 76 */  { "jbe",       0,  Jb,  XX, Cond_Jump },
  /* 77 */  { "jnbe",      0,  Jb,  XX, Cond_Jump },
  /* 78 */  { "js",        0,  Jb,  XX, Cond_Jump },
  /* 79 */  { "jns",       0,  Jb,  XX, Cond_Jump },
  /* 7A */  { "jp",        0,  Jb,  XX, Cond_Jump },
  /* 7B */  { "jnp",       0,  Jb,  XX, Cond_Jump },
  /* 7C */  { "jl",        0,  Jb,  XX, Cond_Jump },
  /* 7D */  { "jnl",       0,  Jb,  XX, Cond_Jump },
  /* 7E */  { "jle",       0,  Jb,  XX, Cond_Jump },
  /* 7F */  { "jnle",      0,  Jb,  XX, Cond_Jump },
  /* 80 */  { GRPN(G1EbIb) },
  /* 81 */  { GRPN(G1EvIv) },
  /* 82 */  { GRPN(G1EbIb) },
  /* 83 */  { GRPN(G1EvIb) },
  /* 84 */  { "testB",     0,  Eb,  Gb, XX },
  /* 85 */  { "testV",     0,  Ev,  Gv, XX },
  /* 86 */  { "xchgB",     0,  Eb,  Gb, XX },
  /* 87 */  { "xchgV",     0,  Ev,  Gv, XX },
  /* 88 */  { "movB",      0,  Eb,  Gb, XX },
  /* 89 */  { "movV",      0,  Ev,  Gv, XX },
  /* 8A */  { "movB",      0,  Gb,  Eb, XX },
  /* 8B */  { "movV",      0,  Gv,  Ev, XX },
  /* 8C */  { "movW",      0,  Ew,  Sw, XX },
  /* 8D */  { "leaV",      0,  Gv,  Mv, XX },
  /* 8E */  { "movW",      0,  Sw,  Ew, XX },
  /* 8F */  { "popV",      0,  Ev,  XX, XX },
  /* 90 */  { "nop",       0,  XX,  XX, XX },
  /* 91 */  { "xchgV",     0, eCX, eAX, XX },
  /* 92 */  { "xchgV",     0, eDX, eAX, XX },
  /* 93 */  { "xchgV",     0, eBX, eAX, XX },
  /* 94 */  { "xchgV",     0, eSP, eAX, XX },
  /* 95 */  { "xchgV",     0, eBP, eAX, XX },
  /* 96 */  { "xchgV",     0, eSI, eAX, XX },
  /* 97 */  { "xchgV",     0, eDI, eAX, XX },
  /* 98 */  { "cbw|cwde",  0,  XX,  XX, XX },
  /* 99 */  { "cwd|cdq",   0,  XX,  XX, XX },
  /* 9A */  { "call far",  0,  Ap,  XX, XX },
  /* 9B */  { "fwait",     0,  XX,  XX, XX },
  /* 9C */  { "pushfD",    0,  XX,  XX, XX },
  /* 9D */  { "popfD",     0,  XX,  XX, XX },
  /* 9E */  { "sahf",      0,  XX,  XX, XX },
  /* 9F */  { "lahf",      0,  XX,  XX, XX },
  /* A0 */  { "movB",      0,  AL,  Ob, XX },
  /* A1 */  { "movV",      0, eAX,  Ov, XX },
  /* A2 */  { "movB",      0,  Ob,  AL, XX },
  /* A3 */  { "movV",      0,  Ov, eAX, XX },
  /* A4 */  { "movsb",     0,  Yb,  Xb, XX },
  /* A5 */  { "movsS",     0,  Yv,  Xv, XX },
  /* A6 */  { "cmpsb",     0,  Yb,  Xb, XX },
  /* A7 */  { "cmpsS",     0,  Yv,  Xv, XX },
  /* A8 */  { "testB",     0,  AL,  Ib, XX },
  /* A9 */  { "testV",     0, eAX,  Iv, XX },
  /* AA */  { "stosb",     0,  Yb,  AL, XX },
  /* AB */  { "stosS",     0,  Yv, eAX, XX },
  /* AC */  { "lodsb",     0,  AL,  Xb, XX },
  /* AD */  { "lodsS",     0, eAX,  Xv, XX },
  /* AE */  { "scasb",     0,  Yb,  AL, XX },
  /* AF */  { "scasS",     0,  Yv, eAX, XX },
  /* B0 */  { "movB",      0,  AL,  Ib, XX },
  /* B1 */  { "movB",      0,  CL,  Ib, XX },
  /* B2 */  { "movB",      0,  DL,  Ib, XX },
  /* B3 */  { "movB",      0,  BL,  Ib, XX },
  /* B4 */  { "movB",      0,  AH,  Ib, XX },
  /* B5 */  { "movB",      0,  CH,  Ib, XX },
  /* B6 */  { "movB",      0,  DH,  Ib, XX },
  /* B7 */  { "movB",      0,  BH,  Ib, XX },
  /* B8 */  { "movV",      0, eAX,  Iv, XX },   
  /* B9 */  { "movV",      0, eCX,  Iv, XX },
  /* BA */  { "movV",      0, eDX,  Iv, XX },
  /* BB */  { "movV",      0, eBX,  Iv, XX },
  /* BC */  { "movV",      0, eSP,  Iv, XX },
  /* BD */  { "movV",      0, eBP,  Iv, XX },
  /* BE */  { "movV",      0, eSI,  Iv, XX },
  /* BF */  { "movV",      0, eDI,  Iv, XX },
  /* C0 */  { GRPN(G2Eb) },
  /* C1 */  { GRPN(G2Ev) },
  /* C2 */  { "retnW",     0,  Iw,  XX, XX },
  /* C3 */  { "retn",      0,  XX,  XX, XX },
  /* C4 */  { "les",       0,  Gv,  Mp, XX },
  /* C5 */  { "lds",       0,  Gv,  Mp, XX },
  /* C6 */  { "movB",      0,  Eb,  Ib, XX },
  /* C7 */  { "movV",      0,  Ev,  Iv, XX },
  /* C8 */  { "enter",     0,  Iw,  Ib, XX },
  /* C9 */  { "leave",     0,  XX,  XX, XX },
  /* CA */  { "retfW",     0,  Iw,  XX, XX },
  /* CB */  { "retf",      0,  XX,  XX, XX },
  /* CC */  { "int3",      0,  XX,  XX, XX },
  /* CD */  { "int",       0,  Ib,  XX, XX },
  /* CE */  { "into",      0,  XX,  XX, XX },
  /* CF */  { "iretD",     0,  XX,  XX, XX },
  /* D0 */  { GRPN(G2Eb1) },
  /* D1 */  { GRPN(G2Ev1) },
  /* D2 */  { GRPN(G2EbCL) },
  /* D3 */  { GRPN(G2EvCL) },
  /* D4 */  { "aam",       0,  Ib,  XX, XX },
  /* D5 */  { "aad",       0,  Ib,  XX, XX },
  /* D6 */  { "salc",      0,  XX,  XX, XX },
  /* D7 */  { "xlat",      0,  XX,  XX, XX },
  /* D8 */  { GRPFP(D8) },
  /* D9 */  { GRPFP(D9) },
  /* DA */  { GRPFP(DA) },
  /* DB */  { GRPFP(DB) },
  /* DC */  { GRPFP(DC) },
  /* DD */  { GRPFP(DD) },
  /* DE */  { GRPFP(DE) },
  /* DF */  { GRPFP(DF) },
  /* E0 */  { "loopne",    0,  Jb,  XX, XX },
  /* E1 */  { "loope",     0,  Jb,  XX, XX },
  /* E2 */  { "loop",      0,  Jb,  XX, XX },
  /* E3 */  { "jcxz",      0,  Jb,  XX, XX },
  /* E4 */  { "inB",       0,  AL,  Ib, XX },
  /* E5 */  { "inV",       0, eAX,  Ib, XX },
  /* E6 */  { "outB",      0,  Ib,  AL, XX },
  /* E7 */  { "outV",      0,  Ib, eAX, XX },
  /* E8 */  { "callV",     0,  Jv,  XX, XX },
  /* E9 */  { "jmpV",      0,  Jv,  XX, XX },
  /* EA */  { "jmp far",   0,  Ap,  XX, XX },
  /* EB */  { "jmpB",      0,  Jb,  XX, XX },
  /* EC */  { "inB",       0,  AL,  DX, XX },
  /* ED */  { "inV",       0, eAX,  DX, XX },
  /* EE */  { "outB",      0,  DX,  AL, XX },
  /* EF */  { "outV",      0,  DX, eAX, XX },
  /* F0 */  { PREFIX_LOCK },    // LOCK:
  /* F1 */  { "int1",      0,  XX,  XX, XX },
  /* F2 */  { PREFIX_REPNE },   // REPNE:
  /* F3 */  { PREFIX_REP },     // REP:
  /* F4 */  { "hlt",       0,  XX,  XX, XX },
  /* F5 */  { "cmc",       0,  XX,  XX, XX },
  /* F6 */  { GRPN(G3Eb) },
  /* F7 */  { GRPN(G3Ev) },
  /* F8 */  { "clc",       0,  XX,  XX, XX },
  /* F9 */  { "stc",       0,  XX,  XX, XX },
  /* FA */  { "cli",       0,  XX,  XX, XX },
  /* FB */  { "sti",       0,  XX,  XX, XX },
  /* FC */  { "cld",       0,  XX,  XX, XX },
  /* FD */  { "std",       0,  XX,  XX, XX },
  /* FE */  { GRPN(G4) },
  /* FF */  { GRPN(G5) },

  // 256 entries for two byte opcodes
  /* 0F 00 */  { GRPN(G6) },
  /* 0F 01 */  { GRPN(G7) },
  /* 0F 02 */  { "larV",      0,  Gv,  Ew, XX },
  /* 0F 03 */  { "lslV",      0,  Gv,  Ew, XX },
  /* 0F 04 */  { INVALID },
  /* 0F 05 */  { "syscall",   0,  XX,  XX, XX },
  /* 0F 06 */  { "clts",      0,  XX,  XX, XX },
  /* 0F 07 */  { "sysret",    0,  XX,  XX, XX },
  /* 0F 08 */  { "invd",      0,  XX,  XX, XX },
  /* 0F 09 */  { "wbinvd",    0,  XX,  XX, XX },
  /* 0F 0A */  { INVALID },
  /* 0F 0B */  { "ud2a",      0,  XX,  XX, XX },
  /* 0F 0C */  { INVALID },
  /* 0F 0D */  { "prefetch",  0,  Mb,  XX, XX }, // 3DNow!
  /* 0F 0E */  { "femms",     0,  XX,  XX, XX }, // 3DNow!
  /* 0F 0F */  { GRP3DNOW     },
  /* 0F 10 */  { GRPSSE(0f10) },
  /* 0F 11 */  { GRPSSE(0f11) },
  /* 0F 12 */  { GRPSSE(0f12) },
  /* 0F 13 */  { GRPSSE(0f13) },
  /* 0F 14 */  { GRPSSE(0f14) },
  /* 0F 15 */  { GRPSSE(0f15) },
  /* 0F 16 */  { GRPSSE(0f16) },
  /* 0F 17 */  { GRPSSE(0f17) },
  /* 0F 18 */  { GRPN(G16) },
  /* 0F 19 */  { INVALID },
  /* 0F 1A */  { INVALID },
  /* 0F 1B */  { INVALID },
  /* 0F 1C */  { INVALID },
  /* 0F 1D */  { INVALID },
  /* 0F 1E */  { INVALID },
  /* 0F 1F */  { INVALID },
  /* 0F 20 */  { "mov",        0,  Rd,  Cd, XX },
  /* 0F 21 */  { "mov",        0,  Rd,  Dd, XX },
  /* 0F 22 */  { "mov",        0,  Cd,  Rd, XX },
  /* 0F 23 */  { "mov",        0,  Dd,  Rd, XX },
  /* 0F 24 */  { "mov",        0,  Rd,  Td, XX },
  /* 0F 25 */  { INVALID },
  /* 0F 26 */  { "mov",        0,  Td,  Rd, XX },
  /* 0F 27 */  { INVALID },
  /* 0F 28 */  { GRPSSE(0f28) },
  /* 0F 29 */  { GRPSSE(0f29) },
  /* 0F 2A */  { GRPSSE(0f2a) },
  /* 0F 2B */  { GRPSSE(0f2b) },
  /* 0F 2C */  { GRPSSE(0f2c) },
  /* 0F 2D */  { GRPSSE(0f2d) },
  /* 0F 2E */  { GRPSSE(0f2e) },
  /* 0F 2F */  { GRPSSE(0f2f) },
  /* 0F 30 */  { "wrmsr",      0,  XX,  XX, XX },
  /* 0F 31 */  { "rdtsc",      0,  XX,  XX, XX },
  /* 0F 32 */  { "rdmsr",      0,  XX,  XX, XX },
  /* 0F 33 */  { "rdpmc",      0,  XX,  XX, XX },
  /* 0F 34 */  { "sysenter",   0,  XX,  XX, XX },
  /* 0F 35 */  { "sysexit",    0,  XX,  XX, XX },
  /* 0F 36 */  { INVALID },
  /* 0F 37 */  { INVALID },
  /* 0F 38 */  { INVALID },
  /* 0F 39 */  { INVALID },
  /* 0F 3A */  { INVALID },
  /* 0F 3B */  { INVALID },
  /* 0F 3C */  { INVALID },
  /* 0F 3D */  { INVALID },
  /* 0F 3E */  { INVALID },
  /* 0F 3F */  { INVALID },
  /* 0F 40 */  { "cmovo",      0,  Gv,  Ev, XX },
  /* 0F 41 */  { "cmovno",     0,  Gv,  Ev, XX },
  /* 0F 42 */  { "cmovc",      0,  Gv,  Ev, XX },
  /* 0F 43 */  { "cmovnc",     0,  Gv,  Ev, XX },
  /* 0F 44 */  { "cmovz",      0,  Gv,  Ev, XX },
  /* 0F 45 */  { "cmovnz",     0,  Gv,  Ev, XX },
  /* 0F 46 */  { "cmovna",     0,  Gv,  Ev, XX },
  /* 0F 47 */  { "cmova",      0,  Gv,  Ev, XX },
  /* 0F 48 */  { "cmovs",      0,  Gv,  Ev, XX },
  /* 0F 49 */  { "cmovns",     0,  Gv,  Ev, XX },
  /* 0F 4A */  { "cmovp",      0,  Gv,  Ev, XX },
  /* 0F 4B */  { "cmocnp",     0,  Gv,  Ev, XX },
  /* 0F 4C */  { "cmovl",      0,  Gv,  Ev, XX },
  /* 0F 4D */  { "cmovnl",     0,  Gv,  Ev, XX },
  /* 0F 4E */  { "cmovng",     0,  Gv,  Ev, XX },
  /* 0F 4F */  { "cmovg",      0,  Gv,  Ev, XX },
  /* 0F 50 */  { GRPSSE(0f50) },
  /* 0F 51 */  { GRPSSE(0f51) },
  /* 0F 52 */  { GRPSSE(0f52) },
  /* 0F 53 */  { GRPSSE(0f53) },
  /* 0F 54 */  { GRPSSE(0f54) },
  /* 0F 55 */  { GRPSSE(0f55) },
  /* 0F 56 */  { GRPSSE(0f56) },
  /* 0F 57 */  { GRPSSE(0f57) },
  /* 0F 58 */  { GRPSSE(0f58) },
  /* 0F 59 */  { GRPSSE(0f59) },
  /* 0F 5A */  { GRPSSE(0f5a) },
  /* 0F 5B */  { GRPSSE(0f5b) },
  /* 0F 5C */  { GRPSSE(0f5c) },
  /* 0F 5D */  { GRPSSE(0f5d) },
  /* 0F 5E */  { GRPSSE(0f5e) },
  /* 0F 5F */  { GRPSSE(0f5f) },
  /* 0F 60 */  { GRPSSE(0f60) },
  /* 0F 61 */  { GRPSSE(0f61) },
  /* 0F 62 */  { GRPSSE(0f62) },
  /* 0F 63 */  { GRPSSE(0f63) },
  /* 0F 64 */  { GRPSSE(0f64) },
  /* 0F 65 */  { GRPSSE(0f65) },
  /* 0F 66 */  { GRPSSE(0f66) },
  /* 0F 67 */  { GRPSSE(0f67) },
  /* 0F 68 */  { GRPSSE(0f68) },
  /* 0F 69 */  { GRPSSE(0f69) },
  /* 0F 6A */  { GRPSSE(0f6a) },
  /* 0F 6B */  { GRPSSE(0f6b) },
  /* 0F 6C */  { GRPSSE(0f6c) },
  /* 0F 6D */  { GRPSSE(0f6d) },
  /* 0F 6E */  { GRPSSE(0f6e) },
  /* 0F 6F */  { GRPSSE(0f6f) },
  /* 0F 70 */  { GRPSSE(0f70) },
  /* 0F 71 */  { GRPN(G12) },
  /* 0F 72 */  { GRPN(G13) },
  /* 0F 73 */  { GRPN(G14) },
  /* 0F 74 */  { GRPSSE(0f74) },
  /* 0F 75 */  { GRPSSE(0f75) },
  /* 0F 76 */  { GRPSSE(0f76) },
  /* 0F 77 */  { "emms",       0,  XX,  XX, XX },
  /* 0F 78 */  { INVALID },
  /* 0F 79 */  { INVALID },
  /* 0F 7A */  { INVALID },
  /* 0F 7B */  { INVALID },
  /* 0F 7C */  { GRPSSE(0f7c) },
  /* 0F 7D */  { GRPSSE(0f7d) },
  /* 0F 7E */  { GRPSSE(0f7e) },
  /* 0F 7F */  { GRPSSE(0f7f) },
  /* 0F 80 */  { "jo",         0,  Jv,  XX, Cond_Jump },
  /* 0F 81 */  { "jno",        0,  Jv,  XX, Cond_Jump },
  /* 0F 82 */  { "jb",         0,  Jv,  XX, Cond_Jump },
  /* 0F 83 */  { "jnb",        0,  Jv,  XX, Cond_Jump },
  /* 0F 84 */  { "jz",         0,  Jv,  XX, Cond_Jump },
  /* 0F 85 */  { "jnz",        0,  Jv,  XX, Cond_Jump },
  /* 0F 86 */  { "jbe",        0,  Jv,  XX, Cond_Jump },
  /* 0F 87 */  { "jnbe",       0,  Jv,  XX, Cond_Jump },
  /* 0F 88 */  { "js",         0,  Jv,  XX, Cond_Jump },
  /* 0F 89 */  { "jns",        0,  Jv,  XX, Cond_Jump },
  /* 0F 8A */  { "jp",         0,  Jv,  XX, Cond_Jump },
  /* 0F 8B */  { "jnp",        0,  Jv,  XX, Cond_Jump },
  /* 0F 8C */  { "jl",         0,  Jv,  XX, Cond_Jump },
  /* 0F 8D */  { "jnl",        0,  Jv,  XX, Cond_Jump },
  /* 0F 8E */  { "jle",        0,  Jv,  XX, Cond_Jump },
  /* 0F 8F */  { "jnle",       0,  Jv,  XX, Cond_Jump },
  /* 0F 90 */  { "setoB",      0,  Eb,  XX, XX },
  /* 0F 91 */  { "setnoB",     0,  Eb,  XX, XX },
  /* 0F 92 */  { "setbB",      0,  Eb,  XX, XX },
  /* 0F 93 */  { "setnbB",     0,  Eb,  XX, XX },
  /* 0F 94 */  { "setzB",      0,  Eb,  XX, XX },
  /* 0F 95 */  { "setnzB",     0,  Eb,  XX, XX },
  /* 0F 96 */  { "setbeB",     0,  Eb,  XX, XX },
  /* 0F 97 */  { "setnbeB",    0,  Eb,  XX, XX },
  /* 0F 98 */  { "setsB",      0,  Eb,  XX, XX },
  /* 0F 99 */  { "setnsB",     0,  Eb,  XX, XX },
  /* 0F 9A */  { "setpB",      0,  Eb,  XX, XX },
  /* 0F 9B */  { "setnpB",     0,  Eb,  XX, XX },
  /* 0F 9C */  { "setlB",      0,  Eb,  XX, XX },
  /* 0F 9D */  { "setnlB",     0,  Eb,  XX, XX },
  /* 0F 9E */  { "setleB",     0,  Eb,  XX, XX },
  /* 0F 9F */  { "setnleB",    0,  Eb,  XX, XX },
  /* 0F A0 */  { "push",       0,  FS,  XX, XX },
  /* 0F A1 */  { "pop",        0,  FS,  XX, XX },
  /* 0F A2 */  { "cpuid",      0,  XX,  XX, XX },
  /* 0F A3 */  { "btV",        0,  Ev,  Gv, XX },
  /* 0F A4 */  { "shldV",      0,  Ev,  Gv, Ib },
  /* 0F A5 */  { "shldV",      0,  Ev,  Gv, CL },
  /* 0F A6 */  { INVALID },
  /* 0F A7 */  { INVALID },
  /* 0F A8 */  { "push",       0,  GS,  XX, XX },
  /* 0F A9 */  { "pop",        0,  GS,  XX, XX },
  /* 0F AA */  { "rsm",        0,  XX,  XX, XX },
  /* 0F AB */  { "btsV",       0,  Ev,  Gv, XX },
  /* 0F AC */  { "shrdV",      0,  Ev,  Gv, Ib },
  /* 0F AD */  { "shrdV",      0,  Ev,  Gv, CL },
  /* 0F AE */  { GRPN(G15) },
  /* 0F AF */  { "imulV",      0,  Gv,  Ev, XX },
  /* 0F B0 */  { "cmpxchgB",   0,  Eb,  Gb, XX },
  /* 0F B1 */  { "cmpxchgV",   0,  Ev,  Gv, XX },
  /* 0F B2 */  { "lss",        0,  Gv,  Mp, XX },
  /* 0F B3 */  { "btrV",       0,  Ev,  Gv, XX },
  /* 0F B4 */  { "lfs",        0,  Gv,  Mp, XX },
  /* 0F B5 */  { "lgs",        0,  Gv,  Mp, XX },
  /* 0F B6 */  { "movzX",      0,  Gv,  Eb, XX },
  /* 0F B7 */  { "movzX",      0,  Gv,  Ew, XX },
  /* 0F B8 */  { INVALID },       
  /* 0F B9 */  { "ud2b",       0,  XX,  XX, XX },
  /* 0F BA */  { GRPN(G8EvIb) },
  /* 0F BB */  { "btcV",       0,  Ev,  Gv, XX },
  /* 0F BC */  { "bsfV",       0,  Gv,  Ev, XX },
  /* 0F BD */  { "bsrV",       0,  Gv,  Ev, XX },
  /* 0F BE */  { "movsX",      0,  Gv,  Eb, XX },
  /* 0F BF */  { "movsX",      0,  Gv,  Ew, XX },
  /* 0F C0 */  { "xaddB",      0,  Eb,  Gb, XX },
  /* 0F C1 */  { "xaddV",      0,  Ev,  Gv, XX },
  /* 0F C2 */  { GRPSSE(0fc2) },
  /* 0F C3 */  { GRPSSE(0fc3) },
  /* 0F C4 */  { GRPSSE(0fc4) },
  /* 0F C5 */  { GRPSSE(0fc5) },
  /* 0F C6 */  { GRPSSE(0fc6) },
  /* 0F C7 */  { GRPN(G9) },
  /* 0F C8 */  { "bswapV",     0, eAX,  XX, XX },
  /* 0F C9 */  { "bswapV",     0, eCX,  XX, XX },
  /* 0F CA */  { "bswapV",     0, eDX,  XX, XX },
  /* 0F CB */  { "bswapV",     0, eBX,  XX, XX },
  /* 0F CC */  { "bswapV",     0, eSP,  XX, XX },
  /* 0F CD */  { "bswapV",     0, eBP,  XX, XX },
  /* 0F CE */  { "bswapV",     0, eSI,  XX, XX },
  /* 0F CF */  { "bswapV",     0, eDI,  XX, XX },
  /* 0F D0 */  { GRPSSE(0fd0) },
  /* 0F D1 */  { GRPSSE(0fd1) },
  /* 0F D2 */  { GRPSSE(0fd2) },
  /* 0F D3 */  { GRPSSE(0fd3) },
  /* 0F D4 */  { GRPSSE(0fd4) },
  /* 0F D5 */  { GRPSSE(0fd5) },
  /* 0F D6 */  { GRPSSE(0fd6) },
  /* 0F D7 */  { GRPSSE(0fd7) },
  /* 0F D8 */  { GRPSSE(0fd8) },
  /* 0F D9 */  { GRPSSE(0fd9) },
  /* 0F DA */  { GRPSSE(0fda) },
  /* 0F DB */  { GRPSSE(0fdb) },
  /* 0F DC */  { GRPSSE(0fdc) },
  /* 0F DD */  { GRPSSE(0fdd) },
  /* 0F DE */  { GRPSSE(0fde) },
  /* 0F DF */  { GRPSSE(0fdf) },
  /* 0F E0 */  { GRPSSE(0fe0) },
  /* 0F E1 */  { GRPSSE(0fe1) },
  /* 0F E2 */  { GRPSSE(0fe2) },
  /* 0F E3 */  { GRPSSE(0fe3) },
  /* 0F E4 */  { GRPSSE(0fe4) },
  /* 0F E5 */  { GRPSSE(0fe5) },
  /* 0F E6 */  { GRPSSE(0fe6) },
  /* 0F E7 */  { GRPSSE(0fe7) },
  /* 0F E8 */  { GRPSSE(0fe8) },
  /* 0F E9 */  { GRPSSE(0fe9) },
  /* 0F EA */  { GRPSSE(0fea) },
  /* 0F EB */  { GRPSSE(0feb) },
  /* 0F EC */  { GRPSSE(0fec) },
  /* 0F ED */  { GRPSSE(0fed) },
  /* 0F EE */  { GRPSSE(0fee) },
  /* 0F EF */  { GRPSSE(0fef) },
  /* 0F F0 */  { GRPSSE(0ff0) },
  /* 0F F1 */  { GRPSSE(0ff1) },
  /* 0F F2 */  { GRPSSE(0ff2) },
  /* 0F F3 */  { GRPSSE(0ff3) },
  /* 0F F4 */  { GRPSSE(0ff4) },
  /* 0F F5 */  { GRPSSE(0ff5) },
  /* 0F F6 */  { GRPSSE(0ff6) },
  /* 0F F7 */  { GRPSSE(0ff7) },
  /* 0F F8 */  { GRPSSE(0ff8) },
  /* 0F F9 */  { GRPSSE(0ff9) },
  /* 0F FA */  { GRPSSE(0ffa) },
  /* 0F FB */  { GRPSSE(0ffb) },
  /* 0F FC */  { GRPSSE(0ffc) },
  /* 0F FD */  { GRPSSE(0ffd) },
  /* 0F FE */  { GRPSSE(0ffe) },
  /* 0F FF */  { INVALID }
};

#undef XX

#endif
