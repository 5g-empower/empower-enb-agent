

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

This repository includes the 5G-EmPOWER eNodeB Agent and the Protocols libraries.

### License
Code is released under the Apache License, Version 2.0.


# Overview

The 5G-EmPOWER platform clearly decouples control-plane operations, which are left at the air interface, from management-plane operations, which are consolidated on top of the operating system layer (i.e., the SD-RAN controller).

An agent is introduced in the eNodeB to implement the management actions defined by the operating system. Communication between the agent and the operating system happens over the protocol described in this readme,

The 5G-EmPOWER protocol is layered on top of the Transmission Control Protocol (TCP) and can use the Transport Layer Security (TLS) protocol. The management planes should listen on TCP port 4433 for RAN elements that want to set up a connection.

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

# Integration with srsLTE

## Overview

Start from `.../srsenb/src/empoweragent.cc` and `.../srsenb/hdr/empoweragent.h`, and`.../srsenb/src/main.cc` and `.../srsenb/src/enb.cc`

1. The Empower agent is started in a single separate thread

2. The agent periodically attempts to (re)open a TCP connection to the
   address and port of the controller every `delay` microseconds
   (address, port and delay are specified in a new section of the
   srsenb configuration file -- see below).

3. Once a connection is opened, the agent waits up to `delayms` microseconds for an incoming message from the controller. If a message is availabile, it is received and processed immediately.

4. A separate thread is created to periodically send HELLO messages to the controller. The period is again specified with the `delayms` parameter

The srsenb configuration has been extended with a new section for the
Empower agent (those in the example below are the default values):

```
[empoweragent]
controller_addr = 127.0.0.1
controller_port = 2110
delayms = 1500
```
## Build

Build and install SRSLte as usual *after ensuring* that the `empower-enb-agent` library and headers have been built and installed in `/usr/local` (for the moment, the changes to the main `CMakeLists.txt` explicitly look for the package `EmpowerAgentProto` in `/usr/local/include/empoweragentproto` and `/usr/local/lib`


# Message Format

All the messages start with a common header. The format of the header is the following:

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Version       |Flags          |TSRC                           |
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

    TSRC

        Action:

          bits 0-13 encode the action involved in the request/response.

        For requests:

            bits 14-15 encode the (general) operation type that we are
                       required to perform:

             `0`: UNDEFINED
             `1`: UPDATE
             `2`: CREATE
             `3`: DELETE
             `4`: RETRIEVE
    
        For replies:

            bit 14 is reserved for future usage and should be cleared (`0`).

            bit 15 tells if this is a success or a failure/error:

            `0`: SUCCESS
            `1`: FAILURE
            
The rest of the message is the following

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | PREAMBLE -- see above                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | PREAMBLE -- continued                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Padding                       |Element ID                     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Element ID (continued)                                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Sequence number                                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Transaction ID                                                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Description of the fields:

    ELEMENT ID

        Element identified (a 48-bit Ethernet address)

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

| Service               | Action | 
|-----------------------|:------:|
| HELLO                 |  0x00  | 
| CAPABILITIES          |  0x01  | 
| UE REPORTS            |  0x02  | 
| UE MEASUREMENTS       |  0x03  | 
| MAC PRB UTILIZATION   |  0x04  | 
| HANDOVER              |  0x05  | 

### Hello service

The Hello message acts as both connection initiator and keepalive.  The Hello message is sent as the first message exchanged between the agent and the controller during the connection setup. The controller has the possibility to reject the agent. In this case the controller terminates the connection. 

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

    Bits 0-13: 0x00 (HELLO)
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
            The hello period of the agent (in ms)

Reply:

    Bits 0-13: 0x00 (HELLO)
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
            The new hello period of the agent (in ms)


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

    Bits 0-13: 0x01 (CAPABILITIES)
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
    |DL_EARFCN                                                      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |DL_EARFCN                                                      |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   
    |PCI                            |PRB            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    Fields:

        DL_EARFCN
            EARFCN code for DL
            
        DL_EARFCN
            EARFCN code for UL

        PCU
            The physical cell id
            
        PRB
            Number of Physical Resource Blocks (6,15,25,50,75,100)

### UE reports service

The UE report request message tells the agent to list all connected UE. The agent also sends a UE Report message every time a UE connects/disconnectes.

Life-cycle:

    Controller           Agent
        | Request          |
        +----------------->|
        |                  |
        |            Reply |
        |<-----------------+
        |                  |
        |   Reply (new UE) |
        |<-----------------+
        v                  v

Request:

    Bits 0-13: 0x02 (UE REPORTS)
    Bits 14-15: 0x00 (UNDEFINED)

  TLVs:

    No TLVs defined

Reply:

    Bits 0-13: 0x02 (UE REPORTS)
    Bits 14-15: 0x00/0x01 (SUCCESS/FAIL)

TLVs:

    Cell Configuration

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |IMSI                                                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |IMSI (continued)                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |TMSI                                                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |RNTI                           |PCI                            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Status         |
    +-+-+-+-+-+-+-+-+
    
    Fields:

        IMSI
            The UE IMSI
            
        TMSI
            The UE TMSI
            
        RNTI
            The UE RNTI

        STATUS
            The status opcode (0x1=connected, 0x2=disconnected)
            
        PCI
           The physical cell id

### MAC PRB utilization service

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

    Bits 0-13: 0x04 (MAC PRB UTILIZATION)
    Bits 14-15: 0x00 (UNDEFINED)

 TLVs:

    No TLVs defined

Reply:

    Bits 0-13: 0x04 (MAC PRB UTILIZATION)
    Bits 14-15: 0x00/0x01 (SUCCESS/FAIL)

TLVs:

    Cell Utilization

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |DL_PRB_COUNTER                                                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |UL_PRB_COUNTER                                                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |PRB                           |PCI                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
    Fields:

        DL_PRB_COUNTER
            The progressive counter of the DL PRBs scheduled so far
            
        UL_PRB_COUNTER
            The progressive counter of the UL PRBs scheduled so far
            
        PRB
            The PRB available on the cell
            
        PCI
           The physical cell id


### UE measurements service

The UE measurement message allows to perform and retrieve information about RCC measurements performed by a UE.

Life-cycle:

    Controller           Agent
        | Request (CREATE) |
        +----------------->|
        |                  |
        |  Reply (MEAS_ID) |
        |<-----------------+
        |                  |
        | Report (MEAS_ID) |
        |<-----------------+
        |                  |
        v                  v

Request:

    Bits 0-13: 0x03 (UE MEASUREMENTS)
    Bits 14-15: 0x02/0x04 (CREATE/DELETE)

TLVs:

    Measurement config (serving cell only)

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |RNTI                           |INTERVAL       |AMOUNT         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
    Fields:

        RNTI
            The UE RNTI to track
            
        INTERVAL
            The interval as specified by 3GPP standards
            
        AMOUNT
            The number of reports as specified by 3GPP standards

Reply:
    
    Bits 0-13: 0x03 (UE MEASUREMENTS)
    Bits 14-15: 0x00/0x01 (SUCCESS/FAIL)
    
TLVs:

    Measurement ID (sent back to the controller after the new measurement has been created)

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |RNTI                           |MEAS_ID/RESULT |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
    Fields:

        RNTI
            The UE RNTI to track
            
        MEAS_ID/RESULT 
            The measurement ID or the error code if the measurement could not be created

    Measurement Report

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Type                           |Length                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |RSRP                           |RSRQ                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |PCI                            |MEAS_ID          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
    Fields:

        MEAS_ID
            The measurement ID
            
        PCI
            The physical cell id monitored 
            
        MEAS_ID/RESULT 
            The measurement ID or the error code if the measurement could not be created
        
        RSRP/RSRQ 
            The measurements
            
### Handover service

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

    Bits 0-13: 0x05 (HANDOVER)
    Bits 14-15: 0x00 (UNDEFINED)

TLVs

    Handover trigger
    
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
    
    Bits 0-13: 0x05 (HANDOVER)
    Bits 14-15: 0x00/0x01 (SUCCESS/FAIL)
    
  TLVs:
    
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


