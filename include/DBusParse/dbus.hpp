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

#include "error.hpp"
#include "parse.hpp"
#include <functional>

enum MessageType {
  MSGTYPE_INVALID = 0,
  MSGTYPE_METHOD_CALL = 1,
  MSGTYPE_METHOD_RETURN = 2,
  MSGTYPE_ERROR = 3,
  MSGTYPE_SIGNAL = 4
};

enum MessageFlags {
  MSGFLAGS_EMPTY = 0x0,
  MSGFLAGS_NO_REPLY_EXPECTED = 0x1,
  MSGFLAGS_NO_AUTO_START = 0x2,
  MSGFLAGS_ALLOW_INTERACTIVE_AUTHORIZATION = 0x4
};

enum HeaderFieldName {
  MSGHDR_INVALID = 0,
  MSGHDR_PATH = 1,
  MSGHDR_INTERFACE = 2,
  MSGHDR_MEMBER = 3,
  MSGHDR_ERROR_NAME = 4,
  MSGHDR_REPLY_SERIAL = 5,
  MSGHDR_DESTINATION = 6,
  MSGHDR_SENDER = 7,
  MSGHDR_SIGNATURE = 8,
  MSGHDR_UNIX_FDS = 9
};

class Serializer {
public:
  Serializer() {}
  virtual ~Serializer() {}

  virtual void writeByte(char c) = 0;
  virtual void writeBytes(const char* buf, size_t bufsize) = 0;
  virtual void writeUint16(uint16_t x) = 0;
  virtual void writeUint32(uint32_t x) = 0;
  virtual void writeUint64(uint64_t x) = 0;
  virtual void writeDouble(double d) = 0;

  // Insert padding bytes until the position is at the next multiple
  // of `alignment`. The alignment must be a power of 2.
  virtual void insertPadding(size_t alignment) = 0;

  // Number of bytes serialized so far.
  virtual size_t getPos() const = 0;

  virtual void recordArraySize(const std::function<uint32_t(uint32_t)>& f) = 0;
};

// Interface for pretty printing.
class Printer {
public:
  virtual ~Printer() {}

  virtual void printChar(char c) = 0;
  virtual void printUint8(uint8_t x) = 0;
  virtual void printInt8(int8_t x) = 0;
  virtual void printUint16(uint16_t x) = 0;
  virtual void printInt16(int16_t x) = 0;
  virtual void printUint32(uint32_t x) = 0;
  virtual void printInt32(int32_t x) = 0;
  virtual void printUint64(uint64_t x) = 0;
  virtual void printInt64(int64_t x) = 0;
  virtual void printDouble(double x) = 0;
  virtual void printString(const std::string& str) = 0;
  virtual void printNewline(size_t indent) = 0;
};

class DBusType;
class DBusTypeChar;
class DBusTypeBoolean;
class DBusTypeUint16;
class DBusTypeInt16;
class DBusTypeUint32;
class DBusTypeInt32;
class DBusTypeUint64;
class DBusTypeInt64;
class DBusTypeDouble;
class DBusTypeUnixFD;
class DBusTypeString;
class DBusTypePath;
class DBusTypeSignature;
class DBusTypeVariant;
class DBusTypeDictEntry;
class DBusTypeArray;
class DBusTypeStruct;

class DBusTypeStorage;

class DBusObject;
class DBusObjectChar;
class DBusObjectBoolean;
class DBusObjectUint16;
class DBusObjectInt16;
class DBusObjectUint32;
class DBusObjectInt32;
class DBusObjectUint64;
class DBusObjectInt64;
class DBusObjectDouble;
class DBusObjectUnixFD;
class DBusObjectString;
class DBusObjectPath;
class DBusObjectSignature;
class DBusObjectVariant;
class DBusObjectDictEntry;
class DBusObjectArray;
class DBusObjectStruct;

class DBusType {
public:
  // Visitor interface
  class Visitor {
  public:
    virtual void visitChar(const DBusTypeChar&) = 0;
    virtual void visitBoolean(const DBusTypeBoolean&) = 0;
    virtual void visitUint16(const DBusTypeUint16&) = 0;
    virtual void visitInt16(const DBusTypeInt16&) = 0;
    virtual void visitUint32(const DBusTypeUint32&) = 0;
    virtual void visitInt32(const DBusTypeInt32&) = 0;
    virtual void visitUint64(const DBusTypeUint64&) = 0;
    virtual void visitInt64(const DBusTypeInt64&) = 0;
    virtual void visitDouble(const DBusTypeDouble&) = 0;
    virtual void visitUnixFD(const DBusTypeUnixFD&) = 0;
    virtual void visitString(const DBusTypeString&) = 0;
    virtual void visitPath(const DBusTypePath&) = 0;
    virtual void visitSignature(const DBusTypeSignature&) = 0;
    virtual void visitVariant(const DBusTypeVariant&) = 0;
    virtual void visitDictEntry(const DBusTypeDictEntry&) = 0;
    virtual void visitArray(const DBusTypeArray&) = 0;
    virtual void visitStruct(const DBusTypeStruct&) = 0;
  };

  // Continuation for parsing a type.
  class ParseTypeCont {
  public:
    virtual ~ParseTypeCont() {}
    virtual std::unique_ptr<Parse::Cont> parse(
      DBusTypeStorage& typeStorage, // Type allocator
      const Parse::State& p,
      const DBusType& t
    ) = 0;
    virtual std::unique_ptr<Parse::Cont> parseCloseParen(
      DBusTypeStorage& typeStorage, // Type allocator
      const Parse::State& p
    ) = 0;
  };

  // Continuation for parsing an object of this type.
  template <Endianness endianness>
  class ParseObjectCont {
  public:
    virtual ~ParseObjectCont() {}
    virtual std::unique_ptr<Parse::Cont> parse(
      const Parse::State& p, std::unique_ptr<DBusObject>&& obj
    ) = 0;
  };

  virtual ~DBusType() {}

  // When D-Bus objects are serialized, they are aligned. For example
  // a UINT32 is 32-bit aligned and a STRUCT is 64-bit aligned. This
  // virtual method returns the alignment for the type. It corresponds
  // to the alignment column of this table:
  // https://dbus.freedesktop.org/doc/dbus-specification.html#idm694
  virtual size_t alignment() const = 0;

  virtual void serialize(Serializer& s) const = 0;

  virtual void print(Printer& p) const = 0;

  std::string toString() const;

  // Create a parser for this type. The parameter is a continuation
  // function, which will receive the `DBusObject` which was parsed.
  // This method uses the template method design pattern to delegate
  // most of the work to `mkObjectParserImpl` (below). But this
  // wrapper takes care of alignment.
  template <Endianness endianness>
  std::unique_ptr<Parse::Cont> mkObjectParser(
    const Parse::State& p,
    std::unique_ptr<ParseObjectCont<endianness>>&& cont
  ) const;

  virtual void accept(Visitor& visitor) const = 0;

protected:
  // Little endian parser.
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const = 0;

  // Big endian parser.
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const = 0;
};

class DBusTypeChar final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(char); }

  // DBusTypeChar is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeChar instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('y');
  }

  virtual void print(Printer& p) const override {
    p.printChar('y');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitChar(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeBoolean final : public DBusType {
public:
  // D-Bus Booleans are 32 bits.
  // https://dbus.freedesktop.org/doc/dbus-specification.html#idm694
  virtual size_t alignment() const override { return sizeof(uint32_t); }

  // DBusTypeBoolean is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeBoolean instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('b');
  }

  virtual void print(Printer& p) const override {
    p.printChar('b');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitBoolean(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeUint16 final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(uint16_t); }

  // DBusTypeUint16 is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeUint16 instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('q');
  }

  virtual void print(Printer& p) const override {
    p.printChar('q');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUint16(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeInt16 final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(int16_t); }

  // DBusTypeInt16 is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeInt16 instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('n');
  }

  virtual void print(Printer& p) const override {
    p.printChar('n');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitInt16(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeUint32 final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(uint32_t); }

  // DBusTypeUint32 is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeUint32 instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('u');
  }

  virtual void print(Printer& p) const override {
    p.printChar('u');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUint32(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeInt32 final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(int32_t); }

  // DBusTypeInt32 is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeInt32 instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('i');
  }

  virtual void print(Printer& p) const override {
    p.printChar('i');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitInt32(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeUint64 final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(uint64_t); }

  // DBusTypeUint64 is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeUint64 instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('t');
  }

  virtual void print(Printer& p) const override {
    p.printChar('t');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUint64(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeInt64 final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(int64_t); }

  // DBusTypeInt64 is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeInt64 instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('x');
  }

  virtual void print(Printer& p) const override {
    p.printChar('x');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitInt64(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeDouble final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(int32_t); }

  // DBusTypeDouble is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeDouble instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('d');
  }

  virtual void print(Printer& p) const override {
    p.printChar('d');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitDouble(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeUnixFD final : public DBusType {
public:
  virtual size_t alignment() const override { return sizeof(int32_t); }

  // DBusTypeUnixFD is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeUnixFD instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('h');
  }

  virtual void print(Printer& p) const override {
    p.printChar('h');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUnixFD(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeString final : public DBusType {
public:
  virtual size_t alignment() const override {
    return sizeof(uint32_t); // For the length
  }

  // DBusTypeString is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeString instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('s');
  }

  virtual void print(Printer& p) const override {
    p.printChar('s');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitString(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypePath final : public DBusType {
public:
  virtual size_t alignment() const override {
    return sizeof(uint32_t); // For the length
  }

  // DBusTypePath is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypePath instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('o');
  }

  virtual void print(Printer& p) const override {
    p.printChar('o');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitPath(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeSignature final : public DBusType {
public:
  virtual size_t alignment() const override {
    return sizeof(char); // The length of a signature fits in a char
  }

  // DBusTypeSignature is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeSignature instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('g');
  }

  virtual void print(Printer& p) const override {
    p.printChar('g');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitSignature(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeVariant final : public DBusType {
public:
  virtual size_t alignment() const override {
    // A serialized variant starts with a signature, which has a 1-byte
    // alignment.
    return sizeof(char);
  }

  // DBusTypeVariant is constant and doesn't have any parameters,
  // so this instance is available for anyone to use.
  static const DBusTypeVariant instance_;

  virtual void serialize(Serializer& s) const override {
    s.writeByte('v');
  }

  virtual void print(Printer& p) const override {
    p.printChar('v');
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitVariant(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const override;
};

class DBusTypeDictEntry : public DBusType {
  // Reference to the key type which is not owned by this class.
  const DBusType& keyType_;

  // Reference to the value type which is not owned by this class.
  const DBusType& valueType_;

public:
  // We keep references to `keyType` and `valueType`, but do not take
  // ownership of them.
  DBusTypeDictEntry(const DBusType& keyType, const DBusType& valueType) :
    keyType_(keyType), valueType_(valueType)
  {}

  const DBusType& getKeyType() const { return keyType_; }
  const DBusType& getValueType() const { return valueType_; }

  virtual size_t alignment() const final override {
    return sizeof(uint64_t); // Same as DBusTypeStruct
  }

  virtual void serialize(Serializer& s) const final override {
    s.writeByte('{');
    keyType_.serialize(s);
    valueType_.serialize(s);
    s.writeByte('}');
  }

  virtual void print(Printer& p) const override {
    p.printChar('{');
    keyType_.print(p);
    valueType_.print(p);
    p.printChar('}');
  }

  virtual void accept(Visitor& visitor) const final override {
    visitor.visitDictEntry(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const final override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const final override;
};

class DBusTypeArray : public DBusType {
  // Reference to the base type which is not owned by this class.
  const DBusType& baseType_;

public:
  // We keep a reference to the baseType, but do not take ownership of it.
  explicit DBusTypeArray(const DBusType& baseType) : baseType_(baseType) {}

  const DBusType& getBaseType() const { return baseType_; }

  virtual size_t alignment() const final override {
    return sizeof(uint32_t); // For the length
  }

  virtual void serialize(Serializer& s) const final override {
    s.writeByte('a');
    baseType_.serialize(s);
  }

  virtual void print(Printer& p) const override {
    p.printChar('a');
    baseType_.print(p);
  }

  virtual void accept(Visitor& visitor) const final override {
    visitor.visitArray(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const final override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const final override;
};

class DBusTypeStruct : public DBusType {
  // The vector of field types is owned by this class, but it contains
  // references to types which we do not own.
  const std::vector<std::reference_wrapper<const DBusType>> fieldTypes_;

public:
  // We take ownership of the vector, but not the field types which it
  // references.
  explicit DBusTypeStruct(
    std::vector<std::reference_wrapper<const DBusType>>&& fieldTypes
  ) :
    fieldTypes_(std::move(fieldTypes))
  {}

  const std::vector<std::reference_wrapper<const DBusType>>&
  getFieldTypes() const {
    return fieldTypes_;
  }

  virtual size_t alignment() const final override {
    return sizeof(uint64_t);
  }

  virtual void serialize(Serializer& s) const final override {
    s.writeByte('(');
    for (const DBusType& i: fieldTypes_) {
      i.serialize(s);
    }
    s.writeByte(')');
  }

  virtual void print(Printer& p) const override {
    p.printChar('(');
    for (const DBusType& i: fieldTypes_) {
      i.print(p);
    }
    p.printChar(')');
  }

  virtual void accept(Visitor& visitor) const final override {
    visitor.visitStruct(*this);
  }

protected:
  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<LittleEndian>>&& cont
  ) const final override;

  virtual std::unique_ptr<Parse::Cont> mkObjectParserImpl(
    const Parse::State& p, std::unique_ptr<ParseObjectCont<BigEndian>>&& cont
  ) const final override;
};

// `DBusType` uses references to refer to sub-types. This is because the
// type is usually embedded in a `DBusObject`, so there is no need to store
// it separately. But there are two occasions where we need to store types
// separately:
//
//   1. During parsing the signature of a `DBusObjectVariant`.
//   2. The element type of a `DBusObjectArray` with zero elements.
//
// Leaf types like `DBusTypeChar` do not need to be allocated because they
// have a global constant instance. So we only need to allocate memory for
// array and struct types, which are the only non-leaf types. This class
// allocates and stores objects of type `DBusTypeArray` and
// `DBusTypeStruct`. It is unlikely to be used much, so it just uses a pair
// of simply linked lists.
class DBusTypeStorage final {
  class ArrayLink final : public DBusTypeArray {
    const std::unique_ptr<ArrayLink> next_;

  public:
    ArrayLink(
      const DBusType& baseType,
      std::unique_ptr<ArrayLink>&& next
    ) :
      DBusTypeArray(baseType),
      next_(std::move(next))
    {}
  };

  class DictEntryLink final : public DBusTypeDictEntry {
    const std::unique_ptr<DictEntryLink> next_;

  public:
    DictEntryLink(
      const DBusType& keyType,
      const DBusType& valueType,
      std::unique_ptr<DictEntryLink>&& next
    ) :
      DBusTypeDictEntry(keyType, valueType),
      next_(std::move(next))
    {}
  };

  class StructLink final : public DBusTypeStruct {
    const std::unique_ptr<StructLink> next_;

  public:
    StructLink(
      std::vector<std::reference_wrapper<const DBusType>>&& fieldTypes,
      std::unique_ptr<StructLink>&& next
    ) :
      DBusTypeStruct(std::move(fieldTypes)),
      next_(std::move(next))
    {}
  };

  std::unique_ptr<ArrayLink> arrays_;
  std::unique_ptr<DictEntryLink> dict_entries_;
  std::unique_ptr<StructLink> structs_;

public:
  DBusTypeStorage() {}

  const DBusTypeArray& allocArray(const DBusType& baseType) {
    arrays_ = std::make_unique<ArrayLink>(baseType, std::move(arrays_));
    return *arrays_;
  }

  const DBusTypeDictEntry& allocDictEntry(
    const DBusType& keyType, const DBusType& valueType
  ) {
    dict_entries_ =
      std::make_unique<DictEntryLink>(
        keyType, valueType, std::move(dict_entries_)
      );
    return *dict_entries_;
  }

  const DBusTypeStruct& allocStruct(
    std::vector<std::reference_wrapper<const DBusType>>&& fieldTypes
  ) {
    structs_ = std::make_unique<StructLink>(
      std::move(fieldTypes), std::move(structs_)
    );
    return *structs_;
  }
};

class ObjectCastError : public Error {
public:
  explicit ObjectCastError(const char* name) :
    Error(std::string("ObjectCastError:" + std::string(name)))
  {}
};

class DBusObject {
public:
  // Visitor interface
  class Visitor {
  public:
    virtual void visitChar(const DBusObjectChar&) = 0;
    virtual void visitBoolean(const DBusObjectBoolean&) = 0;
    virtual void visitUint16(const DBusObjectUint16&) = 0;
    virtual void visitInt16(const DBusObjectInt16&) = 0;
    virtual void visitUint32(const DBusObjectUint32&) = 0;
    virtual void visitInt32(const DBusObjectInt32&) = 0;
    virtual void visitUint64(const DBusObjectUint64&) = 0;
    virtual void visitInt64(const DBusObjectInt64&) = 0;
    virtual void visitDouble(const DBusObjectDouble&) = 0;
    virtual void visitUnixFD(const DBusObjectUnixFD&) = 0;
    virtual void visitString(const DBusObjectString&) = 0;
    virtual void visitPath(const DBusObjectPath&) = 0;
    virtual void visitSignature(const DBusObjectSignature&) = 0;
    virtual void visitVariant(const DBusObjectVariant&) = 0;
    virtual void visitDictEntry(const DBusObjectDictEntry&) = 0;
    virtual void visitArray(const DBusObjectArray&) = 0;
    virtual void visitStruct(const DBusObjectStruct&) = 0;
  };

  DBusObject() {}
  virtual ~DBusObject() = default;

  virtual const DBusType& getType() const = 0;

  // Always call serializePadding before calling this method.
  virtual void serializeAfterPadding(Serializer& s) const = 0;

  virtual void print(Printer& p, size_t indent) const = 0;

  virtual void accept(Visitor& visitor) const = 0;

  virtual const DBusObjectChar& toChar() const {
    throw ObjectCastError("Char");
  }
  virtual const DBusObjectBoolean& toBoolean() const {
    throw ObjectCastError("Boolean");
  }
  virtual const DBusObjectUint16& toUint16() const {
    throw ObjectCastError("Uint16");
  }
  virtual const DBusObjectInt16& toInt16() const {
    throw ObjectCastError("Int16");
  }
  virtual const DBusObjectUint32& toUint32() const {
    throw ObjectCastError("Uint32");
  }
  virtual const DBusObjectInt32& toInt32() const {
    throw ObjectCastError("Int32");
  }
  virtual const DBusObjectUint64& toUint64() const {
    throw ObjectCastError("Uint64");
  }
  virtual const DBusObjectInt64& toInt64() const {
    throw ObjectCastError("Int64");
  }
  virtual const DBusObjectDouble& toDouble() const {
    throw ObjectCastError("Double");
  }
  virtual const DBusObjectUnixFD& toUnixFD() const {
    throw ObjectCastError("UnixFD");
  }
  virtual const DBusObjectString& toString() const {
    throw ObjectCastError("String");
  }
  virtual const DBusObjectPath& toPath() const {
    throw ObjectCastError("Path");
  }
  virtual const DBusObjectSignature& toSignature() const {
    throw ObjectCastError("Signature");
  }
  virtual const DBusObjectVariant& toVariant() const {
    throw ObjectCastError("Variant");
  }
  virtual const DBusObjectDictEntry& toDictEntry() const {
    throw ObjectCastError("DictEntry");
  }
  virtual const DBusObjectArray& toArray() const {
    throw ObjectCastError("Array");
  }
  virtual const DBusObjectStruct& toStruct() const {
    throw ObjectCastError("Struct");
  }

  void print(Printer& p) const {
    print(p, 0);
    p.printNewline(0);
  }

  void serialize(Serializer& s) const {
    s.insertPadding(getType().alignment());
    serializeAfterPadding(s);
  }

  size_t serializedSize() const;
};

class DBusObjectChar final : public DBusObject {
  const char c_;

public:
  explicit DBusObjectChar(char c);

  static std::unique_ptr<DBusObjectChar> mk(char c) {
    return std::make_unique<DBusObjectChar>(c);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeChar::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeByte(c_);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitChar(*this);
  }

  const DBusObjectChar& toChar() const override { return *this; }

  char getValue() const { return c_; }
};

class DBusObjectBoolean final : public DBusObject {
  const bool b_;

public:
  explicit DBusObjectBoolean(bool b);

  static std::unique_ptr<DBusObjectBoolean> mk(bool b) {
    return std::make_unique<DBusObjectBoolean>(b);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeBoolean::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    // D-Bus Booleans are 32 bits.
    // https://dbus.freedesktop.org/doc/dbus-specification.html#idm694
    s.writeUint32(static_cast<uint32_t>(b_));
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitBoolean(*this);
  }

  const DBusObjectBoolean& toBoolean() const override { return *this; }

  bool getValue() const { return b_; }
};

class DBusObjectUint16 final : public DBusObject {
  const uint16_t x_;

public:
  explicit DBusObjectUint16(uint16_t x);

  static std::unique_ptr<DBusObjectUint16> mk(uint16_t x) {
    return std::make_unique<DBusObjectUint16>(x);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeUint16::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint16(x_);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUint16(*this);
  }

  const DBusObjectUint16& toUint16() const override { return *this; }

  uint16_t getValue() const { return x_; }
};

class DBusObjectInt16 final : public DBusObject {
  const int16_t x_;

public:
  explicit DBusObjectInt16(int16_t x);

  static std::unique_ptr<DBusObjectInt16> mk(int16_t x) {
    return std::make_unique<DBusObjectInt16>(x);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeInt16::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint16(static_cast<uint16_t>(x_));
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitInt16(*this);
  }

  const DBusObjectInt16& toInt16() const override { return *this; }

  int16_t getValue() const { return x_; }
};

class DBusObjectUint32 final : public DBusObject {
  const uint32_t x_;

public:
  explicit DBusObjectUint32(uint32_t x);

  static std::unique_ptr<DBusObjectUint32> mk(uint32_t x) {
    return std::make_unique<DBusObjectUint32>(x);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeUint32::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint32(x_);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUint32(*this);
  }

  const DBusObjectUint32& toUint32() const override { return *this; }

  uint32_t getValue() const { return x_; }
};

class DBusObjectInt32 final : public DBusObject {
  const int32_t x_;

public:
  explicit DBusObjectInt32(int32_t x);

  static std::unique_ptr<DBusObjectInt32> mk(int32_t x) {
    return std::make_unique<DBusObjectInt32>(x);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeInt32::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint32(static_cast<uint32_t>(x_));
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitInt32(*this);
  }

  const DBusObjectInt32& toInt32() const override { return *this; }

  int32_t getValue() const { return x_; }
};

class DBusObjectUint64 final : public DBusObject {
  const uint64_t x_;

public:
  explicit DBusObjectUint64(uint64_t x);

  static std::unique_ptr<DBusObjectUint64> mk(uint64_t x) {
    return std::make_unique<DBusObjectUint64>(x);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeUint64::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint64(x_);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUint64(*this);
  }

  const DBusObjectUint64& toUint64() const override { return *this; }

  uint64_t getValue() const { return x_; }
};

class DBusObjectInt64 final : public DBusObject {
  const int64_t x_;

public:
  explicit DBusObjectInt64(int64_t x);

  static std::unique_ptr<DBusObjectInt64> mk(int64_t x) {
    return std::make_unique<DBusObjectInt64>(x);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeInt64::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint64(static_cast<uint64_t>(x_));
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitInt64(*this);
  }

  const DBusObjectInt64& toInt64() const override { return *this; }

  int64_t getValue() const { return x_; }
};

class DBusObjectDouble final : public DBusObject {
  const double d_;

public:
  explicit DBusObjectDouble(double d);

  static std::unique_ptr<DBusObjectDouble> mk(double d) {
    return std::make_unique<DBusObjectDouble>(d);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeDouble::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeDouble(d_);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitDouble(*this);
  }

  const DBusObjectDouble& toDouble() const override { return *this; }

  double getValue() const { return d_; }
};

class DBusObjectUnixFD final : public DBusObject {
  // Note: `i_` is not a file descriptor. It is an index into
  // an array of file descriptors, which is passed out-of-band.
  const uint32_t i_;

public:
  explicit DBusObjectUnixFD(uint32_t i);

  static std::unique_ptr<DBusObjectUnixFD> mk(uint32_t i) {
    return std::make_unique<DBusObjectUnixFD>(i);
  }

  virtual const DBusType& getType() const override {
    return DBusTypeUnixFD::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    s.writeUint32(i_);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitUnixFD(*this);
  }

  const DBusObjectUnixFD& toUnixFD() const override { return *this; }

  uint32_t getValue() const { return i_; }
};

class DBusObjectString final : public DBusObject {
  const std::string str_;

public:
  explicit DBusObjectString(std::string&& str);

  static std::unique_ptr<DBusObjectString> mk(std::string&& str) {
    return std::make_unique<DBusObjectString>(std::move(str));
  }

  virtual const DBusType& getType() const override {
    return DBusTypeString::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    uint32_t len = str_.size();
    s.writeUint32(len);
    s.writeBytes(str_.c_str(), len+1);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitString(*this);
  }

  const DBusObjectString& toString() const override { return *this; }

  const std::string& getValue() const { return str_; }
};

class DBusObjectPath final : public DBusObject {
  const std::string str_;

public:
  explicit DBusObjectPath(std::string&& str);

  static std::unique_ptr<DBusObjectPath> mk(std::string&& str) {
    return std::make_unique<DBusObjectPath>(std::move(str));
  }

  virtual const DBusType& getType() const override {
    return DBusTypePath::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    uint32_t len = str_.size();
    s.writeUint32(len);
    s.writeBytes(str_.c_str(), len+1);
  }

  virtual void print(Printer& p, size_t) const override {
    p.printString(str_);
  }

  virtual void accept(Visitor& visitor) const override {
    visitor.visitPath(*this);
  }

  const DBusObjectPath& toPath() const override { return *this; }

  const std::string& getValue() const { return str_; }
};

// Almost identical to DBusObjectString and DBusObjectPath, except that a
// single byte is used to serialize the length. (The maximum length of a
// signature is 255.)
class DBusObjectSignature final : public DBusObject {
  const std::string str_;

public:
  explicit DBusObjectSignature(std::string&& str);

  static std::unique_ptr<DBusObjectSignature> mk(std::string&& str) {
    return std::make_unique<DBusObjectSignature>(std::move(str));
  }

  virtual const DBusType& getType() const override {
    return DBusTypeSignature::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    uint8_t len = str_.size();
    s.writeByte(len);
    s.writeBytes(str_.c_str(), len+1);
  }

  virtual void print(Printer& p, size_t) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitSignature(*this);
  }

  const DBusObjectSignature& toSignature() const override { return *this; }

  const std::string& getValue() const { return str_; }

  // Parse the sequence of types from the signature string. You need to
  // supply a `DBusTypeStorage` so that the parser can allocate new
  // types. The return value of the function contains references (into the
  // `DBusTypeStorage` object), so you need to make sure that the storage
  // doesn't get deallocated until you are finished with the types.
  std::vector<std::reference_wrapper<const DBusType>> toTypes(
    DBusTypeStorage& typeStorage // Type allocator
  ) const;
};

class DBusObjectVariant final : public DBusObject {
  const std::unique_ptr<DBusObject> object_;
  const DBusObjectSignature signature_;

public:
  explicit DBusObjectVariant(std::unique_ptr<DBusObject>&& object);

  static std::unique_ptr<DBusObjectVariant> mk(
    std::unique_ptr<DBusObject>&& object
  ) {
    return std::make_unique<DBusObjectVariant>(std::move(object));
  }

  virtual const DBusType& getType() const override {
    return DBusTypeVariant::instance_;
  }

  virtual void serializeAfterPadding(Serializer& s) const override {
    signature_.serialize(s);
    object_->serialize(s);
  }

  virtual void print(Printer& p, size_t indent) const override;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitVariant(*this);
  }

  const DBusObjectVariant& toVariant() const override { return *this; }

  const std::unique_ptr<DBusObject>& getValue() const { return object_; }
};

class DBusObjectDictEntry : public DBusObject {
  const std::unique_ptr<DBusObject> key_;
  const std::unique_ptr<DBusObject> value_;
  const DBusTypeDictEntry dictEntryType_;

public:
  DBusObjectDictEntry(
    std::unique_ptr<DBusObject>&& key,
    std::unique_ptr<DBusObject>&& value
  );

  static std::unique_ptr<DBusObjectDictEntry> mk(
    std::unique_ptr<DBusObject>&& key,
    std::unique_ptr<DBusObject>&& value
  ) {
    return std::make_unique<DBusObjectDictEntry>(
      std::move(key), std::move(value)
    );
  }

  virtual const DBusType& getType() const final override {
    return dictEntryType_;
  }

  virtual void serializeAfterPadding(Serializer& s) const final override {
    key_->serialize(s);
    value_->serialize(s);
  }

  virtual void print(Printer& p, size_t indent) const override final;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitDictEntry(*this);
  }

  const DBusObjectDictEntry& toDictEntry() const override { return *this; }

  const std::unique_ptr<DBusObject>& getKey() const { return key_; }
  const std::unique_ptr<DBusObject>& getValue() const { return value_; }
};

class DBusObjectSeq final {
  const std::vector<std::unique_ptr<DBusObject>> elements_;

public:
  explicit DBusObjectSeq(std::vector<std::unique_ptr<DBusObject>>&& elements);

  size_t length() const { return elements_.size(); }

  std::vector<std::reference_wrapper<const DBusType>> elementTypes() const {
    std::vector<std::reference_wrapper<const DBusType>> types;
    types.reserve(elements_.size());
    for (auto& element : elements_) {
      types.push_back(std::cref(element->getType()));
    }
    return types;
  }

  void print(Printer& p, size_t indent, char lbracket, char rbracket) const;

  void serialize(Serializer& s) const {
    for (auto& p: elements_) {
      p->serialize(s);
    }
  }

  const std::unique_ptr<DBusObject>& getElement(size_t i) const {
    return elements_.at(i);
  }
};

class DBusObjectArray : public DBusObject {
  const DBusObjectSeq seq_;

  // Note: this type contains a reference to the base type of the array
  // type. It doesn't own the base type, so we need to make sure that it
  // cannot become a dangling pointer. That's easy when the array has
  // at least one element because we can just make it a reference to the
  // type of element zero. But if there are zero elements, then we need
  // to make sure that we own the base type. This problem is solved by
  // DBusObjectArray0, which owns any struct or array types that are used
  // in the base type.
  const DBusTypeArray arrayType_;

public:
  // DBusObjectArray keeps a reference to baseType, so the
  // lifetime of baseType must exceed that of the DBusObjectArray.
  DBusObjectArray(
    const DBusType& baseType,
    std::vector<std::unique_ptr<DBusObject>>&& elements
  );

  // Constructing an array with zero elements needs to be handled as
  // a special case to avoid the `arrayType_` field containing a dangling
  // pointer. See the comment on `arrayType_`.
  static std::unique_ptr<DBusObjectArray> mk0(const DBusType& baseType);

  // If the number of elements is non-zero, then we can deduce the base type
  // from the zero'th element.
  static std::unique_ptr<DBusObjectArray> mk1(
    std::vector<std::unique_ptr<DBusObject>>&& elements
  ) {
    return std::make_unique<DBusObjectArray>(
      elements.at(0)->getType(), std::move(elements)
    );
  }

  static std::unique_ptr<DBusObjectArray> mk(
    const DBusType& baseType,
    std::vector<std::unique_ptr<DBusObject>>&& elements
  ) {
    if (elements.size() == 0) {
      return mk0(baseType);
    } else {
      return mk1(std::move(elements));
    }
  }

  virtual const DBusType& getType() const final override { return arrayType_; }

  virtual void serializeAfterPadding(Serializer& s) const final override {
    s.recordArraySize(
      [this, &s](uint32_t arraySize) {
        s.writeUint32(arraySize);
        s.insertPadding(arrayType_.getBaseType().alignment());
        const size_t posBefore = s.getPos();
        seq_.serialize(s);
        const size_t posAfter = s.getPos();
        return posAfter - posBefore;
      }
    );
  }

  virtual void print(Printer& p, size_t indent) const override final;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitArray(*this);
  }

  const DBusObjectArray& toArray() const override { return *this; }

  size_t numElements() const { return seq_.length(); }

  const std::unique_ptr<DBusObject>& getElement(size_t i) const {
    return seq_.getElement(i);
  }
};

// An array with zero elements.
class DBusObjectArray0 final : public DBusObjectArray {
  DBusTypeStorage typeStorage_;

public:
  DBusObjectArray0(
    const DBusType& baseType,
    std::vector<std::unique_ptr<DBusObject>>&& elements,
    DBusTypeStorage&& typeStorage
  );
};

class DBusObjectStruct : public DBusObject {
  const DBusObjectSeq seq_;
  const DBusTypeStruct structType_;

public:
  explicit DBusObjectStruct(std::vector<std::unique_ptr<DBusObject>>&& elements);

  static std::unique_ptr<DBusObjectStruct> mk(
    std::vector<std::unique_ptr<DBusObject>>&& elements
  ) {
    return std::make_unique<DBusObjectStruct>(std::move(elements));
  }

  virtual const DBusType& getType() const final override { return structType_; }

  virtual void serializeAfterPadding(Serializer& s) const final override {
    seq_.serialize(s);
  }

  virtual void print(Printer& p, size_t indent) const override final;

  virtual void accept(Visitor& visitor) const override {
    visitor.visitStruct(*this);
  }

  const DBusObjectStruct& toStruct() const override { return *this; }

  size_t numFields() const { return seq_.length(); }

  const std::unique_ptr<DBusObject>& getElement(size_t i) const {
    return seq_.getElement(i);
  }
};

// Utility for constructing the message header.
class DBusHeaderField final : public DBusObjectStruct {
public:
  DBusHeaderField(HeaderFieldName name, std::unique_ptr<DBusObjectVariant>&& v);

  static std::unique_ptr<DBusHeaderField> mk(
    HeaderFieldName name, std::unique_ptr<DBusObjectVariant>&& v
  ) {
    return std::make_unique<DBusHeaderField>(name, std::move(v));
  }
};

class DBusMessageBody {
  const DBusObjectSeq seq_;

public:
  explicit DBusMessageBody(std::vector<std::unique_ptr<DBusObject>>&& elements);

  // Create an empty message body.
  static std::unique_ptr<DBusMessageBody> mk0();

  // Create a message body with 1 element.
  static std::unique_ptr<DBusMessageBody> mk1(
    std::unique_ptr<DBusObject>&& element
  );

  // Create a message body with multiple elements.
  static std::unique_ptr<DBusMessageBody> mk(
    std::vector<std::unique_ptr<DBusObject>>&& elements
  );

  std::string signature() const;

  void serialize(Serializer& s) const;

  size_t serializedSize() const;

  void print(Printer& p, size_t indent) const;

  size_t numElements() const { return seq_.length(); }

  const std::unique_ptr<DBusObject>& getElement(size_t i) const {
    return seq_.getElement(i);
  }
};

class DBusMessage {
  std::unique_ptr<DBusObject> header_;
  std::unique_ptr<DBusMessageBody> body_;

public:
  DBusMessage(
    std::unique_ptr<DBusObject>&& header,
    std::unique_ptr<DBusMessageBody>&& body
  ) :
    header_(std::move(header)), body_(std::move(body))
  {}

  const DBusObjectStruct& getHeader() const {
    return header_->toStruct();
  }

  const DBusMessageBody& getBody() const {
    return *body_;
  }

  // Read the endianness value in the header.
  char getHeader_endianness() const {
    return getHeader().getElement(0)->toChar().getValue();
  }

  // Read the message type in the header.
  MessageType getHeader_messageType() const {
    return static_cast<MessageType>(
      getHeader().getElement(1)->toChar().getValue()
    );
  }

  // Read the message flags in the header.
  MessageFlags getHeader_messageFlags() const {
    return static_cast<MessageFlags>(
      getHeader().getElement(2)->toChar().getValue()
    );
  }

  // Read the major protocol version in the header.
  uint8_t getHeader_protocolVersion() const {
    return static_cast<uint8_t>(
      getHeader().getElement(3)->toChar().getValue()
    );
  }

  // Read the body size value in the header.
  uint32_t getHeader_bodySize() const {
    return getHeader().getElement(4)->toUint32().getValue();
  }

  // Read the reply serial number in the header.
  uint32_t getHeader_replySerial() const {
    return getHeader().getElement(5)->toUint32().getValue();
  }

  const DBusObjectVariant& getHeader_lookupField(HeaderFieldName name) const {
    const DBusObjectArray& fields = getHeader().getElement(6)->toArray();
    const size_t n = fields.numElements();
    for (size_t i = 0; i < n; i++) {
      const DBusObjectStruct& field = fields.getElement(i)->toStruct();
      if (field.getElement(0)->toChar().getValue() == name) {
        return field.getElement(1)->toVariant();
      }
    }
    throw ObjectCastError("DBusMessage::getHeader_lookupField");
  }

  // Parse a `DBusMessage`. On success the message is assigned
  // to `result`.
  template <Endianness endianness>
  static std::unique_ptr<Parse::Cont> parse(
    std::unique_ptr<DBusMessage>& result
  );

  // Shorthand for `parse<LittleEndian>`.
  static std::unique_ptr<Parse::Cont> parseLE(
    std::unique_ptr<DBusMessage>& result
  );

  // Shorthand for `parse<BigEndian>`.
  static std::unique_ptr<Parse::Cont> parseBE(
    std::unique_ptr<DBusMessage>& result
  );

  void serialize(Serializer& s) const;

  void print(Printer& p, size_t indent) const;
};

// The type of the header of a DBus message.
extern const DBusTypeStruct headerType;

// Utility for downcasting to std::unique_ptr<DBusObject>.
inline std::unique_ptr<DBusObject> _obj(std::unique_ptr<DBusObject>&& o) {
  return std::unique_ptr<DBusObject>(std::move(o));
}

// Make a deep copy of the type. The leaf types, like `DBusTypeChar` do not
// need to be allocated because they have a global constant instance. But
// we need to allocate memory for arrays and structs. This is done by adding
// elements to the `arrays` or `structs` vectors.
const DBusType& cloneType(
  DBusTypeStorage& typeStorage, // Type allocator
  const DBusType& t
);
