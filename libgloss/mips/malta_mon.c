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
#include <stdbool.h>
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
#define ANSI_BLUE "\x1B[34m"

#define malta_printf(msg, ...) do { \
	char _debug_buf[512];		\
	snprintf(_debug_buf, sizeof(_debug_buf), msg,##__VA_ARGS__); \
	yamon_print(_debug_buf);	\
} while(0)

#define warning_printf(msg, ...) malta_printf(ANSI_YELLOW msg ANSI_RESET,##__VA_ARGS__)
#define error_printf(msg, ...) malta_printf(ANSI_RED msg ANSI_RESET,##__VA_ARGS__)
#define debug_printf(msg, ...) malta_printf(ANSI_BLUE msg ANSI_RESET,##__VA_ARGS__)
#define debug_msg(msg) do { yamon_print(ANSI_BLUE); yamon_print(msg); yamon_print(ANSI_RESET); } while (0)
#define die(msg, ...) do { error_printf(msg "\n", ##__VA_ARGS__); do_malta_shutdown(); } while (0)



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
	malta_printf(/* ANSI_RED "Shutting down now!\n" */
		     ANSI_GREEN "Exit code was %ld\n" ANSI_RESET, status);
	do_malta_shutdown();
}

extern char __stub_exception_handler;
extern char __stub_exception_handler_end;

void hardware_exception_handler(void* epc, register_t cause, void* bad_vaddr, register_t status, register_t count) {
	error_printf("Exception: Cause=%lx EPC=%p BadVaddr=%p, Status=%lx, count=%lx\n",
		cause, epc, bad_vaddr, status, count);
	error_printf("Cannot continue, ABORTING!\n");
	// Don't call destructors if we've crashed, just shutdown
	do_malta_shutdown();
}

static register_t bootloader_memsize = 0;
static register_t env_memsize = 0;
static register_t env_ememsize = 0;
static register_t total_memsize = 0;

extern long atol(const char *nptr);
extern int strcmp(const char *s1, const char *s2);
extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);
extern size_t strlen(const char *s);

register_t yamon_argc;
yamon_ptr* yamon_argv;
yamon_env_t* yamon_envp;
// YAMON passes 32 bit pointers so we need to convert argv

void hardware_hazard_hook(register_t argc, yamon_ptr* argv, yamon_env_t* envp, register_t memsize) {
	debug_printf("%s: argc=%ld, argv=%p, envp=%p, memsize=0x%lx\n", __func__, argc, argv, envp, memsize);
	yamon_argc = argc;
	yamon_argv = argv;
	yamon_envp = envp;
	bootloader_memsize = memsize;
	// parse bootloader envp to find the real memsize
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
	// The MAX() macro evaluate arguments twice...
	total_memsize = MAX(bootloader_memsize, env_memsize);
	total_memsize = MAX(env_ememsize, total_memsize);
	debug_printf("Installing exception handler\n");
	memcpy((void*)(intptr_t)(int32_t)0x80000080, &__stub_exception_handler,
	       &__stub_exception_handler_end - &__stub_exception_handler);
	memcpy((void*)(intptr_t)(int32_t)0x80000180, &__stub_exception_handler,
		   &__stub_exception_handler_end - &__stub_exception_handler);
	crt_call_constructors();
}


extern char** environ;
static char* fake_environ[] = { "FOO=BAR", NULL };
#define ARGC_MAX 32
char* converted_argv[ARGC_MAX]; // Or should we allocate it dynamically?
register_t converted_argc;


static inline void add_argument(char* arg_start, char* pos) {
	if (converted_argc >= ARGC_MAX) {
		error_printf("FATAL ERROR: too many arguments! Current max is %ld\n", converted_argc);
		do_malta_shutdown();
	}
	if (pos == arg_start) {
		// the argument was empty -> don't add it
		debug_printf("Skipping empty argument %ld\n", converted_argc);
		return;
	}
	converted_argv[converted_argc] = arg_start;
	debug_printf("adding argv[%ld] = %s\n", converted_argc, arg_start);
	converted_argc++;
}

char** convert_argv(void) {
	if (yamon_argc != 2) {
		die("Got unexpected number of arguments: %ld", yamon_argc);
	}
	// first argument is the kernel binary
	converted_argv[0] = yamon_ptr_to_real_ptr(char, yamon_argv[0]);
	// second argument is whatever was passed in QEMU's -append option
	// argv and envp entries are 32-bit pointers
	converted_argc = 1;
	bool escaped = false;
	char* arg_start = yamon_ptr_to_real_ptr(char, yamon_argv[1]);
	for (char* pos = arg_start;; ++pos) {
		if (*pos == '\0') {
			// end of arguments
			add_argument(arg_start, pos);
			break;
		}
		if (*pos == ' ' && !escaped) {
			// split now
			*pos = '\0';
			add_argument(arg_start, pos);
			// prepare for next arg
			arg_start = pos + 1;
		} else if (*pos == '\\' && !escaped) {
			// remove the backslash and shift everything by one
			memmove(pos, pos + 1, strlen(pos) + 1);
			escaped = true;
		} else {
			escaped = false;
		}
	}
	environ = &fake_environ[0];
	return converted_argv;
}

asm(
".text\n\t"
".global hardware_argv_hook\n\t"
".global convert_argv\n\t"
".global converted_argc\n\t"
".global environ\n\t"
".ent hardware_argv_hook\n\t"
"hardware_argv_hook:\n\t"
"move $s0, $ra\n\t"  // save return address so so can call convert_argv
"dla $t9, convert_argv\n\t"
"jalr $t9\n\t"
"nop\n\t" // delay slot
"ld $a0, converted_argc\n\t"
"move $a1, $v0\n\t"
"ld $a2, environ\n\t"
"move $ra, $s0\n\t"  // restore the original return address
"jr $ra\n\t"
"nop\n\t" // delay slot
".end hardware_argv_hook"
);

// extern int errno;

int read(int fd, char* buf, size_t size) {
	error_printf("Attempting to read %zd bytes from fd %d\n", size, fd);
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
		error_printf("Attempting to write %zd bytes to fd %d: "
			       "Message is " ANSI_GREEN "%s\n", size, fd, buf);
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

extern char _ftext[]; /* Defined in qemu-malta.ld */
extern char _end[];   /* Defined in qemu-malta.ld */

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
