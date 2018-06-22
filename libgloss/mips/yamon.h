/*-
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _MALTA_YAMON_H_
#define _MALTA_YAMON_H_

#define YAMON_FUNCTION_BASE	0x1fc00500ul

#define YAMON_PRINT_COUNT_OFS	(YAMON_FUNCTION_BASE + 0x04)
#define YAMON_EXIT_OFS		(YAMON_FUNCTION_BASE + 0x20)
#define YAMON_FLUSH_CACHE_OFS	(YAMON_FUNCTION_BASE + 0x2c)
#define YAMON_PRINT_OFS		(YAMON_FUNCTION_BASE + 0x34)
#define YAMON_REG_CPU_ISR_OFS	(YAMON_FUNCTION_BASE + 0x38)
#define YAMON_DEREG_CPU_ISR_OFS	(YAMON_FUNCTION_BASE + 0x3c)
#define YAMON_REG_IC_ISR_OFS	(YAMON_FUNCTION_BASE + 0x40)
#define YAMON_DEREG_IC_ISR_OFS	(YAMON_FUNCTION_BASE + 0x44)
#define YAMON_REG_ESR_OFS	(YAMON_FUNCTION_BASE + 0x48)
#define YAMON_DEREG_ESR_OFS	(YAMON_FUNCTION_BASE + 0x4c)
#define YAMON_GETCHAR_OFS	(YAMON_FUNCTION_BASE + 0x50)
#define YAMON_SYSCON_READ_OFS	(YAMON_FUNCTION_BASE + 0x54)

#ifdef __CHERI_PURE_CAPABILITY__
#define ADDR_TO_FUNCPTR(addr) __builtin_cheri_offset_set(__builtin_cheri_program_counter_get(), (vaddr_t)addr)
#define ADDR_TO_DATAPTR(addr) __builtin_cheri_offset_set(__builtin_cheri_global_data_get(), (vaddr_t)addr)
#else
#define ADDR_TO_DATAPTR(addr) ((void*)(addr))
#define ADDR_TO_FUNCPTR(addr) ((void*)(addr))
#endif
#define YAMON_FUNC_ADDR(ofs)		((long)(*(int32_t *)ADDR_TO_DATAPTR(MIPS_PHYS_TO_KSEG1(ofs))))

// Calling into YAMON must adhere to the MIPS calling convention and not CHERI purecap!
#ifndef __CHERI_PURE_CAPABILITY__
typedef void (*t_yamon_3_arg)(register_t arg1, register_t arg2, register_t arg3);
#define YAMON_3_ARG(ofs, arg1, arg2, arg3) \
	((t_yamon_3_arg)(YAMON_FUNC_ADDR(ofs)))(arg1, arg2, arg3)
#else
extern void _YAMON_3_ARG(register_t arg1, register_t arg2, register_t arg3, register_t address);
asm(".set noreorder\n\t"
     ".text\n\t"
     ".ent _YAMON_3_ARG\n\t"
     ".global _YAMON_3_ARG\n\t"
     "_YAMON_3_ARG:\n\t"
     "move $t9, $a3\n\t"
     "cmove $c18, $c17\n\t"  // Save $c17 since we are about to clobber it
     "cgetpccsetoffset $c12, $t9\n\t"
     "cjalr $c12, $c17\n\t"
     "cgetaddr $ra, $c17\n\t" // So that YAMON can return it needs $ra set
     "cjr $c18\n\t"
     "nop\n\t"
     ".end _YAMON_3_ARG\n\t");
#define YAMON_3_ARG(ofs, arg1, arg2, arg3) _YAMON_3_ARG(arg1, arg2, arg3, YAMON_FUNC_ADDR(ofs))
#endif

#define YAMON_2_ARG(address, arg1, arg2) YAMON_3_ARG(address, arg1, arg2, 0)
#define YAMON_1_ARG(address, arg1, arg2) YAMON_2_ARG(address, arg1, 0, 0)

typedef void (*t_yamon_print_count)(uint32_t port, vaddr_t s, uint32_t count);
#define YAMON_PRINT_COUNT(s, count) YAMON_3_ARG(YAMON_PRINT_COUNT_OFS, 0, s, count)

typedef void (*t_yamon_exit)(uint32_t rc);
// #define YAMON_EXIT(rc) ((t_yamon_exit)(YAMON_FUNC(YAMON_EXIT_OFS)))(rc)
#define YAMON_EXIT(rc) YAMON_1_ARG(YAMON_FUNC(YAMON_EXIT_OFS), 0, s, count)

typedef void (*t_yamon_print)(uint32_t port, vaddr_t s);
#define YAMON_PRINT(s) YAMON_2_ARG(YAMON_PRINT_OFS, 0, s)

typedef int (*t_yamon_getchar)(uint32_t port, vaddr_t *ch);
#define YAMON_GETCHAR(s) YAMON_2_ARG(YAMON_GETCHAR_OFS, 0, ch)

typedef int t_yamon_syscon_id;
typedef int (*t_yamon_syscon_read)(t_yamon_syscon_id id, vaddr_t param,
				   uint32_t size);
#define YAMON_SYSCON_READ(id, param, size)  YAMON_3_ARG(YAMON_SYSCON_READ_OFS, id, param, size)

// the yamon pointers need to be sign extended!
typedef int32_t yamon_ptr;
#ifdef __CHERI_PURE_CAPABILITY__
#define yamon_ptr_to_real_ptr(type, ptr) ((type*)ADDR_TO_DATAPTR((int64_t)ptr))
#else
#define yamon_ptr_to_real_ptr(type, ptr) ((type*)(intptr_t)ptr)
#endif

typedef struct {
	yamon_ptr name; // actually a 32bit char*
	yamon_ptr value; // actually a 32bit char*
} yamon_env_t;

#define SYSCON_BOARD_CPU_CLOCK_FREQ_ID	34	/* UINT32 */
#define SYSCON_BOARD_BUS_CLOCK_FREQ_ID	35	/* UINT32 */
#define SYSCON_BOARD_PCI_FREQ_KHZ_ID	36	/* UINT32 */

char*		yamon_getenv(char *name);
uint32_t	yamon_getcpufreq(void);

extern yamon_env_t *fenvp;

#endif /* _MALTA_YAMON_H_ */
