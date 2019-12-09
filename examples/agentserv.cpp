#include <empoweragentproto/empoweragentproto.hh>
#include <iostream>

namespace AGT = Empower::Agent;
namespace NL = Empower::NetworkLib;

int main(void) {

    AGT::IO io;

    try {
        auto readBuffer = io.makeMessageBuffer();
        auto writeBuffer = io.makeMessageBuffer();

        for (;;) {
            bool performPeriodicTasks = false;
            bool dataIsAvailable = false;

            if (io.isConnectionClosed()) {
                io.openSocket();
            }

            // Retest
            if (io.isConnectionClosed()) {
                // Connection still closed. Sleep for a while
                io.sleep();
                performPeriodicTasks = true;
            } else {
                // The connection is opened. Let's see if there's data
                dataIsAvailable = io.isDataAvailable();

                if (!dataIsAvailable) {
                    // Timeout expired
                    performPeriodicTasks = true;
                }
            }

            if (dataIsAvailable) {

                // Read a message
                auto messageBuffer = io.readMessage(readBuffer);

                if (!messageBuffer.empty()) {

                    std::cout << "Received message\n" << messageBuffer;

                    // Decode the message
                    AGT::MessageDecoder messageDecoder(messageBuffer);

                    if (!messageDecoder.isFailure()) {

                        switch (messageDecoder.header().entityClass()) {
                        case AGT::EntityClass::ECHO_SERVICE:
                            std::cout << "Got message class for ECHO SERVICE\n";

                            {
                                AGT::TLVBinaryData tlv;
                                messageDecoder.get(tlv);
                                tlv.stringData(tlv.stringData() +
                                               " Here I am!");

                                AGT::MessageEncoder messageEncoder(writeBuffer);

                                messageEncoder.header()
                                    .messageClass(
                                        AGT::MessageClass::RESPONSE_SUCCESS)
                                    .entityClass(
                                        AGT::EntityClass::ECHO_SERVICE);

                                messageEncoder.add(tlv).end();

                                std::cout << "Sending back reply\n"
                                          << messageEncoder.data();

                                // Write a message to the socket
                                size_t len =
                                    io.writeMessage(messageEncoder.data());

                                std::cout << "Wrote " << len << " bytes\n";
                            }

                            break;

                        default:
                            std::cout << "Got unmanaged message class\n";
                            break;
                        }
                    }
                }
            } else if (performPeriodicTasks) {
                // Timeout expired
                std::cout << "AGENT: still waiting for messages... "
                             "(isConnectionClosed() is "
                          << io.isConnectionClosed() << ")\n";

            } else {
            }
        }

    } catch (std::exception &e) {
        std::cerr << "Caught exception in main agent loop: " << e.what()
                  << '\n';
    }

    return 0;
}
