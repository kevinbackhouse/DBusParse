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

#pragma once

#include "dbus.hpp"

// Implementation of the `Printer` interface which sends its output to a
// file descriptor.
class PrinterFD final : public Printer {
  // This class is not responsible for closing the file descriptor.
  const int fd_;

  // Decimal or hexadecimal output?
  const int base_;

  const size_t tabsize_;

  void printBytes(const char *buf, size_t bufsize);

public:
  PrinterFD(int fd, size_t base, size_t tabsize)
      : fd_(fd), base_(base), tabsize_(tabsize) {}

  void printChar(char c) override;
  void printUint8(uint8_t x) override;
  void printInt8(int8_t x) override;
  void printUint16(uint16_t x) override;
  void printInt16(int16_t x) override;
  void printUint32(uint32_t x) override;
  void printInt32(int32_t x) override;
  void printUint64(uint64_t x) override;
  void printInt64(int64_t x) override;
  void printDouble(double x) override;
  void printString(const std::string &str) override;

  // Print a newline character, followed by `tabsize_ * indent`
  // space chracters.
  void printNewline(size_t indent) override;
};
