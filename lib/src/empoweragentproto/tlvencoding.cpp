#include <empoweragentproto/tlvencoding.hh>

// Just for TLVType::NONE
#include <empoweragentproto/tlvs.hh>

namespace Empower {
namespace Agent {

std::ostream &operator<<(std::ostream &ostr, const TLVType &v) {
    ostr << static_cast<std::uint16_t>(v);
    return ostr;
}

MessageEncoder::MessageEncoder(NetworkLib::BufferWritableView buffer)
    : mBuffer{buffer}, mHeaderEncoder{buffer}, mCurrentOffset{
                                                   mHeaderEncoder.size()} {}

MessageEncoder &MessageEncoder::add(TLVBase &tlv) {
    // Get a BufferWritableView of the 'free' space at the end of the buffer
    auto subBuffer_TL = mBuffer.getSub(mCurrentOffset);

    // Leave out the type and the length
    auto subBuffer_V = subBuffer_TL.getSub(TLVHeader::dataOffset);

    // Encode the data and keep the length
    auto tlvTotalLength = TLVHeader::headerLength + tlv.encode(subBuffer_V);

    // Set type and length
    subBuffer_TL.setUint16At(TLVHeader::typeOffset,
                             static_cast<std::uint16_t>(tlv.type()));
    subBuffer_TL.setUint16At(TLVHeader::lengthOffset, tlvTotalLength);

    // Advance the current offset
    mCurrentOffset += tlvTotalLength;

    return *this;
}

void MessageEncoder::end() {
    // Just set the total length in the message common header
    mHeaderEncoder.totalLengthBytes(mCurrentOffset);
}

/**********************************************************************/

MessageDecoder::MessageDecoder(NetworkLib::BufferView buffer)
    : mBuffer(buffer),
      mHeaderDecoder(buffer), mCurrentOffset{mHeaderDecoder.size()} {}

MessageDecoder &MessageDecoder::get(TLVBase &obj) {

    // Get the type and length of the encoded TLV
    auto subBuffer_TL = mBuffer.getSub(mCurrentOffset, TLVHeader::headerLength);

    TLVType tlvType =
        static_cast<TLVType>(subBuffer_TL.getUint16At(TLVHeader::typeOffset));
    std::size_t tlvLength = subBuffer_TL.getUint16At(TLVHeader::lengthOffset);

    if (tlvType != obj.type()) {
        // Mismatched TLV type...
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": encoded TLV has type "
            << tlvType << ", expected TLV has type " << obj.type();
        throw std::runtime_error(err.str());
    }

    auto subBuffer_V = mBuffer.getSub(mCurrentOffset + TLVHeader::headerLength,
                                      tlvLength - TLVHeader::headerLength);
    std::size_t reportedLength =
        TLVHeader::headerLength + obj.decode(subBuffer_V);

    if (reportedLength != tlvLength) {
        // Mismatched TLV length when decoding...
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": encoded TLV has length "
            << tlvLength << ", but decoding gives length " << reportedLength;
        throw std::runtime_error(err.str());
    }

    mCurrentOffset += tlvLength;

    return *this;
}

TLVType MessageDecoder::getNextTLVType() const {

    if ((mCurrentOffset + TLVHeader::headerLength) >= mBuffer.size()) {
        // We are already at the end, so there's no next TLV
        return TLVType::NONE;
    }

    // Get the type and length of the encoded TLV
    auto subBuffer_TL = mBuffer.getSub(mCurrentOffset, TLVHeader::headerLength);

    TLVType tlvType =
        static_cast<TLVType>(subBuffer_TL.getUint16At(TLVHeader::typeOffset));
    std::size_t tlvLength = subBuffer_TL.getUint16At(TLVHeader::lengthOffset);

    if ((mCurrentOffset + tlvLength) > mBuffer.size()) {
        // TLV is truncated in the buffer.
        return TLVType::NONE;
    }

    return tlvType;
}

} // namespace Agent
} // namespace Empower
