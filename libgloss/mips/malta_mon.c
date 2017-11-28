typedef __UINTPTR_TYPE__ uintptr_t;
typedef __INTPTR_TYPE__ intptr_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INT32_TYPE__ int32_t;
typedef __INT64_TYPE__ int64_t;
typedef __SIZE_TYPE__ size_t;

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

// QEMU-CHERI extension:
#define	 MALTA_SHUTDOWN 0x44 /* write this to MALTA_SOFTRES for board shutdown */

void hardware_exit_hook(void) {
	volatile int* softres = (volatile int*)MIPS_PHYS_TO_KSEG1(MALTA_SOFTRES);
	debug_msg(ANSI_RED "Shutting down now!\n");
	*softres = MALTA_SHUTDOWN; // CHERI Shutdown extension
	for (volatile int i = 0; i < 100; i++) {
		__asm__ volatile ("ssnop" :::"memory");
	}
	yamon_print("Shutdown request failed, falling back to reset!\n");
	*softres = MALTA_GORESET;  // fall back to restart
}

#include <errno.h>

// extern int errno;


extern int snprintf(char *__restrict, size_t, const char *__restrict, ...) __attribute__((__format__ (__printf__, 3, 4)));

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
// POSIX API:
int posix_memalign(void** ptr, size_t alignment, size_t nbytes) {
	void* ret = memalign(alignment, nbytes);
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
  mem->size = BOARD_MEM_SIZE - (_end - _ftext);
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
