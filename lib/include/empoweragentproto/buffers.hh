#ifndef EMPOWER_NETWORKLIB_BUFFERS_HH
#define EMPOWER_NETWORKLIB_BUFFERS_HH

#include <empoweragentproto/utils.hh>

#include <array>
#include <cstddef>
#include <deque>
#include <memory>

namespace Empower {
namespace NetworkLib {

/**
 * @brief Interface for a generic **writable** buffer of ``unsigned char``
 *        for a network packet.
 *
 * Neither this type nor its specializations are meant to be used
 * directly: instead, use BufferView and BufferWritableView.
 */
class PacketBuffer {
  public:
    virtual ~PacketBuffer() {}

    /// @brief Get the PacketBuffer size.
    virtual std::size_t size() const = 0;

    /// @brief Get a pointer to the underlying buffer.
    virtual unsigned char *data() = 0;
};

/**
 * @brief A template implementing the PacketBuffer interface, using a
 *        ``std::array<unsigned char, s>`` of the given size.
 *
 * This type is not meant to be used directly: instead, use BufferView
 * and BufferWritableView.
 *
 * @param s the size of the buffer.
 */
template <std::size_t s> class PacketBufferArrayBased : public PacketBuffer {
  public:
    virtual ~PacketBufferArrayBased() {}
    virtual std::size_t size() const override { return mData.size(); }
    unsigned char *data() override { return mData.data(); }

  private:
    std::array<unsigned char, s> mData;
};

// Pre-declaration of class BufferWritableView, for BufferView.copyTo()
class BufferWritableView;

/**
 * @brief A **read-only** view on a buffer of ``unsigned char``, with
 *        methods to get sub-views and extract data from the buffer
 *        content.
 *
 * As the name implies, there can be multiple BufferView objects sharing
 * the same underlying buffer (or just some areas of it).
 *
 * The underlying buffer can be:
 *
 * * nothing at all: in this case we have an empty BufferView;
 *
 * * a PacketBuffer: the BufferView shares ownership of the
 *   PacketBuffer via a ``std::shared_ptr<PacketBuffer>``;
 *
 * * a generic buffer of ``unsigned char`` of arbitrary size,
 *   **non-owned**.  Users must ensure that the underlying buffer exists
 *   as long as all there are BufferView objects referring to it.
 *
 * A BufferView is read-only, and there's a corresponding writable
 * version, called BufferWritableView.
 *
 * A BufferView is something in between a C++17's ``std::string_view`` and
 * a C++20's ``std::span``, with the following differences:
 *
 * * it's not a template;
 *
 * * it can use a ``std::shared_ptr<PacketBuffer>`` to manage the lifecycle
 *   of the underlying PacketBuffer;
 *
 * * data in the buffer is read-only;
 *
 * * most of all, it provides methods to easily access data as
 *   commonly stored in network packet buffers (e.g. integers of
 *   variuos sizes stored in network order, addresses, etc.).
 */
class BufferView {
  public:
    /// @brief Hex dump of the BufferView content.
    friend std::ostream &operator<<(std::ostream &ostr, const BufferView &obj);

    ///@name Constructors
    ///@{

    /// @brief Default constructor (empty BufferView).
    ///
    /// An empty BufferView is pretty useless in itself, but this allows
    /// declaring a BufferView that can be assigned to in a subsequent
    /// moment.
    BufferView() noexcept : mBufferPtr(nullptr), mSize(0), mPtr(nullptr) {}

    /// @brief Constructor from a `std::shared_ptr<PacketBuffer>`.
    ///
    /// @param b A std::shared_ptr to a PacketBuffer, possibly with a
    ///        custom deleter. When its value is `nullptr`,
    ///        just construct an empty BufferView.
    ///
    /// The resulting BufferView comprises all the underlying
    /// PacketBuffer.
    ///
    explicit BufferView(std::shared_ptr<PacketBuffer> b)
        : mBufferPtr(b), mSize{b ? b->size() : 0}, mPtr{(b != nullptr &&
                                                         mSize > 0)
                                                            ? b->data()
                                                            : nullptr} {}

    /// @brief Make a BufferView which is attached to some buffer
    ///        allocated externally.
    ///
    /// The caller is responsibile to ensure that the underlying
    /// buffer continues existing as long as there are BufferView
    /// objects referring to the same buffer.
    ///
    /// @param ptr Raw pointer to the buffer start;
    ///
    /// @param length Length (in bytes) of the buffer.
    static BufferView makeNonOwningBufferView(const unsigned char *ptr,
                                              std::size_t length) {

        // Note: we cast away constness from ptr, but a BufferView
        //       won't ever change data through it.
        return BufferView(nullptr, const_cast<unsigned char *>(ptr), length);
    }

    ///@}

    ///@name Copy semantic
    ///@{
    BufferView(const BufferView &) = default;
    BufferView &operator=(const BufferView &) = default;
    ///@}

    ///@name Move semantic
    ///@{
    BufferView(BufferView &&) noexcept = default;
    BufferView &operator=(BufferView &&) = default;
    ///@}

    /// @brief Return true when the BufferView is empty.
    bool empty() const { return mPtr == nullptr || mSize == 0; }

    /// @brief Return the BufferView size.
    std::size_t size() const { return mSize; }

    /// @brief Sum the BufferView contents as 16-bit integers.
    ///
    /// Consider the BufferView content as an array of
    /// ``std::uint16_t`` stored in **network order** and return the
    /// sum of all values.
    ///
    /// If the BufferView size is odd, consider it as if there was an
    /// additional byte at the end set to ``0x00``.
    ///
    /// This method is needed to compute UDP and IPv4 checksums. See
    /// also RFC 1071, 1141 et alt.
    std::uint32_t getSum16() const {
        // Note: don't bother checking if the BufferView is empty,
        //       as the result will be 0 anyways.
        const std::size_t evenSize = mSize - (mSize % 2);
        const unsigned char *p = mPtr;
        const unsigned char *endP = mPtr + evenSize;
        std::uint_fast32_t sum1 = 0;
        std::uint_fast32_t sum2 = 0;
        std::uint32_t result = 0;

        while (p != endP) {
            sum1 += *p++;
            sum2 += *p++;
        }

        result = sum2 + (sum1 << 8);

        if (mSize != evenSize) {
            // Add the bits of the last byte as if a 0 was appended
            // to make the size even.
            result += (*p) << 8;
        }

        return result;
    }

    /// @brief Get a new BufferView entirely contained within this
    ///         BufferView, starting at the given offset for the given
    ///         length, checking bounds.
    ///
    /// The new BufferView shares its buffer with its parent
    /// BufferView. Think of this methods as C++20's
    /// ``std::span<T>::subspan()``.
    ///
    /// @param offset the starting offset for the new BufferView;
    ///
    /// @param len the length of the new BufferView.
    BufferView getSub(std::size_t offset, std::size_t len) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, len);

        // Note: keep in mind that mBufferPtr can (legally) be NULL.
        return BufferView(mBufferPtr, mPtr + offset, len);
    }

    /// @brief Get a new BufferView contained within this BufferView,
    ///        starting at the given offset up to the end, checking
    ///        bounds.
    ///
    /// It is equivalent to:
    ///
    ///    bufferView.getSub(offset, bufferview.size() - offset)
    ///
    /// The new BufferView shares its buffer with its parent
    /// Bufferview.
    ///
    /// @param offset the starting offset of the new BufferWiew.
    BufferView getSub(std::size_t offset) const {
        if (offset > mSize) {
            // Throw exception
            std::ostringstream err;
            err << NETWORKLIB_CURRENT_FUNCTION
                << ": requested area out of bounds (offset: " << offset
                << ", buffer size: " << mSize << ")";
            throw std::out_of_range(err.str());
        }

        return BufferView(mBufferPtr, mPtr + offset, mSize - offset);
    }

    /// @brief Shrink the size of this BufferView to the given new
    ///        size.
    ///
    /// @param newSize The new size. Must not be larger than the
    ///        original size.
    ///
    /// Example:
    ///
    ///       bv.shrink(newSize);
    ///
    /// @note This is slightly more efficient than doing
    ///
    ///       BufferView bv = ...
    ///       bv = bv.getSub(0, newSize)
    ///
    ///       especially when the BufferView has
    ///       shared ownership of the underlying buffer
    void shrinkTo(std::size_t newSize) {
        if (newSize > mSize) {
            std::ostringstream err;
            err << NETWORKLIB_CURRENT_FUNCTION
                << ": requested area out of bounds (requested: " << newSize
                << ", buffer size: " << mSize << ")";
            throw std::out_of_range(err.str());
        }

        mSize = newSize;
    }

    ///@name Data getters (checking bounds)
    ///
    /// Get the value of 64, 32, 16 and 8 bit integers (in **network
    /// order**) and other data types stored at the given offset,
    /// checking bounds.
    ///
    /// There are also equivalent methods not checking bounds
    /// (which can be used when bounds have already been checked
    /// in advance).
    ///
    ///@{

    std::uint64_t getUint64At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 8);
        return NetworkLib::getUint64At(mPtr + offset);
    }

    std::int64_t getInt64At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 8);
        return NetworkLib::getInt64At(mPtr + offset);
    }

    std::uint32_t getUint32At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 4);
        return NetworkLib::getUint32At(mPtr + offset);
    }

    std::int32_t getInt32At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 4);
        return NetworkLib::getInt32At(mPtr + offset);
    }

    std::uint16_t getUint16At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 2);
        return NetworkLib::getUint16At(mPtr + offset);
    }

    std::int16_t getInt16At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 2);
        return NetworkLib::getInt16At(mPtr + offset);
    }

    std::uint8_t getUint8At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 1);
        return *(mPtr + offset);
    }

    std::int8_t getInt8At(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 1);
        return *(mPtr + offset);
    }

    /// @brief Get an IPv4Address at the given offset, checking
    ///        bounds.
    ///
    /// @param offset The offset at which the IPv4 address is stored.
    ///        It must be less than ```bufferView.size() - 4```
    IPv4Address getIPv4AddressAt(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 4);
        return IPv4Address(*(mPtr + offset), *(mPtr + offset + 1),
                           *(mPtr + offset + 2), *(mPtr + offset + 3));
    }

    /// @brief Get a Ethernet MAC Address stored at the given offset,
    ///        checking bounds.
    ///
    /// @param offset The offset at which the MAC address is stored.
    ///        It must be less than ```bufferView.size() - 6```
    MACAddress getMACAddressAt(std::size_t offset) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 6);
        return MACAddress(*(mPtr + offset), *(mPtr + offset + 1),
                          *(mPtr + offset + 2), *(mPtr + offset + 3),
                          *(mPtr + offset + 4), *(mPtr + offset + 5));
    }

    /// @brief Get a zero-terminated string of char, stored at the
    ///        given offset, checking bounds.
    std::string getCStringAt(std::size_t offset) const {

        // First check that the offset is within bounds
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 0);

        // Then check that the string is entirely within bounds.
        auto end = offset;

        while ((end < mSize) && (*(mPtr + end) != 0)) {
            ++end;
        }

        if (end == mSize) {
            /// We reached the end of the buffer without finding the
            /// terminating NUL.
            std::ostringstream err;
            err << NETWORKLIB_CURRENT_FUNCTION
                << ": string not zero-terminated within bounds (offset: "
                << offset << ", buffer size: " << mSize << ")";
            throw std::runtime_error(err.str());
        }

        return std::string(reinterpret_cast<const char *>(mPtr + offset));
    }

    ///@}

    ///@name Data getters (non-bounds-checking variants)
    ///
    /// Get the value of 64, 32, 16 and 8 bit integers (in **network
    /// order**) and other data types stored at the given offset,
    /// **without** checking bounds.
    ///
    /// These variants should be used whenever previous checks already
    /// ensure that the value being accessed is well within bounds
    /// (it's often the case within various packet decoders, like for
    /// example IPv4Decoder, where constructors check that the
    /// provided BufferView size is large at least as the minimal
    /// header).
    ///
    ///@{

    std::uint64_t getUint64At_nocheck(std::size_t offset) const noexcept {
        return NetworkLib::getUint64At(mPtr + offset);
    }

    std::int64_t getInt64At_nocheck(std::size_t offset) const noexcept {
        return NetworkLib::getInt64At(mPtr + offset);
    }

    std::uint32_t getUint32At_nocheck(std::size_t offset) const noexcept {
        return NetworkLib::getUint32At(mPtr + offset);
    }

    std::int32_t getInt32At_nocheck(std::size_t offset) const noexcept {
        return NetworkLib::getInt32At(mPtr + offset);
    }

    std::uint16_t getUint16At_nocheck(std::size_t offset) const noexcept {
        return NetworkLib::getUint16At(mPtr + offset);
    }

    std::int16_t getInt16At_nocheck(std::size_t offset) const noexcept {
        return NetworkLib::getInt16At(mPtr + offset);
    }

    std::uint8_t getUint8At_nocheck(std::size_t offset) const noexcept {
        return *(mPtr + offset);
    }

    std::int8_t getInt8At_nocheck(std::size_t offset) const noexcept {
        return *(mPtr + offset);
    }

    /// @brief Get an IPv4Address at the given offset without checking
    ///        bounds.
    ///
    /// @param offset The offset at which the IPv4 address is stored.
    ///        It must be less than ```bufferView.size() - 4```
    IPv4Address getIPv4AddressAt_nocheck(std::size_t offset) const noexcept {
        return IPv4Address(*(mPtr + offset), *(mPtr + offset + 1),
                           *(mPtr + offset + 2), *(mPtr + offset + 3));
    }

    /// @brief Get a Ethernet MAC Address the given offset without
    ///        checking bounds.
    ///
    /// @param offset The offset at which the MAC address is stored.
    ///        It must be less than ```bufferView.size() - 6```
    MACAddress getMACAddressAt_nocheck(std::size_t offset) const noexcept {
        return MACAddress(*(mPtr + offset), *(mPtr + offset + 1),
                          *(mPtr + offset + 2), *(mPtr + offset + 3),
                          *(mPtr + offset + 4), *(mPtr + offset + 5));
    }

    ///@}

    ///@name Other utilities.
    ///@{

    /// @brief Copy the BufferView's content to an output iterator via
    ///       `std::copy()`, checking source bounds.
    ///
    template <class OutputIt>
    OutputIt copyTo(std::size_t offset, std::size_t len, OutputIt first) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, len);
        return std::copy(mPtr + offset, mPtr + offset + len, first);
    }

    /// @brief Copy the BufferView's content to a BufferWritableView,
    ///        raising an exception if the destination is not large
    ///        enough. After copying, the destination is *not*
    ///        shrinked (use shrinkTo() if you need that).
    void copyTo(BufferWritableView &destination) const;
    ///@}

    ///@name Access to some internals
    ///
    /// Use these with caution.
    ///
    ///@{

    /// @brief Read access to the underlying buffer pointer.
    ///
    /// It has a very long method name, instead of just `data()`, to
    /// discourage usage.
    const unsigned char *getUnderlyingBufferPtr() const { return mPtr; }

    /// Check if the area starting at 'offset' and extending for
    /// 'length' bytes is entirely within the bounds of this
    /// BufferView, and throw an exception if not.
    void throwExceptionIfOutOfBounds(const char *method, std::size_t offset,
                                     std::size_t length) const {
#if NETWORKLIB_CHECK_BOUNDS
        if ((offset + length) > mSize) {
            std::ostringstream err;
            err << method
                << ": requested area out of bounds (offset: " << offset
                << ", len: " << length << ", buffer size: " << mSize << ")";
            throw std::runtime_error(err.str());
        }
#else
        (void)method;
        (void)offset;
        (void)length;
#endif
    }

    ///@}

  protected:
    /// @brief Points either to `nullptr` or to the underlying PacketBuffer
    std::shared_ptr<PacketBuffer> mBufferPtr;

    /// @brief Size of the view
    std::size_t mSize;

    /// @brief Pointer either to nullptr, or to the buffer.
    unsigned char *mPtr;

    /// @brief Constructor to be used by specializations.
    explicit BufferView(std::shared_ptr<PacketBuffer> b, unsigned char *ptr,
                        std::size_t size)
        : mBufferPtr(b), mSize(size), mPtr{(size > 0) ? ptr : nullptr} {}
};

/**
 * @brief A **writable** BufferView
 *
 * It inherits from BufferView, so each valid BufferWritableView is a
 * valid BufferView as well.
 *
 * Note that all set*() methods are ``const``, as they don't change
 * the BufferWritableView itself -- only the data of the underlying
 * buffer.
 *
 * This means that it's possibile to change a PacketBuffer content
 * also using a **const** BufferWritableView: if you don't want that,
 * use a BufferView instead.
 *
 * There are basically three methods to get a new BufferWritableView
 * pointing to a free buffer:
 *
 * 1. get one from a PacketBufferPool (or from a
 *    PacketBufferSizedPool). Its std::shared_ptr takes care of
 *    returning the underlying PacketBuffer to the buffer pool when
 *    it's not needed any more.
 *
 * 2. allocate a new one on the heap via static method
 *    `makeEthBuffer()`.  Its std::shared_ptr takes care of freeing
 *    the underlying PacketBuffer when it's not needed any more.
 *    Please use this sparingly.
 *
 * 3. make a new BufferWritableView attached to an exising raw buffer
 *    via static method `makeNonOwningBufferWritableView()`. In this
 *    case the caller is responsible to ensure that the raw buffer
 *    stays valid as long as there are BufferWritableView and
 *    BufferView objects referring to it, and to free it when it's not
 *    needed any more.
 *
 * Like with BufferView objects, you can get a BufferWritableView
 * which shares its underlying buffer with another BufferWritableView
 * via method `getSub()`.
 */
class BufferWritableView : public BufferView {
  public:
    ///@name Constructors
    ///
    ///@{

    /// @brief Constructor of an empty BufferWritableView.
    ///
    /// Such object is pretty useless, but this allows declaring a
    /// BufferWritableView that can be assigned to in a subsequent
    /// moment.
    BufferWritableView() noexcept : BufferView(nullptr, nullptr, 0) {}

    /// Constructor from a std::shared_ptr<PacketBuffer>.
    ///
    /// The BufferWritableView comprises all the PacketBuffer.
    ///
    /// When the shared pointer is null, construct an empty BufferWritableView
    explicit BufferWritableView(std::shared_ptr<PacketBuffer> b)
        : BufferView(b, (b ? b->data() : nullptr), (b ? b->size() : 0)) {}

    ///@}

    ///@name Factories
    ///@{

    /// @brief Make a BufferWritableView which is attached to some
    ///        buffer allocated externally.
    ///
    /// The caller is responsibile to ensure that the underlying
    /// buffer continues existing for all BufferWritableView and
    /// BufferView objects referring to the same buffer.
    ///
    /// @param ptr Raw pointer to the buffer start;
    ///
    /// @param length length of the buffer.
    static BufferWritableView
    makeNonOwningBufferWritableView(unsigned char *ptr, std::size_t length) {
        return BufferWritableView(nullptr, ptr, length);
    }

    /// @brief Allocate on the heap a BufferWritableView suitable for
    ///        storing a Ethernet Frame.
    static BufferWritableView makeEthBuffer() {
        auto pd = std::make_shared<PacketBufferArrayBased<66500>>();
        auto pb = std::static_pointer_cast<PacketBuffer>(pd);
        return BufferWritableView(pb);
    }

    ///@}

    ///@name Copy semantic
    ///@{
    BufferWritableView(const BufferWritableView &) = default;
    BufferWritableView &operator=(const BufferWritableView &) = default;
    ///@}

    ///@name Move semantic
    ///@{
    BufferWritableView(BufferWritableView &&) noexcept = default;
    BufferWritableView &operator=(BufferWritableView &&) = default;
    ///@}

    /// @brief Read/write access to the underlying buffer pointer.
    ///
    /// It has a very long method name, instead of just ``data()``, to
    /// discourage usage. Use sub-views instead.
    ///
    /// Note that we are intentionally not overloading the method
    /// having a similar name in the base class to make user code
    /// express its intentions in a clearer way.
    unsigned char *getUnderlyingWritableBufferPtr() const { return mPtr; }

    /// @brief Get a new BufferWritableView contained inside a
    ///        BufferWritableView, starting at the given offset for
    ///        the given length, checking bounds.
    ///
    /// The new BufferWritableView shares its buffer with its parent
    /// BufferWritableView. Think of it as C++20's
    /// ``std::span<T>::subspan()``.
    ///
    /// @param offset the starting offset for the new BufferWritableView.
    ///
    /// @param len the length of the new BufferWritableView.

    BufferWritableView getSub(std::size_t offset, std::size_t len) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, len);
        return BufferWritableView(mBufferPtr, mPtr + offset, len);
    }

    /// @brief Get a new BufferWritableView contained inside a
    ///        BufferWritableView, starting at the given offset up to
    ///        the end, checking bounds.
    ///
    /// It is equivalent to
    ///
    ///    bufferWritableView.getSub(offset, bufferWritableView.size() - offset)
    ///
    /// The new BufferWritableView shares its buffer with its parent
    /// BufferWritableView.
    ///
    /// @param offset the starting offset for the new BufferWritableView.
    BufferWritableView getSub(std::size_t offset) const {
        if (offset > mSize) {
            // Throw exception
            std::ostringstream err;
            err << NETWORKLIB_CURRENT_FUNCTION
                << ": requested area out of bounds (offset: " << offset
                << ", buffer size: " << mSize << ")";
            throw std::runtime_error(err.str());
        }

        return BufferWritableView(mBufferPtr, mPtr + offset, mSize - offset);
    }

    ///@name Data setters (checking bounds)
    ///
    /// These methods provide write access to 64, 32, 16 and 8 bit
    /// integers (in **network order**) and other data types stored at
    /// the given offset, checking bounds.
    ///
    /// There are also similar methods not checking bounds.
    ///
    /// @note Note that these methods are `const`, as it is the
    ///       content of the underlying PacketBuffer being modified,
    ///       not the BufferWritableView itself. If you don't want the
    ///       PacketBuffer to be changed, use BufferView instead.
    ///
    ///@{

    const BufferWritableView &setUint64At(std::size_t offset,
                                          std::uint64_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 8);
        NetworkLib::setUint64At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setInt64At(std::size_t offset,
                                         std::int64_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 8);
        NetworkLib::setInt64At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setUint32At(std::size_t offset,
                                          std::uint32_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 4);
        NetworkLib::setUint32At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setInt32At(std::size_t offset,
                                         std::int32_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 4);
        NetworkLib::setInt32At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setUint16At(std::size_t offset,
                                          std::uint16_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 2);
        NetworkLib::setUint16At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setInt16At(std::size_t offset,
                                         std::int16_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 2);
        NetworkLib::setInt16At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setUint8At(std::size_t offset,
                                         std::uint8_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 1);
        *(mPtr + offset) = v;
        return *this;
    }

    const BufferWritableView &setInt8At(std::size_t offset,
                                        std::int8_t v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 1);
        *(mPtr + offset) = v;
        return *this;
    }

    /// Set an IPv4Address at the given offset, checking bounds
    const BufferWritableView &setIPv4AddressAt(std::size_t offset,
                                               const IPv4Address &v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 4);
        std::copy(v.array().begin(), v.array().end(), (mPtr + offset));
        return *this;
    }

    /// Set a Ethernet MAC Address at the given offset, checking bounds
    const BufferWritableView &setMACAddressAt(std::size_t offset,
                                              const MACAddress &v) const {
        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset, 6);
        std::copy(v.array().begin(), v.array().end(), (mPtr + offset));
        return *this;
    }

    /// @brief Set a zero-terminated string at the given offset,
    ///        checking bounds.
    const BufferWritableView &setCStringAt(std::size_t offset,
                                           const std::string &str) const {

        throwExceptionIfOutOfBounds(NETWORKLIB_CURRENT_FUNCTION, offset,
                                    str.size() + 1);
        auto out = reinterpret_cast<char *>(mPtr + offset);
        auto x = std::copy(str.begin(), str.end(), out);

        // Ensure that the encoded string is terminated by zero.
        *x = '\0';

        return *this;
    }

    ///@}

    ///@name Data setters (non-bounds-checking variants)
    ///
    /// Set the value of 64, 32, 16 and 8 bit integers (in network order)
    /// or other data types stored at the given offset, NOT checking
    /// bounds
    ///
    /// These should be used whenever previous checks already ensure
    /// that the value being set is well within bounds.
    ///
    /// @note Note that these methods are `const`, as it is the
    ///       content of the underlying PacketBuffer being modified,
    ///       not the BufferWritableView itself. If you don't want the
    ///       PacketBuffer to be changed, use BufferView instead.
    ///
    ///@{

    const BufferWritableView &setUint64At_nocheck(std::size_t offset,
                                                  std::uint64_t v) const
        noexcept {
        NetworkLib::setUint64At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setInt64At_nocheck(std::size_t offset,
                                                 std::int64_t v) const
        noexcept {
        NetworkLib::setInt64At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setUint32At_nocheck(std::size_t offset,
                                                  std::uint32_t v) const
        noexcept {
        NetworkLib::setUint32At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setInt32At_nocheck(std::size_t offset,
                                                 std::int32_t v) const
        noexcept {
        NetworkLib::setInt32At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setUint16At_nocheck(std::size_t offset,
                                                  std::uint16_t v) const
        noexcept {
        NetworkLib::setUint16At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setInt16At_nocheck(std::size_t offset,
                                                 std::int16_t v) const
        noexcept {
        NetworkLib::setInt16At(mPtr + offset, v);
        return *this;
    }

    const BufferWritableView &setUint8At_nocheck(std::size_t offset,
                                                 std::uint8_t v) const
        noexcept {
        *(mPtr + offset) = v;
        return *this;
    }

    const BufferWritableView &setInt8At_nocheck(std::size_t offset,
                                                std::int8_t v) const noexcept {
        *(mPtr + offset) = v;
        return *this;
    }

    /// Set an IPv4Address at the given offset,
    const BufferWritableView &
    setIPv4AddressAt_nocheck(std::size_t offset, const IPv4Address &v) const {
        std::copy(v.array().begin(), v.array().end(), (mPtr + offset));
        return *this;
    }

    /// Set a Ethernet MAC Address the given offset
    const BufferWritableView &
    setMACAddressAt_nocheck(std::size_t offset, const MACAddress &v) const {
        std::copy(v.array().begin(), v.array().end(), (mPtr + offset));
        return *this;
    }

    ///@}

  protected:
    /// @brief An ad-hoc constructor
    explicit BufferWritableView(std::shared_ptr<PacketBuffer> b,
                                unsigned char *ptr, std::size_t size)
        : BufferView(b, ptr, size) {}
};

/**
 * @brief A pool of PacketBuffer objects of the given size.
 *
 * This template implements a simple pool of PacketBuffer buffers, all
 * of the given size, giving out BufferWritableView objects which are
 * automatically returned to the pool when they are not needed any
 * more.
 *
 * @param s The desired size of the PacketBuffer
 */
template <std::size_t s> class PacketBufferSizedPool {
  public:
    /// @brief Default constructor
    ///
    /// @param initial_capacity The initial capacity of the pool.
    PacketBufferSizedPool(std::size_t initial_capacity = 16)
        : mPool(initial_capacity) {
        // The pool already contains its initial capacity.  Let's
        // fix the free deque, just as growBy() would do.
        for (auto &i : mPool) {
            mFree.push_back(&i);
        }
    }

    /// @brief Get a BufferWritableView from the pool.
    ///
    /// Release to the pool is automatic when all the
    /// BufferWritableView and BufferView objects referring to the
    /// PacketBuffer are destroyed..
    BufferWritableView getBufferWritableView() {
        return BufferWritableView(getPacketBuffer());
    }

    ///@name Info on the pool
    ///@{

    // @brief Return how many PacketBuffer instances there are
    //        currently in the pool (both busy and free).
    std::size_t capacity() const { return mPool.size(); }

    // @brief Return how many free PacketBuffer instances there are
    //        currently in the pool.
    std::size_t free_count() const { return mFree.size(); }

    ///@}

  private:
    using PacketBufferImplType = PacketBufferArrayBased<s>;

    std::deque<PacketBufferImplType *> mFree;
    std::deque<PacketBufferImplType> mPool;

    void growBy(std::size_t size) {
        for (std::size_t i = 0; i < size; ++i) {
            // Add a new free buffer
            mPool.emplace_back();
            mFree.push_back(&(mPool.back()));
        }
    }

    void releaseToPool(PacketBuffer *p) {
        auto *p1 = dynamic_cast<PacketBufferImplType *>(p);

        if (p1 == nullptr) {
            // Note: this should be completely impossible, as
            //       this method is private, thus the release of
            //       buffers can happen only through our
            //       custom deleter;
            return;
        }

        try {
            mFree.push_back(p1);
        } catch (...) {
            // Don't allow exceptions to propagate, as that would
            // result in a call to std::terminate().
        }
    }

    // Get a PacketBuffer from the pool
    std::shared_ptr<PacketBuffer> getPacketBuffer() {
        if (mFree.empty()) {
            growBy(1);
        }

        // Get a buffer from the pool
        PacketBuffer *p = mFree.back();
        mFree.pop_back();

        // Return a std::shared_ptr with a custom deleter
        // which returns the buffer to the pool
        return std::shared_ptr<PacketBuffer>(
            p, [this](PacketBuffer *p) { this->releaseToPool(p); });
    }
};

/// @brief A type for a pool of PacketBuffer objects sized for common
///        needs.
///
/// The PacketBuffer size in this case is just a bit more than the
/// common maximum IPv4/IPv6 packet/fragment size, plus some more room
/// for an Ethernet header which could have 802.1Q tags.
using PacketBufferPool = PacketBufferSizedPool<66500>;

} // namespace NetworkLib
} // namespace Empower

#endif
