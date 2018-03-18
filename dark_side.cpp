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

#include "dark_side.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace {

const fs::path PROC{"/proc"};

pid_t to_pid(std::string_view str) {
	std::stringstream ss{str.data()};
	pid_t pid{};
	ss >> pid;
	return pid;
}

}

std::vector<pid_t> all_pids() {
	std::vector<pid_t> pids{};
	for (auto& p: fs::directory_iterator(PROC)) {
		auto pid = to_pid(p.path().filename().string());
		if (pid) {
			pids.push_back(pid);
		}
	}
	return pids;
}
