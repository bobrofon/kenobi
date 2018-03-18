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

#include <dlfcn.h>
#include <unistd.h>

#include <cstring>

#include <stdexcept>

namespace {

using namespace std::string_literals;

constexpr auto LD_PRELOAD = "LD_PRELOAD";

void greeting() {
	fputs("- Hello there.\n", stderr);
	fputs(("- General pid("s + std::to_string(getpid()) + ")!\n").data(), stderr);
}

std::string_view current_preload_path() {
    const auto current_preload_ptr = getenv(LD_PRELOAD);
    if (current_preload_ptr == nullptr) {
        return std::string_view{};
    }
    return current_preload_ptr;
}

std::string_view current_so_path() {
    Dl_info so_info{};
    const auto some_addr_from_so{reinterpret_cast<void*>(current_so_path)};
    if (!dladdr(some_addr_from_so, &so_info)) {
        throw std::runtime_error("I am not a dynamic library");
    }
    return so_info.dli_fname;
}

void export_ld_preload() {
    std::string preload_path{current_preload_path()};
    std::string self_path{current_so_path()};

    if (preload_path.find(self_path) != preload_path.npos) {
        return;
    }

	const auto new_preload = self_path + " " + preload_path;
	auto err = setenv(LD_PRELOAD, new_preload.data(), true);
    if (err) {
	    throw std::runtime_error("'setenv("s + LD_PRELOAD + ", " + new_preload
	                             + ")' failed: " + strerror(err));
    }
}

} // namespace

using fork_t = pid_t (*)();

extern "C" {

// TODO нужно переопределять exec функции, но там сложно разбирать vararg аргументы

// надеемся, что это часть fork exec
pid_t fork(void) {
	auto real_fork = reinterpret_cast<fork_t >(dlsym(RTLD_NEXT,"fork"));
	export_ld_preload();
	return real_fork();
}

pid_t vfork(void) {
	auto real_vfork = reinterpret_cast<fork_t >(dlsym(RTLD_NEXT,"vfork"));
	export_ld_preload();
	return real_vfork();
}

}

__attribute__((constructor))
void init() {
    greeting();
	unsetenv(LD_PRELOAD);
}
