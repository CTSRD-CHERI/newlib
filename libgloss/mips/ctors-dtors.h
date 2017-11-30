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

typedef unsigned long long ctor_dtor_entry;
typedef void (*mips_function_ptr)();


static void call_ctors_dtors(ctor_dtor_entry* start, ctor_dtor_entry* end) {
	for (ctor_dtor_entry* entry = start; entry < end; entry++) {
		if (*entry != (ctor_dtor_entry)-1) {
			mips_function_ptr func = (mips_function_ptr)*entry;
			debug_printf("Calling ctor/dtor %p\n", (void*)func);
			func();
		}
	}
}

__attribute__((weak)) extern ctor_dtor_entry __ctors_start;
__attribute__((weak)) extern ctor_dtor_entry __ctors_end;
extern mips_function_ptr __DTOR_END__;
static void crt_call_constructors(void) {
	call_ctors_dtors(&__ctors_start, &__ctors_end);
}

__attribute__((weak)) extern ctor_dtor_entry __dtors_start;
__attribute__((weak)) extern ctor_dtor_entry __dtors_end;

static void crt_call_destructors(void) {
	call_ctors_dtors(&__dtors_start, &__dtors_end);
}
