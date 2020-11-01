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


#include "dbus_print.hpp"
#include "utils.hpp"
#include <unistd.h>

void DBusObjectChar::print(Printer& p, size_t) const {
  p.printUint8(c_);
}

void DBusObjectBoolean::print(Printer& p, size_t) const {
  p.printUint32(static_cast<uint32_t>(b_));
}

void DBusObjectUint16::print(Printer& p, size_t) const {
  p.printUint16(x_);
}

void DBusObjectInt16::print(Printer& p, size_t) const {
  p.printInt16(x_);
}

void DBusObjectUint32::print(Printer& p, size_t) const {
  p.printUint32(x_);
}

void DBusObjectInt32::print(Printer& p, size_t) const {
  p.printInt32(x_);
}

void DBusObjectUint64::print(Printer& p, size_t) const {
  p.printUint64(x_);
}

void DBusObjectInt64::print(Printer& p, size_t) const {
  p.printInt64(x_);
}

void DBusObjectDouble::print(Printer& p, size_t) const {
  p.printDouble(d_);
}

void DBusObjectUnixFD::print(Printer& p, size_t) const {
  p.printUint32(i_);
}

void DBusObjectString::print(Printer& p, size_t) const {
  p.printString(str_);
}

void DBusObjectSignature::print(Printer& p, size_t) const {
  p.printString(str_);
}

void DBusObjectVariant::print(Printer& p, size_t indent) const {
  p.printString(_s("Variant "));
  signature_.print(p, indent);
  p.printNewline(indent);
  object_->print(p, indent);
}

void DBusObjectDictEntry::print(Printer& p, size_t indent) const {
  p.printChar('{');
  ++indent;
  p.printNewline(indent);
  key_->print(p, indent);
  p.printChar(',');
  p.printNewline(indent);
  value_->print(p, indent);
  --indent;
  p.printNewline(indent);
  p.printChar('}');
}

void DBusObjectSeq::print(
  Printer& p, size_t indent, char lbracket, char rbracket
) const {
  p.printChar(lbracket);
  ++indent;
  const size_t n = elements_.size();
  if (n > 0) {
    p.printNewline(indent);
    elements_[0]->print(p, indent);
    for (size_t i = 1; i < n; i++) {
      p.printChar(',');
      p.printNewline(indent);
      elements_[i]->print(p, indent);
    }
  }
  --indent;
  p.printNewline(indent);
  p.printChar(rbracket);
}

void DBusObjectArray::print(Printer& p, size_t indent) const {
  seq_.print(p, indent, '[', ']');
}

void DBusObjectStruct::print(Printer& p, size_t indent) const {
  seq_.print(p, indent, '(', ')');
}

static void printMessageType(Printer& p, MessageType t) {
  switch (t) {
  case MSGTYPE_INVALID: p.printString("INVALID"); break;
  case MSGTYPE_METHOD_CALL: p.printString("METHOD_CALL"); break;
  case MSGTYPE_METHOD_RETURN: p.printString("METHOD_RETURN"); break;
  case MSGTYPE_ERROR: p.printString("ERROR"); break;
  case MSGTYPE_SIGNAL: p.printString("SIGNAL"); break;
  default: p.printString("UNKNOWN"); break;
  }
}

static void printMessageFlags(Printer& p, MessageFlags flags) {
  if (flags & MSGFLAGS_NO_REPLY_EXPECTED) {
    p.printString(" NO_REPLY_EXPECTED");
  }
  if (flags & MSGFLAGS_NO_AUTO_START) {
    p.printString(" NO_AUTO_START");
  }
  if (flags & MSGFLAGS_ALLOW_INTERACTIVE_AUTHORIZATION) {
    p.printString(" ALLOW_INTERACTIVE_AUTHORIZATION");
  }
}

static void printHeaderFieldName(Printer& p, HeaderFieldName name) {
  switch (name) {
  case MSGHDR_INVALID: p.printString("INVALID"); break;
  case MSGHDR_PATH: p.printString("PATH"); break;
  case MSGHDR_INTERFACE: p.printString("INTERFACE"); break;
  case MSGHDR_MEMBER: p.printString("MEMBER"); break;
  case MSGHDR_ERROR_NAME: p.printString("ERROR_NAME"); break;
  case MSGHDR_REPLY_SERIAL: p.printString("REPLY_SERIAL"); break;
  case MSGHDR_DESTINATION: p.printString("DESTINATION"); break;
  case MSGHDR_SENDER: p.printString("SENDER"); break;
  case MSGHDR_SIGNATURE: p.printString("SIGNATURE"); break;
  case MSGHDR_UNIX_FDS: p.printString("UNIX_FDS"); break;
  default: p.printString("UNKNOWN"); break;
  }
}

void DBusMessageBody::print(Printer& p, size_t indent) const {
  seq_.print(p, indent, '(', ')');
}

void DBusMessage::print(Printer& p, size_t indent) const {
  p.printString(_s("Header:"));
  indent++;
  p.printNewline(indent);

  const DBusObjectStruct& header = getHeader();
  p.printString("endianness: ");
  p.printChar(getHeader_endianness());
  p.printNewline(indent);

  p.printString("message type: ");
  printMessageType(p, getHeader_messageType());
  p.printNewline(indent);

  p.printString("message flags:");
  printMessageFlags(p, getHeader_messageFlags());
  p.printNewline(indent);

  p.printString("major protocol version: ");
  p.printUint8(getHeader_protocolVersion());
  p.printNewline(indent);

  p.printString("body size: ");
  p.printUint32(getHeader_bodySize());
  p.printNewline(indent);

  p.printString("serial number: ");
  p.printUint32(getHeader_replySerial());
  p.printNewline(indent);

  p.printString("header fields:");
  const DBusObjectArray& headerFields = header.getElement(6)->toArray();
  const size_t numHeaderFields = headerFields.numElements();
  for (size_t i = 0; i < numHeaderFields; i++) {
    const DBusObjectStruct& headerField = headerFields.getElement(i)->toStruct();
    p.printNewline(2);
    printHeaderFieldName(
      p, static_cast<HeaderFieldName>(headerField.getElement(0)->toChar().getValue())
    );
    p.printChar(':');
    p.printNewline(3);
    headerField.getElement(1)->print(p, 3);
  }

  if (body_) {
    p.printNewline(0);
    p.printString(_s("Body:"));
    p.printNewline(indent);
    body_->print(p, indent);
  }
}

template <class T>
static size_t numberToString(char* buf, T x, size_t base) {
  static const char digits[36] =
    { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
      'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
      'U', 'V', 'W', 'X', 'Y', 'Z'
    };
  size_t n = 0;
  do {
    const size_t digit = x % base;
    x = x / base;
    buf[n] = digits[digit];
    ++n;
  } while (x != 0);

  // The digits came out in reverse order, so reverse them.
  size_t i = 0;
  size_t j = n-1;
  for (; i < j; ++i, --j) {
    std::swap(buf[i], buf[j]);
  }

  return i+j+1;
}

void PrinterFD::printBytes(const char* buf, size_t bufsize) {
  const int r = write(fd_, buf, bufsize);
  if (r < 0) {
    throw ErrorWithErrno("Write failed during pretty printing.");
  }
}

void PrinterFD::printChar(char c) {
  printBytes(&c, sizeof(c));
}

void PrinterFD::printUint8(uint8_t x) {
  char buf[8];
  const size_t n = numberToString<uint8_t>(buf, x, base_);
  printBytes(buf, n);
}

void PrinterFD::printInt8(int8_t x) {
  if (x < 0) {
    printChar('-');
    printUint16(-static_cast<uint8_t>(x));
  } else {
    printUint16(static_cast<uint8_t>(x));
  }
}

void PrinterFD::printUint16(uint16_t x) {
  char buf[16];
  const size_t n = numberToString<uint16_t>(buf, x, base_);
  printBytes(buf, n);
}

void PrinterFD::printInt16(int16_t x) {
  if (x < 0) {
    printChar('-');
    printUint16(-static_cast<uint16_t>(x));
  } else {
    printUint16(static_cast<uint16_t>(x));
  }
}

void PrinterFD::printUint32(uint32_t x) {
  char buf[32];
  const size_t n = numberToString<uint32_t>(buf, x, base_);
  printBytes(buf, n);
}

void PrinterFD::printInt32(int32_t x) {
  if (x < 0) {
    printChar('-');
    printUint32(-static_cast<uint32_t>(x));
  } else {
    printUint32(static_cast<uint32_t>(x));
  }
}

void PrinterFD::printUint64(uint64_t x)  {
  char buf[64];
  const size_t n = numberToString<uint64_t>(buf, x, base_);
  printBytes(buf, n);
}

void PrinterFD::printInt64(int64_t x) {
  if (x < 0) {
    printChar('-');
    printUint64(-static_cast<uint64_t>(x));
  } else {
    printUint64(static_cast<uint64_t>(x));
  }
}

void PrinterFD::printDouble(double d)  {
  char buf[128];
  const int n = snprintf(buf, sizeof(buf), "%f", d);
  if (0 <= n && static_cast<size_t>(n) <= sizeof(buf)) {
    printBytes(buf, n);
  }
}

void PrinterFD::printString(const std::string& str) {
  printBytes(str.c_str(), str.size());
}

// Print a newline character, followed by `tabsize_ * indent`
// space chracters.
void PrinterFD::printNewline(size_t indent) {
  static const char spaces[66] =
    "\n                                                                ";
  size_t nspaces = tabsize_ * indent;
  // The indent will usually be relatively small, so the fast
  // case just prints the first `nspaces+1` characters of `spaces`.
  // (The +1 is for the newline character at the beginning.)
  if (likely(nspaces <= sizeof(spaces)-2)) {
    printBytes(spaces, nspaces+1);
    return;
  }
  printBytes(spaces, sizeof(spaces)-1);
  nspaces -= sizeof(spaces)-2;
  while (nspaces > sizeof(spaces)-2) {
    printBytes(&spaces[1], sizeof(spaces)-2);
    nspaces -= sizeof(spaces)-2;
  }
  printBytes(&spaces[1], nspaces);
}
