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

#include "dbus.hpp"
#include "utils.hpp"
#include <assert.h>

static_assert(!std::is_polymorphic<DBusTypeStorage>::value,
              "DBusTypeStorage does not have any virtual methods");

const DBusTypeChar DBusTypeChar::instance_;
const DBusTypeBoolean DBusTypeBoolean::instance_;
const DBusTypeUint16 DBusTypeUint16::instance_;
const DBusTypeInt16 DBusTypeInt16::instance_;
const DBusTypeUint32 DBusTypeUint32::instance_;
const DBusTypeInt32 DBusTypeInt32::instance_;
const DBusTypeUint64 DBusTypeUint64::instance_;
const DBusTypeInt64 DBusTypeInt64::instance_;
const DBusTypeDouble DBusTypeDouble::instance_;
const DBusTypeUnixFD DBusTypeUnixFD::instance_;
const DBusTypeString DBusTypeString::instance_;
const DBusTypePath DBusTypePath::instance_;
const DBusTypeSignature DBusTypeSignature::instance_;
const DBusTypeVariant DBusTypeVariant::instance_;

DBusObjectChar::DBusObjectChar(char c) : c_(c) {}

DBusObjectBoolean::DBusObjectBoolean(bool b) : b_(b) {}

DBusObjectUint16::DBusObjectUint16(uint16_t x) : x_(x) {}

DBusObjectInt16::DBusObjectInt16(int16_t x) : x_(x) {}

DBusObjectUint32::DBusObjectUint32(uint32_t x) : x_(x) {}

DBusObjectInt32::DBusObjectInt32(int32_t x) : x_(x) {}

DBusObjectUint64::DBusObjectUint64(uint64_t x) : x_(x) {}

DBusObjectInt64::DBusObjectInt64(int64_t x) : x_(x) {}

DBusObjectDouble::DBusObjectDouble(double d) : d_(d) {}

DBusObjectUnixFD::DBusObjectUnixFD(uint32_t i) : i_(i) {}

DBusObjectString::DBusObjectString(std::string &&str) : str_(std::move(str)) {
  // String length must fit in a `uint32_t`.
  assert((str_.size() >> 32) == 0);
}

DBusObjectPath::DBusObjectPath(std::string &&str) : str_(std::move(str)) {
  // String length must fit in a `uint32_t`.
  assert((str_.size() >> 32) == 0);
}

DBusObjectSignature::DBusObjectSignature(std::string &&str)
    : str_(std::move(str)) {
  // String length must fit in a `uint8_t`.
  assert((str_.size() >> 8) == 0);
}

DBusObjectVariant::DBusObjectVariant(std::unique_ptr<DBusObject> &&object)
    : object_(std::move(object)), signature_(object_->getType().toString()) {}

DBusObjectDictEntry::DBusObjectDictEntry(std::unique_ptr<DBusObject> &&key,
                                         std::unique_ptr<DBusObject> &&value)
    : key_(std::move(key)), value_(std::move(value)),
      dictEntryType_(key_->getType(), value_->getType()) {}

DBusObjectSeq::DBusObjectSeq(
    std::vector<std::unique_ptr<DBusObject>> &&elements)
    : elements_(std::move(elements)) {}

DBusObjectArray::DBusObjectArray(
    const DBusType &baseType,
    std::vector<std::unique_ptr<DBusObject>> &&elements)
    : seq_(std::move(elements)), arrayType_(baseType) {}

DBusObjectArray0::DBusObjectArray0(
    const DBusType &baseType,
    std::vector<std::unique_ptr<DBusObject>> &&elements,
    DBusTypeStorage &&typeStorage)
    : DBusObjectArray(baseType, std::move(elements)),
      typeStorage_(std::move(typeStorage)) {}

std::unique_ptr<DBusObjectArray>
DBusObjectArray::mk0(const DBusType &baseType) {
  // Clone the base type to ensure that it won't contain
  // any dangling references.
  DBusTypeStorage typeStorage;
  const DBusType &newBaseType = cloneType(typeStorage, baseType);

  return std::make_unique<DBusObjectArray0>(
      newBaseType, std::vector<std::unique_ptr<DBusObject>>(),
      std::move(typeStorage));
}

DBusObjectStruct::DBusObjectStruct(
    std::vector<std::unique_ptr<DBusObject>> &&elements)
    : seq_(std::move(elements)), structType_(seq_.elementTypes()) {}

DBusHeaderField::DBusHeaderField(HeaderFieldName name,
                                 std::unique_ptr<DBusObjectVariant> &&v)
    : DBusObjectStruct(_vec(_obj(DBusObjectChar::mk(name)), std::move(v))) {}

static const DBusTypeStruct headerFieldType(
    _vec(std::reference_wrapper<const DBusType>(DBusTypeChar::instance_),
         std::reference_wrapper<const DBusType>(DBusTypeVariant::instance_)));

static const DBusTypeArray headerFieldsType(headerFieldType);

// The type of the header of a DBus message.
const DBusTypeStruct headerType(
    _vec(std::reference_wrapper<const DBusType>(DBusTypeChar::instance_),
         std::reference_wrapper<const DBusType>(DBusTypeChar::instance_),
         std::reference_wrapper<const DBusType>(DBusTypeChar::instance_),
         std::reference_wrapper<const DBusType>(DBusTypeChar::instance_),
         std::reference_wrapper<const DBusType>(DBusTypeUint32::instance_),
         std::reference_wrapper<const DBusType>(DBusTypeUint32::instance_),
         std::reference_wrapper<const DBusType>(headerFieldsType)));

DBusMessageBody::DBusMessageBody(
    std::vector<std::unique_ptr<DBusObject>> &&elements)
    : seq_(std::move(elements)) {}

std::unique_ptr<DBusMessageBody> DBusMessageBody::mk0() {
  return std::make_unique<DBusMessageBody>(
      std::vector<std::unique_ptr<DBusObject>>());
}

std::unique_ptr<DBusMessageBody>
DBusMessageBody::mk1(std::unique_ptr<DBusObject> &&element) {
  return std::make_unique<DBusMessageBody>(_vec(std::move(element)));
}

std::unique_ptr<DBusMessageBody>
DBusMessageBody::mk(std::vector<std::unique_ptr<DBusObject>> &&elements) {
  return std::make_unique<DBusMessageBody>(std::move(elements));
}

const DBusType &cloneType(DBusTypeStorage &typeStorage, // Type allocator
                          const DBusType &t) {
  class CloneVisitor final : public DBusType::Visitor {
    DBusTypeStorage &typeStorage_; // Not owned
    const DBusType *result_;

  public:
    CloneVisitor(DBusTypeStorage &typeStorage // Type allocator
                 )
        : typeStorage_(typeStorage), result_(0) {}

    virtual void visitChar(const DBusTypeChar &) override {
      result_ = &DBusTypeChar::instance_;
    }
    virtual void visitBoolean(const DBusTypeBoolean &) override {
      result_ = &DBusTypeBoolean::instance_;
    }
    virtual void visitUint16(const DBusTypeUint16 &) override {
      result_ = &DBusTypeUint16::instance_;
    }
    virtual void visitInt16(const DBusTypeInt16 &) override {
      result_ = &DBusTypeInt16::instance_;
    }
    virtual void visitUint32(const DBusTypeUint32 &) override {
      result_ = &DBusTypeUint32::instance_;
    }
    virtual void visitInt32(const DBusTypeInt32 &) override {
      result_ = &DBusTypeInt32::instance_;
    }
    virtual void visitUint64(const DBusTypeUint64 &) override {
      result_ = &DBusTypeUint64::instance_;
    }
    virtual void visitInt64(const DBusTypeInt64 &) override {
      result_ = &DBusTypeInt64::instance_;
    }
    virtual void visitDouble(const DBusTypeDouble &) override {
      result_ = &DBusTypeDouble::instance_;
    }
    virtual void visitUnixFD(const DBusTypeUnixFD &) override {
      result_ = &DBusTypeUnixFD::instance_;
    }
    virtual void visitString(const DBusTypeString &) override {
      result_ = &DBusTypeString::instance_;
    }
    virtual void visitPath(const DBusTypePath &) override {
      result_ = &DBusTypePath::instance_;
    }
    virtual void visitSignature(const DBusTypeSignature &) override {
      result_ = &DBusTypeSignature::instance_;
    }
    virtual void visitVariant(const DBusTypeVariant &) override {
      result_ = &DBusTypeVariant::instance_;
    }

    virtual void visitDictEntry(const DBusTypeDictEntry &t) override {
      const DBusType &keyType = cloneType(typeStorage_, t.getKeyType());
      const DBusType &valueType = cloneType(typeStorage_, t.getValueType());
      // Allocate a `DBusTypeDictEntry` by adding it to `typeStorage_`.
      result_ = &typeStorage_.allocDictEntry(keyType, valueType);
    }

    virtual void visitArray(const DBusTypeArray &t) override {
      const DBusType &baseType = cloneType(typeStorage_, t.getBaseType());
      // Allocate a `DBusTypeArray` by adding it to `typeStorage_`.
      result_ = &typeStorage_.allocArray(baseType);
    }

    virtual void visitStruct(const DBusTypeStruct &t) override {
      auto fieldTypes = t.getFieldTypes();
      const size_t n = fieldTypes.size();
      for (size_t i = 0; i < n; i++) {
        fieldTypes[i] = cloneType(typeStorage_, fieldTypes[i]);
      }
      // Allocate a `DBusTypeStruct` by adding it to `structStorage_`.
      result_ = &typeStorage_.allocStruct(std::move(fieldTypes));
    }

    const DBusType &getResult() const { return *result_; }
  };

  CloneVisitor visitor(typeStorage);
  t.accept(visitor);
  return visitor.getResult();
}
