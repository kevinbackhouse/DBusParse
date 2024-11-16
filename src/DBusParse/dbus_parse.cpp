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

static std::unique_ptr<Parse::Cont>
parseType(DBusTypeStorage &typeStorage, // Type allocator
          std::unique_ptr<DBusType::ParseTypeCont> &&cont) {
  class ContDictCloseBrace final : public ParseChar::Cont {
    DBusTypeStorage &typeStorage_; // Not owned
    const DBusType &keyType_;
    const DBusType &valueType_;
    const std::unique_ptr<DBusType::ParseTypeCont> cont_;

  public:
    ContDictCloseBrace(DBusTypeStorage &typeStorage, const DBusType &keyType,
                       const DBusType &valueType,
                       std::unique_ptr<DBusType::ParseTypeCont> &&cont)
        : typeStorage_(typeStorage), keyType_(keyType), valueType_(valueType),
          cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               char c) override {
      if (c != '}') {
        throw ParseError(p.getPos(), "Expected a '}' character.");
      }
      return cont_->parse(typeStorage_, p,
                          typeStorage_.allocDictEntry(keyType_, valueType_));
    }
  };

  class ContDictValue final : public DBusType::ParseTypeCont {
    const DBusType &keyType_;
    std::unique_ptr<DBusType::ParseTypeCont> cont_;

  public:
    ContDictValue(const DBusType &keyType,
                  std::unique_ptr<DBusType::ParseTypeCont> &&cont)
        : keyType_(keyType), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(DBusTypeStorage &typeStorage, // Type allocator
          const Parse::State &, const DBusType &valueType) override {
      return ParseChar::mk(std::make_unique<ContDictCloseBrace>(
          typeStorage, keyType_, valueType, std::move(cont_)));
    }

    virtual std::unique_ptr<Parse::Cont>
    parseCloseParen(DBusTypeStorage &, // Type allocator
                    const Parse::State &p) override {
      throw ParseError(p.getPos(),
                       "Unexpected close paren while parsing dict entry type.");
    }
  };

  class ContDictKey final : public DBusType::ParseTypeCont {
    std::unique_ptr<DBusType::ParseTypeCont> cont_;

  public:
    ContDictKey(std::unique_ptr<DBusType::ParseTypeCont> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(DBusTypeStorage &typeStorage, // Type allocator
          const Parse::State &, const DBusType &keyType) override {
      return parseType(typeStorage, std::make_unique<ContDictValue>(
                                        keyType, std::move(cont_)));
    }

    virtual std::unique_ptr<Parse::Cont>
    parseCloseParen(DBusTypeStorage &, // Type allocator
                    const Parse::State &p) override {
      throw ParseError(p.getPos(),
                       "Unexpected close paren while parsing dict entry type.");
    }
  };

  class ContStruct final : public DBusType::ParseTypeCont {
    std::vector<std::reference_wrapper<const DBusType>> fieldTypes_;
    std::unique_ptr<DBusType::ParseTypeCont> cont_;

  public:
    ContStruct(std::vector<std::reference_wrapper<const DBusType>> &&fieldTypes,
               std::unique_ptr<DBusType::ParseTypeCont> &&cont)
        : fieldTypes_(std::move(fieldTypes)), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(DBusTypeStorage &typeStorage, // Type allocator
          const Parse::State &, const DBusType &t) override {
      // Add the type to the vector of field types and continuing parsing
      // field types.
      fieldTypes_.push_back(t);
      return parseType(typeStorage,
                       std::make_unique<ContStruct>(std::move(fieldTypes_),
                                                    std::move(cont_)));
    }

    virtual std::unique_ptr<Parse::Cont>
    parseCloseParen(DBusTypeStorage &typeStorage, // Type allocator
                    const Parse::State &p) override {
      // Allocate a `DBusTypeStruct` by adding it to `structStorage`.
      return cont_->parse(typeStorage, p,
                          typeStorage.allocStruct(std::move(fieldTypes_)));
    }
  };

  class ContArray final : public DBusType::ParseTypeCont {
    const std::unique_ptr<DBusType::ParseTypeCont> cont_;

  public:
    ContArray(std::unique_ptr<DBusType::ParseTypeCont> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(DBusTypeStorage &typeStorage, // Type allocator
          const Parse::State &p, const DBusType &t) override {
      // Allocate a `DBusTypeArray` by adding it to `typeStorage`.
      return cont_->parse(typeStorage, p, typeStorage.allocArray(t));
    }

    virtual std::unique_ptr<Parse::Cont>
    parseCloseParen(DBusTypeStorage &, // Type allocator
                    const Parse::State &p) override {
      throw ParseError(p.getPos(),
                       "Unexpected close paren while parsing array type.");
    }
  };

  class Cont final : public ParseChar::Cont {
    DBusTypeStorage &typeStorage_; // Not owned
    std::unique_ptr<DBusType::ParseTypeCont> cont_;

  public:
    Cont(DBusTypeStorage &typeStorage, // Type allocator
         std::unique_ptr<DBusType::ParseTypeCont> &&cont)
        : typeStorage_(typeStorage), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               char c) override {
      switch (c) {
      case 'y':
        return cont_->parse(typeStorage_, p, DBusTypeChar::instance_);
      case 'b':
        return cont_->parse(typeStorage_, p, DBusTypeBoolean::instance_);
      case 'q':
        return cont_->parse(typeStorage_, p, DBusTypeUint16::instance_);
      case 'n':
        return cont_->parse(typeStorage_, p, DBusTypeInt16::instance_);
      case 'u':
        return cont_->parse(typeStorage_, p, DBusTypeUint32::instance_);
      case 'i':
        return cont_->parse(typeStorage_, p, DBusTypeInt32::instance_);
      case 't':
        return cont_->parse(typeStorage_, p, DBusTypeUint64::instance_);
      case 'x':
        return cont_->parse(typeStorage_, p, DBusTypeInt64::instance_);
      case 'd':
        return cont_->parse(typeStorage_, p, DBusTypeDouble::instance_);
      case 'h':
        return cont_->parse(typeStorage_, p, DBusTypeUnixFD::instance_);
      case 's':
        return cont_->parse(typeStorage_, p, DBusTypeString::instance_);
      case 'o':
        return cont_->parse(typeStorage_, p, DBusTypePath::instance_);
      case 'g':
        return cont_->parse(typeStorage_, p, DBusTypeSignature::instance_);
      case 'v':
        return cont_->parse(typeStorage_, p, DBusTypeVariant::instance_);
      case 'a':
        return parseType(typeStorage_,
                         std::make_unique<ContArray>(std::move(cont_)));
      case '(':
        return parseType(
            typeStorage_,
            std::make_unique<ContStruct>(
                std::vector<std::reference_wrapper<const DBusType>>(),
                std::move(cont_)));
      case ')':
        return cont_->parseCloseParen(typeStorage_, p);
      case '{':
        return parseType(typeStorage_,
                         std::make_unique<ContDictKey>(std::move(cont_)));

      default:
        throw ParseError(p.getPos(), _s("Invalid type character: ") +
                                         std::to_string(int(c)));
      }
    }
  };

  return ParseChar::mk(std::make_unique<Cont>(typeStorage, std::move(cont)));
}

// Utility for parsing the correct number of alignment bytes.
static std::unique_ptr<Parse::Cont>
parse_alignment(const Parse::State &p, const DBusType &t,
                std::unique_ptr<ParseZeros::Cont> &&cont) {
  const size_t pos = p.getPos();
  const size_t padding = (t.alignment() - 1) & -pos;
  return ParseZeros::mk(p, padding, std::move(cont));
}

template <Endianness endianness>
std::unique_ptr<Parse::Cont> DBusType::mkObjectParser(
    const Parse::State &p,
    std::unique_ptr<ParseObjectCont<endianness>> &&cont) const {
  // Continuation for parsing padding bytes.
  class PaddingCont final : public ParseZeros::Cont {
    // Reference to the current type.
    const DBusType &t_;

    std::unique_ptr<ParseObjectCont<endianness>> cont_;

  public:
    PaddingCont(const DBusType &t,
                std::unique_ptr<ParseObjectCont<endianness>> &&cont)
        : t_(t), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) override {
      return t_.mkObjectParserImpl(p, std::move(cont_));
    }
  };

  return parse_alignment(p, *this,
                         std::make_unique<PaddingCont>(*this, std::move(cont)));
}

// Template instantiation. This is to make sure that the little-endian
// instantiation of mkObjectParser is included in the library.
template std::unique_ptr<Parse::Cont> DBusType::mkObjectParser(
    const Parse::State &p,
    std::unique_ptr<ParseObjectCont<LittleEndian>> &&cont) const;

// Template instantiation. This is to make sure that the big-endian
// instantiation of mkObjectParser is included in the library.
template std::unique_ptr<Parse::Cont> DBusType::mkObjectParser(
    const Parse::State &p,
    std::unique_ptr<ParseObjectCont<BigEndian>> &&cont) const;

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeChar_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseChar::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               char c) override {
      return cont_->parse(p, DBusObjectChar::mk(c));
    }
  };

  return ParseChar::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeChar::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeChar_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeChar::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeChar_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeBoolean_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint32<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t b) override {
      // The value of x must be either 0 or 1.
      if (b > 1) {
        throw ParseError(p.getPos(), "Boolean value that is not 0 or 1.");
      }
      return cont_->parse(p, DBusObjectBoolean::mk(b));
    }
  };

  return ParseUint32<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeBoolean::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeBoolean_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeBoolean::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeBoolean_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeUint16_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint16<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint16_t x) override {
      return cont_->parse(p, DBusObjectUint16::mk(x));
    }
  };

  return ParseUint16<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeUint16::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeUint16_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeUint16::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeUint16_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeInt16_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint16<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint16_t x) override {
      return cont_->parse(p, DBusObjectInt16::mk(static_cast<int16_t>(x)));
    }
  };

  return ParseUint16<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeInt16::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeInt16_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeInt16::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeInt16_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeUint32_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint32<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t x) override {
      return cont_->parse(p, DBusObjectUint32::mk(x));
    }
  };

  return ParseUint32<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeUint32::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeUint32_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeUint32::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeUint32_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeInt32_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint32<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t x) override {
      return cont_->parse(p, DBusObjectInt32::mk(static_cast<int32_t>(x)));
    }
  };

  return ParseUint32<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeInt32::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeInt32_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeInt32::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeInt32_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeUint64_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint64<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint64_t x) override {
      return cont_->parse(p, DBusObjectUint64::mk(x));
    }
  };

  return ParseUint64<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeUint64::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeUint64_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeUint64::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeUint64_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeInt64_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint64<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint64_t x) override {
      return cont_->parse(p, DBusObjectInt64::mk(static_cast<int64_t>(x)));
    }
  };

  return ParseUint64<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeInt64::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeInt64_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeInt64::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeInt64_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeDouble_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint64<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint64_t i) override {
      union Cast {
        double d_;
        uint64_t x_;
      };
      Cast c;
      c.x_ = i;
      return cont_->parse(p, DBusObjectDouble::mk(c.d_));
    }
  };

  return ParseUint64<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeDouble::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeDouble_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeDouble::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeDouble_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeUnixFD_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseUint32<endianness>::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t i) override {
      return cont_->parse(p, DBusObjectUnixFD::mk(i));
    }
  };

  return ParseUint32<endianness>::mk(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeUnixFD::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeUnixFD_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeUnixFD::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeUnixFD_mkObjectParserImpl(std::move(cont));
}

// Utility for parsing a string with a known length.
static std::unique_ptr<Parse::Cont>
parseString(const Parse::State &p, size_t len,
            std::unique_ptr<ParseNChars::Cont> &&cont) {
  class ZerosCont final : public ParseZeros::Cont {
    std::string str_;
    const std::unique_ptr<ParseNChars::Cont> cont_;

  public:
    ZerosCont(std::string &&str, std::unique_ptr<ParseNChars::Cont> &&cont)
        : str_(std::move(str)), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) override {
      return cont_->parse(p, std::move(str_));
    }
  };

  class StringCont final : public ParseNChars::Cont {
    std::unique_ptr<ParseNChars::Cont> cont_;

  public:
    StringCont(std::unique_ptr<ParseNChars::Cont> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               std::string &&str) override {
      return ParseZeros::mk(
          p, 1, std::make_unique<ZerosCont>(std::move(str), std::move(cont_)));
    }
  };

  return ParseNChars::mk(p, std::string(), len,
                         std::make_unique<StringCont>(std::move(cont)));
}

// Utility for parsing a string with a 32-bit size prefix.
template <Endianness endianness>
static std::unique_ptr<Parse::Cont>
parseString32(std::unique_ptr<ParseNChars::Cont> &&cont) {
  class LengthCont final : public ParseUint32<endianness>::Cont {
    std::unique_ptr<ParseNChars::Cont> cont_;

  public:
    LengthCont(std::unique_ptr<ParseNChars::Cont> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t len) override {
      return parseString(p, len, std::move(cont_));
    }
  };

  // Parse the size
  return ParseUint32<endianness>::mk(
      std::make_unique<LengthCont>(std::move(cont)));
}

// Utility for parsing a string with an 8-bit size prefix.
std::unique_ptr<Parse::Cont>
parseString8(std::unique_ptr<ParseNChars::Cont> &&cont) {
  class LengthCont final : public ParseChar::Cont {
    std::unique_ptr<ParseNChars::Cont> cont_;

  public:
    LengthCont(std::unique_ptr<ParseNChars::Cont> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               char c) override {
      uint8_t len = (uint8_t)c;
      return parseString(p, len, std::move(cont_));
    }
  };

  // Parse the size
  return ParseChar::mk(std::make_unique<LengthCont>(std::move(cont)));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeString_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseNChars::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               std::string &&str) override {
      return cont_->parse(p, DBusObjectString::mk(std::move(str)));
    }
  };

  return parseString32<endianness>(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeString::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeString_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeString::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeString_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypePath_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseNChars::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               std::string &&str) override {
      return cont_->parse(p, DBusObjectPath::mk(std::move(str)));
    }
  };

  return parseString32<endianness>(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypePath::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypePath_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypePath::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypePath_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeSignature_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseNChars::Cont {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               std::string &&str) override {
      return cont_->parse(p, DBusObjectSignature::mk(std::move(str)));
    }
  };

  return parseString8(std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeSignature::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeSignature_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeSignature::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeSignature_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeVariant_mkObjectParserImpl(
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class ObjectCont final : public DBusType::ParseObjectCont<endianness> {
    // We need to own `typeStorage_` until the object parsing is complete
    // so that the type doesn't go out of scope too soon.
    DBusTypeStorage typeStorage_;

    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    ObjectCont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    DBusTypeStorage &getTypeStorage() { return typeStorage_; }

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &p, std::unique_ptr<DBusObject> &&obj) override {
      return cont_->parse(p, DBusObjectVariant::mk(std::move(obj)));
    }
  };

  class ZerosCont final : public ParseZeros::Cont {
    std::unique_ptr<DBusTypeStorage> typeStorage_;
    const DBusType &t_;
    std::unique_ptr<ObjectCont> cont_;

  public:
    ZerosCont(const DBusType &t, std::unique_ptr<ObjectCont> &&cont)
        : t_(t), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) override {
      return t_.mkObjectParser<endianness>(p, std::move(cont_));
    }
  };

  class TypeCont final : public DBusType::ParseTypeCont {
    const size_t endpos_; // Byte position where the signature should end
    std::unique_ptr<ObjectCont> cont_;

  public:
    TypeCont(size_t endpos, std::unique_ptr<ObjectCont> &&cont)
        : endpos_(endpos), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(DBusTypeStorage &,
                                               const Parse::State &p,
                                               const DBusType &t) override {
      const size_t pos = p.getPos();
      if (pos != endpos_) {
        throw ParseError(pos, "Incorrect variant signature length.");
      }

      // Parse the terminating zero byte.
      return ParseZeros::mk(p, 1,
                            std::make_unique<ZerosCont>(t, std::move(cont_)));
    }

    virtual std::unique_ptr<Parse::Cont>
    parseCloseParen(DBusTypeStorage &, const Parse::State &p) override {
      throw ParseError(
          p.getPos(),
          "Unexpected close paren while parsing variant signature.");
    }
  };

  class LengthCont final : public ParseChar::Cont {
    std::unique_ptr<ObjectCont> cont_;

  public:
    LengthCont(std::unique_ptr<ObjectCont> &&cont) : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               char c) override {
      uint8_t len = (uint8_t)c;

      const size_t pos = p.getPos();
      size_t endpos = 0;
      if (__builtin_add_overflow(pos, len, &endpos)) {
        throw ParseError(pos, "Signature length integer overflow.");
      }

      DBusTypeStorage &typeStorage = cont_->getTypeStorage();
      return parseType(typeStorage,
                       std::make_unique<TypeCont>(endpos, std::move(cont_)));
    }
  };

  // Parse the length of the signature. It isn't actually needed for
  // parsing the signature because we know that signature contains exactly
  // one type, so all we're going to do with it is verify that it's
  // correct.
  return ParseChar::mk(std::make_unique<LengthCont>(
      std::make_unique<ObjectCont>(std::move(cont))));
}

std::unique_ptr<Parse::Cont> DBusTypeVariant::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeVariant_mkObjectParserImpl(std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeVariant::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeVariant_mkObjectParserImpl(std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeDictEntry_mkObjectParserImpl(
    const Parse::State &p, const DBusType &keyType, const DBusType &valueType,
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class ValueCont final : public DBusType::ParseObjectCont<endianness> {
    std::unique_ptr<DBusObject> key_;
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    ValueCont(std::unique_ptr<DBusObject> &&key,
              std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : key_(std::move(key)), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &p, std::unique_ptr<DBusObject> &&value) override {
      return cont_->parse(
          p, DBusObjectDictEntry::mk(std::move(key_), std::move(value)));
    }
  };

  class KeyCont final : public DBusType::ParseObjectCont<endianness> {
    const DBusType &valueType_;
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    KeyCont(const DBusType &valueType,
            std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : valueType_(valueType), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &p, std::unique_ptr<DBusObject> &&key) override {
      return valueType_.mkObjectParser<endianness>(
          p, std::make_unique<ValueCont>(std::move(key), std::move(cont_)));
    }
  };

  return keyType.mkObjectParser<endianness>(
      p, std::make_unique<KeyCont>(valueType, std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeDictEntry::mkObjectParserImpl(
    const Parse::State &p,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeDictEntry_mkObjectParserImpl(p, keyType_, valueType_,
                                              std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeDictEntry::mkObjectParserImpl(
    const Parse::State &p,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeDictEntry_mkObjectParserImpl(p, keyType_, valueType_,
                                              std::move(cont));
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont>
parseArray(const Parse::State &p, const DBusType &elemType, size_t endpos,
           std::vector<std::unique_ptr<DBusObject>> &&elements,
           std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public DBusType::ParseObjectCont<endianness> {
    const DBusType &elemType_;
    const size_t endpos_;
    std::vector<std::unique_ptr<DBusObject>> elements_;
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(const DBusType &elemType, size_t endpos,
         std::vector<std::unique_ptr<DBusObject>> &&elements,
         std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : elemType_(elemType), endpos_(endpos), elements_(std::move(elements)),
          cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &p, std::unique_ptr<DBusObject> &&obj) override {
      elements_.push_back(std::move(obj));
      return parseArray(p, elemType_, endpos_, std::move(elements_),
                        std::move(cont_));
    }
  };

  const size_t pos = p.getPos();
  if (pos < endpos) {
    return elemType.mkObjectParser<endianness>(
        p, std::make_unique<Cont>(elemType, endpos, std::move(elements),
                                  std::move(cont)));
  } else if (pos == endpos) {
    return cont->parse(p, DBusObjectArray::mk(elemType, std::move(elements)));
  } else {
    throw ParseError(pos, "Incorrect array length.");
  }
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont> DBusTypeArray_mkObjectParserImpl(
    const DBusType &baseType,
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  // Continuation for parsing padding bytes.
  class PaddingCont final : public ParseZeros::Cont {
    const DBusType &elemType_;
    const uint32_t len_;
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    PaddingCont(const DBusType &elemType, uint32_t len,
                std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : elemType_(elemType), len_(len), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) override {
      const size_t pos = p.getPos();
      size_t endpos = 0;
      if (__builtin_add_overflow(pos, len_, &endpos)) {
        throw ParseError(pos, "Array length integer overflow.");
      }
      return parseArray(p, elemType_, endpos,
                        std::vector<std::unique_ptr<DBusObject>>(),
                        std::move(cont_));
    }
  };

  class LengthCont final : public ParseUint32<endianness>::Cont {
    const DBusType &elemType_;
    std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    LengthCont(const DBusType &elemType,
               std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : elemType_(elemType), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p,
                                               uint32_t len) override {
      return parse_alignment(
          p, elemType_,
          std::make_unique<PaddingCont>(elemType_, len, std::move(cont_)));
    }
  };

  // Parse the size
  return ParseUint32<endianness>::mk(
      std::make_unique<LengthCont>(baseType, std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeArray::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return DBusTypeArray_mkObjectParserImpl(baseType_, std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeArray::mkObjectParserImpl(
    const Parse::State &,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return DBusTypeArray_mkObjectParserImpl(baseType_, std::move(cont));
}

// Continuation argument to parseObjects.
template <Endianness endianness> class ParseObjectsCont {
protected:
  std::vector<std::unique_ptr<DBusObject>> objects_;

public:
  virtual ~ParseObjectsCont() {}

  void addObject(std::unique_ptr<DBusObject> &&obj) {
    objects_.push_back(std::move(obj));
  }

  virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) = 0;
};

template <Endianness endianness>
static std::unique_ptr<Parse::Cont>
parseObjects(const Parse::State &p,
             const std::vector<std::reference_wrapper<const DBusType>> &types,
             size_t i, std::unique_ptr<ParseObjectsCont<endianness>> &&cont) {
  class Cont final : public DBusType::ParseObjectCont<endianness> {
    const std::vector<std::reference_wrapper<const DBusType>> &types_;
    const size_t i_;
    std::unique_ptr<ParseObjectsCont<endianness>> cont_;

  public:
    Cont(const std::vector<std::reference_wrapper<const DBusType>> &types,
         size_t i, std::unique_ptr<ParseObjectsCont<endianness>> &&cont)
        : types_(types), i_(i), cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &p, std::unique_ptr<DBusObject> &&obj) override {
      cont_->addObject(std::move(obj));
      return parseObjects(p, types_, i_ + 1, std::move(cont_));
    }
  };

  if (i < types.size()) {
    const DBusType &t = types[i];
    return t.mkObjectParser<endianness>(
        p, std::make_unique<Cont>(types, i, std::move(cont)));
  } else {
    return cont->parse(p);
  }
}

template <Endianness endianness>
static std::unique_ptr<Parse::Cont>
parseStruct(const Parse::State &p, const DBusTypeStruct &structType,
            std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont) {
  class Cont final : public ParseObjectsCont<endianness> {
    const std::unique_ptr<DBusType::ParseObjectCont<endianness>> cont_;

  public:
    Cont(std::unique_ptr<DBusType::ParseObjectCont<endianness>> &&cont)
        : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) override {
      return cont_->parse(p, DBusObjectStruct::mk(std::move(
                                 ParseObjectsCont<endianness>::objects_)));
    }
  };

  return parseObjects<endianness>(p, structType.getFieldTypes(), 0,
                                  std::make_unique<Cont>(std::move(cont)));
}

std::unique_ptr<Parse::Cont> DBusTypeStruct::mkObjectParserImpl(
    const Parse::State &p,
    std::unique_ptr<DBusType::ParseObjectCont<LittleEndian>> &&cont) const {
  return parseStruct(p, *this, std::move(cont));
}

std::unique_ptr<Parse::Cont> DBusTypeStruct::mkObjectParserImpl(
    const Parse::State &p,
    std::unique_ptr<DBusType::ParseObjectCont<BigEndian>> &&cont) const {
  return parseStruct(p, *this, std::move(cont));
}

std::vector<std::reference_wrapper<const DBusType>>
DBusObjectSignature::toTypes(DBusTypeStorage &typeStorage // Type allocator
) const {
  class TypeCont final : public DBusType::ParseTypeCont {
    const size_t endpos_;
    std::vector<std::reference_wrapper<const DBusType>> &result_;

  public:
    TypeCont(size_t endpos,
             std::vector<std::reference_wrapper<const DBusType>> &result)
        : endpos_(endpos), result_(result) {}

    virtual std::unique_ptr<Parse::Cont> parse(DBusTypeStorage &typeStorage,
                                               const Parse::State &p,
                                               const DBusType &t) override {
      result_.push_back(t);
      if (p.getPos() < endpos_) {
        return parseType(typeStorage,
                         std::make_unique<TypeCont>(endpos_, result_));
      } else {
        return ParseStop::mk();
      }
    }

    virtual std::unique_ptr<Parse::Cont>
    parseCloseParen(DBusTypeStorage &, const Parse::State &p) override {
      throw ParseError(p.getPos(),
                       "Unexpected close paren while parsing signature.");
    }
  };

  std::vector<std::reference_wrapper<const DBusType>> result;
  const size_t endpos = str_.size();
  Parse p(parseType(typeStorage, std::make_unique<TypeCont>(endpos, result)));

  while (true) {
    const size_t required = p.maxRequiredBytes();
    const size_t pos = p.getPos();
    if (required == 0) {
      assert(pos == endpos);
      return result;
    }
    if (required > endpos - pos) {
      throw ParseError(pos, "DBusType::fromSignature not enough bytes");
    }
    p.parse(str_.c_str() + pos, required);
  }
}

template <Endianness endianness>
std::unique_ptr<Parse::Cont>
DBusMessage::parse(std::unique_ptr<DBusMessage> &result) {
  class BodyCont final : public ParseObjectsCont<endianness> {
    // We need to own `typeStorage_` until the object parsing is complete
    // so that the type doesn't go out of scope too soon.
    DBusTypeStorage typeStorage_;

    std::unique_ptr<DBusMessage> &result_;

    // We need to own `bodyTypes_` until the object parsing is complete
    // so that the vector doesn't go out of scope too soon.
    const std::vector<std::reference_wrapper<const DBusType>> bodyTypes_;

    // Utility for initializing `bodyTypes_` in the constructor.
    static std::vector<std::reference_wrapper<const DBusType>>
    getBodyTypesFromHeader(DBusTypeStorage &typeStorage,
                           std::unique_ptr<DBusMessage> &result) {
      const uint32_t bodySize = result->getHeader_bodySize();
      if (bodySize == 0) {
        // No message body, so return an empty vector.
        return std::vector<std::reference_wrapper<const DBusType>>();
      }

      const DBusObjectSignature &bodySig =
          result->getHeader_lookupField(MSGHDR_SIGNATURE)
              .getValue()
              ->toSignature();

      return bodySig.toTypes(typeStorage);
    }

  public:
    explicit BodyCont(std::unique_ptr<DBusMessage> &result)
        : result_(result),
          bodyTypes_(getBodyTypesFromHeader(typeStorage_, result_)) {}

    const std::vector<std::reference_wrapper<const DBusType>> &
    getBodyTypes() const {
      return bodyTypes_;
    }

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &) override {
      result_->body_ = DBusMessageBody::mk(
          std::move(ParseObjectsCont<endianness>::objects_));
      return ParseStop::mk();
    }
  };

  // Continuation for parsing padding bytes.
  class PaddingCont final : public ParseZeros::Cont {
    std::unique_ptr<BodyCont> cont_;

  public:
    PaddingCont(std::unique_ptr<BodyCont> cont) : cont_(std::move(cont)) {}

    virtual std::unique_ptr<Parse::Cont> parse(const Parse::State &p) override {
      auto &bodyTypes = cont_->getBodyTypes();
      return parseObjects<endianness>(p, bodyTypes, 0, std::move(cont_));
    }
  };

  class HeaderCont final : public DBusType::ParseObjectCont<endianness> {
    std::unique_ptr<DBusMessage> &result_;

  public:
    HeaderCont(std::unique_ptr<DBusMessage> &result) : result_(result) {}

    virtual std::unique_ptr<Parse::Cont>
    parse(const Parse::State &p,
          std::unique_ptr<DBusObject> &&header) override {
      result_ = std::make_unique<DBusMessage>(std::move(header),
                                              DBusMessageBody::mk0());

      // The body is 8-byte aligned.
      return parse_alignment(
          p, DBusTypeUint64::instance_,
          std::make_unique<PaddingCont>(std::make_unique<BodyCont>(result_)));
    }
  };

  return headerType.mkObjectParser<endianness>(
      Parse::State::initialState_, std::make_unique<HeaderCont>(result));
}

std::unique_ptr<Parse::Cont>
DBusMessage::parseLE(std::unique_ptr<DBusMessage> &result) {
  return parse<LittleEndian>(result);
}

std::unique_ptr<Parse::Cont>
DBusMessage::parseBE(std::unique_ptr<DBusMessage> &result) {
  return parse<BigEndian>(result);
}
