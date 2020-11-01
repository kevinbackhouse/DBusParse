// Copyright 2020 Kevin Backhouse.
//
// This file is part of DBusParse.
//
// DBusParse is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DBusParse is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DBusParse.  If not, see <https://www.gnu.org/licenses/>.


#pragma once

#include <string>
#include <vector>

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

// This class automatically closes the file descriptor in its destructor.
class AutoCloseFD {
  const int fd_;

public:
  AutoCloseFD() = delete;  // No default constructor.
  explicit AutoCloseFD(const int fd) : fd_(fd) {}
  ~AutoCloseFD();

  int get() const { return fd_; }
};

// Shorthand for creating a std::string from a string literal.
inline std::string _s(const char* s) {
  return std::string(s);
}

inline std::string _s(const std::string& s) {
  return std::string(s);
}

// Utility for constructing a std::vector.
template <class... Ts>
std::vector<typename std::common_type<Ts...>::type> _vec(Ts&&... args) {
  std::vector<typename std::common_type<Ts...>::type> result;
  result.reserve(sizeof...(args));
  int bogus[] = { ((void)result.emplace_back(std::forward<Ts>(args)), 0)... };
  static_assert(sizeof(bogus) == sizeof(int) * sizeof...(args));
  return result;
}

// Get the process's start time by reading /proc/[pid]/stat.
uint64_t process_start_time(pid_t pid);
