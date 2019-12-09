#ifndef EMPOWER_AGENT_TLVENCODING_HH
#define EMPOWER_AGENT_TLVENCODING_HH

#include <empoweragentproto/networklib.hh>
#include <empoweragentproto/protocol.hh>
#include <ostream>

namespace Empower {
namespace Agent {

// Forward declaration of main enum assigning an ID to each TLV type
// (actual declaration is in `src/empoweragentproto/tlvs.cpp` together
// with TLVs).
enum class TLVType : std::uint16_t;

/// @brief Obtain a textual representation of a TLVtype
std::ostream &operator<<(std::ostream &ostr, const TLVType &v);

/**
 * @brief Basic interface for all classes representing TLVs.
 *
 * The general idea is that a class representing a TLV:
 *
 * * holds the data relevant for that TLV;
 *
 * * knows its identifier (i.e. TLVType);
 *
 * * knows how to encode and decode data to/from a
 *   BufferView/BufferWritableView.
 *
 * Classes TLVEncoder and TLVDecoder provide a generic
 * way to encode/decode a sequence of TLVs.
 */
class TLVBase {
  public:
    /// @brief Return the ID identifying a TLV type
    virtual TLVType type() const = 0;

    /// @brief Encode the TLV data at the beginning of the given
    /// NetworkLib::BufferWritableView.
    ///
    /// Throw exception if the buffer is too small.
    ///
    /// @return The number of bytes used for encoding.
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) = 0;

    /// @brief Decode the data stored in the given
    /// NetworkLib::BufferView for this TLV type.
    ///
    /// @return The number of bytes decoded (should be equal to
    /// `buffer.size()`).
    virtual std::size_t decode(NetworkLib::BufferView buffer) = 0;

    virtual ~TLVBase() {}
};

/**
 * @brief A class which helps encoding a message made of a generic
 *        header and series of TLVs in a
 *        NetworkLib::BufferWritableView.
 */
class MessageEncoder {
  public:
    //
    MessageEncoder(NetworkLib::BufferWritableView);

    /// @brief Append a TLV, encoding it.
    MessageEncoder &add(TLVBase &tlv);

    /// @brief Tell the encoder that we finished adding TLVs.
    void end();

    /// @brief Return a NetworkLib::BufferWritableView with the encoded data.
    NetworkLib::BufferWritableView data() {
        return mBuffer.getSub(0, mCurrentOffset);
    }

    /// @brief Provide access to the generic head encoder.
    CommonHeaderEncoder &header() { return mHeaderEncoder; }

  private:
    NetworkLib::BufferWritableView mBuffer;
    CommonHeaderEncoder mHeaderEncoder;
    std::size_t mCurrentOffset;
};

/**
 * @brief A class which helps decoding a series of TLVs stored
 * in a NetworkLib::BufferView.
 */
class MessageDecoder {
  public:
    MessageDecoder(NetworkLib::BufferView buffer);

    /// @brief Provide access to the generic head encoder.
    CommonHeaderDecoder &header() { return mHeaderDecoder; }

    bool isFailure() const {
        return mHeaderDecoder.messageClass() == MessageClass::RESPONSE_FAILURE;
    }

    bool isSuccess() const {
        return mHeaderDecoder.messageClass() == MessageClass::RESPONSE_SUCCESS;
    }

    bool isRequest() const {
        switch (mHeaderDecoder.messageClass()) {
        case MessageClass::REQUEST_SET:
        case MessageClass::REQUEST_ADD:
        case MessageClass::REQUEST_DEL:
        case MessageClass::REQUEST_GET:
            return true;

        default:
            return false;
        }
    }

    /// @brief decode the next TLV
    MessageDecoder &get(TLVBase &tlv);

    /// @brief Return the type of the next TLV, if any.
    ///
    /// Return TLVType::NONE if there's no next TLV.
    TLVType getNextTLVType() const;

  private:
    NetworkLib::BufferView mBuffer;
    CommonHeaderDecoder mHeaderDecoder;
    std::size_t mCurrentOffset;
};

} // namespace Agent
} // namespace Empower

#endif
