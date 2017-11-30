/*-
 * Copyright (c) 2017 Alex Richardson
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory under DARPA/AFRL contract (FA8750-10-C-0237)
 * ("CTSRD"), as part of the DARPA CRASH research programme.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __INTPTR_TYPE__ intptr_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
typedef __SIZE_TYPE__ size_t;
typedef long register_t;

/* FreeBSD sys/mips/include/cpuregs.h */
#define	MIPS_KUSEG_START		0x00000000
#define	MIPS_KSEG0_START		((intptr_t)(int32_t)0x80000000)
#define	MIPS_KSEG0_END			((intptr_t)(int32_t)0x9fffffff)
#define	MIPS_KSEG1_START		((intptr_t)(int32_t)0xa0000000)
#define	MIPS_KSEG1_END			((intptr_t)(int32_t)0xbfffffff)
#define	MIPS_KSSEG_START		((intptr_t)(int32_t)0xc0000000)
#define	MIPS_KSSEG_END			((intptr_t)(int32_t)0xdfffffff)
#define	MIPS_KSEG3_START		((intptr_t)(int32_t)0xe0000000)
#define	MIPS_KSEG3_END			((intptr_t)(int32_t)0xffffffff)
#define MIPS_KSEG2_START		MIPS_KSSEG_START
#define MIPS_KSEG2_END			MIPS_KSSEG_END

#define	MIPS_PHYS_TO_KSEG0(x)		((uintptr_t)(x) | MIPS_KSEG0_START)
#define	MIPS_PHYS_TO_KSEG1(x)		((uintptr_t)(x) | MIPS_KSEG1_START)

#include "maltareg.h"
#include "yamon.h"

#include <sys/param.h>
#include <errno.h>
extern int snprintf(char *__restrict, size_t, const char *__restrict, ...) __attribute__((__format__ (__printf__, 3, 4)));

// monitor API:
static inline void yamon_print(const char* s) {
	YAMON_PRINT(s);
}
static inline void yamon_print_count(const char* s, size_t count) {
	YAMON_PRINT_COUNT(s, count);
}

#define ANSI_RESET "\x1B[00m"
#define ANSI_BOLD "\x1B[1m"
#define ANSI_RED "\x1B[31m"
#define ANSI_GREEN "\x1B[32m"
#define ANSI_YELLOW "\x1B[33m"

#define debug_msg(msg) do { yamon_print(ANSI_YELLOW); yamon_print(msg); yamon_print(ANSI_RESET); } while (0)
#define debug_printf(msg, ...) do { \
	char _debug_buf[512];		\
	snprintf(_debug_buf, sizeof(_debug_buf), ANSI_YELLOW msg ANSI_RESET,##__VA_ARGS__); \
	yamon_print(_debug_buf);	\
} while(0)


#include "ctors-dtors.h"


// QEMU-CHERI extension:
#define	 MALTA_SHUTDOWN 0x44 /* write this to MALTA_SOFTRES for board shutdown */

static void do_malta_shutdown(void) {
	volatile int* softres = (volatile int*)MIPS_PHYS_TO_KSEG1(MALTA_SOFTRES);
	*softres = MALTA_SHUTDOWN; // CHERI Shutdown extension
	for (volatile int i = 0; i < 100; i++) {
		__asm__ volatile ("ssnop" :::"memory");
	}
	yamon_print("Shutdown request failed, falling back to reset!\n");
	*softres = MALTA_GORESET;  // fall back to restart
}

void hardware_exit_hook(register_t status) {
	crt_call_destructors();
	debug_printf(ANSI_RED "Shutting down now!\n"
		     ANSI_GREEN "Exit code was %ld\n", status);
	do_malta_shutdown();
}

extern char __stub_exception_handler;
extern char __stub_exception_handler_end;

void hardware_exception_handler(void* epc, register_t cause, void* bad_vaddr, register_t status, register_t count) {
	debug_printf(ANSI_RED "Exception: Cause=%lx EPC=%p BadVaddr=%p, Status=%lx, count=%lx\n",
		cause, epc, bad_vaddr, status, count);
	debug_printf(ANSI_RED "Cannot continue, ABORTING!\n");
	// Don't call destructors if we've crashed, just shutdown
	do_malta_shutdown();
}

yamon_env_t* fenvp;
static register_t bootloader_memsize = 0;
static register_t env_memsize = 0;
static register_t env_ememsize = 0;
static register_t total_memsize = 0;

extern long atol(const char *nptr);
extern int strcmp(const char *s1, const char *s2);
extern void *memcpy(void *dest, const void *src, size_t n);

void hardware_hazard_hook(register_t argc, yamon_ptr* argv, yamon_env_t* envp, register_t memsize) {
	debug_printf("%s 123: argc=%ld, argv=%p, envp=%p, memsize=0x%lx\n", __func__, argc, argv, envp, memsize);
	// argv and envp entries are 32-bit pointers
	for (int i = 0; i < argc; ++i) {
		debug_printf("argv[%d] = %s\n", i, yamon_ptr_to_real_ptr(char, argv[i]));
	}
	for (int i = 0; envp[i].name != 0 ; ++i) {
		const char* name = yamon_ptr_to_real_ptr(char, envp[i].name);
		const char* value = yamon_ptr_to_real_ptr(char, envp[i].value);
		debug_printf("envp[%d]: '%s'='%s'\n", i, name, value);
		if (strcmp(name, "memsize") == 0) {
			env_memsize = atol(value);
		} else if (strcmp(name, "ememsize") == 0) {// memsize above 256MB
			env_ememsize = atol(value);
		}
	}
	fenvp = envp;
	bootloader_memsize = memsize;
	// The MAX() macro evaluate arguments twice...
	total_memsize = MAX(bootloader_memsize, env_memsize);
	total_memsize = MAX(env_ememsize, total_memsize);
	debug_printf("Installing exception handler\n");
	memcpy((void*)(intptr_t)(int32_t)0x80000080, &__stub_exception_handler,
	       &__stub_exception_handler_end - &__stub_exception_handler);
	crt_call_constructors();
}


// extern int errno;

int read(int fd, char* buf, size_t size) {
	char tmpbuf[256];
	snprintf(tmpbuf, sizeof(tmpbuf), "Attempting to read %zd bytes from fd %d\n", size, fd);
	debug_msg(tmpbuf);
	errno = ENOSYS;
	return -1;
}

int write(int fd, char* buf, size_t size) {
	char tmpbuf[256];
	if (fd == 1 || fd == 2) {
		yamon_print_count(buf, size);
		return size;
	}
	else {
		snprintf(tmpbuf, sizeof(tmpbuf), "Attempting to write %zd bytes to "
			"fd %d: Message is " ANSI_GREEN "%s\n", size, fd, buf);
		debug_msg(tmpbuf);
		errno = ENOSYS;
		return -1;
	}
}

int close(int fd) {
	errno = EBADF;
	return -1;
}


// Needed by C++ new:
extern void* memalign(size_t align, size_t nbytes); // provided by newlib
// POSIX API (needed by C++17):
int posix_memalign(void** ptr, size_t alignment, size_t nbytes) {
	void* ret = memalign(alignment, nbytes);
	debug_printf("posix_memalign allocation of %zd bytes with alignment %zd -> %p\n",
		     nbytes, alignment, ret);
	if (ret) {
		*ptr = ret;
		return 0;
	}
	*ptr = NULL;
	return ENOMEM;
}

struct s_mem
{
  unsigned int size;
  unsigned int icsize;
  unsigned int dcsize;
};
#warning "FIXME: actually get the memory size from qemu"
// Assume we have at least 64 MB

extern char _ftext[]; /* Defined in qemu-malta.ld */
extern char _end[];   /* Defined in qemu-malta.ld */

#define BOARD_MEM_SIZE (64 * 1024 * 1024)
void get_mem_info (struct s_mem *mem) {
  mem->size = total_memsize - (_end - _ftext);
}


#if 0

/* From FreeBSD sys/mips/malta/yamon.c */
char *
yamon_getenv(char *name)
{
	char *value;
	yamon_env_t *p;

	value = NULL;
	for (p = *fenvp; p->name != NULL; ++p) {
	    if (!strcmp(name, p->name)) {
		value = p->value;
		break;
	    }
	}

	return (value);
}

uint32_t
yamon_getcpufreq(void)
{
	uint32_t freq;
	int ret;

	freq = 0;
	ret = YAMON_SYSCON_READ(SYSCON_BOARD_CPU_CLOCK_FREQ_ID, &freq,
	    sizeof(freq));
	if (ret != 0)
		freq = 0;

	return (freq);
}
#endif
