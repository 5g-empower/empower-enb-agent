# Empower Agent

This is a C++ archive library providing means to encode/decode and
transmit/receive messages across a TCP/IP connection, specifically
meant to be used to implement an agent within SRS ENB (part of the SRS
LTE package).

Messages are composed of a fixed-size common header (stating their
type) followed by zero or more Type-Length-Value fields (TLVs for short).

Message length is up to 2^32 bytes, with each TLV at most 2^16 bytes.

The *default* send/receive buffer size currently allows for messages up
to 2^16 bytes. The default can be enlarged (see `IO::makeMessageBuffer()`,
or you can provide your own function to allocate buffers (all it has to do
is to return a `NetworkLib::BufferWritableView` of the desired size).

By itself, the library *does not* implement any main loop. This has to
be implemented in the program using the library, using
`examples/agentserv.cpp` as an example.

# Involved technologies

* C++11 on Ubuntu 18.04 64-bit

  At the moment, libraries are built on Ubuntu 18.04 either with
  **GCC 7.x** (`g++-7`) or with **CLang 6.x** (`clang-6.0`), but other
  versions should be ok as long as they are able to correctly compile
  C++14 and C99 64-bit code;

* **CMake 3.10** or newer; older versions could be ok but are untested.

* **GNU Make**.

* **clang-format** as the tool to reformat source files.

* **Doxygen** as the tool to generate user documentation.

# Source tree

* `lib/*` contains the library and its include files

* `examples/*`: sources of some small example programs:

   * `agentserv`: sample agent establishing a TCP connection
      to some controller and then waiting for messages

   * `agentclient`: sample controller, listening for and accepting a
     TCP connection, then sending a simple ECHO message and waiting
     for a reply.

# Build

The build system is based on CMake, which generates automatically
whatever's needed to build the libraries.

The build options are specified via CMake variables, which can be
specified when generating the build environment.

## Build dependencies

* **GCC** or **CLang** supporting C++14 and C99.

  Any GCC version >= 5.x should work. Any CLang version >= 3.4 should work.

  Both GCC 7.3 and CLang 6.0.0 are known to work.

* **CMake 3.10** or newer; older versions could be ok, but are untested.

* **GNU Make** as a backend for CMake.

  GNU Make 4.1 is known to work.

* Linux standard C headers for networking development (sockets, etc.);

## How to build and install

On a clean checkout, just invoke CMake telling if this is a 'Release'
build or a 'Debug' build, plus any other option, and then use `make`
to build and install everything.

A few CMake variables that you may want to  define when invoking CMake with
option -D:

* `CMAKE_BUILD_TYPE` should be set to `Release` or `Debug`. There is
  no default value (as there can't really be one because of the way
  CMake works);

* `CMAKE_INSTALL_PREFIX` defaults to `/usr/local` on Linux-based systems,
   and is the prefix used for installation;


Example for a **release** build on a Unix-like system using the
default compilers in your $PATH and attempting to build the library
and install it and its headers in `/usr/local`

```
cmake -DCMAKE_BUILD_TYPE=Release .
make
sudo make install
```

## Documentation build

Example for building Doxygen development documentation (when Doxygen
is available):

```
cmake -DCMAKE_BUILD_TYPE=Release .
make doc
```


The documentation can then be found in .../doc/html/index.html
