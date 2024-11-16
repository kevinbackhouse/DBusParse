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
#include <random>

// Parameters for generating random DBus types and objects.
class DBusRandom {
public:
  // Return a randomly chosen letter corresponding to a valid DBus type.
  // If `maxdepth` is zero, then the result should not be a basic type,
  // not a container type (like array or struct).
  // https://dbus.freedesktop.org/doc/dbus-specification.html#idm487
  virtual char randomType(const size_t maxdepth) = 0;

  // Choose a random number of fields for a struct.
  virtual size_t randomNumFields() = 0;

  // Choose a random number of elements for an array.
  virtual size_t randomArraySize() = 0;

  virtual char randomChar() = 0;
  virtual bool randomBoolean() = 0;
  virtual uint16_t randomUint16() = 0;
  virtual uint32_t randomUint32() = 0;
  virtual uint64_t randomUint64() = 0;
  virtual double randomDouble() = 0;
  virtual std::string randomString() = 0;
  virtual std::string randomPath() = 0;
};

// Implementation of DBusRandom, using a Mersenne Twister
// pseudo random number generator.
class DBusRandomMersenne : public DBusRandom {
  std::mt19937_64 gen_; // Random number generator

  // Keeps track of the total number of struct and array elements allocated
  // so far, to prevent the total size from getting too big.
  size_t maxsize_;

public:
  DBusRandomMersenne(std::uint_fast64_t seed, size_t maxsize);

  char randomType(const size_t maxdepth) final override;

  // Choose a random number of fields for a struct.
  size_t randomNumFields() final override;

  char randomChar() final override;
  bool randomBoolean() final override;
  uint16_t randomUint16() final override;
  uint32_t randomUint32() final override;
  uint64_t randomUint64() final override;
  double randomDouble() final override;
  std::string randomString() final override;
  std::string randomPath() final override;

  // Choose a random number of elements for an array.
  size_t randomArraySize() final override;
};

// Generate a random DBusType.
const DBusType &randomType(DBusRandom &r,
                           DBusTypeStorage &typeStorage, // Type allocator
                           const size_t maxdepth);

// Generate a random DBusObject.
std::unique_ptr<DBusObject> randomObject(DBusRandom &r, const DBusType &t,
                                         const size_t maxdepth);
