#include <empoweragentproto/tlvs.hh>

namespace Empower {
namespace Agent {

/**********************************************************************/

std::size_t TLVError::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint16At(errorCodeOffset, mErrorCode);
    buffer.setCStringAt(errorMessageOffset, mErrorMessage);
    return 2 + (mErrorMessage.size() + 1);
}

std::size_t TLVError::decode(NetworkLib::BufferView buffer) {
    mErrorCode = buffer.getUint16At(errorCodeOffset);
    mErrorMessage = buffer.getCStringAt(errorMessageOffset);
    return 2 + (mErrorMessage.size() + 1);
}

/**********************************************************************/

std::size_t TLVBinaryData::encode(NetworkLib::BufferWritableView buffer) {
    mBuffer.copyTo(buffer);
    return mBuffer.size();
}

std::size_t TLVBinaryData::decode(NetworkLib::BufferView buffer) {
    mBuffer = IO::makeMessageBuffer();
    buffer.copyTo(mBuffer);
    mBuffer.shrinkTo(buffer.size());
    return buffer.size();
}

/**********************************************************************/

std::size_t
TLVKeyValueStringPairs::encode(NetworkLib::BufferWritableView buffer) {

    // First pass: check if there's enough room
    std::size_t requiredSize = 0;
    for (auto &a : mValue) {
        requiredSize += a.first.size() + 1 + a.second.size() + 1;
    }

    if (requiredSize > buffer.size()) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": TLV requires a buffer of "
            << requiredSize << ", available size is " << buffer.size();
        throw std::runtime_error(err.str());
    }

    // Second pass: encode data
    std::size_t offset = 0;
    for (auto &a : mValue) {
        buffer.setCStringAt(offset, a.first);
        offset += a.first.size() + 1;

        buffer.setCStringAt(offset, a.second);
        offset += a.second.size() + 1;
    }

    return requiredSize;
}

std::size_t
TLVKeyValueStringPairs::decode(const NetworkLib::BufferView buffer) {
    mValue.clear();

    std::size_t offset = 0;
    while (offset < buffer.size()) {
        std::string a = buffer.getCStringAt(offset);
        offset += a.size() + 1;
        std::string b = buffer.getCStringAt(offset);
        offset += b.size() + 1;
        mValue.push_back(std::make_pair(a, b));
    }

    // Offset is also the total length
    return offset;
}

/**********************************************************************/

TLVList::TLVList() : mTLVType{TLVType::NONE}, mCount{0} {}

std::size_t TLVList::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint16At(tlvTypeOffset, static_cast<std::uint16_t>(mTLVType));
    buffer.setUint16At(countOffset, mCount);
    return 4;
}

std::size_t TLVList::decode(NetworkLib::BufferView buffer) {
    mTLVType = static_cast<TLVType>(buffer.getUint16At(tlvTypeOffset));
    mCount = buffer.getUint16At(countOffset);
    return 4;
}

/**********************************************************************/

std::size_t TLVPeriodicityMs::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint32At(millisecondsOffset, mMilliseconds);
    return 4;
}

std::size_t TLVPeriodicityMs::decode(NetworkLib::BufferView buffer) {
    mMilliseconds = buffer.getUint32At(millisecondsOffset);
    return 4;
}

/**********************************************************************/

std::size_t TLVCell::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint16At(pciOffset, mPci);
    buffer.setUint32At(dlEarfcnOffset, mDlEarfcn);
    buffer.setUint32At(ulEarfcnOffset, mUlEarfcn);
    buffer.setUint8At(nPrbOffset, mNPrb);
    return 11;
}

std::size_t TLVCell::decode(NetworkLib::BufferView buffer) {
    mPci = buffer.getUint16At(pciOffset);
    mDlEarfcn = buffer.getUint32At(dlEarfcnOffset);
    mUlEarfcn = buffer.getUint32At(ulEarfcnOffset);
    mNPrb = buffer.getUint8At(nPrbOffset);
    return 11;
}

/**********************************************************************/

std::size_t TLVUEReport::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint64At(imsiOffset, mIMSI);
    buffer.setUint32At(tmsiOffset, mTMSI);
    buffer.setUint16At(rntiOffset, mRNTI);
    buffer.setUint8At(statusOffset, mStatus);
    buffer.setUint16At(pciOffset, mPCI);
    return 17;
}

std::size_t TLVUEReport::decode(NetworkLib::BufferView buffer) {
	mIMSI = buffer.getUint64At(imsiOffset);
	mTMSI = buffer.getUint32At(tmsiOffset);
	mRNTI = buffer.getUint16At(rntiOffset);
	mStatus = buffer.getUint8At(statusOffset);
	mPCI = buffer.getUint16At(pciOffset);
	return 17;
}

/**********************************************************************/

std::size_t TLVUEMeasurementConfig::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint16At(idOffset, mID);
    buffer.setUint16At(rntiOffset, mRNTI);
    buffer.setUint16At(earfcnOffset, mEARFCN);
    buffer.setUint16At(intervalOffset, mInterval);
    buffer.setUint16At(maxCellOffset, mMaxCells);
    buffer.setUint16At(maxUsersOffset, mMaxUsers);
    return 12;
}

std::size_t TLVUEMeasurementConfig::decode(NetworkLib::BufferView buffer) {
	mID = buffer.getUint16At(idOffset);
	mRNTI = buffer.getUint16At(rntiOffset);
	mEARFCN = buffer.getUint16At(earfcnOffset);
	mInterval = buffer.getUint16At(intervalOffset);
	mMaxCells = buffer.getUint16At(maxCellOffset);
	mMaxUsers = buffer.getUint16At(maxUsersOffset);
	return 12;
}

/**********************************************************************/

std::size_t TLVMACPrbReportReport::encode(NetworkLib::BufferWritableView buffer) {
    buffer.setUint16At(nPrbOffset, mNPrb);
    buffer.setUint32At(dlOffset, mDL);
    buffer.setUint32At(ulOffset, mUL);
    buffer.setUint16At(pciOffset, mPCI);
    return 12;
}

std::size_t TLVMACPrbReportReport::decode(NetworkLib::BufferView buffer) {
	mNPrb = buffer.getUint16At(nPrbOffset);
	mDL = buffer.getUint32At(dlOffset);
	mUL = buffer.getUint32At(ulOffset);
	mPCI = buffer.getUint16At(pciOffset);
	return 12;
}

/**********************************************************************/

} // namespace Agent
} // namespace Empower
