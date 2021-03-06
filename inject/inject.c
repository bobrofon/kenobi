/*MIT License

Copyright (c) 2018 bobrofon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "inject.h"

// This file is just a copy-paste from linux-inject/inject-x86_64.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <wait.h>
#include <sys/stat.h>

#include "utils.h"
#include "ptrace.h"

/*
 * injectSharedLibrary()
 *
 * This is the code that will actually be injected into the target process.
 * This code is responsible for loading the shared library into the target
 * process' address space.  First, it calls malloc() to allocate a buffer to
 * hold the filename of the library to be loaded. Then, it calls
 * __libc_dlopen_mode(), libc's implementation of dlopen(), to load the desired
 * shared library. Finally, it calls free() to free the buffer containing the
 * library name. Each time it needs to give control back to the injector
 * process, it breaks back in by executing an "int $3" instruction. See the
 * comments below for more details on how this works.
 *
 */

static void injectSharedLibrary(__attribute__((unused)) long mallocaddr, __attribute__((unused)) long freeaddr, __attribute__((unused)) long dlopenaddr)
{
	// here are the assumptions I'm making about what data will be located
	// where at the time the target executes this code:
	//
	//   rdi = address of malloc() in target process
	//   rsi = address of free() in target process
	//   rdx = address of __libc_dlopen_mode() in target process
	//   rcx = size of the path to the shared library we want to load

	// align to 16
	asm(
		"pushq %rsp\n"
		"pushq (%rsp)\n"
		"andq $-0x10, %rsp"
	);

	// save addresses of free() and __libc_dlopen_mode() on the stack for later use
	asm(
	// rsi is going to contain the address of free(). it's going to get wiped
	// out by the call to malloc(), so save it on the stack for later
	"push %rsi \n"
		// same thing for rdx, which will contain the address of _dl_open()
		"push %rdx"
	);

	// call malloc() from within the target process
	asm(
	// save previous value of r9, because we're going to use it to call malloc()
	"push %r9 \n"
		// now move the address of malloc() into r9
		"mov %rdi,%r9 \n"
		// choose the amount of memory to allocate with malloc() based on the size
		// of the path to the shared library passed via rcx
		"mov %rcx,%rdi \n"
		// now call r9; malloc()
		"callq *%r9 \n"
		// after returning from malloc(), pop the previous value of r9 off the stack
		"pop %r9 \n"
		// break in so that we can see what malloc() returned
		"int $3"
	);

	// call __libc_dlopen_mode() to load the shared library
	asm(
	// get the address of __libc_dlopen_mode() off of the stack so we can call it
	"pop %rdx \n"
		// as before, save the previous value of r9 on the stack
		"push %r9 \n"
		// copy the address of __libc_dlopen_mode() into r9
		"mov %rdx,%r9 \n"
		// 1st argument to __libc_dlopen_mode(): filename = the address of the buffer returned by malloc()
		"mov %rax,%rdi \n"
		// 2nd argument to __libc_dlopen_mode(): flag = RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND
		"movabs $1,%rsi \n"
		// call __libc_dlopen_mode()
		"callq *%r9 \n"
		// restore old r9 value
		"pop %r9 \n"
		// break in so that we can see what __libc_dlopen_mode() returned
		"int $3"
	);

	// call free() to free the buffer we allocated earlier.
	//
	// Note: I found that if you put a nonzero value in r9, free() seems to
	// interpret that as an address to be freed, even though it's only
	// supposed to take one argument. As a result, I had to call it using a
	// register that's not used as part of the x64 calling convention. I
	// chose rbx.
	asm(
	// at this point, rax should still contain our malloc()d buffer from earlier.
	// we're going to free it, so move rax into rdi to make it the first argument to free().
	"mov %rax,%rdi \n"
		// pop rsi so that we can get the address to free(), which we pushed onto the stack a while ago.
		"pop %rsi \n"
		// save previous rbx value
		"push %rbx \n"
		// load the address of free() into rbx
		"mov %rsi,%rbx \n"
		// zero out rsi, because free() might think that it contains something that should be freed
		"xor %rsi,%rsi \n"
		// break in so that we can check out the arguments right before making the call
		"int $3 \n"
		// call free()
		"callq *%rbx \n"
		// restore previous rbx value
		"pop %rbx"
	);

	// we already overwrote the RET instruction at the end of this function
	// with an INT 3, so at this point the injector will regain control of
	// the target's execution.
	asm(
	"movq 8(%rsp), %rsp"
	);
}

/*
 * injectSharedLibrary_end()
 *
 * This function's only purpose is to be contiguous to injectSharedLibrary(),
 * so that we can use its address to more precisely figure out how long
 * injectSharedLibrary() is.
 *
 */

static void injectSharedLibrary_end(void)
{
}

typedef union {
	void* void_ptr;
	void (*inject_start_ptr)(long, long, long);
	void (*inject_end_ptr)(void);
	const unsigned char* byte_ptr;
	intptr_t intptr;
} cast;

void maybe_inject(pid_t target, const char* libname)
{
	unsigned long long addr = 0;
	size_t injectSharedLibrary_size = 0;
	char* backup = NULL;
	char* newcode = NULL;
	char* libPath = realpath(libname, NULL);
	if (libPath == NULL) return ;
	printf("targeting process with pid %d\n", target);

	size_t libPathLength = strlen(libPath) + 1;

	int mypid = getpid();
	unsigned long long mylibcaddr = getlibcaddr(mypid);
	if (!mylibcaddr) return ;

	// find the addresses of the syscalls that we'd like to use inside the
	// target, as loaded inside THIS process (i.e. NOT the target process)
	unsigned long long mallocAddr = getFunctionAddress("malloc");
	if (!mallocAddr) return ;
	unsigned long long freeAddr = getFunctionAddress("free");
	if (!freeAddr) return ;
	unsigned long long dlopenAddr = getFunctionAddress("__libc_dlopen_mode");
	if (!dlopenAddr) return ;

	// use the base address of libc to calculate offsets for the syscalls
	// we want to use
	unsigned long long mallocOffset = mallocAddr - mylibcaddr;
	unsigned long long freeOffset = freeAddr - mylibcaddr;
	unsigned long long dlopenOffset = dlopenAddr - mylibcaddr;


	// get the target process' libc address and use it to find the
	// addresses of the syscalls we want to use inside the target process
	unsigned long long targetLibcAddr = getlibcaddr(target);
	if (!targetLibcAddr) return ;
	unsigned long long targetMallocAddr = targetLibcAddr + mallocOffset;
	unsigned long long targetFreeAddr = targetLibcAddr + freeOffset;
	unsigned long long targetDlopenAddr = targetLibcAddr + dlopenOffset;

	struct user_regs_struct oldregs, regs;
	memset(&oldregs, 0, sizeof(struct user_regs_struct));
	memset(&regs, 0, sizeof(struct user_regs_struct));

	if (ptrace_attach(target)) return ;

	if(ptrace_getregs(target, &oldregs)) goto CLEANUP;
	memcpy(&regs, &oldregs, sizeof(struct user_regs_struct));

	// find a good address to copy code to
	addr = freespaceaddr(target);
	if (!addr) goto CLEANUP;
	addr += sizeof(long);

	// now that we have an address to copy code to, set the target's rip to
	// it. we have to advance by 2 bytes here because rip gets incremented
	// by the size of the current instruction, and the instruction at the
	// start of the function to inject always happens to be 2 bytes long.
	regs.rip = addr + 2;

	// pass arguments to my function injectSharedLibrary() by loading them
	// into the right registers. note that this will definitely only work
	// on x64, because it relies on the x64 calling convention, in which
	// arguments are passed via registers rdi, rsi, rdx, rcx, r8, and r9.
	// see comments in injectSharedLibrary() for more details.
	regs.rdi = targetMallocAddr;
	regs.rsi = targetFreeAddr;
	regs.rdx = targetDlopenAddr;
	regs.rcx = libPathLength;
	if(ptrace_setregs(target, &regs)) goto CLEANUP;

	// figure out the size of injectSharedLibrary() so we know how big of a buffer to allocate.
	injectSharedLibrary_size = (size_t)((intptr_t)injectSharedLibrary_end - (intptr_t)injectSharedLibrary);

	// also figure out where the RET instruction at the end of
	// injectSharedLibrary() lies so that we can overwrite it with an INT 3
	// in order to break back into the target process. note that on x64,
	// gcc and clang both force function addresses to be word-aligned,
	// which means that functions are padded with NOPs. as a result, even
	// though we've found the length of the function, it is very likely
	// padded with NOPs, so we need to actually search to find the RET.
	cast start_ptr = {.inject_start_ptr = injectSharedLibrary};
	cast end_ptr = {.inject_end_ptr = injectSharedLibrary_end};
	cast ret_ptr = {.byte_ptr = findRet(end_ptr.void_ptr)};
	intptr_t injectSharedLibrary_ret = ret_ptr.intptr - (intptr_t)start_ptr.void_ptr;

	// back up whatever data used to be at the address we want to modify.
	backup = malloc(injectSharedLibrary_size * sizeof(char));
	if (ptrace_read(target, addr, backup, injectSharedLibrary_size)) goto CLEANUP;

	// set up a buffer to hold the code we're going to inject into the
	// target process.
	newcode = malloc(injectSharedLibrary_size * sizeof(char));
	memset(newcode, 0, injectSharedLibrary_size * sizeof(char));

	// copy the code of injectSharedLibrary() to a buffer.
	memcpy(newcode, start_ptr.void_ptr, injectSharedLibrary_size - 1);
	// overwrite the RET instruction with an INT 3.
	newcode[injectSharedLibrary_ret] = INTEL_INT3_INSTRUCTION;

	// copy injectSharedLibrary()'s code to the target address inside the
	// target process' address space.
	if (ptrace_write(target, addr, newcode, injectSharedLibrary_size)) goto CLEANUP;

	// now that the new code is in place, let the target run our injected
	// code.
	if (ptrace_cont(target)) goto CLEANUP;

	// at this point, the target should have run malloc(). check its return
	// value to see if it succeeded, and bail out cleanly if it didn't.
	struct user_regs_struct malloc_regs;
	memset(&malloc_regs, 0, sizeof(struct user_regs_struct));
	if (ptrace_getregs(target, &malloc_regs)) goto CLEANUP;
	unsigned long long targetBuf = malloc_regs.rax;
	if(targetBuf == 0)
	{
		fprintf(stderr, "malloc() failed to allocate memory\n");
		restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
		free(backup);
		free(newcode);
		return ;
	}

	// if we get here, then malloc likely succeeded, so now we need to copy
	// the path to the shared library we want to inject into the buffer
	// that the target process just malloc'd. this is needed so that it can
	// be passed as an argument to __libc_dlopen_mode later on.

	// read the current value of rax, which contains malloc's return value,
	// and copy the name of our shared library to that address inside the
	// target process.
	if (ptrace_write(target, targetBuf, libPath, libPathLength)) goto CLEANUP;

	// continue the target's execution again in order to call
	// __libc_dlopen_mode.
	if (ptrace_cont(target)) goto CLEANUP;

	// check out what the registers look like after calling dlopen.
	struct user_regs_struct dlopen_regs;
	memset(&dlopen_regs, 0, sizeof(struct user_regs_struct));
	if (ptrace_getregs(target, &dlopen_regs)) goto CLEANUP;
	unsigned long long libAddr = dlopen_regs.rax;

	// if rax is 0 here, then __libc_dlopen_mode failed, and we should bail
	// out cleanly.
	if(libAddr == 0)
	{
		fprintf(stderr, "__libc_dlopen_mode() failed to load %s\n", libname);
		restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
		free(backup);
		free(newcode);
		return ;
	}

	// now check /proc/pid/maps to see whether injection was successful.
	if(checkloaded(target, libname))
	{
		printf("\"%s\" successfully injected\n", libname);
	}
	else
	{
		fprintf(stderr, "could not inject \"%s\"\n", libname);
	}

CLEANUP:
	// as a courtesy, free the buffer that we allocated inside the target
	// process. we don't really care whether this succeeds, so don't
	// bother checking the return value.
	ptrace_cont(target);

	// at this point, if everything went according to plan, we've loaded
	// the shared library inside the target process, so we're done. restore
	// the old state and detach from the target.
	if (backup) {
		restoreStateAndDetach(target, addr, backup, injectSharedLibrary_size, oldregs);
		free(backup);
	}
	if (newcode) {
		free(newcode);
	}
}
