#ifndef EMPOWER_NETWORKLIB_UTILS_HH
#define EMPOWER_NETWORKLIB_UTILS_HH

// For std::size_t
#include <cstddef>

// For std::uintNN_t
#include <cstdint>

// For std::ostringstream
#include <sstream>

// For std::hex and such
#include <iomanip>

// For std::exchange
#include <utility>

// For std::string
#include <string>

///@file

/// @brief Declare packed structures.
///
/// This macro is used to declare packed structures (for the few struct
/// which are layered on top of a memory buffer via a
/// reinterpret_cast<>()).
///
/// '__attribute__((packed))' is supported both by GCC and CLang.
#define NETWORKLIB_PACKED_ATTRIBUTE __attribute__((packed))

/// @brief Enable bounds checkings in methods of BufferView and
///        BufferWritableView.
///
/// This affects only calls where bounds checking is normally
/// performed and needed. Places where bounds checking is redundant
/// (because they have already been checked) already use `_nocheck`
/// variants which never perform any additional check.
#define NETWORKLIB_CHECK_BOUNDS 1

/// @brief A macro to get the full name of the containing (member)
/// function, only for logging purposes. It's supposed to expand to a
/// string literal.
///
/// Note: BOOST_CURRENT_FUNCTION from the Boost libraries supports more
/// platforms
///
#if defined(__GNUC__) || defined(__CLANG__)
// __PRETTY_FUNCTION__ is available on GCC and CLang
#define NETWORKLIB_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
// __FUNCSIG__ is available on MSVC
#define NETWORKLIB_CURRENT_FUNCTION __FUNCSIG__
#else
// Fall back to C++11 standard __func__
#define NETWORKLIB_CURRENT_FUNCTION __func__
#endif

namespace Empower {

/// @brief Cross-platform code dealing with generic networking stuff.
namespace NetworkLib {

/// @brief RAII guard saving/restoring std::ios format status
class Iosguard {
  public:
    explicit Iosguard(std::ios &ios)
        : mIos{ios}, mSavedFmtflags{ios.flags()}, mSavedFill{ios.fill()} {}

    ~Iosguard() try {
        mIos.flags(mSavedFmtflags);
        mIos.fill(mSavedFill);
    } catch (...) {
        // Catch everything as we don't want calls to std::terminate()
        // because of this.
    }

  private:
    std::ios &mIos;
    const std::ios::fmtflags mSavedFmtflags;
    const std::ios::char_type mSavedFill;
};

///@name Byte ordering and data access functions
///
/// They provide a superset of the functionalities of ntohl(), ntohs(),
/// etc.
///
/// Note: looking at the assembly output of both GCC and CLang, they
///       result in code that's as efficient as using ntohl() and
///       such, while still being a bit more handy to read values from
///       buffers.
///
///@{

/// @brief Swap byte order on a 32-bit unsigned value
inline std::uint32_t swapByteOrder(std::uint32_t value) noexcept {
    return ((value & 0xFF) << 24) | (((value >> 8) & 0xFF) << 16) |
           (((value >> 16) & 0xFF) << 8) | (((value >> 24) & 0xFF));
}

/// @brief Swap byte order on a 32-bit signed value
inline std::int32_t swapByteOrder(std::int32_t value) noexcept {
    const std::uint32_t v1 = value;
    return swapByteOrder(v1);
}

/// @brief Swap byte order on a 16-bit unsigned value
inline std::uint16_t swapByteOrder(std::uint16_t value) noexcept {
    return ((value & 0xFF) << 8) | ((value >> 8) & 0xFF);
}

/// @brief Swap byte order on a 16-bit signed value
inline std::int16_t swapByteOrder(std::int16_t value) noexcept {
    const std::uint16_t v1 = value;
    return swapByteOrder(v1);
}

/// @brief  Get a `std::uint64_t` stored at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the returned value is in host order (i.e. `ntohl()` with a
/// pointer).
inline std::uint64_t getUint64At(const void *ptr) noexcept {
    return (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 0))
            << 56) |
           (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 1))
            << 48) |
           (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 2))
            << 40) |
           (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 3))
            << 32) |
           (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 4))
            << 24) |
           (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 5))
            << 16) |
           (static_cast<std::uint64_t>(
                *(reinterpret_cast<const std::uint8_t *>(ptr) + 6))
            << 8) |
           (static_cast<std::uint64_t>(
               *(reinterpret_cast<const std::uint8_t *>(ptr) + 7)));
}

/// @brief  Get a `std::int64_t` stored at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the returned value is in host order (i.e. `ntohl()` with a
/// pointer).
inline std::int64_t getInt64At(const void *ptr) noexcept {
    return getUint64At(ptr);
}

/// @brief  Get a `std::uint32_t` stored at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the returned value is in host order (i.e. `ntohl()` with a
/// pointer).
inline std::uint32_t getUint32At(const void *ptr) noexcept {
    return ((*(reinterpret_cast<const std::uint8_t *>(ptr) + 0)) << 24) |
           ((*(reinterpret_cast<const std::uint8_t *>(ptr) + 1)) << 16) |
           ((*(reinterpret_cast<const std::uint8_t *>(ptr) + 2)) << 8) |
           ((*(reinterpret_cast<const std::uint8_t *>(ptr) + 3)));
}

/// @brief  Get a `std::int32_t` stored at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the returned value is in host order (i.e. `ntohl()` with a
/// pointer).
inline std::int32_t getInt32At(const void *ptr) noexcept {
    return getUint32At(ptr);
}

/// @brief  Get a `std::uint16_t` stored at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the returned value is in host order (i.e. `ntohs()` with a
/// pointer).
inline std::uint16_t getUint16At(const void *ptr) noexcept {
    return ((*(reinterpret_cast<const std::uint8_t *>(ptr) + 0)) << 8) |
           ((*(reinterpret_cast<const std::uint8_t *>(ptr) + 1)));
}

/// @brief  Get a `std::int16_t` stored at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the returned value is in host order (i.e. `ntohs()` with a
/// pointer).
inline std::int16_t getInt16At(const void *ptr) noexcept {
    return getUint16At(ptr);
}

/// @brief  Set a `std::uint64_t` at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the given value is in host order (i.e. `htonl()` with a
/// pointer).
inline void setUint64At(void *ptr, std::uint64_t v) noexcept {
    *(reinterpret_cast<std::uint8_t *>(ptr) + 0) = (v >> 56) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 1) = (v >> 48) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 2) = (v >> 40) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 3) = (v >> 32) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 4) = (v >> 24) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 5) = (v >> 16) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 6) = (v >> 8) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 7) = (v)&0xFF;
}

/// @brief  Set a `std::int64_t` at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the given value is in host order (i.e. `htonl()` with a
/// pointer).
inline void setInt64At(void *ptr, std::int64_t v) noexcept {
    setUint64At(ptr, static_cast<std::uint64_t>(v));
}

/// @brief  Set a `std::uint32_t` at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the given value is in host order (i.e. `htonl()` with a
/// pointer).
inline void setUint32At(void *ptr, std::uint32_t v) noexcept {
    *(reinterpret_cast<std::uint8_t *>(ptr) + 0) = (v >> 24) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 1) = (v >> 16) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 2) = (v >> 8) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 3) = (v)&0xFF;
}

/// @brief  Set a `std::int32_t` at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the given value is in host order (i.e. `htonl()` with a
/// pointer).
inline void setInt32At(void *ptr, std::int32_t v) noexcept {
    setUint32At(ptr, static_cast<std::uint32_t>(v));
}

/// @brief  Set a `std::uint16_t` at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the given value is in host order (i.e. `htons()` with a
/// pointer).
inline void setUint16At(void *ptr, std::uint16_t v) noexcept {
    *(reinterpret_cast<std::uint8_t *>(ptr) + 0) = (v >> 8) & 0xFF;
    *(reinterpret_cast<std::uint8_t *>(ptr) + 1) = (v)&0xFF;
}

/// @brief  Set a `std::int16_t` at the given address.
///
/// The value stored in memory is assumed to be in network order,
/// while the given value is in host order (i.e. `htons()` with a
/// pointer).
inline void setInt16At(void *ptr, std::int16_t v) noexcept {
    setUint16At(ptr, static_cast<std::uint16_t>(v));
}

///@}

///@name Helpers to print out hex numbers.
///
/// @brief Return a std::string with the hex representation of some
///        integers.
///
/// They are more handy than using std::setw(), std::setfill() and
/// std::hex each time and then having to save and restore ios flags,
/// at the price of being somewhat less efficient.
///
///@{

/// @brief Format `value` as a two-digit hex number (e.g. `0x1a`).
inline std::string asHex8(std::uint8_t value) {
    std::ostringstream o;
    o << "0x" << std::setw(2) << std::setfill('0') << std::hex << +(value);
    return o.str();
}

/// @brief Format `value` as a four-digits hex number
///        (e.g. `0x1a2b`).
inline std::string asHex16(std::uint16_t value) {
    std::ostringstream o;
    o << "0x" << std::setw(4) << std::setfill('0') << std::hex << +(value);
    return o.str();
}

/// @brief Format `value` as a eight-digits hex number
///        (e.g. `0x1a2b3c4d`).
inline std::string asHex32(std::uint32_t value) {
    std::ostringstream o;
    o << "0x" << std::setw(8) << std::setfill('0') << std::hex << +(value);
    return o.str();
}

///@}

/// @brief Ignore leading and trailing whitespace in a std::string
///
/// Given a `std::string`, return a pair of iterators to the first
/// non-whitespace character from the beginning, and one pas the last
/// non-whitespace character.
std::pair<std::string::const_iterator, std::string::const_iterator>
trim(const std::string &str);

/**
 * @brief A class representing a Ethernet MAC address.
 */
class MACAddress {
  public:
    ///@name Constants
    ///@{

    /// @brief Convenience constant for the broadcast MAC address
    ///        (i.e. ``FF:FF:FF:FF:FF:FF``)
    static const MACAddress broadcast;

    ///@}

    /// @brief Type used to store the data of a MAC address.
    using value_type = std::array<unsigned char, 6>;

    ///@name Constructors
    ///@{

    /// @brief Default constructor (i.e. ``00:00:00:00:00:00``).
    MACAddress() = default;

    /// @brief Constructor from 6 integers.
    MACAddress(unsigned char a, unsigned char b, unsigned char c,
               unsigned char d, unsigned char e, unsigned char f) noexcept
        : mData{a, b, c, d, e, f} {}

    /// @brief Constructor from a std::string
    ///        (e.g. `ab:cd:ef:01:02:03` or also `AB-CD-EF-01-02-03`),
    ///        allowing leading and trailing whitespace (throws if
    ///        invalid).
    MACAddress(const std::string &str);

    ///@}

    ///@name Copy semantic
    ///@{
    MACAddress(const MACAddress &) = default;
    MACAddress &operator=(const MACAddress &other) = default;
    ///@}

    ///@name Move semantic
    ///@{
    MACAddress(MACAddress &&) noexcept = default;
    MACAddress &operator=(MACAddress &&other) = default;
    ///@}

    ///@name Comparison operators
    ///@{

    /// @brief Equality
    friend bool operator==(const MACAddress &lhs, const MACAddress &rhs) {
        return lhs.mData == rhs.mData;
    }

    /// @brief Diversity
    friend bool operator!=(const MACAddress &lhs, const MACAddress &rhs) {
        return !(lhs == rhs);
    }

    ///@}

    /// @brief Return a read-only reference to the underlying array.
    const value_type &array() const { return mData; }

  private:
    // The array storing the address;
    value_type mData;
};

/// @brief Dump a Empower::NetworkLib::MACAddress in a human-readable form.
inline std::ostream &operator<<(std::ostream &ostr,
                                const MACAddress &macAddress) {
    auto guard = Iosguard(ostr);

    auto const &a = macAddress.array();
    ostr << std::hex << std::setfill('0') << std::setw(2) << +(a[0]) << ':'
         << std::setw(2) << +(a[1]) << ':' << std::setw(2) << +(a[2]) << ':'
         << std::setw(2) << +(a[3]) << ':' << std::setw(2) << +(a[4]) << ':'
         << std::setw(2) << +(a[5]);

    return ostr;
}

/**
 * @brief A class representing an IPv4 address.
 */
class IPv4Address {
  public:
    ///@name Comparision operators
    ///@{

    ///@brief Equality.
    friend bool operator==(const IPv4Address &lhs, const IPv4Address &rhs) {
        return lhs.mData == rhs.mData;
    }

    ///@brief Diversity.
    friend bool operator!=(const IPv4Address &lhs, const IPv4Address &rhs) {
        return !(lhs == rhs);
    }

    ///@}

    /// @brief Type to store an IPv4 address data
    using value_type = std::array<unsigned char, 4>;

    ///@name Constructors
    ///@{

    /// @brief Default constructor
    IPv4Address() = default;

    /// @brief Constructor from four 8-bit integers.
    IPv4Address(unsigned char a, unsigned char b, unsigned char c,
                unsigned char d) noexcept
        : mData{a, b, c, d} {}

    /// @brief Constructor from a 32-bit integer.
    explicit IPv4Address(const std::uint32_t addr) noexcept
        : mData{static_cast<unsigned char>((addr >> 24) & 0xFF),
                static_cast<unsigned char>((addr >> 16) & 0xFF),
                static_cast<unsigned char>((addr >> 8) & 0xFF),
                static_cast<unsigned char>(addr & 0xFF)} {}

    /// @brief Constructor from std::string in quad-dot form (e.g.
    ///        `192.168.1.10`), allowing leading and trailing
    ///        whitespace (throws if invalid).
    IPv4Address(const std::string &str);

    ///@}

    ///@name Copy semantic
    ///@{
    IPv4Address(const IPv4Address &) = default;
    IPv4Address &operator=(const IPv4Address &) = default;
    ///@}

    ///@name Move semantic
    ///@{
    IPv4Address(IPv4Address &&) noexcept = default;
    IPv4Address &operator=(IPv4Address &&) = default;
    ///@}

    /// @brief Return a const reference to the underlying array
    ///        (storing the address in network order).
    const value_type &array() const { return mData; }

    /// @brief Convert the stored address to a 32-bit integer.
    operator std::uint32_t() const {
        return ((static_cast<std::uint32_t>(mData[0]) << 24) |
                ((static_cast<std::uint32_t>(mData[1]) << 16)) |
                ((static_cast<std::uint32_t>(mData[2]) << 8)) |
                ((static_cast<std::uint32_t>(mData[3]))));
    }

    /// @brief Apply a CIDR mask to the current address and return
    ///        the network part of the address.
    IPv4Address getNetworkByCIDRMask(unsigned int maskBits) const {
        const std::uint32_t addr = operator std::uint32_t();

        std::uint32_t mask;

        if (maskBits >= 32) {
            mask = 0xFFFFFFFF;
        } else if (maskBits == 0) {
            mask = 0;
        } else {
            mask = ~(static_cast<std::uint32_t>((1 << (32 - maskBits)) - 1));
        }

        return IPv4Address(addr & mask);
    }

  private:
    // The array storing the address;
    // (note that the address is stored in network order)
    value_type mData;
};

/// @brief Dump a Empower::NetworkLib::IPv4Address in a human-readable form.
inline std::ostream &operator<<(std::ostream &ostr,
                                const IPv4Address &ipv4Address) {
    auto guard = Iosguard(ostr);
    auto const &a = ipv4Address.array();
    ostr << std::dec << +(a[0]) << '.' << +(a[1]) << '.' << +(a[2]) << '.'
         << +(a[3]);
    return ostr;
}

/**
 * @brief A class representing an IPv4 CIDR (IPv4 network address +
 *        mask bits)
 */
class IPv4CIDR {
  public:
    ///@name Comparision operators
    ///@{

    ///@brief Equality.
    friend bool operator==(const IPv4CIDR &lhs, const IPv4CIDR &rhs) {
        return (lhs.mAddress == rhs.mAddress) &&
               (lhs.mMaskBits == rhs.mMaskBits);
    }

    ///@brief Diversity.
    friend bool operator!=(const IPv4CIDR &lhs, const IPv4CIDR &rhs) {
        return !(lhs == rhs);
    }

    ///@}

    /// @brief Dump a Empwoer::NetworkLib::IPv4CIDR in a human-readable form.
    friend std::ostream &operator<<(std::ostream &ostr,
                                    const IPv4CIDR &ipv4cidr) {
        auto guard = Iosguard(ostr);
        ostr << ipv4cidr.mAddress << '/' << std::dec << +(ipv4cidr.mMaskBits);
        return ostr;
    }

    ///@name Constructors
    ///@{

    /// @brief Default constructor
    IPv4CIDR() = default;

    /// @brief Constructor from an IPv4Address and a number of mask bits
    IPv4CIDR(IPv4Address address, unsigned int maskBits)
        : mAddress{address.getNetworkByCIDRMask(maskBits)},
          mMaskBits{(maskBits > 32) ? 32 : maskBits} {}

    /// @brief Constructor from std::string in quad-dot form plus mask (e.g.
    ///        `192.168.1.10/23`), allowing leading and trailing
    ///        whitespace (throws if invalid).
    IPv4CIDR(const std::string &str);

    ///@}

    ///@name Copy semantic
    ///@{
    IPv4CIDR(const IPv4CIDR &) = default;
    IPv4CIDR &operator=(const IPv4CIDR &) = default;
    ///@}

    ///@name Move semantic
    ///@{
    IPv4CIDR(IPv4CIDR &&) noexcept = default;
    IPv4CIDR &operator=(IPv4CIDR &&) = default;
    ///@}

    /// @brief Return true if this CIDR matches the given address
    bool matchAddress(const IPv4Address &address) const {
        return (address.getNetworkByCIDRMask(mMaskBits).
                operator std::uint32_t()) ==
               (mAddress.operator std::uint32_t());
    }

  private:
    IPv4Address mAddress;

    // The number of bits in the mask.
    unsigned int mMaskBits = 0;
};

/// @brief Final action (an action scheduled to be executed on exiting
///        scope). Create these through function template `finally()`.
///
/// This is a very basic implementation straight from C++ Core
/// Guidelines, C.30
template <typename A> struct final_action {
    A act;
    final_action(A a) : act{a} {}
    ~final_action() { act(); }
};

/// @brief Return a new final_action instance.
///
/// This is a very basic implementation straight from C++ Core
/// Guidelines, C.30
template <typename A> final_action<A> finally(A act) {
    return final_action<A>{act};
}

} // namespace NetworkLib
} // namespace Empower

// Specialization of std::hash for custom types
// (so they can be used with unordered collections)
namespace std {
using namespace Empower;

/// @brief A specialization of the hash function for class
///        Empower::NetworkLib::IPv4Address, so it can be used as a key in
///        `std::unordered_map` and such.
template <> struct hash<NetworkLib::IPv4Address> {

    /// @brief Hashing function argument type
    typedef NetworkLib::IPv4Address argument_type;

    /// @brief Hashing function result type
    typedef std::size_t result_type;

    /// @brief Compute the hash of a IPv4 address.
    result_type operator()(argument_type const &addr) const noexcept {
        // Just reuse the hash function for std::uint32_t;
        return std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(addr));
    }
};

/// @brief A specialization of the hash function for class
///        Empower::NetworkLib::MACAddress, so it can be used as a key in
///        `std::unordered_map` an such.
template <> struct hash<NetworkLib::MACAddress> {

    /// @brief Hashing function argument type
    typedef NetworkLib::MACAddress argument_type;

    /// @brief Hashing function result type
    typedef std::size_t result_type;

    /// @brief Compute the hash of a MAC address.
    result_type operator()(argument_type const &addr) const noexcept {
        const argument_type::value_type &array = addr.array();
        const std::uint16_t d1 = (array[0] << 8) | array[1];
        const std::uint32_t d2 =
            (array[2] << 24) | (array[3] << 16) | (array[4] << 8) | (array[5]);
        const result_type h1 = std::hash<std::uint16_t>{}(d1);
        const result_type h2 = std::hash<std::uint32_t>{}(d2);

        return h1 ^ (h2 << 1);
    }
};

} // namespace std

#endif
