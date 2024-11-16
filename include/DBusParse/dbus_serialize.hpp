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
#include <string.h>

// Round pos up to a multiple of alignment.
inline size_t alignup(size_t pos, size_t alignment) {
  // Check that alignment is a power of 2.
  assert((alignment & (alignment - 1)) == 0);
  return (pos + alignment - 1) & ~(alignment - 1);
}

// This implementation of the Serializer interface is
// used to count how many bytes the output buffer will need.
class SerializerDryRunBase : public Serializer {
  size_t pos_;

public:
  SerializerDryRunBase() : pos_(0) {}

  virtual void writeByte(char) override { pos_ += sizeof(char); }
  virtual void writeBytes(const char *, size_t bufsize) override {
    pos_ += bufsize;
  }
  virtual void writeUint16(uint16_t) override { pos_ += sizeof(uint16_t); }
  virtual void writeUint32(uint32_t) override { pos_ += sizeof(uint32_t); }
  virtual void writeUint64(uint64_t) override { pos_ += sizeof(uint64_t); }
  virtual void writeDouble(double) override { pos_ += sizeof(double); }

  virtual void insertPadding(size_t alignment) override;

  virtual size_t getPos() const override { return pos_; }
};

// This implementation of the Serializer interface is
// used to count how many bytes the output buffer will need.
class SerializerDryRun final : public SerializerDryRunBase {
  size_t arrayCount_;

public:
  SerializerDryRun() : arrayCount_(0) {}

  size_t getArrayCount() const { return arrayCount_; }

  virtual void
  recordArraySize(const std::function<uint32_t(uint32_t)> &f) override;
};

class SerializerInitArraySizes final : public SerializerDryRunBase {
  std::vector<uint32_t> &arraySizes_; // Not owned

public:
  SerializerInitArraySizes(std::vector<uint32_t> &arraySizes)
      : arraySizes_(arraySizes) {}

  virtual void
  recordArraySize(const std::function<uint32_t(uint32_t)> &f) override;
};

class SerializeToBufferBase : public Serializer {
  size_t arrayCount_;
  const std::vector<uint32_t> &arraySizes_; // Not owned

public:
  SerializeToBufferBase(const std::vector<uint32_t> &arraySizes)
      : arrayCount_(0), arraySizes_(arraySizes) {}

  virtual void
  recordArraySize(const std::function<uint32_t(uint32_t)> &f) override;
};

template <Endianness endianness>
class SerializeToBuffer final : public SerializeToBufferBase {
  size_t pos_;
  char *buf_; // Not owned by this class

public:
  SerializeToBuffer(const std::vector<uint32_t> &arraySizes, char *buf)
      : SerializeToBufferBase(arraySizes), pos_(0), buf_(buf) {}

  virtual void writeByte(char c) override {
    buf_[pos_] = c;
    pos_ += sizeof(char);
  }

  virtual void writeBytes(const char *buf, size_t bufsize) override {
    memcpy(&buf_[pos_], buf, bufsize);
    pos_ += bufsize;
  }

  virtual void writeUint16(uint16_t x) override {
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    if (endianness == LittleEndian) {
      *(uint16_t *)&buf_[pos_] = htole16(x);
    } else {
      *(uint16_t *)&buf_[pos_] = htobe16(x);
    }
    pos_ += sizeof(uint16_t);
  }

  virtual void writeUint32(uint32_t x) override {
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    if (endianness == LittleEndian) {
      *(uint32_t *)&buf_[pos_] = htole32(x);
    } else {
      *(uint32_t *)&buf_[pos_] = htobe32(x);
    }
    pos_ += sizeof(uint32_t);
  }

  virtual void writeUint64(uint64_t x) override {
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    if (endianness == LittleEndian) {
      *(uint64_t *)&buf_[pos_] = htole64(x);
    } else {
      *(uint64_t *)&buf_[pos_] = htobe64(x);
    }
    pos_ += sizeof(uint64_t);
  }

  virtual void writeDouble(double d) override {
    // double is the same size as uint64_t, so we cast the value
    // to uint64_t and use writeUint64.
    static_assert(sizeof(double) == sizeof(uint64_t));
    union Cast {
      double d_;
      uint64_t x_;
    };
    Cast c;
    c.d_ = d;
    writeUint64(c.x_);
  }

  virtual void insertPadding(size_t alignment) override {
    // Calculate the new position.
    const size_t newpos = alignup(pos_, alignment);
    // Insert zero bytes.
    memset(&buf_[pos_], 0, newpos - pos_);
    pos_ = newpos;
  }

  virtual size_t getPos() const override { return pos_; }
};

// Warning: this serializer is only suitable for serializing types, not
// objects. That's because you can't put '\0' bytes into a std::string, and
// '\0' bytes are required for objects. (Types serialize to pure ASCII, so
// they're ok.)
template <Endianness endianness>
class SerializeToString final : public SerializeToBufferBase {
  std::string &str_; // Not owned

public:
  SerializeToString(const std::vector<uint32_t> &arraySizes, std::string &str)
      : SerializeToBufferBase(arraySizes), str_(str) {}

  virtual void writeByte(char c) override { str_.push_back(c); }

  virtual void writeBytes(const char *buf, size_t bufsize) override {
    str_.append(buf, bufsize);
  }

  virtual void writeUint16(uint16_t x) override {
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    if (endianness == LittleEndian) {
      x = htole16(x);
    } else {
      x = htobe16(x);
    }
    writeBytes((const char *)&x, sizeof(x));
  }

  virtual void writeUint32(uint32_t x) override {
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    if (endianness == LittleEndian) {
      x = htole32(x);
    } else {
      x = htobe32(x);
    }
    writeBytes((const char *)&x, sizeof(x));
  }

  virtual void writeUint64(uint64_t x) override {
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    if (endianness == LittleEndian) {
      x = htole64(x);
    } else {
      x = htobe64(x);
    }
    writeBytes((const char *)&x, sizeof(x));
  }

  virtual void writeDouble(double d) override {
    // double is the same size as uint64_t, so we cast the value
    // to uint64_t and use writeUint64.
    static_assert(sizeof(double) == sizeof(uint64_t));
    union Cast {
      double d_;
      uint64_t x_;
    };
    Cast c;
    c.d_ = d;
    writeUint64(c.x_);
  }

  virtual void insertPadding(size_t alignment) override {
    // Check that alignment is a power of 2.
    assert((alignment & (alignment - 1)) == 0);
    // Calculate the new position.
    const size_t pos = getPos();
    const size_t newpos = (pos + alignment - 1) & ~(alignment - 1);
    // Insert zero bytes.
    str_.append('\0', newpos - pos);
  }

  virtual size_t getPos() const override { return str_.length(); }
};
