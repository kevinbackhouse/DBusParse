// Copyright 2020-2024 Kevin Backhouse.
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

#include "utils.hpp"
#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

AutoCloseFD::~AutoCloseFD() { close(fd_); }

// Get the process's start time by reading /proc/[pid]/stat.
uint64_t process_start_time(pid_t pid) {
  char filename[64];
  char buf[1024];
  snprintf(filename, sizeof(filename), "/proc/%d/stat", pid);
  AutoCloseFD fd(open(filename, O_RDONLY | O_CLOEXEC, 0));
  const ssize_t n = read(fd.get(), buf, sizeof(buf) - 1);
  if (n < 0) {
    const int err = errno;
    fprintf(stderr, "could not open %s: %s\n", filename, strerror(err));
    return ~size_t(0);
  }
  buf[n] = '\0';

  // Search backwards for ')' character.
  ssize_t i = n;
  do {
    if (i == 0) {
      fprintf(stderr, "could not find ')' character in %s.\n", filename);
      return ~size_t(0);
    }
    --i;
  } while (buf[i] != ')');

  ++i;
  size_t j = 0;
  while (true) {
    if (i >= n || buf[i] != ' ') {
      fprintf(stderr, "unexpected character in %s.\n", filename);
      return ~size_t(0);
    }
    ++i;
    ++j;
    if (j == 20) {
      break;
    }
    while (i < n && buf[i] != ' ') {
      ++i;
    }
  }

  return strtoull(&buf[i], 0, 10);
}
