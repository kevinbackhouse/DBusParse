Copyright 2020-2024 Kevin Backhouse.

# DBusParse

DBusParse is a C++ library for parsing
[D-Bus](https://www.freedesktop.org/wiki/Software/dbus/) messages.  It
can also serialize a D-Bus message to an array of bytes, ready for
sending.  DBusParse includes some simple utility functions for sending
and receiving D-Bus messages, but is not intended to be a fully
fledged D-Bus message handler. For example, it does not include logic
for generating message serial numbers. However, it is certainly
suitable to be used as a sub-component of a complete D-Bus
implementation.

The parser is designed to be able to parse an incomplete input. You
can feed it some bytes, then save its state until you have received
more bytes. This is handy when reading bytes from a socket, because
you might not receive the complete message in a single read. It means,
for example, that the parser is well-suited for use in an
[epoll](https://man7.org/linux/man-pages/man7/epoll.7.html) event
loop. The parser implementation uses
[continuation-passing style](https://en.wikipedia.org/wiki/Continuation-passing_style).
The parser state includes a continuation function, which is invoked
when more bytes are received.

I wrote DBusParse because I was doing security research on
[dbus-daemon](https://dbus.freedesktop.org/doc/dbus-daemon.1.html) and
several D-Bus services, and wanted to gain a thorough understanding of
the message format.  I also, stupidly, thought that it wouldn't be
much work. Initially, I only needed the ability to send messages and I
wasn't aware of the
[dbus-send](https://dbus.freedesktop.org/doc/dbus-send.1.html) tool,
so I wrote a serializer. But I quickly discovered that I also needed
to parse the reply messages, so I started implementing a parser
too. Now that it has grown into a complete implementation, I figure I
might as well make it open source.

## Usage

DBusParse is a library. For an example of how to use it in a simple
application, please see the
[DBusParseDemo](https://github.com/kevinbackhouse/DBusParseDemo) demo
repository.

## Building

On Linux, you can build DBusParse as follows:

```bash
mkdir build
cd build
cmake ..
make
```

To run the tests:

```bash
make test
```

You can install DBusParse on your system like this:

```bash
sudo make install
```

However, if you are not keen to use `sudo` to modify your system, you
can instead use DBusParse by including it as a sub-module in your project.
The [DBusParseDemo](https://github.com/kevinbackhouse/DBusParseDemo)
project does exactly that.

## Design notes

[dbus.hpp](/include/DBusParse/dbus.hpp) defines a class named
`DBusType` with sub-types such as `DBusTypeInt32` and `DBusTypeArray`.
Similarly, it defines a class named `DBusObject` with sub-types such as
`DBusObjectInt32` and `DBusObjectArray`. These class hierarchies
correspond to the types and objects defined in the [D-Bus
specification](http://dbus.freedesktop.org/doc/dbus-specification.html).

Smart pointers are used to manage memory automatically. For example,
composite objects such as `DBusObjectStruct` use
[`std::unique_ptr`](https://en.cppreference.com/w/cpp/memory/unique_ptr)
to point to their child objects.
