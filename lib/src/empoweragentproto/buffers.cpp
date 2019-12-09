#include <empoweragentproto/buffers.hh>

// For std::hex and such
#include <iomanip>

// For std::ostringstream
#include <sstream>

namespace Empower {
namespace NetworkLib {

void BufferView::copyTo(BufferWritableView &destination) const {
    if (destination.size() < size()) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION
            << ": destination too small (required: " << size()
            << ", available: " << destination.size() << ")";
        throw std::runtime_error(err.str());
    }

    unsigned char *dst = destination.getUnderlyingWritableBufferPtr();
    std::copy(mPtr, mPtr + mSize, dst);
}

std::ostream &operator<<(std::ostream &ostr, const BufferView &obj) {

    auto guard = Iosguard(ostr);
    const int dumpedBytesPerLine = 32;

    std::string dumpedChars;

    // Round up to next greater multiple;
    auto actualSize = obj.mSize;
    if ((actualSize % dumpedBytesPerLine) != 0) {
        actualSize =
            (((obj.mSize / dumpedBytesPerLine) + 1) * dumpedBytesPerLine);
    }

    ostr << std::setfill('0') << std::hex;

    for (std::size_t i = 0; i < actualSize; ++i) {

        if ((i % dumpedBytesPerLine) == 0) {
            if (i > 0) {
                ostr << '|' << dumpedChars << "|\n";
                dumpedChars.clear();
            }

            ostr << std::setw(4) << i << ": ";
        }

        if (i < obj.mSize) {
            ostr << std::setw(2) << +(obj.mPtr[i]) << ' ';

            if (std::isprint(static_cast<unsigned char>(obj.mPtr[i]))) {
                dumpedChars += static_cast<char>(obj.mPtr[i]);
            } else {
                dumpedChars += '.';
            }

        } else {
            // Advance 3 chars
            ostr << "-- ";
            dumpedChars += ' ';
        }
    }

    ostr << '|' << dumpedChars << "|\n";
    dumpedChars.clear();

    return ostr;
}

} // namespace NetworkLib
} // namespace Empower
