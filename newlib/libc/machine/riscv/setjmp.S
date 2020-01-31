/* Copyright (c) 2017  SiFive Inc. All rights reserved.

   This copyrighted material is made available to anyone wishing to use,
   modify, copy, or redistribute it subject to the terms and conditions
   of the FreeBSD License.   This program is distributed in the hope that
   it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
   including the implied warranties of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  A copy of this license is available at
   http://www.opensource.org/licenses.
*/

#include <sys/asm.h>

#ifdef __CHERI_PURE_CAPABILITY__
# define FPREGSOFF 15*SZREG
#else
# define FPREGSOFF 14*SZREG
#endif

/* int setjmp (jmp_buf);  */
  .globl  setjmp
  .type   setjmp, @function
setjmp:
	REG_S REG(ra),  0*SZREG(REG(a0))
	REG_S REG(s0),  1*SZREG(REG(a0))
	REG_S REG(s1),  2*SZREG(REG(a0))

#ifndef __riscv_32e
	REG_S REG(s2),  3*SZREG(REG(a0))
	REG_S REG(s3),  4*SZREG(REG(a0))
	REG_S REG(s4),  5*SZREG(REG(a0))
	REG_S REG(s5),  6*SZREG(REG(a0))
	REG_S REG(s6),  7*SZREG(REG(a0))
	REG_S REG(s7),  8*SZREG(REG(a0))
	REG_S REG(s8),  9*SZREG(REG(a0))
	REG_S REG(s9), 10*SZREG(REG(a0))
	REG_S REG(s10),11*SZREG(REG(a0))
	REG_S REG(s11),12*SZREG(REG(a0))
	REG_S REG(sp), 13*SZREG(REG(a0))
#else
	REG_S REG(sp), 3*SZREG(REG(a0))
#endif

#ifdef __CHERI_PURE_CAPABILITY__
	cspecialr ct0, ddc
#ifndef __riscv_32e
	csc ct0, 14*SZREG(ca0)
#else
	csc ct0, 4*SZREG(ca0)
#endif
#endif

#ifndef __riscv_float_abi_soft
	FREG_S fs0, FPREGSOFF+ 0*SZFREG(REG(a0))
	FREG_S fs1, FPREGSOFF+ 1*SZFREG(REG(a0))
	FREG_S fs2, FPREGSOFF+ 2*SZFREG(REG(a0))
	FREG_S fs3, FPREGSOFF+ 3*SZFREG(REG(a0))
	FREG_S fs4, FPREGSOFF+ 4*SZFREG(REG(a0))
	FREG_S fs5, FPREGSOFF+ 5*SZFREG(REG(a0))
	FREG_S fs6, FPREGSOFF+ 6*SZFREG(REG(a0))
	FREG_S fs7, FPREGSOFF+ 7*SZFREG(REG(a0))
	FREG_S fs8, FPREGSOFF+ 8*SZFREG(REG(a0))
	FREG_S fs9, FPREGSOFF+ 9*SZFREG(REG(a0))
	FREG_S fs10,FPREGSOFF+10*SZFREG(REG(a0))
	FREG_S fs11,FPREGSOFF+11*SZFREG(REG(a0))
#endif

	li    a0, 0
	RET
	.size	setjmp, .-setjmp

/* volatile void longjmp (jmp_buf, int);  */
  .globl  longjmp
  .type   longjmp, @function
longjmp:
	REG_L REG(ra),  0*SZREG(REG(a0))
	REG_L REG(s0),  1*SZREG(REG(a0))
	REG_L REG(s1),  2*SZREG(REG(a0))
#ifndef __riscv_32e
	REG_L REG(s2),  3*SZREG(REG(a0))
	REG_L REG(s3),  4*SZREG(REG(a0))
	REG_L REG(s4),  5*SZREG(REG(a0))
	REG_L REG(s5),  6*SZREG(REG(a0))
	REG_L REG(s6),  7*SZREG(REG(a0))
	REG_L REG(s7),  8*SZREG(REG(a0))
	REG_L REG(s8),  9*SZREG(REG(a0))
	REG_L REG(s9), 10*SZREG(REG(a0))
	REG_L REG(s10),11*SZREG(REG(a0))
	REG_L REG(s11),12*SZREG(REG(a0))
	REG_L REG(sp), 13*SZREG(REG(a0))
#else
	REG_L REG(sp), 3*SZREG(REG(a0))
#endif

#ifdef __CHERI_PURE_CAPABILITY__
#ifndef __riscv_32e
	clc ct0, 14*SZREG(ca0)
#else
	clc ct0, 4*SZREG(ca0)
#endif
	cspecialw ddc, ct0
#endif

#ifndef __riscv_float_abi_soft
	FREG_L fs0, FPREGSOFF+ 0*SZFREG(REG(a0))
	FREG_L fs1, FPREGSOFF+ 1*SZFREG(REG(a0))
	FREG_L fs2, FPREGSOFF+ 2*SZFREG(REG(a0))
	FREG_L fs3, FPREGSOFF+ 3*SZFREG(REG(a0))
	FREG_L fs4, FPREGSOFF+ 4*SZFREG(REG(a0))
	FREG_L fs5, FPREGSOFF+ 5*SZFREG(REG(a0))
	FREG_L fs6, FPREGSOFF+ 6*SZFREG(REG(a0))
	FREG_L fs7, FPREGSOFF+ 7*SZFREG(REG(a0))
	FREG_L fs8, FPREGSOFF+ 8*SZFREG(REG(a0))
	FREG_L fs9, FPREGSOFF+ 9*SZFREG(REG(a0))
	FREG_L fs10,FPREGSOFF+10*SZFREG(REG(a0))
	FREG_L fs11,FPREGSOFF+11*SZFREG(REG(a0))
#endif

	seqz a0, a1
	add  a0, a0, a1   # a0 = (a1 == 0) ? 1 : a1
	RET
	.size	longjmp, .-longjmp
