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

#include "endianness.hpp"
#include <assert.h>
#include <memory>
#include <string>

// Continuation-passing-style parser implementation. The main class
// is `Parse`. You initialize the parser with a continuation of
// type `Parse::Cont`. The continuation consumes a fixed number of
// bytes and returns a new continuation to keep parsing the rest of
// the input. The continuation-passing design has several benefits:
//
// 1. It consumes the input in small chunks, so it is easy to feed it with
//    network data. This means that we can easily pause parsing while we
//    wait for more data, so it works well in conjunction with something
//    like epoll.
// 2. Because the parser processes data incrementally it can reject
//    invalid messages early, without needing to wait for the whole
//    message to arrive.
// 3. The implementation does not use recursion, so it is impossible for a
//    malicious input to trigger stack exhaustion in the parser. (Of course
//    a stack exhaustion could still happen later if we run a recursive
//    algorithm over the parsed object.) There is a still a parsing stack,
//    but it consists of a linked list of continuations on the heap.
//
// Implementing this design in C++ may have been a mistake.

// Thrown as an exception when the parser encounters an invalid input.
class ParseError final : public std::exception {
  // Byte position of the parse error.
  const size_t pos_;

  // Error message.
  std::string msg_;

public:
  ParseError() = delete; // No default constructor.
  explicit ParseError(size_t pos, const char *msg) : pos_(pos), msg_(msg) {}
  explicit ParseError(size_t pos, std::string &&msg)
      : pos_(pos), msg_(std::move(msg)) {}

  size_t getPos() const noexcept { return pos_; }

  const char *what() const noexcept override { return msg_.c_str(); }
};

class Parse final {
public:
  // This class has a virtual method which is the continuation function.
  class Cont;

  // The parsing state is currently just the number of bytes parsed so far,
  // but it is wrapped in a class to make it easier to add other status
  // information in the future. A const reference to the parsing state is
  // passed as an argument to `Parse::Cont::parse()`, so that the parsers
  // can inspect the current state. For example, this is often used to
  // calculate the alignment of the current byte position.
  class State {
    friend Parse;

  protected:
    // The number of bytes parsed so far.
    // This is used for calculating alignments.
    size_t pos_;

    explicit State(size_t pos) : pos_(pos) {}

    void reset() { pos_ = 0; }

  public:
    // No copy constructor
    State(const State &) = delete;

    size_t getPos() const { return pos_; }

    static const State initialState_;
  };

private:
  State state_;
  std::unique_ptr<Parse::Cont> cont_;

public:
  // No copy constructor
  Parse(const Parse &) = delete;

  // For initializing the parser.
  explicit Parse(std::unique_ptr<Parse::Cont> &&cont)
      : state_(0), cont_(std::move(cont)) {}

  void reset(std::unique_ptr<Parse::Cont> &&cont) {
    state_.reset();
    cont_ = std::move(cont);
  }

  // Before calling this method, you should call `minRequiredBytes()`
  // and `maxRequiredBytes()` to find
  // out how many bytes the parser is prepared to accept. You must call
  // this method with a value of `bufsize` that satisfies these rules:
  //
  //   1. minRequiredBytes() <= bufsize
  //   2. bufsize <= maxRequiredBytes()
  //
  // The reason why the number of required bytes is specified as a range
  // is to enable the caller to use a fixed-size buffer. A fixed size
  // buffer of 255 bytes is guaranteed to be sufficient. So the caller
  // can keep feeding the parser small chunks of bytes until parsing
  // is complete.
  void parse(const char *buf, size_t bufsize);

  // The number of bytes parsed so far.
  size_t getPos() const { return state_.pos_; }

  // The minimum number of bytes that the parser needs to make
  // progress. This simplifies the implementation of the parsers for simple
  // types like `uint32_t`, because it means that they don't need to be
  // able to accept partial input. For example, the parser for `uint32_t`
  // can specify that it needs at least 4 bytes to make progress. Note
  // that the result of this function is a `uint8_t`, which means that the
  // caller doesn't need to implement an arbitrary sized buffer. A 255
  // byte fixed-size buffer is guaranteed to be sufficient.
  uint8_t minRequiredBytes() const;

  // The maximum number of bytes that the parser is prepared to
  // consume at this time. If this method returns 0 then it
  // means that parsing is complete.
  size_t maxRequiredBytes() const;
};

class Parse::Cont {
public:
  virtual ~Cont() {}
  virtual std::unique_ptr<Parse::Cont>
  parse(const Parse::State &p, const char *buf, size_t bufsize) = 0;

  // The minimum number of bytes that this continuation is willing to
  // accept.
  virtual uint8_t minRequiredBytes() const = 0;

  // The maximum number of bytes that this continuation is willing to
  // accept.
  virtual size_t maxRequiredBytes() const = 0;
};

// This continuation is used to indicate that parsing is complete.
// It does so by returning 0 in `maxRequiredBytes`.
class ParseStop final : public Parse::Cont {
public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseStop() {}

  virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                             const char *buf, size_t) override;

  // Factory method.
  static std::unique_ptr<Parse::Cont> mk();

  uint8_t minRequiredBytes() const override { return 0; }
  size_t maxRequiredBytes() const override { return 0; }
};

class ParseChar final : public Parse::Cont {
public:
  class Cont {
  public:
    virtual ~Cont() {}
    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               char c) = 0;
  };

private:
  // Continuation
  const std::unique_ptr<Cont> cont_;

public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseChar(std::unique_ptr<Cont> &&cont) : cont_(std::move(cont)) {}

  virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                             const char *buf, size_t) override;

  // Factory method.
  static std::unique_ptr<Parse::Cont> mk(std::unique_ptr<Cont> &&cont);

  uint8_t minRequiredBytes() const override { return sizeof(char); }
  size_t maxRequiredBytes() const override { return sizeof(char); }
};

template <Endianness endianness> class ParseUint16 final : public Parse::Cont {
public:
  class Cont {
  public:
    virtual ~Cont() {}
    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint16_t c) = 0;
  };

private:
  // Continuation
  const std::unique_ptr<Cont> cont_;

public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseUint16(std::unique_ptr<Cont> &&cont) : cont_(std::move(cont)) {}

  virtual std::unique_ptr<Parse::Cont>
  parse(const Parse::State &p, const char *buf, size_t bufsize) override {
    (void)bufsize;
    assert(bufsize == sizeof(uint16_t));
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    const uint16_t x = endianness == LittleEndian ? le16toh(*(uint16_t *)buf)
                                                  : be16toh(*(uint16_t *)buf);
    return cont_->parse(p, x);
  }

  // Factory method.
  static std::unique_ptr<Parse::Cont> mk(std::unique_ptr<Cont> &&cont) {
    return std::make_unique<ParseUint16>(std::move(cont));
  }

  uint8_t minRequiredBytes() const override { return sizeof(uint16_t); }
  size_t maxRequiredBytes() const override { return sizeof(uint16_t); }
};

template <Endianness endianness> class ParseUint32 final : public Parse::Cont {
public:
  class Cont {
  public:
    virtual ~Cont() {}
    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t c) = 0;
  };

private:
  // Continuation
  const std::unique_ptr<Cont> cont_;

public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseUint32(std::unique_ptr<Cont> &&cont) : cont_(std::move(cont)) {}

  virtual std::unique_ptr<Parse::Cont>
  parse(const Parse::State &p, const char *buf, size_t bufsize) override {
    (void)bufsize;
    assert(bufsize == sizeof(uint32_t));
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    const uint32_t x = endianness == LittleEndian ? le32toh(*(uint32_t *)buf)
                                                  : be32toh(*(uint32_t *)buf);
    return cont_->parse(p, x);
  }

  // Factory method.
  static std::unique_ptr<Parse::Cont> mk(std::unique_ptr<Cont> &&cont) {
    return std::make_unique<ParseUint32>(std::move(cont));
  }

  uint8_t minRequiredBytes() const override { return sizeof(uint32_t); }
  size_t maxRequiredBytes() const override { return sizeof(uint32_t); }
};

template <Endianness endianness> class ParseUint64 final : public Parse::Cont {
public:
  class Cont {
  public:
    virtual ~Cont() {}
    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint64_t c) = 0;
  };

private:
  // Continuation
  const std::unique_ptr<Cont> cont_;

public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseUint64(std::unique_ptr<Cont> &&cont) : cont_(std::move(cont)) {}

  virtual std::unique_ptr<Parse::Cont>
  parse(const Parse::State &p, const char *buf, size_t bufsize) override {
    (void)bufsize;
    assert(bufsize == sizeof(uint64_t));
    static_assert(endianness == LittleEndian || endianness == BigEndian);
    const uint64_t x = endianness == LittleEndian ? le64toh(*(uint64_t *)buf)
                                                  : be64toh(*(uint64_t *)buf);
    return cont_->parse(p, x);
  }

  // Factory method.
  static std::unique_ptr<Parse::Cont> mk(std::unique_ptr<Cont> &&cont) {
    return std::make_unique<ParseUint64>(std::move(cont));
  }

  uint8_t minRequiredBytes() const override { return sizeof(uint64_t); }
  size_t maxRequiredBytes() const override { return sizeof(uint64_t); }
};

// Parser for a std::string with a known length.
class ParseNChars final : public Parse::Cont {
public:
  class Cont {
  public:
    virtual ~Cont() {}
    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               std::string &&str) = 0;
  };

private:
  // Buffer for the bytes. (May already contain some bytes
  // which were already received on a previous iteration.)
  std::string str_;

  // Number of bytes we expect to receive.
  const size_t n_;

  // Continuation
  std::unique_ptr<Cont> cont_;

public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseNChars(std::string &&str, size_t n, std::unique_ptr<Cont> &&cont)
      : str_(std::move(str)), n_(n), cont_(std::move(cont)) {}

  virtual std::unique_ptr<Parse::Cont>
  parse(const Parse::State &p, const char *buf, size_t bufsize) override;

  // Factory method.
  static std::unique_ptr<Parse::Cont> mk(const Parse::State &p,
                                         std::string &&str, size_t n,
                                         std::unique_ptr<Cont> &&cont);

  uint8_t minRequiredBytes() const override { return 0; }
  size_t maxRequiredBytes() const override { return n_; }
};

// Parse N bytes and check that they are all zero bytes.
class ParseZeros final : public Parse::Cont {
public:
  class Cont {
  public:
    virtual ~Cont() {}
    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) = 0;
  };

private:
  // Number of bytes we expect to receive.
  const size_t n_;

  // Continuation
  std::unique_ptr<Cont> cont_;

public:
  // This constructor is only public for std::make_unique's benefit.
  // Use factory method `mk()` to construct.
  ParseZeros(size_t n, std::unique_ptr<Cont> &&cont)
      : n_(n), cont_(std::move(cont)) {}

  virtual std::unique_ptr<Parse::Cont>
  parse(const Parse::State &p, const char *buf, size_t bufsize) override;

  // Factory method.
  // Note: if `n == 0` then this will invoke the continuation immediately.
  static std::unique_ptr<Parse::Cont> mk(const Parse::State &p, size_t n,
                                         std::unique_ptr<Cont> &&cont);

  uint8_t minRequiredBytes() const override { return 0; }
  size_t maxRequiredBytes() const override { return n_; }
};
