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

#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <experimental/filesystem>

using namespace std::string_literals;

namespace fs = std::experimental::filesystem;

namespace {

constexpr auto PROC = "/proc";

pid_t to_pid(std::string_view str) {
	std::stringstream ss{str.data()};
	pid_t pid{};
	ss >> pid;
	return pid;
}

template <typename T>
std::set<T> difference(const std::set<T>& a, const std::set<T>& b) {
	std::set<T> res{};
	std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::inserter(res, res.begin()));
	return res;
}

bool has_ownership(const fs::path& path) {
	struct stat info{};
	if (auto err = stat(path.string().data(), &info); err) {
		return false;
	}
	return info.st_uid == getuid();
}

} // namespace

std::set<pid_t> all_pids() {
	std::set<pid_t> pids{};
	for (auto& p: fs::directory_iterator(fs::path(PROC))) {
		auto pid = to_pid(p.path().filename().string());
		if (pid && has_ownership(p)) {
			pids.insert(pid);
		}
	}
	return pids;
}

std::set<pid_t> pid_watcher::new_pids() {
	const auto old_pids = std::move(pids_);
	pids_ = all_pids();

	return difference(pids_, old_pids);
}
