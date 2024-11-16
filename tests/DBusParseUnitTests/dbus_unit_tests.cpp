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

#include "dbus.hpp"
#include "dbus_print.hpp"
#include "dbus_random.hpp"
#include "dbus_serialize.hpp"
#include "endianness.hpp"
#include <memory>
#include <unistd.h>

template <Endianness endianness>
std::unique_ptr<char[]> dbus_object_to_buffer(const DBusObject &object,
                                              size_t &size) {
  std::vector<uint32_t> arraySizes;
  SerializerInitArraySizes s0(arraySizes);
  object.serialize(s0);
  size = s0.getPos();

  std::unique_ptr<char[]> result(new char[size]);
  SerializeToBuffer<endianness> s1(arraySizes, result.get());
  object.serialize(s1);

  return result;
}

template <Endianness endianness>
std::unique_ptr<DBusObject> parse_dbus_object_from_buffer(const DBusType &t,
                                                          const char *buf,
                                                          const size_t buflen) {
  class Cont final : public DBusType::ParseObjectCont<endianness> {
    std::unique_ptr<DBusObject> &result_;

  public:
    explicit Cont(std::unique_ptr<DBusObject> &result) : result_(result) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &, std::unique_ptr<DBusObject> &&object) override {
      result_ = std::move(object);
      return ParseStop::mk();
    }
  };

  std::unique_ptr<DBusObject> result;
  Parse p(t.mkObjectParser<endianness>(Parse::State::initialState_,
                                       std::make_unique<Cont>(result)));

  while (true) {
    const size_t required = p.maxRequiredBytes();
    const size_t pos = p.getPos();
    if (required == 0) {
      assert(pos == buflen);
      return result;
    }
    if (required > buflen - pos) {
      throw ParseError(pos, "parse_dbus_object_from_buffer: not enough bytes");
    }
    p.parse(buf + pos, required);
  }

  return result;
}

#define DEBUGPRINT 0

// This function checks the serializer and parser for consistency.
// It does that by running the following steps:
//
// 1. Serialize `object` to a buffer named `buf0`.
// 2. Parse `buf0`. The new object is called `parsedObject`.
// 3. Serialize `parsedObject` to a buffer named `buf1`.
// 4. Check that `buf0` and `buf1` are identical.
//
// It would also be valuable to compare `object` and `parsedObject` for
// equality, but `DBusObject` doesn't currently have an `operator==`.
template <Endianness endianness>
void check_serialize_and_parse(const DBusType &t, const DBusObject &object) {
  if (DEBUGPRINT) {
    PrinterFD printFD(STDOUT_FILENO, 16, 2);
    t.print(printFD);
    printFD.printNewline(0);
    object.print(printFD);
    printFD.printNewline(0);
  }

  size_t size0 = 0;
  std::unique_ptr<char[]> buf0 =
      dbus_object_to_buffer<endianness>(object, size0);

  std::unique_ptr<DBusObject> parsedObject =
      parse_dbus_object_from_buffer<endianness>(t, buf0.get(), size0);

  if (DEBUGPRINT) {
    PrinterFD printFD(STDOUT_FILENO, 16, 2);
    parsedObject->print(printFD);
    printFD.printNewline(0);
  }

  size_t size1 = 0;
  std::unique_ptr<char[]> buf1 =
      dbus_object_to_buffer<endianness>(*parsedObject, size1);

  // Check that the two serialized buffers are identical.
  if (size0 != size1) {
    throw Error("Serialized string sizes don't match.");
  }
  if (memcmp(buf0.get(), buf1.get(), size0) != 0) {
    throw Error("Serialized strings don't match.");
  }
}

int main() {
  for (size_t i = 0; i < 100000; i++) {
    DBusRandomMersenne r(i, 1000);
    DBusTypeStorage typeStorage;
    const size_t maxdepth = 20;
    const DBusType &t = randomType(r, typeStorage, maxdepth);
    std::unique_ptr<DBusObject> object = randomObject(r, t, maxdepth);
    check_serialize_and_parse<LittleEndian>(t, *object);
    check_serialize_and_parse<BigEndian>(t, *object);
  }
  return 0;
}
