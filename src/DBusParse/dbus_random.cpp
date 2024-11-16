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

#include "dbus_random.hpp"
#include <algorithm>
#include <limits>

DBusRandomMersenne::DBusRandomMersenne(std::uint_fast64_t seed, size_t maxsize)
    : gen_(seed), maxsize_(maxsize) {}

char DBusRandomMersenne::randomType(const size_t maxdepth) {
  static const char types[] = {'y', 'b', 'n', 'q', 'i', 'u', 'x', 't', 'd',
                               'h', 's', 'o', 'g', 'v', 'a', '(', '{'};
  if (maxdepth == 0) {
    // Skip the 4 non-trivial types (variant, array, struct, dict).
    std::uniform_int_distribution<> dis(0, sizeof(types) - 5);
    return types[dis(gen_)];
  } else {
    std::uniform_int_distribution<> dis(0, sizeof(types) - 1);
    return types[dis(gen_)];
  }
}

size_t DBusRandomMersenne::randomNumFields() {
  size_t numfields = std::min(size_t(8), maxsize_);
  maxsize_ -= numfields;
  std::uniform_int_distribution<> dis(0, numfields);
  return dis(gen_);
}

size_t DBusRandomMersenne::randomArraySize() {
  size_t numelements = std::min(size_t(8), maxsize_);
  maxsize_ -= numelements;
  std::uniform_int_distribution<> dis(0, numelements);
  return dis(gen_);
}

char DBusRandomMersenne::randomChar() {
  std::uniform_int_distribution<> dis(std::numeric_limits<char>::min(),
                                      std::numeric_limits<char>::max());
  return dis(gen_);
}

bool DBusRandomMersenne::randomBoolean() {
  std::uniform_int_distribution<> dis(0, 1);
  return dis(gen_);
}

uint16_t DBusRandomMersenne::randomUint16() {
  std::uniform_int_distribution<> dis(std::numeric_limits<uint16_t>::min(),
                                      std::numeric_limits<uint16_t>::max());
  return dis(gen_);
}

uint32_t DBusRandomMersenne::randomUint32() {
  std::uniform_int_distribution<> dis(std::numeric_limits<uint32_t>::min(),
                                      std::numeric_limits<uint32_t>::max());
  return dis(gen_);
}

uint64_t DBusRandomMersenne::randomUint64() { return gen_(); }

double DBusRandomMersenne::randomDouble() {
  std::uniform_int_distribution<> switchdis(0, 11);
  switch (switchdis(gen_)) {
  case 0:
    return 0.0;
  case 1:
    return 1.0;
  case 2:
    return 2.0;
  case 3:
    return 1.0 / 0.0;
  case 4:
    return 0.0 / 0.0;
  case 5:
    return -randomDouble();
  case 6:
    return randomDouble() * randomDouble();
  case 7:
    return randomDouble() / randomDouble();
  default:
    return static_cast<double>(randomUint64());
  }
}

std::string DBusRandomMersenne::randomString() {
  std::uniform_int_distribution<> lendis(0, 32);
  size_t len = lendis(gen_);
  std::string result;
  result.reserve(len);
  for (size_t i = 0; i < len; i++) {
    std::uniform_int_distribution<> chardis(1, 127);
    result.push_back(chardis(gen_));
  }
  return result;
}

std::string DBusRandomMersenne::randomPath() {
  // TODO: generate a valid path rather than just an arbitrary string.
  return randomString();
}

static const DBusTypeStruct &
randomStructType(DBusRandom &r,
                 DBusTypeStorage &typeStorage, // Type allocator
                 const size_t maxdepth) {
  std::vector<std::reference_wrapper<const DBusType>> fieldTypes;
  const size_t numFields = r.randomNumFields();
  fieldTypes.reserve(numFields);
  for (size_t i = 0; i < numFields; i++) {
    fieldTypes.push_back(randomType(r, typeStorage, maxdepth));
  }
  return typeStorage.allocStruct(std::move(fieldTypes));
}

const DBusType &randomType(DBusRandom &r,
                           DBusTypeStorage &typeStorage, // Type allocator
                           const size_t maxdepth) {
  switch (r.randomType(maxdepth)) {
  case 'y':
    return DBusTypeChar::instance_;
  case 'b':
    return DBusTypeBoolean::instance_;
  case 'q':
    return DBusTypeUint16::instance_;
  case 'n':
    return DBusTypeInt16::instance_;
  case 'u':
    return DBusTypeUint32::instance_;
  case 'i':
    return DBusTypeInt32::instance_;
  case 't':
    return DBusTypeUint64::instance_;
  case 'x':
    return DBusTypeInt64::instance_;
  case 'd':
    return DBusTypeDouble::instance_;
  case 'h':
    return DBusTypeUnixFD::instance_;
  case 's':
    return DBusTypeString::instance_;
  case 'o':
    return DBusTypePath::instance_;
  case 'g':
    return DBusTypeSignature::instance_;
  case 'v':
    return DBusTypeVariant::instance_;
  case 'a':
    assert(maxdepth > 0); // Invariant of `DBusRandom::randomType()`.
    return typeStorage.allocArray(randomType(r, typeStorage, maxdepth - 1));
  case '(':
    assert(maxdepth > 0); // Invariant of `DBusRandom::randomType()`.
    return randomStructType(r, typeStorage, maxdepth - 1);
  case '{':
    assert(maxdepth > 0); // Invariant of `DBusRandom::randomType()`.
    return typeStorage.allocDictEntry(
        randomType(r, typeStorage, 0), // key is required to be a basic type
        randomType(r, typeStorage, maxdepth - 1));
  default:
    assert(false); // Invariant of `DBusRandom::randomType()`.
    throw Error("Bad type in randomType.");
  }
}

std::unique_ptr<DBusObject> randomObject(DBusRandom &r, const DBusType &t,
                                         const size_t maxdepth) {
  class Visitor : public DBusType::Visitor {
    DBusRandom &r_;
    const size_t maxdepth_;
    std::unique_ptr<DBusObject> result_;

  public:
    Visitor(DBusRandom &r, const size_t maxdepth)
        : r_(r), maxdepth_(maxdepth) {}

    virtual void visitChar(const DBusTypeChar &) {
      result_ = DBusObjectChar::mk(r_.randomChar());
    }

    virtual void visitBoolean(const DBusTypeBoolean &) {
      result_ = DBusObjectBoolean::mk(r_.randomBoolean());
    }

    virtual void visitUint16(const DBusTypeUint16 &) {
      result_ = DBusObjectUint16::mk(r_.randomUint16());
    }

    virtual void visitInt16(const DBusTypeInt16 &) {
      result_ = DBusObjectInt16::mk(static_cast<int16_t>(r_.randomUint16()));
    }

    virtual void visitUint32(const DBusTypeUint32 &) {
      result_ = DBusObjectUint32::mk(r_.randomUint32());
    }

    virtual void visitInt32(const DBusTypeInt32 &) {
      result_ = DBusObjectInt32::mk(static_cast<int32_t>(r_.randomUint32()));
    }

    virtual void visitUint64(const DBusTypeUint64 &) {
      result_ = DBusObjectUint64::mk(r_.randomUint64());
    }

    virtual void visitInt64(const DBusTypeInt64 &) {
      result_ = DBusObjectInt64::mk(static_cast<int64_t>(r_.randomUint64()));
    }

    virtual void visitDouble(const DBusTypeDouble &) {
      result_ = DBusObjectDouble::mk(r_.randomDouble());
    }

    virtual void visitUnixFD(const DBusTypeUnixFD &) {
      result_ = DBusObjectUnixFD::mk(r_.randomUint32());
    }

    virtual void visitString(const DBusTypeString &) {
      result_ = DBusObjectString::mk(r_.randomString());
    }

    virtual void visitPath(const DBusTypePath &) {
      result_ = DBusObjectPath::mk(r_.randomPath());
    }

    virtual void visitSignature(const DBusTypeSignature &) {
      DBusTypeStorage typeStorage;
      const DBusType &t = randomType(r_, typeStorage, maxdepth_);
      result_ = DBusObjectSignature::mk(t.toString());
    }

    virtual void visitVariant(const DBusTypeVariant &) {
      size_t newdepth = maxdepth_ > 0 ? maxdepth_ - 1 : 0;
      DBusTypeStorage typeStorage;
      const DBusType &t = randomType(r_, typeStorage, newdepth);
      result_ = DBusObjectVariant::mk(randomObject(r_, t, newdepth));
    }

    virtual void visitDictEntry(const DBusTypeDictEntry &dictType) {
      size_t newdepth = maxdepth_ > 0 ? maxdepth_ - 1 : 0;
      result_ = DBusObjectDictEntry::mk(
          randomObject(r_, dictType.getKeyType(), 0),
          randomObject(r_, dictType.getValueType(), newdepth));
    }

    virtual void visitArray(const DBusTypeArray &arrayType) {
      size_t newdepth = maxdepth_ > 0 ? maxdepth_ - 1 : 0;
      const DBusType &baseType = arrayType.getBaseType();
      const size_t n = r_.randomArraySize();
      std::vector<std::unique_ptr<DBusObject>> elements;
      elements.reserve(n);
      for (size_t i = 0; i < n; i++) {
        elements.push_back(randomObject(r_, baseType, newdepth));
      }
      result_ = DBusObjectArray::mk(baseType, std::move(elements));
    }

    virtual void visitStruct(const DBusTypeStruct &structType) {
      size_t newdepth = maxdepth_ > 0 ? maxdepth_ - 1 : 0;
      const std::vector<std::reference_wrapper<const DBusType>> &fieldTypes =
          structType.getFieldTypes();
      const size_t n = fieldTypes.size();
      std::vector<std::unique_ptr<DBusObject>> elements;
      elements.reserve(n);
      for (size_t i = 0; i < n; i++) {
        elements.push_back(randomObject(r_, fieldTypes[i], newdepth));
      }
      result_ = DBusObjectStruct::mk(std::move(elements));
    }

    std::unique_ptr<DBusObject> getResult() { return std::move(result_); }
  };

  Visitor v(r, maxdepth);
  t.accept(v);
  return v.getResult();
}
