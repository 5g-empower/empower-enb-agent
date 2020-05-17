#ifndef EMPOWER_AGENT_TLVS_HH
#define EMPOWER_AGENT_TLVS_HH

#include <empoweragentproto/tlvencoding.hh>

// For IO::makeMessageBuffer()
#include <empoweragentproto/io.hh>
#include <string>
#include <vector>

namespace Empower {
namespace Agent {

/**
 * @brief The main enum assigning an ID to each TLV type.
 */
enum class TLVType : std::uint16_t {
    /// @brief NONE is reserved
    NONE = 0x0,
    ERROR = 0x1,
    KEY_VALUE_STRING_PAIRS = 0x2,
    LIST_OF_TLV = 0x3,
    BINARY_DATA = 0x4,
    PERIODICITY = 0x5,
    CELL = 0x6,
    UE_REPORT = 0x7,
    UE_MEASUREMENTS_CONFIG = 0x8,
    UE_MEASUREMENT_REPORT = 0x9,
    UE_MEASUREMENT_ID = 0xB,
    MAC_PRB_UTILIZATION_REPORT = 0xA,
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
 * @brief A list of TLVs
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

    std::uint64_t imsi() const { return mIMSI; }
    TLVUEReport &imsi(std::uint64_t v) {
        mIMSI = v;
        return *this;
    }

    std::uint32_t tmsi() const { return mTMSI; }
    TLVUEReport &tmsi(std::uint32_t v) {
        mTMSI = v;
        return *this;
    }

    std::uint16_t rnti() const { return mRNTI; }
    TLVUEReport &rnti(std::uint16_t v) {
        mRNTI = v;
        return *this;
    }

    std::uint8_t status() const { return mStatus; }
    TLVUEReport &status(std::uint8_t v) {
        mStatus = v;
        return *this;
    }

    std::uint16_t pci() const { return mPCI; }
    TLVUEReport &pci(std::uint8_t v) {
        mPCI = v;
        return *this;
    }

    /// @}

  private:
    std::uint64_t mIMSI = 0;
    std::uint32_t mTMSI = 0;
    std::uint16_t mRNTI = 0;
    std::uint8_t mStatus = 0;
    std::uint16_t mPCI = 0;

    enum {
        imsiOffset = 0,
        tmsiOffset = 8,
        rntiOffset = 12,
        statusOffset = 14,
        pciOffset = 15
    };
};

/**
 * The configuration of a UE Measurement
 */
class TLVUEMeasurementConfig : public TLVBase {
  public:
    virtual ~TLVUEMeasurementConfig() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::UE_MEASUREMENTS_CONFIG; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint16_t rnti() const { return mRNTI; }
    TLVUEMeasurementConfig &rnti(std::uint16_t v) {
        mRNTI = v;
        return *this;
    }

    std::uint8_t measId() const { return mMeasId; }
    TLVUEMeasurementConfig &measId(std::uint8_t v) {
        mMeasId = v;
        return *this;
    }

    std::uint8_t interval() const { return mInterval; }
    TLVUEMeasurementConfig &interval(std::uint8_t v) {
        mInterval = v;
        return *this;
    }

    std::uint8_t amount() const { return mAmount; }
    TLVUEMeasurementConfig &amount(std::uint8_t v) {
        mAmount = v;
        return *this;
    }

    /// @}

  private:
    std::uint16_t mRNTI = 0;
    std::uint8_t mMeasId = 0;
    std::uint8_t mInterval = 0;
    std::uint8_t mAmount = 0;

    enum {
        rntiOffset = 0,
        measIdOffset = 2,
        intervalOffset = 3,
        amountOffset = 4
    };
};

/**
 * The ID of a UE Measurement
 */
class TLVUEMeasurementId : public TLVBase {
  public:
    virtual ~TLVUEMeasurementId() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::UE_MEASUREMENT_ID; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint16_t rnti() const { return mRNTI; }
    TLVUEMeasurementId &rnti(std::uint16_t v) {
        mRNTI = v;
        return *this;
    }

    std::uint8_t measId() const { return mMeasId; }
    TLVUEMeasurementId &measId(std::uint8_t v) {
        mMeasId = v;
        return *this;
    }

    /// @}

  private:
    std::uint16_t mRNTI = 0;
    std::uint8_t mMeasId = 0;

    enum {
        rntiOffset = 0,
        measIdOffset = 2
    };
};

/**
 * An UE Measurement report
 */
class TLVUEMeasurementReport : public TLVBase {
  public:
    virtual ~TLVUEMeasurementReport() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::UE_MEASUREMENT_REPORT; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint16_t rnti() const { return mRNTI; }
    TLVUEMeasurementReport &rnti(std::uint16_t v) {
        mRNTI = v;
        return *this;
    }

    std::uint8_t measId() const { return mMeasId; }
    TLVUEMeasurementReport &measId(std::uint8_t v) {
        mMeasId = v;
        return *this;
    }

    std::uint8_t rsrp() const { return mRSRP; }
    TLVUEMeasurementReport &rsrp(std::uint8_t v) {
        mRSRP = v;
        return *this;
    }

    std::uint8_t rsrq() const { return mRSRQ; }
    TLVUEMeasurementReport &rsrq(std::uint8_t v) {
        mRSRQ = v;
        return *this;
    }

    /// @}

  private:
    std::uint16_t mRNTI = 0;
    std::uint8_t mMeasId = 0;
    std::uint8_t mRSRP = 0;
    std::uint8_t mRSRQ = 0;

    enum {
        rntiOffset = 0,
        measIdOffset = 2,
        rsrpOffset = 3,
        rsrqOffset = 4
    };

};

/**
 * The configuration of a UE Measurement
 */
class TLVMACPrbReportReport : public TLVBase {
  public:
    virtual ~TLVMACPrbReportReport() {}

    /// @name TLVBase interface
    /// @{
    virtual TLVType type() const override { return TLVType::MAC_PRB_UTILIZATION_REPORT; }
    virtual std::size_t encode(NetworkLib::BufferWritableView buffer) override;
    virtual std::size_t decode(NetworkLib::BufferView buffer) override;
    /// @}

    /// @name Getters and setters
    /// @{

    std::uint16_t nPrb() const { return mNPrb; }
    TLVMACPrbReportReport &nPrb(std::uint16_t v) {
        mNPrb = v;
        return *this;
    }

    std::uint32_t dlPrbCounters() const { return mDL; }
    TLVMACPrbReportReport &dlPrbCounters(std::uint32_t v) {
        mDL = v;
        return *this;
    }

    std::uint32_t ulPrbCounters() const { return mUL; }
    TLVMACPrbReportReport &ulPrbCounters(std::uint32_t v) {
        mUL = v;
        return *this;
    }

    std::uint16_t pci() const { return mPCI; }
    TLVMACPrbReportReport &pci(std::uint16_t v) {
        mPCI = v;
        return *this;
    }

    /// @}

  private:
    std::uint16_t mNPrb = 0;
    std::uint32_t mDL = 0;
    std::uint32_t mUL = 0;
    std::uint16_t mPCI = 0;

    enum {
        nPrbOffset = 0,
        dlOffset = 2,
        ulOffset = 6,
        pciOffset = 10,
    };
};

} // namespace Agent
} // namespace Empower

#endif
