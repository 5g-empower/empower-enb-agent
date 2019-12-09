#include <empoweragentproto/utils.hh>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>

namespace Empower {
namespace NetworkLib {

std::pair<std::string::const_iterator, std::string::const_iterator>
trim(const std::string &str) {

    auto const a = std::find_if_not(str.begin(), str.end(),
                                    [](int c) { return std::isspace(c); });

    if (a == str.end()) {
        return std::make_pair(str.end(), str.end());
    }

    auto const reverse_b = std::find_if_not(
        str.rbegin(), str.rend(), [](int c) { return std::isspace(c); });
    auto const b = reverse_b.base();
    return std::make_pair(a, b);
}

static IPv4Address parseIPv4Address(
    std::pair<std::string::const_iterator, std::string::const_iterator> range) {

    // An IPv4 address has 4 parts (0-3)
    constexpr const std::size_t maxPartIdx = 3;

    // An IPv4 address has max 3 digits per part
    const int maxDigits = 3;

    auto it = range.first;
    auto end = range.second;

    int parts[maxPartIdx + 1] = {};
    int partDigitCount = 0;

    std::size_t partIdx = 0;

    enum Status { PART_BEGIN, PART };

    Status status = PART_BEGIN;
    bool thereWereErrors = false;
    const char *errorCause = "(internal)";

    while ((!thereWereErrors) && (it != end)) {
        switch (status) {
        case PART_BEGIN:
            if (partIdx > maxPartIdx) {
                // Too many parts
                errorCause = "(too many parts)";
                thereWereErrors = true;
                break;
            } else if (std::isdigit(*it)) {
                // Begin of a new part
                partDigitCount = 0;
                parts[partIdx] = 0;
                status = PART;

                // Restart from current position
                continue;
            } else {
                // Not a digit? Error.
                errorCause = "(expected digit at part start)";
                thereWereErrors = true;
            }

            break;

        case PART:
            if (it == end) {
                // We finished parsing this part, possibly all the
                // parts.
                break;
            }

            partDigitCount++;

            if (partDigitCount > maxDigits) {
                // Too many digits in a single part? Error.
                errorCause = "(too many digits in a part)";
                thereWereErrors = true;
                break;
            }

            // Add the current digit to the current part
            parts[partIdx] = (parts[partIdx] * 10) + (*it - '0');

            if (parts[partIdx] > 255) {
                // Value is too large
                errorCause = "(part value is too large)";
                thereWereErrors = true;
                break;
            }

            // Advance to next char
            it++;

            if (it == end) {
                // We finished parsing this part, possibly all the
                // parts.
                break;
            } else if (std::isdigit(static_cast<unsigned char>(*it))) {
                // More digits in this part
                continue;

            } else if ((*it == '.') && (partDigitCount > 0)) {
                // We finished this part and we expect more parts
                partIdx++;
                status = PART_BEGIN;

                // Skip the current character.
                it++;
                continue;
            } else {
                // Empty part or unexpected char
                errorCause = "(missing or unexpected char)";
                thereWereErrors = true;
                break;
            }
            break;
        }
    }

    // Check that we have all the parts AND that the last part had at
    // least 1 digit.
    if (!thereWereErrors) {
        if (partIdx != maxPartIdx) {
            // Wrong number of parts.
            errorCause = "(wrong number of parts)";
            thereWereErrors = true;
        } else if (partDigitCount == 0) {
            // Missing digits in the last part.
            errorCause = "(wrong number of digits)";
            thereWereErrors = true;
        }
    }

    if (thereWereErrors) {
        std::ostringstream err;
        err << "IPv4Address::IPv4Address(const std::string &): bad IPv4 "
               "address \""
            << std::string(range.first, range.second) << "\": " << errorCause;
        throw std::invalid_argument(err.str());
    }

    return IPv4Address(parts[0], parts[1], parts[2], parts[3]);
}

IPv4Address::IPv4Address(const std::string &str) {
    *this = parseIPv4Address(trim(str));
}

/****/

static int hexDigitToInt(char c) {
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    } else if ((c >= 'A') && (c <= 'F')) {
        return (c - 'A' + 10);
    } else if ((c >= 'a') && (c <= 'f')) {
        return (c - 'a' + 10);
    }

    return 0;
}

MACAddress::MACAddress(const std::string &str) {

    // Number of parts
    constexpr const std::size_t maxIdx = 5;

    // Max digits per part
    const int maxDigits = 2;

    auto it = str.begin();
    const auto &end = str.end();

    int parts[maxIdx + 1] = {};
    int partDigitCount = 0;

    std::size_t idx = 0;

    enum Status { BEGIN, PART_BEGIN, PART };

    char sep = 0;

    Status status = BEGIN;
    bool thereWereErrors = false;
    const char *errorCause = "(internal)";

    while ((!thereWereErrors) && (it != end)) {
        switch (status) {
        case BEGIN:
            if (std::isspace(*it)) {
                it++;
                continue;
            } else {
                // Restart
                status = PART_BEGIN;
                continue;
            }
            break;

        case PART_BEGIN:
            if (idx > maxIdx) {
                // Too many parts
                errorCause = "(too many parts)";
                thereWereErrors = true;
                break;
            } else if (std::isxdigit(*it)) {
                // Begin of a new part
                partDigitCount = 0;
                parts[idx] = 0;
                status = PART;
                continue;
            } else {
                // Not a digit? Error.
                errorCause = "(expected digit at part start)";
                thereWereErrors = true;
            }

            break;

        case PART:
            partDigitCount++;

            if (partDigitCount > maxDigits) {
                // Too many digits in a single part? Error.
                errorCause = "(too many digits in a part)";
                thereWereErrors = true;
                break;
            }

            // Add the current digit to the current part
            parts[idx] = (parts[idx] * 16) + hexDigitToInt(*it);

            if (parts[idx] > 255) {
                // Value is too large
                errorCause = "(part value is too large)";
                thereWereErrors = true;
                break;
            }

            // Advance to next char
            it++;

            if (it == end) {
                // We finished parsing this part, possibly all the
                // parts.
                break;
            } else if (std::isxdigit(*it)) {
                // More digits in this part
                continue;

            } else if ((*it == ':' || *it == '-') && (partDigitCount == 2) &&
                       (idx < maxIdx)) {
                // We finished this part and we expect more parts

                if (sep == 0) {
                    // Save the first separator char for later...
                    sep = *it;
                } else if (*it != sep) {
                    // ... and check the same separator is used for all parts
                    errorCause = "(inconsistent separator)";
                    thereWereErrors = true;
                    break;
                }

                idx++;
                parts[idx] = 0;
                partDigitCount = 0;
                status = PART_BEGIN;
                it++;
                continue;
            } else if (std::isspace(*it) && (idx == maxIdx)) {
                // Consume all whitespace to the end
                while (it != end) {
                    if (!std::isspace(*it)) {
                        errorCause = "(trailing unexpeced char)";
                        thereWereErrors = true;
                        break;
                    }

                    it++;
                }
                break;

            } else {
                // Empty part or unexpected char
                errorCause = "(missing or unexpected char)";
                thereWereErrors = true;
                break;
            }
            break;
        }
    }

    // Check that we have all the parts
    if (!thereWereErrors) {
        if (idx != maxIdx) {
            // Wrong number of parts
            errorCause = "(wrong number of parts)";
            thereWereErrors = true;
        } else if (partDigitCount != 2) {
            // Wrong number of digits in the last part
            errorCause = "(wrong number of digits)";
            thereWereErrors = true;
        }
    }

    if (thereWereErrors) {
        std::ostringstream err;
        err << "MACAddress::MACAddress(const std::string &): bad MAC address \""
            << str << "\": " << errorCause;
        throw std::invalid_argument(err.str());
    }

    // Initialize the address, and we are done.
    for (std::size_t i = 0; i <= maxIdx; ++i) {
        mData[i] = parts[i];
    }
}

} // namespace NetworkLib
} // namespace Empower
