#include <empoweragentproto/networklib.hh>
#include <empoweragentproto/protocol.hh>
#include <sstream>
#include <utility>

namespace Empower {
namespace Agent {

CommonHeaderDecoder::CommonHeaderDecoder(
    const NetworkLib::BufferView &messageData)
    : mBufferView(messageData) {
    throwIfBufferIsUnsuitable(NETWORKLIB_CURRENT_FUNCTION);
}

CommonHeaderDecoder::CommonHeaderDecoder(NetworkLib::BufferView &&messageData)
    : mBufferView(std::move(messageData)) {
    throwIfBufferIsUnsuitable(NETWORKLIB_CURRENT_FUNCTION);
}

std::uint8_t CommonHeaderDecoder::version() const {
    // Bounds already checked on construction
    return mBufferView.getUint8At_nocheck(CommonHeader::versionOffset);
}

std::uint8_t CommonHeaderDecoder::flags() const {
    // Bounds already checked on construction
    return mBufferView.getUint8At_nocheck(CommonHeader::flagsOffset);
}

std::uint16_t CommonHeaderDecoder::tsRc() const {
    // Bounds already checked on construction
    return mBufferView.getUint16At_nocheck(CommonHeader::tsrcOffset);
}

std::size_t CommonHeaderDecoder::totalLengthBytes() const {
    // Bounds already checked on construction
    return mBufferView.getUint32At_nocheck(CommonHeader::lengthOffset);
}

std::uint32_t CommonHeaderDecoder::sequence() const {
    // Bounds already checked on construction
    return mBufferView.getUint32At_nocheck(CommonHeader::sequenceOffset);
}

std::uint64_t CommonHeaderDecoder::elementId() const {
    // Bounds already checked on construction
    return mBufferView.getUint64At_nocheck(CommonHeader::elementIdOffset);
}

std::uint32_t CommonHeaderDecoder::transactionId() const {
    // Bounds already checked on construction
    return mBufferView.getUint32At_nocheck(CommonHeader::transactionIdOffset);
}

MessageClass CommonHeaderDecoder::messageClass() const {
    MessageClass result = MessageClass::INVALID;

    if ((flags() & flagsRequestOrResponseMask) == 0) {
        // This is a request

        // Consider only bits 14-15.
        const std::uint8_t op = (tsRc() >> 13) & 0x03;
        switch (op) {
        case 0:
            result = MessageClass::REQUEST_SET;
            break;

        case 1:
            result = MessageClass::REQUEST_ADD;
            break;

        case 2:
            result = MessageClass::REQUEST_DEL;
            break;

        case 3:
            result = MessageClass::REQUEST_GET;
            break;
        }

    } else {
        // This is a response. Consider just bit 15
        const std::uint8_t op = (tsRc() >> 14);

        if (op == 0) {
            result = MessageClass::RESPONSE_SUCCESS;
        } else {
            result = MessageClass::RESPONSE_FAILURE;
        }
    }

    return result;
}

EntityClass CommonHeaderDecoder::entityClass() const {

    // Consider only bits 0-13
    const std::uint16_t entity = tsRc() & 0x3FFF;
    return EntityClass(entity);
}

void CommonHeaderDecoder::throwIfBufferIsUnsuitable(const char *method) {
    // Catch some quirks early
    if (mBufferView.size() < 28) {
        std::ostringstream err;
        err << method
            << ": called with "
               "BufferView.size() == "
            << mBufferView.size() << " (min size is 28)";
        throw std::length_error(err.str());
    }

    if (version() != 2) {
        std::ostringstream err;
        err << method << ": wrong version (version is " << +(version())
            << ", should be 2)";
        throw std::runtime_error(err.str());
    }
}

/****/

CommonHeaderEncoder::CommonHeaderEncoder(
    const NetworkLib::BufferWritableView &messageData)
    : mBufferWritableView(messageData) {
    throwIfBufferIsUnsuitable(NETWORKLIB_CURRENT_FUNCTION);
    setDefaults();
}

void CommonHeaderEncoder::setDefaults() {
    version(2).cellIdentifier(0).sequence(0).elementId(0).transactionId(0);
}

CommonHeaderEncoder::CommonHeaderEncoder(
    NetworkLib::BufferWritableView &&messageData)
    : mBufferWritableView(std::move(messageData)) {
    throwIfBufferIsUnsuitable(NETWORKLIB_CURRENT_FUNCTION);
    setDefaults();
}

CommonHeaderEncoder &CommonHeaderEncoder::totalLengthBytes(std::size_t s) {
    // Bounds already checked on construction
    mBufferWritableView.setUint32At_nocheck(CommonHeader::lengthOffset, s);
    return *this;
}

CommonHeaderEncoder &CommonHeaderEncoder::version(std::uint8_t version) {
    mBufferWritableView.setUint8At_nocheck(CommonHeader::versionOffset,
                                           version);
    return *this;
}

CommonHeaderEncoder &
CommonHeaderEncoder::messageClass(MessageClass messageClass) {

    // Save old value of bits 0-6 of flags
    std::uint8_t savedFlags =
        mBufferWritableView.getUint8At_nocheck(CommonHeader::flagsOffset) &
        0x7F;

    // Save old value of bits 0-13 of ts_rc
    const std::uint16_t savedBits =
        mBufferWritableView.getUint16At_nocheck(CommonHeader::tsrcOffset) &
        0x3FFF;

    bool isRequest = false;
    std::uint16_t highBits = 0;

    switch (messageClass) {
    case MessageClass::INVALID: {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION
            << ": called with invalid message class";
        throw std::invalid_argument(err.str());
    } break;

    case MessageClass::REQUEST_SET:
        isRequest = true;
        highBits = 0;
        break;

    case MessageClass::REQUEST_ADD:
        isRequest = true;
        highBits = 1;
        break;

    case MessageClass::REQUEST_DEL:
        isRequest = true;
        highBits = 2;
        break;

    case MessageClass::REQUEST_GET:
        isRequest = true;
        highBits = 3;
        break;

    case MessageClass::RESPONSE_SUCCESS:
        isRequest = false;
        highBits = 0;
        break;

    case MessageClass::RESPONSE_FAILURE:
        isRequest = false;
        highBits = 2;
        break;
    }

    if (isRequest) {
        // Clear bit 7 in the preamble to mark this as a request
        mBufferWritableView.setUint8At_nocheck(CommonHeader::flagsOffset,
                                               savedFlags);
    } else {
        // Set bit 7 in the preamble to mark this a a response
        mBufferWritableView.setUint8At_nocheck(CommonHeader::flagsOffset,
                                               savedFlags | 0x80);
    }

    // Set bits 14-15
    mBufferWritableView.setUint16At_nocheck(CommonHeader::tsrcOffset,
                                            savedBits | (highBits << 14));

    return *this;
}

CommonHeaderEncoder &CommonHeaderEncoder::entityClass(EntityClass entityClass) {

    // Save old value of bits 14-15
    const std::uint16_t savedBits =
        mBufferWritableView.getUint16At_nocheck(CommonHeader::tsrcOffset) &
        0xC000;
    const std::uint16_t newValue =
        (static_cast<std::uint16_t>(entityClass) & 0x3FFF) | savedBits;

    mBufferWritableView.setUint16At_nocheck(CommonHeader::tsrcOffset, newValue);
    return *this;
}

void CommonHeaderEncoder::throwIfBufferIsUnsuitable(const char *method) {
    // Catch some quirks early
    if (mBufferWritableView.size() < CommonHeader::totalLength) {
        std::ostringstream err;
        err << method
            << ": called with "
               "BufferWritableView.size() == "
            << mBufferWritableView.size() << " (min size is "
            << CommonHeader::totalLength << ')';
        throw std::length_error(err.str());
    }
}

CommonHeaderEncoder &CommonHeaderEncoder::sequence(std::uint32_t v) {
    mBufferWritableView.setUint32At_nocheck(CommonHeader::sequenceOffset, v);
    return *this;
}

CommonHeaderEncoder &CommonHeaderEncoder::elementId(std::uint64_t v) {
    // Bounds already checked on construction
    mBufferWritableView.setUint64At_nocheck(CommonHeader::elementIdOffset, v);
    return *this;
}

CommonHeaderEncoder &CommonHeaderEncoder::transactionId(std::uint32_t v) {
    // Bounds already checked on construction
    mBufferWritableView.setUint32At_nocheck(CommonHeader::transactionIdOffset,
                                            v);
    return *this;
}

} // namespace Agent
} // namespace Empower
