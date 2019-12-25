#ifndef EMPOWER_AGENT_PROTOCOL_HH
#define EMPOWER_AGENT_PROTOCOL_HH

// For std::uint8_t and such
#include <cstdint>

#include <empoweragentproto/networklib.hh>

/// @brief Generic namespace for the Empower project
namespace Empower {

/// @brief Specific namespace for the Agent used in
///        the Empower project and its communication protocol.
namespace Agent {

/// @brief This enum tells the *subject* of the message.
///
/// It actually tells which *logical* enitity is involved
/// with the message. Some entities have obviously just
/// one kind of request and one kind of reply, therefore
/// the entity determines also the kind of messages being
/// transmitted.
enum class EntityClass : std::uint16_t {

    // Simple echo service
    ECHO_SERVICE = 0xff,

    // This service sends out periodic requests to the controller
    // specifying the periodicity and expects back a responses.
    HELLO_SERVICE = 0x0,

    // This service provide the list of capabilities of the eNB
    CAPABILITIES_SERVICE = 0x1,

    // Add more entity types here.
};

/// @brief An inline namespace with reference structs illustrating
///        the common parts of messages.
///
/// These structs are just for documentation purposes. They are *not*
/// used to actually encode/decode messages (only the enums defined in
/// them are used).
inline namespace ReferenceProtocolStructs {

/**
 * @brief A struct illustrating the structure of the preamble to all
 *        messages. *The preamble is part of the common header.*
 *
 * The preamble is structured as follows
 * ```
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Version        |Flags          |<RESERVED>                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Length                                                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ```
 */
struct Preamble {
    /// @brief 8-bit version number of the protocol. For this
    ///        implementation it should be set to 2. On the receiving
    ///        side, if the version number is not 2, the entire
    ///        message should be skipped (i.e. we should read further
    ///        `(Length - 4)` bytes). This allows sending the same
    ///        message using multiple protocol versions.
    std::uint8_t version = 2;

    /// @brief 8-bit bitfield of flags.
    ///
    /// Bits 0-6 are currently reserved, and should be cleared (`0`).
    ///
    /// Bit 7 is cleared (`0`) for requests, and set (`1`) for
    ///     responses. It tells how field `ts_rc` should be
    ///     interpreted.
    std::uint8_t flags = 0;

    /// @brief 16-bit field encoding general info on requests/responses.
    ///
    /// * bits 0-13 encode the kind of entity/service/whatever
    ///             involved in the request/response.
    ///
    /// For requests:
    ///
    /// * bits 14-15 encode the (general) operation type that we are
    ///              required to perform:
    ///   * `0`: SET
    ///   * `1`: ADD
    ///   * `2`: DEL
    ///   * `3`: GET
    ///
    /// For replies:
    ///
    /// * bit 14 is reserved for future usage and should be cleared
    ///          (`0`).
    ///
    /// * bit 15 tells if this is a success or a failure/error:
    ///
    ///   * `0`: SUCCESS
    ///   * `1`: FAILURE
    std::uint16_t ts_rc = 0;

    /// @brief The whole message length, in bytes, including preamble and
    /// headers.
    std::uint32_t length = 0;

    enum {
        /// @brief The size in bytes of an encoded Preamble.
        size = 8,
    };

    /// @Brief Offsets of fields within an encoded Preamble.
    enum {
        versionOffset = 0,
        flagsOffset = 1,
        tsrcOffset = 2,
        lengthOffset = 4,
    };

    // Just to make clear that this struct is not meant to be
    // instantiated.
    Preamble() = delete;
};

/**
 * @brief A struct illustrating the structure of the common header of
 *        all messages. It *includes* the preamble.
 *
 * ```
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | PREAMBLE -- see above                                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | PREAMBLE -- continued                                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |TSRC                           |Cell ID                        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Element ID                                                     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Element ID (continued)                                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Sequence number                                                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Transaction ID                                                 |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ```
 */
struct CommonHeader {
    /// @brief The preamble
    Preamble preamble;

    /// @brief Identifier of the element (of the network)
    std::uint64_t element_id = 0;

    /// @brief Sequence number of this message
    std::uint32_t sequence = 0;

    /// @brief Unique identifier of a (pending) transaction.
    std::uint32_t transaction_id = 0;

    /// @brief Offsets of fields within an encoded CommonHeader.
    enum {
        versionOffset = Preamble::versionOffset,
        flagsOffset = Preamble::flagsOffset,
        lengthOffset = Preamble::lengthOffset,
        tsrcOffset = Preamble::tsrcOffset,
        elementIdOffset = Preamble::size + 0,
        sequenceOffset = Preamble::size + 8,
        transactionIdOffset = Preamble::size + 12,
    };

    enum {
        /// @brief The total length in bytes of a CommonHeader
        totalLength = Preamble::size + 16,
    };

    // Just to make clear that this struct is not meant to be
    // instantiated.
    CommonHeader() = delete;
};

/**
 * @brief A struct illustrating the structure of the common header of
 *        all TLVs in a message.
 *
 * ```
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |Type                           |Length                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | TLV data (not part of the TLV header -- indicated here just   |
 * | for illustration purposes)                                    |
 * .                                                               .
 * .                                                               .
 * .                                                               .
 * |                                                               |
 * | (note: length is not necessarily a multiple of _______________|
 * | 4 bytes)                                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * ```
 */
struct TLVHeader {
    /// @brief TLV type unique identifier (see enum TLVType).
    std::uint16_t type;

    /// @brief Total length in bytes of this TLV (including the
    ///        TLVHeader). Length is not necessarily a multiple of 4.
    std::uint16_t length;

    /// @brief Offsets of fields within an encoded TLVHeader, plus
    ///        the offset of the TLV data.
    enum {
        typeOffset = 0,
        lengthOffset = 2,
        dataOffset = 4,
    };

    enum {
        /// @brief The total length in bytes of an encoded
        ///        TLVHeader.
        headerLength = 4,
    };

    // Just to make clear that this struct is not meant to be
    // instantiated.
    TLVHeader() = delete;
};

} // namespace ReferenceProtocolStructs

enum class MessageClass : std::uint8_t {
    INVALID = 0,
    REQUEST_SET = 1,
    REQUEST_ADD = 2,
    REQUEST_DEL = 3,
    REQUEST_GET = 4,
    RESPONSE_SUCCESS = 64,
    RESPONSE_FAILURE = 65,
};

/**
 * @brief Decode a preamble + common header from a data stored in a
 *        BufferView.
 */
class CommonHeaderDecoder {
  public:
    ///@name Constructors
    ///@{

    /// @brief Constructor attaching to the given BufferView.
    ///
    /// Throws exceptions if the BufferView is unsuitable (empty, too
    /// short, etc.).
    CommonHeaderDecoder(const NetworkLib::BufferView &messageData);

    /// @brief Constructor from the given BufferView (moved).
    ///
    /// Throws exceptions if the BufferView is unsuitable (empty, too
    /// short, etc.).
    CommonHeaderDecoder(NetworkLib::BufferView &&messageData);
    ///@}

    ///@name No default constructor
    ///@{
    CommonHeaderDecoder() = delete;
    ///@}

    ///@name No copy semantic
    ///@{
    CommonHeaderDecoder(const CommonHeaderDecoder &) = delete;
    CommonHeaderDecoder &operator=(const CommonHeaderDecoder &) = delete;
    ///@}

    ///@name No move semantic
    ///@{
    CommonHeaderDecoder(CommonHeaderDecoder &&) = delete;
    CommonHeaderDecoder &operator=(CommonHeaderDecoder &&) = delete;

    ///@}

    /// @name Read access to common fields. Note that preamble fields
    ///       are not accessible, because they are managed internally.
    ///
    /// @{

    std::uint16_t tsRc() const;
    std::size_t totalLengthBytes() const;
    std::uint16_t cellIdentifier() const;
    std::uint32_t sequence() const;
    std::uint64_t elementId() const;
    std::uint32_t transactionId() const;

    /// @}

    ///@name Utilities
    ///@{

    /// @brief Tells if this is a request to set, add, del, get some
    ///        value on a entity, of if this is a response which is
    ///        either successfull or a failure.
    MessageClass messageClass() const;

    /// @brief Tells the class of the entity involved in this message
    ///        exchange (think of it as a service type).
    EntityClass entityClass() const;

    /// @brief Return a BufferView with the payload *after* the
    /// preamble and common header..
    NetworkLib::BufferView data() const {
        return mBufferView.getSub(CommonHeader::totalLength,
                                  totalLengthBytes() -
                                      CommonHeader::totalLength);
    }

    ///@}

    /// @brief Return the size of an encoded CommonHeader (preamble
    ///        included)
    std::size_t size() const { return CommonHeader::totalLength; }

  private:
    enum {
        flagsRequestOrResponseMask = 1 << 7,
    };

    const NetworkLib::BufferView mBufferView;

    /// @brief The version (from the preamble).
    std::uint8_t version() const;

    /// @brief The flags (from the preamble).
    std::uint8_t flags() const;

    // Sanity checks performed on construction
    void throwIfBufferIsUnsuitable(const char *method);
};

/**
 * @brief Encode a common header into a given BufferWritableView.
 */
class CommonHeaderEncoder {
  public:
    ///@name Constructors
    ///@{

    /// @brief Constructor attaching to the given BufferWritableView.
    ///
    /// Throws exceptions if the BufferWritableView is unsuitable (too
    /// short, etc.).
    CommonHeaderEncoder(const NetworkLib::BufferWritableView &buffer);

    /// @brief Constructor from the given BufferWritableView (moved).
    ///
    /// Throws exceptions if the BufferWritableView is unsuitable (too
    /// short, etc.).
    CommonHeaderEncoder(NetworkLib::BufferWritableView &&buffer);
    ///@}

    ///@name No default constructor
    ///@{
    CommonHeaderEncoder() = delete;
    ///@}

    ///@name No copy semantic
    ///@{
    CommonHeaderEncoder(const CommonHeaderEncoder &) = delete;
    CommonHeaderEncoder &operator=(const CommonHeaderEncoder &) = delete;
    ///@}

    ///@name No move semantic
    ///@{
    CommonHeaderEncoder(CommonHeaderEncoder &&) = delete;
    CommonHeaderEncoder &operator=(CommonHeaderEncoder &&) = delete;

    ///@}

    /// @name Write access to common fields
    ///@{

    CommonHeaderEncoder &totalLengthBytes(std::size_t);
    CommonHeaderEncoder &cellIdentifier(std::uint16_t);
    CommonHeaderEncoder &sequence(std::uint32_t);
    CommonHeaderEncoder &elementId(std::uint64_t);
    CommonHeaderEncoder &transactionId(std::uint32_t);

    ///@}

    ///@name Utilities
    ///@{

    CommonHeaderEncoder &messageClass(MessageClass);
    CommonHeaderEncoder &entityClass(EntityClass);

    /// @brief Return the size of an encoded CommonHeader (preamble
    ///        included)
    std::size_t size() const { return CommonHeader::totalLength; }

    ///@}

  private:
    enum {
        flagsRequestOrResponseMask = 1 << 7,
    };

    CommonHeaderEncoder &version(std::uint8_t version);

    const NetworkLib::BufferWritableView mBufferWritableView;

    // Sanity checks performed on construction
    void throwIfBufferIsUnsuitable(const char *method);

    void setDefaults();
};

} // namespace Agent
} // namespace Empower

#endif
