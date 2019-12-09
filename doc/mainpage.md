
@mainpage Empower Agent source documentation

@section Overview

This is a kit of C++ libraries implementing:

* generic cross-platform utilities to manage memory buffers where data
  is encoded/decoded (see namespace NetworkLib)

* an agent specific to the Empower project and a network protocol
  to exchange control messages with it (see namespace Empower::Agent).
  Messages are encoded in a custom binary form.

@section wireformat Protocol messages wire format

Each encoded message is made of:

1. a preamble (see Empower::Agent::ReferenceStructs::Preamble).

2. a common header (see Empower::Agent::ReferenceStructs::CommonHeader)

3. zero or more Type-Length-Value (TLV) fields depending on the kind
   of message. Examples of TLVs are

   * Empower::Agent::TLVError
   * Empower::Agent::TLVKeyValueStringPairs
