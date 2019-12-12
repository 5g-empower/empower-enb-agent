
5G-EmPOWER: Mobile Networks Operating System
============================================

### What is 5G-EmPOWER?
5G-EmPOWER is a mobile network operating system designed for heterogeneous wireless/mobile networks.

### Top-Level Features
* Supports both LTE and WiFi networks
* Northbound abstractions for a global network view, network graph, and application intents.
* REST API and native (Python) API for accessing the Northbound abstractions
* Flexible southbound interface supporting WiFi APs and LTE eNBs

Checkout out our [website](http://5g-empower.io/) and our [wiki](https://github.com/5g-empower/5g-empower.github.io/wiki)

This repository includes the 5G-EmPOWER eNodeB Agent and the protocols libraries.

### License
Code is released under the Apache License, Version 2.0.


# Overview

This is a C++ archive library providing means to encode/decode and transmit/receive messages across a TCP/IP connection, specifically meant to be used to implement an agent within SRS ENB (part of the SRS LTE package).

Messages are composed of a fixed-size common header (stating their type) followed by zero or more Type-Length-Value fields (TLVs for short).

Message length is up to 2^32 bytes, with each TLV at most 2^16 bytes.

The *default* send/receive buffer size currently allows for messages up to 2^16 bytes. The default can be enlarged (see `IO::makeMessageBuffer()`, or you can provide your own function to allocate buffers (all it has to do is to return a `NetworkLib::BufferWritableView` of the desired size).

By itself, the library *does not* implement any main loop. This has to be implemented in the program using the library, using `examples/agentserv.cpp` as an example.

# Involved technologies

* **C++11 on Ubuntu 18.04 64-bit**, At the moment, libraries are built on Ubuntu 18.04 either with **GCC 7.x** (`g++-7`) or with **CLang 6.x** (`clang-6.0`), but other versions should be ok as long as they are able to correctly compile C++14 and C99 64-bit code;

* **CMake 3.10** or newer; older versions could be ok but are untested.

* **GNU Make**.

* **clang-format** as the tool to reformat source files.

* **Doxygen** as the tool to generate user documentation.

# Source tree

* `lib/*` contains the library and its include files

* `examples/*`: sources of some small example programs:

   * `agentserv`: sample agent establishing a TCP connection to some controller and then waiting for messages

   * `agentclient`: sample controller, listening for and accepting a TCP connection, then sending a simple ECHO message and waiting for a reply.

# Build

The build system is based on CMake, which generates automatically whatever's needed to build the libraries.

The build options are specified via CMake variables, which can be specified when generating the build environment.

## Build dependencies

* **GCC** or **CLang** supporting C++14 and C99.

  Any GCC version >= 5.x should work. Any CLang version >= 3.4 should work.

  Both GCC 7.3 and CLang 6.0.0 are known to work.

* **CMake 3.10** or newer; older versions could be ok, but are untested.

* **GNU Make** as a backend for CMake.

  GNU Make 4.1 is known to work.

* Linux standard C headers for networking development (sockets, etc.);

## How to build and install

On a clean checkout, just invoke CMake telling if this is a 'Release' build or a 'Debug' build, plus any other option, and then use `make`to build and install everything.

A few CMake variables that you may want to  define when invoking CMake with option -D:

* `CMAKE_BUILD_TYPE` should be set to `Release` or `Debug`. There is no default value (as there can't really be one because of the way CMake works);

* `CMAKE_INSTALL_PREFIX` defaults to `/usr/local` on Linux-based systems, and is the prefix used for installation;

Example for a **release** build on a Unix-like system using the default compilers in your $PATH and attempting to build the library and install it and its headers in `/usr/local`

```
cmake -DCMAKE_BUILD_TYPE=Release .
make
sudo make install
```

## Documentation build

Example for building Doxygen development documentation (when Doxygen is available):

```
cmake -DCMAKE_BUILD_TYPE=Release .
make doc
```

The documentation can then be found in `.../doc/html/index.html`.


## Message Format

The 5G-EmPOWER platform clearly decouples control-plane operations, which are left at the air interface, from management-plane operations, which are consolidated on top of the operating system layer (i.e., the SD-RAN controller).

An agent is introduced in the eNodeB to implement the management actions defined by the operating system. Communication between the agent and the operating system happens over the protocol described in this readme,

The 5G-EmPOWER protocol is layered on top of the Transmission Control Protocol (TCP) and can use the Transport Layer Security (TLS) protocol. The management planes should listen on TCP port 4433 for RAN elements that want to set up a connection.

All the messages in OpenEmpower start with a common header. The format of the header is the following:

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Version       |Flags          | <RESERVED>                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Length                                                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Description of the fields:

    VERSION
        Protocol version. Always set to 2.

    FLAGS
        Protocol Flags
        Bits 0-6 are currently reserved, and should be cleared (`0`).
        Bit 7 is cleared (`0`) for requests, and set (`1`) for responses. 
        It tells how field `ts_rc` should be

The rest of the message is the following

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | PREAMBLE -- see above                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | PREAMBLE -- continued                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |TSRC                           |Cell ID                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Element ID                                                     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Element ID (continued)                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Sequence number                                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Transaction ID                                                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Description of the fields:

    TSRC

        Action:

          bits 0-13 encode the action involved in the request/response.

        For requests:

            bits 14-15 encode the (general) operation type that we are
                       required to perform:

             `0`: UNDEFINED
             `1`: CREATE/UPDATE
             `2`: DELETE
             `3`: RETRIEVE
    
        For replies:

            bit 14 is reserved for future usage and should be cleared (`0`).

            bit 15 tells if this is a success or a failure/error:

            `0`: SUCCESS
            `1`: FAILURE

    CELL ID

        Cell identifier

    ELEMENT ID

        Element identified

    SEQUENCE NUMBER

        Progressive sequence number

    TRANSACTION ID

        Unique identifier of a (pending) transaction.

The common header is then followed by a variable number of information elements encoded
in a Type Length Value (TLV) structure

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | TLV data (not part of the TLV header -- indicated here just   |
    | for illustration purposes)                                    |
    .                                                               .
    .                                                               .
    .                                                               .
    |                                                               |
    | (note: length is not necessarily a multiple of _______________|
    | 4 bytes)                                      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

## Messages
    
Here follows the list of currently supported actions and their encoding.

| Service           | Action | 
|-------------------|:------:|
| Hello             |  0x01  | 
| Capabilities      |  0x02  | 

### Hello Service

The Hello message acts as both connection initiator and keepalive. 

The Hello message is sent as the first message exchanged between the agent and the controller during the connection setup.

The controller has the possibility to reject the agent. In this case the controller terminates the connection. 

Life-cycle:

     Agent              Controller
        | Request          |
        +----------------->|
        |                  |
        |            Reply |
        |<-----------------+
        |                  |
        v                  v

Request:

    Bits 0-13: 0x01 (HELLO)
    Bits 14-15: 0x00 (UNDEFINED)

  TLVs:

    Hello Period

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Period                                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    Fields:

        PERIOD
            The hello period of the agent

Reply:

    Bits 0-13: 0x01 (HELLO)
    Bits 14-15: 0x00/0x01 (SUCCESS/FAIL)

  TLVs:

    Hello Period

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Period                                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    Fields:

        PERIOD
            The new hello period of the agent


### Capabilities Service

The Capabilities service is used to retrieve the configuration of the eNB.

Life-cycle:

    Controller           Agent
        | Request          |
        +----------------->|
        |                  |
        |            Reply |
        |<-----------------+
        |                  |
        v                  v

Request:

    Bits 0-13: 0x02 (CAPABILITIES)
    Bits 14-15: 0x00 (UNDEFINED)

  TLVs:

    No TLVs defined


Reply:

    Bits 0-13: 0x01 (CAPABILITIES)
    Bits 14-15: 0x00/0x01 (SUCCESS/FAIL)

  TLVs:

    Cell Configuration

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |dl_earfcn                                                      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |n_prbs         |
    +-+-+-+-+-+-+-+-+

    Fields:

        DL_EARFCN
            EARFCN code for DL

        N_PRBS
            Number of Physical Resource Blocks (6,15,25,50,75,100)

## Obsolete messages (to be revised)

### Handover message

The Handover message is used to trigger an X2 handover at the eNB.

Life-cycle:

    Controller         Agent_src          Agent_dst
        | Request          |                  |
        +----------------->|                  |
        |                  |----------------->|
        |        Neg Reply |                  |
        |<-----------------+                  |
        |                  |    Neg/Pos Reply |
        |<-----------------|------------------|
        v                  v                  v

Request:

    Operation: UNSPECIFIED

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |               RNTI            |         Target eNB         -->|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |<--       Target eNB           |         Target PCI            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |     Cause     | | | | | | | | | | | | | | | | | | | | | | | | |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    RNTI
        16-bits Radio Network Temporary Identifier of the target UE.
        
    TARGET ENB
        32-bits ID of the eNB where the RNTI should be transfered to.
        
    TARGET PCI
        16-bits Physical Cell Identificator of the target cell.
        
    CAUSE
        8-bits field which identify the cause of Handover.

Reply:

    Operation: SUCCESS/FAIL

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          Source eNB                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |            Origin PCI         |          Origin RNTI          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |           Target RNTI         | | | | | | | | | | | | | | | | |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    SOURCE ENB
        32-bits ID of the Handover initiator.
        
    ORIGIN PCI
        16-bits Physical Cell ID of the Handover initiator.
        
    ORIGIN RNTI
        16-bits original Radio Network Temporary Identifiers.
        
    TARGET RNTI
        16-bits Radio Network Temporary Identifiers assumed by the UE after the
        Handover operation. 

### Cell Measurement message

The Cell measurement message is used to request cell measurements. Different measurements can be taken for each cell.

Life-cycle:

    Controller           Agent
        | Request          |
        +----------------->|
        |                  |
        |            Reply |
        |<-----------------+
        |                  |
        v                  v

Request:

    Operation: ADD/SET/GET/REM

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                   Zero or more TLV entries                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    TLV TOKENS:
        One of the following tokens:
            EP_TLV_CELL_PRB_REQ

Reply:

    Operation: SUCCESS/FAIL

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                   Zero or more TLV entries                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    TLV TOKENS:
        One of the following tokens:
            EP_TLV_CELL_PRB_REPORT

### UE report message

The UE report message tells the agent to start reporting changes about the UEs connected to the eNB. Even if the IMSI field is present in the message, the particular agent implementation can decide to leave it filled with zero.

This message carry a complete view of the situation, and not only a difference between two status. This means that the UEs listed here are the current ones connected through the eNB.

Life-cycle:

    Controller           Agent
        | Request          |
        +----------------->|
        |                  |
        |            Reply |
        |<-----------------+
        |                  |
        v                  v

Request:

    Operation: ADD/REM

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                             ID                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Fields:

    ID
        A generic 32-bits field used as identificator of the Hello procedure.
        This element is currently left to 0.

Reply:

    Operation: SUCCESS/FAIL

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      UE report TLVs                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Fields:

    TLV TOKENS:
        One of the following tokens:
            EP_TLV_UE_REP_ID

### UE measurement 

The UE measurement message allows to perform and retrieve information about RCC
measurements performed by a single UE. The measurement is a RRC reconfiguration
message issued to an UE which follows the LTE standards.

Life-cycle:

    Controller           Agent
        | Request          |
        +----------------->|
        |                  |
        |            Reply |
        |<-----------------+
        |                  |
        v                  v

Request:

    Operation: ADD/SET/GET/REM

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                   Zero or more TLV entries                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    TLV TOKENS:
        One of the following tokens:
            EP_TLV_UE_RRC_MEAS_REQ

Reply:

    Operation: SUCCESS/FAIL
    
     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                   Zero or more TLV entries                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    TLV TOKENS:
        One of the following tokens:
            EP_TLV_UE_RRC_MEAS_REPORT
