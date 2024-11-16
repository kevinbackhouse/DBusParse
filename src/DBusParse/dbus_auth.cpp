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

#include "dbus_auth.hpp"
#include "error.hpp"
#include <cstdint>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static size_t write_string(char *buf, size_t pos, const char *str) {
  const size_t len = strlen(str);
  memcpy(&buf[pos], str, len);
  return pos + len;
}

static ssize_t sendbuf(const int fd, char *buf, size_t pos, size_t bufsize) {
  const ssize_t wr = write(fd, buf, pos);
  if (wr < 0) {
    throw ErrorWithErrno("Write failed");
  } else if (static_cast<size_t>(wr) != pos) {
    char msg[128];
    snprintf(msg, sizeof(msg), "Write incomplete: %ld < %lu\n", wr, pos);
    throw Error(msg);
  }

  return read(fd, buf, bufsize);
}

void dbus_sendauth(const uid_t uid, const int fd) {
  char buf[1024];
  size_t pos = 0;

  buf[pos++] = '\0';
  pos = write_string(buf, pos, "AUTH EXTERNAL ");

  char uidstr[16];
  snprintf(uidstr, sizeof(uidstr), "%d", uid);
  size_t n = strlen(uidstr);
  for (size_t i = 0; i < n; i++) {
    char tmp[4];
    snprintf(tmp, sizeof(tmp), "%.2x", (int)uidstr[i]);
    pos = write_string(buf, pos, tmp);
  }
  pos = write_string(buf, pos, "\r\n");

  sendbuf(fd, buf, pos, sizeof(buf));

  pos = write_string(buf, 0, "NEGOTIATE_UNIX_FD\r\n");
  sendbuf(fd, buf, pos, sizeof(buf));

  pos = write_string(buf, 0, "BEGIN\r\n");
  const ssize_t wr = write(fd, buf, pos);
  if (wr < 0) {
    throw ErrorWithErrno("Write failed");
  } else if (static_cast<size_t>(wr) != pos) {
    char msg[128];
    snprintf(msg, sizeof(msg), "Write incomplete: %ld < %lu\n", wr, pos);
    throw Error(msg);
  }
}
