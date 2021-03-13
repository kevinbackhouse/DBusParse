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


#include <unistd.h>
#include <sys/socket.h>
#include "dbus_serialize.hpp"
#include "dbus_print.hpp"
#include "dbus_utils.hpp"
#include "utils.hpp"

void send_dbus_message_with_fds(
  const int fd, const DBusMessage& message,
  const size_t nfds, const int* fds
) {
  std::vector<uint32_t> arraySizes;
  SerializerInitArraySizes s0(arraySizes);
  message.serialize(s0);

  const size_t size = s0.getPos();
  std::unique_ptr<char[]> buf(new char[size]);
  SerializeToBuffer<LittleEndian> s1(arraySizes, buf.get());
  message.serialize(s1);

  struct msghdr msg = {}; // Zero initialize.
  struct iovec io = {};
  io.iov_base = buf.get();
  io.iov_len = size;

  const size_t fds_size = nfds * sizeof(int);
  msg.msg_iov = &io;
  msg.msg_iovlen = 1;
  msg.msg_controllen = CMSG_SPACE(fds_size);
  msg.msg_control = malloc(msg.msg_controllen);

  struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(fds_size);
  memcpy(CMSG_DATA(cmsg), &fds[0], fds_size);

  const ssize_t wr = sendmsg(fd, &msg, 0);
  if (wr < 0) {
    const int err = errno;
    fprintf(stderr, "sendmsg failed: %s\n", strerror(err));
  } else if (static_cast<size_t>(wr) != size) {
    fprintf(stderr, "sendmsg incomplete: %ld < %lu\n", wr, size);
  }

  free(msg.msg_control);
}

void send_dbus_message(const int fd, const DBusMessage& message) {
  std::vector<uint32_t> arraySizes;
  SerializerInitArraySizes s0(arraySizes);
  message.serialize(s0);

  const size_t size = s0.getPos();
  std::unique_ptr<char[]> buf(new char[size]);
  SerializeToBuffer<LittleEndian> s1(arraySizes, buf.get());
  message.serialize(s1);

  const ssize_t wr = write(fd, buf.get(), size);
  if (wr < 0) {
    const int err = errno;
    fprintf(stderr, "write failed: %s\n", strerror(err));
  } else if (static_cast<size_t>(wr) != size) {
    fprintf(stderr, "write incomplete: %ld < %lu\n", wr, size);
  }
}

// Note: this is a very simplistic implementation. It expects to loop until
// it has read the entire message. It is only designed to be used with a
// blocking socket.
std::unique_ptr<DBusMessage> receive_dbus_message(const int fd) {
  std::unique_ptr<DBusMessage> message;
  Parse p(DBusMessage::parseLE(message));

  while (true) {
    char buf[256];
    size_t required = p.maxRequiredBytes();
    if (required == 0) {
      return message;
    }
    if (required > sizeof(buf)) {
      required = sizeof(buf);
    }
    const ssize_t n = read(fd, buf, required);
    if (n < 0 || size_t(n) < required) {
      // Note: this error could happen by accident if `fd` is a
      // non-blocking socket, because we might get a partial read. (See
      // comment at top of function.)
      throw ParseError(p.getPos(), _s("No more input. n=") + std::to_string(n));
    }
    p.parse(buf, required);
  }
}

void print_dbus_object(const int fd, const DBusObject& obj) {
  PrinterFD printFD(fd, 16, 2);
  obj.print(printFD);
  printFD.printNewline(0);
}

void print_dbus_message(const int fd, const DBusMessage& message) {
  PrinterFD printFD(fd, 16, 2);
  message.print(printFD, 0);
  printFD.printNewline(0);
}

void dbus_method_call_with_fds(
  const int fd,
  const uint32_t serialNumber,
  std::unique_ptr<DBusMessageBody>&& body,
  std::string&& path,
  std::string&& interface,
  std::string&& destination,
  std::string&& member,
  const size_t nfds,
  const int* fds,
  const MessageFlags flags
) {
  const size_t bodySize = body->serializedSize();

  std::unique_ptr<DBusObject> header =
    DBusObjectStruct::mk(
      _vec<std::unique_ptr<DBusObject>>(
        // Header
        DBusObjectChar::mk('l'), // Little endian
        DBusObjectChar::mk(MSGTYPE_METHOD_CALL),
        DBusObjectChar::mk(flags),
        DBusObjectChar::mk(1), // Major protocol version
        DBusObjectUint32::mk(bodySize), // body_len_unsigned
        DBusObjectUint32::mk(serialNumber), // serial number
        DBusObjectArray::mk1(
          _vec<std::unique_ptr<DBusObject>>(
            DBusHeaderField::mk(
              MSGHDR_PATH,
              DBusObjectVariant::mk(
                DBusObjectPath::mk(std::move(path))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_INTERFACE,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(interface))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_DESTINATION,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(destination))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_MEMBER,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(member))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_SIGNATURE,
              DBusObjectVariant::mk(
                DBusObjectSignature::mk(body->signature())
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_UNIX_FDS,
              DBusObjectVariant::mk(
                DBusObjectUint32::mk(nfds)
              )
            )
          )
        )
      )
    );

  const DBusMessage message(std::move(header), std::move(body));

  send_dbus_message_with_fds(fd, message, nfds, fds);
}

void dbus_method_call(
  const int fd,
  const uint32_t serialNumber,
  std::unique_ptr<DBusMessageBody>&& body,
  std::string&& path,
  std::string&& interface,
  std::string&& destination,
  std::string&& member,
  const MessageFlags flags
) {
  const size_t bodySize = body->serializedSize();

  std::unique_ptr<DBusObject> header =
    DBusObjectStruct::mk(
      _vec<std::unique_ptr<DBusObject>>(
        // Header
        DBusObjectChar::mk('l'), // Little endian
        DBusObjectChar::mk(MSGTYPE_METHOD_CALL),
        DBusObjectChar::mk(flags),
        DBusObjectChar::mk(1), // Major protocol version
        DBusObjectUint32::mk(bodySize), // body_len_unsigned
        DBusObjectUint32::mk(serialNumber), // serial number
        DBusObjectArray::mk1(
          _vec<std::unique_ptr<DBusObject>>(
            DBusHeaderField::mk(
              MSGHDR_PATH,
              DBusObjectVariant::mk(
                DBusObjectPath::mk(std::move(path))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_INTERFACE,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(interface))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_DESTINATION,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(destination))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_MEMBER,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(member))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_SIGNATURE,
              DBusObjectVariant::mk(
                DBusObjectSignature::mk(body->signature())
              )
            )
          )
        )
      )
    );

  const DBusMessage message(std::move(header), std::move(body));

  send_dbus_message(fd, message);
}

void dbus_method_reply(
  const int fd,
  const uint32_t serialNumber,
  const uint32_t replySerialNumber, // serial number that we are replying to
  std::unique_ptr<DBusMessageBody>&& body,
  std::string&& destination
) {
  const size_t bodySize = body->serializedSize();

  std::unique_ptr<DBusObject> header =
    DBusObjectStruct::mk(
      _vec<std::unique_ptr<DBusObject>>(
        // Header
        DBusObjectChar::mk('l'), // Little endian
        DBusObjectChar::mk(MSGTYPE_METHOD_RETURN),
        DBusObjectChar::mk(MSGFLAGS_EMPTY),
        DBusObjectChar::mk(1), // Major protocol version
        DBusObjectUint32::mk(bodySize), // body_len_unsigned
        DBusObjectUint32::mk(serialNumber), // serial number
        DBusObjectArray::mk1(
          _vec<std::unique_ptr<DBusObject>>(
            DBusHeaderField::mk(
              MSGHDR_DESTINATION,
              DBusObjectVariant::mk(
                DBusObjectString::mk(std::move(destination))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_SIGNATURE,
              DBusObjectVariant::mk(
                DBusObjectSignature::mk(body->signature())
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_REPLY_SERIAL,
              DBusObjectVariant::mk(
                DBusObjectUint32::mk(replySerialNumber)
              )
            )
          )
        )
      )
    );

  const DBusMessage message(std::move(header), std::move(body));

  send_dbus_message(fd, message);
}

void dbus_send_hello(const int fd) {
  dbus_method_call(
    fd,
    0x1001,
    DBusMessageBody::mk0(),
    _s("/org/freedesktop/DBus"),
    _s("org.freedesktop.DBus"),
    _s("org.freedesktop.DBus"),
    _s("Hello")
  );

#if 0
  std::unique_ptr<DBusMessageBody> body = DBusMessageBody::mk0();
  const size_t bodySize = body->serializedSize();

  std::unique_ptr<DBusObject> header =
    DBusObjectStruct::mk(
      _vec<std::unique_ptr<DBusObject>>(
        // Header
        DBusObjectChar::mk('l'), // Little endian
        DBusObjectChar::mk(MSGTYPE_METHOD_CALL),
        DBusObjectChar::mk(MSGFLAGS_EMPTY),
        DBusObjectChar::mk(1), // Major protocol version
        DBusObjectUint32::mk(bodySize), // body_len_unsigned
        DBusObjectUint32::mk(0x1001), // serial number
        DBusObjectArray::mk1(
          _vec<std::unique_ptr<DBusObject>>(
            DBusHeaderField::mk(
              MSGHDR_PATH,
              DBusObjectVariant::mk(
                DBusObjectPath::mk(_s("/org/freedesktop/DBus"))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_INTERFACE,
              DBusObjectVariant::mk(
                DBusObjectString::mk(_s("org.freedesktop.DBus"))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_DESTINATION,
              DBusObjectVariant::mk(
                DBusObjectString::mk(_s("org.freedesktop.DBus"))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_MEMBER,
              DBusObjectVariant::mk(
                DBusObjectString::mk(_s("Hello"))
              )
            ),
            DBusHeaderField::mk(
              MSGHDR_REPLY_SERIAL,
              DBusObjectVariant::mk(
                DBusObjectUint32::mk(0x2001)
              )
            )
          )
        )
      )
    );

  const DBusMessage message(std::move(header), std::move(body));

  send_dbus_message(fd, message);
#endif
}
