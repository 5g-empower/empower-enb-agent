#include <empoweragentproto/empoweragentproto.hh>
#include <iostream>

namespace AGT = Empower::Agent;
namespace NL = Empower::NetworkLib;

int main(void) {

    AGT::IO io;

    // Leave default destination address (127.0.0.1) and port (2210).

    try {

        std::cout << "Opening listening connection\n";

        std::cout << "Connection accepted\n";

        while (true) {

            // Retry connecting until we make it.
            if (io.isConnectionClosed()) {
                io.openListeningSocket();
                io.acceptConnectionIfNeeded();

                if (io.isConnectionClosed()) {
                    continue;
                }
            }

            auto ioBuffer = AGT::IO::makeMessageBuffer();

            // Prepare a request for the ECHO SERVICE
            AGT::MessageEncoder messageEncoder(ioBuffer);

            messageEncoder.header()
                .messageClass(AGT::MessageClass::REQUEST_GET)
                .entityClass(AGT::EntityClass::ECHO_SERVICE);

            auto tlv =
                AGT::TLVBinaryData().stringData("Is there anybody out there?");

            messageEncoder.add(tlv).end();

            std::cout << "Sending message\n" << messageEncoder.data();

            // Write a message to the socket
            io.writeMessage(messageEncoder.data());

            // Wait for a reply
            NL::BufferView reply = io.readMessage(ioBuffer);

            if (reply.size() > 0) {

                AGT::MessageDecoder messageDecoder(reply);

                std::cout << "Got back a message\n";

                if (messageDecoder.isSuccess() &&
                    (messageDecoder.header().entityClass() ==
                     AGT::EntityClass::ECHO_SERVICE)) {

                    AGT::TLVBinaryData tlv;
                    messageDecoder.get(tlv);

                    std::cout << "Got back message: " << tlv.stringData()
                              << '\n';

                } else if (messageDecoder.isFailure()) {
                    AGT::TLVError err;
                    messageDecoder.get(err);

                    std::cout << "Errcode is " << err.errcode()
                              << ", message: " << err.message() << '\n';
                } else {
                    std::cout << "Unexpected reply\n";
                }
            } else {
                io.closeConnection();
            }

            io.sleep();
        }

    } catch (std::exception &e) {
        std::cerr << "Caught exception in main agent loop: " << e.what()
                  << '\n';
    }

    io.closeConnection();

    return 0;
}
