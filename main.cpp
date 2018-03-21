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

#include <unistd.h>

#include <iostream>
#include <thread>

#include "dark_side.h"

extern "C" {
#include "inject/inject.h"
} // extern C

constexpr auto EVIL_LIB = "libhello_there.so";

namespace {

void make_noise() {
	const auto pid = getpid();
	const auto tid = std::this_thread::get_id();
	for (;;) {
		std::cout << "evil process is alive: " << "pid=" << pid << " tid=" << tid << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

} // namespace

int main(int argc, char *argv[]) {
	std::thread noise{make_noise};
	pid_watcher watcher{};

	for (;;) {
		for (auto pid: watcher.new_pids()) {
			std::cout << "found new pid '" << pid << "'" << std::endl
				<< "trying to inject " << EVIL_LIB << std::endl;
			maybe_inject(pid, EVIL_LIB);
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}
