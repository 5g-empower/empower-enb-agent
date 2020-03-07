#ifndef EMPOWER_AGENT_TLVS_HH
#define EMPOWER_AGENT_TLVS_HH

#include <empoweragentproto/tlvencoding.hh>

// For IO::makeMessageBuffer()
#include <empoweragentproto/io.hh>
#include <string>
#include <vector>

namespace Empower {
namespace Agent {

/// @brief The main enum assigning an ID to each TLV type.
///
/// When implementing new TLVs (or a new version of an existing
/// TLV... which is really a new TLV), add a new entry here with a
/// unique ID.
enum class TLVType : std::uint16_t {
    /// @brief NONE is reserved
    NONE = 0,

    ERROR = 1,
    KEY_VALUE_STRING_PAIRS = 2,
    LIST_OF_TLV = 3,
    PERIODICITY = 4,
    BINARY_DATA = 5,
    CELL = 6,
    UE_REPORT = 7,
};

/**
 * @brief A TLV representing an error, with a unsigned 16-bit error
 *        code and a string error message.
 *
 *        The exact meaning of the error code completely depends on
 *        the context to which this TLV refers to. The error message
 *        is free human-readable text.
 */
class TLVError : public TLVBase {
  public:
    virtual ~TLVError() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::ERROR; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    /// @brief Return the error message
    std::string message() const { return mErrorMessage; }

    /// @brief Set the error message
    TLVError &message(std::string msg) {
        mErrorMessage = msg;
        return *this;
    }

    /// @brief Return the Error code
    std::uint16_t errcode() const { return mErrorCode; }

    /// @breif Set the error code
    TLVError &errcode(std::uint16_t errCode) {
        mErrorCode = errCode;
        return *this;
    }

    ///@}

  private:
    std::string mErrorMessage = "";
    std::uint16_t mErrorCode = 0;

    enum {
        errorCodeOffset = 0,
        errorMessageOffset = 2,
    };
};

/**
 * @brief A TLV for generic binary data
 */
class TLVBinaryData : public TLVBase {
  public:
    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::BINARY_DATA; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    TLVBinaryData &data(const NetworkLib::BufferView &d) {
        // Actual copy of content
        mBuffer = IO::makeMessageBuffer();
        d.copyTo(mBuffer);
        mBuffer.shrinkTo(d.size());
        return *this;
    }

    NetworkLib::BufferView data() const { return mBuffer; }

    /// @brief Convenience setter for string data.
    TLVBinaryData &stringData(const std::string &s) {
        // Actual copy of content
        mBuffer = IO::makeMessageBuffer();
        mBuffer.setCStringAt(0, s);
        mBuffer.shrinkTo(s.size() + 1);
        return *this;
    }

    /// @brief Convenience getter for string data.
    std::string stringData() const { return mBuffer.getCStringAt(0); }

    /// @}

  private:
    NetworkLib::BufferWritableView mBuffer;
};

/**
 * @brief A TLV representing a variable-length vector of key-value
 * pairs (as strings).
 */
class TLVKeyValueStringPairs : public TLVBase {
  public:
    using value_type = std::vector<std::pair<std::string, std::string>>;
    using reference = value_type &;
    using const_reference = const value_type &;

    virtual ~TLVKeyValueStringPairs() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override {
        return TLVType::KEY_VALUE_STRING_PAIRS;
    }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{
    const_reference getValue() const { return mValue; }
    void setValue(const_reference &v) { mValue = v; }

    /// @}

  private:
    value_type mValue;
};

/**
 *
 */

class TLVList : public TLVBase {
  public:
    TLVList();
    virtual ~TLVList() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::LIST_OF_TLV; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

  private:
    // Note: initialized in the default constructor as enum value
    //       TLVType::NONE is not yet defined here.
    TLVType mTLVType;
    std::uint16_t mCount;

    enum {
        tlvTypeOffset = 0,
        countOffset = 2,
    };
};

/**
 * A periodicity expressed as a number of milliseconds.
 */

class TLVPeriodicityMs : public TLVBase {
  public:
    virtual ~TLVPeriodicityMs() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::PERIODICITY; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint32_t milliseconds() const { return mMilliseconds; }
    TLVPeriodicityMs &milliseconds(std::uint32_t v) {
        mMilliseconds = v;
        return *this;
    }

    /// @}

  private:
    std::uint32_t mMilliseconds = 0;

    enum {
        millisecondsOffset = 0,
    };
};

/**
 * The configuration of a cell
 */

class TLVCell : public TLVBase {
  public:
    virtual ~TLVCell() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::CELL; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint16_t pci() const { return mPci; }
    TLVCell &pci(std::uint16_t v) {
        mPci = v;
        return *this;
    }

    std::uint32_t dlEarfcn() const { return mDlEarfcn; }
    TLVCell &dlEarfcn(std::uint32_t v) {
        mDlEarfcn = v;
        return *this;
    }

    std::uint32_t ulEarfcn() const { return mUlEarfcn; }
    TLVCell &ulEarfcn(std::uint32_t v) {
        mUlEarfcn = v;
        return *this;
    }

    std::uint8_t nPrb() const { return mNPrb; }
    TLVCell &nPrb(std::uint8_t v) {
        mNPrb = v;
        return *this;
    }

    /// @}

  private:
    std::uint16_t mPci = 0;
    std::uint32_t mDlEarfcn = 0;
    std::uint32_t mUlEarfcn = 0;
    std::uint8_t mNPrb = 0;

    enum {
        pciOffset = 0,
        dlEarfcnOffset = 2,
        ulEarfcnOffset = 6,
        nPrbOffset = 10,
    };
};

/**
 * The configuration of a cell
 */

class TLVUEReport : public TLVBase {
  public:
    virtual ~TLVUEReport() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::UE_REPORT; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint16_t rnti() const { return mRNTI; }
    TLVUEReport &rnti(std::uint16_t v) {
        mRNTI = v;
        return *this;
    }

    /// @}

  private:
    std::uint16_t mRNTI = 0;

    enum {
        rntiOffset = 0,
    };
};

} // namespace Agent
} // namespace Empower

#endif
