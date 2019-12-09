#include <empoweragentproto/empoweragentproto.hh>
#include <iostream>

namespace agent = Empower::Agent;
namespace n = Empower::NetworkLib;

n::BufferView encodeResponseFailure(std::string errorMessage,
                                    std::uint16_t errorCode) {

    agent::IO io;
    agent::MessageEncoder messageEncoder(io.makeMessageBuffer());

    messageEncoder.header()
        .messageClass(agent::MessageClass::RESPONSE_FAILURE)
        .entityClass(agent::EntityClass::ECHO_SERVICE);

    // Add the Error TLV
    messageEncoder
        .add(agent::TLVError().message(errorMessage).errcode(errorCode))
        .end();

    return messageEncoder.data();
}

int main(void) {

    n::BufferView result = encodeResponseFailure("12345", 42);

    std::cout << result << '\n';

    agent::MessageDecoder messageDecoder(result);

    if (messageDecoder.isFailure()) {
        agent::TLVError tlvError;

        messageDecoder.get(tlvError);

        std::cout << "Failure (" << tlvError.errcode()
                  << "): " << tlvError.message() << '\n';
    }
}
