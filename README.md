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

# How To Use

## Pre-requisites

In order to successfully build the eNodeB Agent you need to install the linux build tools

`sudo apt-get install build-essential`

## Build from source

The standard build assumes that you want to install the software and the necessary headers in the default directories `/usr/include` for headers, and `/usr/lib` for the shared objects. 

You can the defaults by modifying the `INCLDIR` and `INSTDIR` variables present in the Makefile .

To build the software run the following command:

`make`

## Install

After having built the software, to install it run the following command:

`sudo make install`

## Uninstalling

You can uninstall the software run the following command:

`sudo make uninstall`

## Overview

The 5G-EmPOWER platform clearly decouples control-plane operations, which are left at the air interface, from management-plane operations, which are consolidated on top of the operating system layer. 

An agent is introduced in the eNB to implement the management actions defined by the operating system. Communication between the agent and the operating system happens over the OpenEmpower protocol.

The OpenEmpower protocol is layered on top of the Transmission Control Protocol (TCP) and can use the Transport Layer Security (TLS) protocol. The management planes should listen on TCP port 4433 for RAN elements that want to set up a connection.

All the messages in OpenEmpower start with a common header. The format of the header is the following:

    0               1               2               3
    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |    Version    |    Action     |            Opcode            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |                          Element ID                          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |                          Element ID                          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |            Cell ID            |            Length            |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |                         Transaction ID                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |                            Sequence                          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |                            Interval                          |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
    |                            Payload                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
 
 Description of the fields:

    VERSION
        Protocol version. Always set to 1.
        
    ACTION
        Message action to be performed (see below).
    
    Opcode
        Operation. The first byte can be one of the following values (notice some values are valid only for certain actions)
            UNSPECIFIED, unspecified operation
            ADD, add a new entity (0x01)
            GET, get one or more values (0x02)
            SET, update the state (0x03)
            REM, remove an entity (0x04)
            SUCCESS, action completed with success (0x05)
            FAIL, error (0x07)
        The second byte depends on the action. For example it can be used to specify the type of error.
        
    Element ID
        64-bits identificator of a base station.
        
    CELL ID
        16-bits identificator for a cell within a base station. This usually
        matches the Physical Cell Id.
        
    LENGTH
        Message length in bytes.
        
    TRANSACTION ID
        32-bits token associated with a certain request. Replies must use the same ID as in the request in order to facilitate pairing. This is necessary because all the communications using the OpenEmpower protocol are asynchronous.
        
    SEQUENCE 
        32-bits sequence number. Incremented by one for each message sent by either controller or agent.

    INTERVAL
        Interval for the execution of periodic actions (in ms).

    PAYLOAD
        The message payload. 

## Messages

The protocols are built around 3 major events that can occurs in the agent sub-system, which are:

* SINGLE EVENTS. These are simple, standalone, events, requested by the controller/agent and notified back immediately by the agent/controller.
    
* SCHEDULED EVENTS. These are events which will be requested once, and performed periodically. 

* TRIGGERED EVENTS. These events should ideally enable/disable a functionality in the agent. Such component works with threshold or particular event which can happens in a not predictable way, like the connection of a new UE. 
    
Here follows the list of currently supported message and their type.

|                   | Single | Scheduled | Triggered | Action |
|-------------------|:------:|:---------:|:---------:|:------:|
| Hello             |   X    |           |           |  0x01  |
| eNB capabilities  |   X    |           |           |  0x02  |
| Ue reports        |        |           |     X     |  0x03  |
| Handover          |   X    |           |           |  0x04  |
| Cell measurements |        |     X     |           |  0x20  |
| Ue measurements   |        |           |     X     |  0x21  |

### Hello message

The Hello message acts as both connection initiator and keepalive. 

The Hello message is sent as the first message exchanged between the agent and the controller during the connection setup.

The controller has the possibility to reject the agent. In this case the controller terminates the connection. 

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

    Operation: UNSPECIFIED

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
    |                             ID                                |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    ID
        A generic 32-bits field used as identificator of the Hello procedure.
        This element is currently left to 0.

### eNB Capabilities message

The eNB Capabilities message is used to retrieve the configuration of the eNB. The capabilities can then be activated/deactivated by the controller.

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

    Operation: UNSPECIFIED

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
    |                   Zero or more TLV entries                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
Fields:

    TLV TOKENS:
        One of the following tokens:
            EP_TLV_CELL_CAP
            EP_TLV_RAN_CAPS

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
