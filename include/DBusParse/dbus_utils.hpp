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
#include <memory>

void send_dbus_message_with_fds(const int fd, const DBusMessage &message,
                                const size_t nfds, const int *fds);

void send_dbus_message(const int fd, const DBusMessage &message);

void print_dbus_object(const int fd, const DBusObject &obj);

void print_dbus_message(const int fd, const DBusMessage &message);

std::unique_ptr<DBusMessage> receive_dbus_message(const int fd);

std::unique_ptr<DBusMessage> mk_dbus_method_call_msg(
    const uint32_t serialNumber, std::unique_ptr<DBusMessageBody> &&body,
    std::string &&path, std::string &&interface, std::string &&destination,
    std::string &&member, const size_t nfds, const MessageFlags flags);

std::unique_ptr<DBusMessage> mk_dbus_method_reply_msg(
    const uint32_t serialNumber,
    const uint32_t replySerialNumber, // serial number that we are replying to
    std::unique_ptr<DBusMessageBody> &&body, std::string &&destination);

std::unique_ptr<DBusMessage> mk_dbus_method_error_reply_msg(
    const uint32_t serialNumber,
    const uint32_t replySerialNumber, // serial number that we are replying to
    std::string &&destination, std::string &&errmsg);

void dbus_method_call_with_fds(const int fd, const uint32_t serialNumber,
                               std::unique_ptr<DBusMessageBody> &&body,
                               std::string &&path, std::string &&interface,
                               std::string &&destination, std::string &&member,
                               const size_t nfds, const int *fds,
                               const MessageFlags flags = MSGFLAGS_EMPTY);

void dbus_method_call(const int fd, const uint32_t serialNumber,
                      std::unique_ptr<DBusMessageBody> &&body,
                      std::string &&path, std::string &&interface,
                      std::string &&destination, std::string &&member,
                      const MessageFlags flags = MSGFLAGS_EMPTY);

void dbus_method_reply(
    const int fd, const uint32_t serialNumber,
    const uint32_t replySerialNumber, // serial number that we are replying to
    std::unique_ptr<DBusMessageBody> &&body, std::string &&destination);

void dbus_method_error_reply(
    const int fd, const uint32_t serialNumber,
    const uint32_t replySerialNumber, // serial number that we are replying to
    std::string &&destination, std::string &&errmsg);

void dbus_send_hello(const int fd);
