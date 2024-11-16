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

#include "dbus_serialize.hpp"
#include <string.h>
#include <unistd.h>

void SerializerDryRunBase::insertPadding(size_t alignment) {
  pos_ = alignup(pos_, alignment);
}

void SerializerDryRun::recordArraySize(
    const std::function<uint32_t(uint32_t)> &f) {
  (void)f(0xDEADBEEF);
  ++arrayCount_;
}

void SerializerInitArraySizes::recordArraySize(
    const std::function<uint32_t(uint32_t)> &f) {
  const size_t i = arraySizes_.size();
  // Create a slot in the array. When `f` returns, it will give us the
  // value which we need to write into the slot.
  arraySizes_.push_back(0xDEADBEEF);
  // Run `f` and write the value that it returns into the slot
  // that we just created.
  arraySizes_[i] = f(0xDEADBEEF);
}

void SerializeToBufferBase::recordArraySize(
    const std::function<uint32_t(uint32_t)> &f) {
  (void)f(arraySizes_.at(arrayCount_++));
}

std::string DBusType::toString() const {
  SerializerDryRun s0;
  serialize(s0);
  size_t size = s0.getPos();

  // arraySizes is not used, but it's required as an argument to
  // SerializeToString.
  std::vector<uint32_t> arraySizes;

  std::string result;
  result.reserve(size);
  // Note: types only contain ASCII characters so the endianness of the
  // serializer doesn't matter. So the choice of LittleEndian here is
  // arbitrary.
  SerializeToString<LittleEndian> s1(arraySizes, result);
  serialize(s1);
  return result;
}

static void mkSignatureHelper(const DBusObjectSeq &seq, Serializer &s) {
  const size_t n = seq.length();
  for (size_t i = 0; i < n; i++) {
    seq.getElement(i)->getType().serialize(s);
  }
}

std::string DBusMessageBody::signature() const {
  SerializerDryRun s0;
  mkSignatureHelper(seq_, s0);
  size_t size = s0.getPos();

  // arraySizes is not used, but it's required as an argument to
  // SerializeToString.
  std::vector<uint32_t> arraySizes;

  std::string result;
  result.reserve(size);
  // Note: types only contain ASCII characters so the endianness of the
  // serializer doesn't matter. So the choice of LittleEndian here is
  // arbitrary.
  SerializeToString<LittleEndian> s1(arraySizes, result);
  mkSignatureHelper(seq_, s1);
  return result;
}

void DBusMessageBody::serialize(Serializer &s) const { seq_.serialize(s); }

size_t DBusMessageBody::serializedSize() const {
  SerializerDryRun s;
  serialize(s);
  return s.getPos();
}

size_t DBusObject::serializedSize() const {
  SerializerDryRun s;
  serialize(s);
  return s.getPos();
}

void DBusMessage::serialize(Serializer &s) const {
  header_->serialize(s);
  if (body_) {
    // The body should be 8-byte aligned.
    s.insertPadding(DBusTypeUint64::instance_.alignment());
    body_->serialize(s);
  }
}
