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

#include "parse.hpp"
#include <assert.h>

static_assert(!std::is_polymorphic<Parse::State>::value,
              "Parse::State does not have any virtual methods");

static_assert(!std::is_polymorphic<Parse>::value,
              "Parse does not have any virtual methods");

const Parse::State Parse::State::initialState_(0);

void Parse::parse(const char *buf, size_t bufsize) {
  assert(minRequiredBytes() <= bufsize);
  assert(bufsize <= maxRequiredBytes());
  if (__builtin_add_overflow(bufsize, state_.pos_, &state_.pos_)) {
    throw ParseError(state_.pos_, "Integer overflow in Parse::parse");
  }
  cont_ = cont_->parse(state_, buf, bufsize);
}

uint8_t Parse::minRequiredBytes() const { return cont_->minRequiredBytes(); }

size_t Parse::maxRequiredBytes() const { return cont_->maxRequiredBytes(); }

std::unique_ptr<Parse::Cont> ParseStop::parse(const Parse::State &,
                                              const char *, size_t) {
  // Parsing is finished, so this function is never called.
  assert(false);
  return ParseStop::mk();
}

std::unique_ptr<Parse::Cont> ParseStop::mk() {
  return std::make_unique<ParseStop>();
}

std::unique_ptr<Parse::Cont> ParseChar::parse(const Parse::State &p,
                                              const char *buf, size_t bufsize) {
  (void)bufsize;
  assert(bufsize == sizeof(char));
  return cont_->parse(p, buf[0]);
}

std::unique_ptr<Parse::Cont> ParseChar::mk(std::unique_ptr<Cont> &&cont) {
  return std::make_unique<ParseChar>(std::move(cont));
}

std::unique_ptr<Parse::Cont>
ParseNChars::parse(const Parse::State &p, const char *buf, size_t bufsize) {
  (void)bufsize;
  assert(bufsize <= n_);
  str_.append(buf, bufsize);

  // Parse remaining bytes.
  return mk(p, std::move(str_), n_ - bufsize, std::move(cont_));
}

std::unique_ptr<Parse::Cont> ParseNChars::mk(const Parse::State &p,
                                             std::string &&str, size_t n,
                                             std::unique_ptr<Cont> &&cont) {
  if (n == 0) {
    // There's nothing to parse, so invoke the next continuation immediately.
    return cont->parse(p, std::move(str));
  }

  return std::make_unique<ParseNChars>(std::move(str), n, std::move(cont));
}

std::unique_ptr<Parse::Cont>
ParseZeros::parse(const Parse::State &p, const char *buf, size_t bufsize) {
  assert(bufsize <= n_);

  // Check that the bytes are zero.
  for (size_t i = 0; i < bufsize; i++) {
    if (buf[i] != '\0') {
      throw ParseError(p.getPos() + i, "Unexpected non-zero byte.");
    }
  }

  // Parse remaining bytes.
  return mk(p, n_ - bufsize, std::move(cont_));
}

std::unique_ptr<Parse::Cont> ParseZeros::mk(const Parse::State &p, size_t n,
                                            std::unique_ptr<Cont> &&cont) {
  if (n == 0) {
    // There's nothing to parse, so invoke the next continuation
    // immediately.
    return cont->parse(p);
  }

  return std::make_unique<ParseZeros>(n, std::move(cont));
}
