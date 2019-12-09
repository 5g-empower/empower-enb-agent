#ifndef EMPOWER_AGENT_IO_HH
#define EMPOWER_AGENT_IO_HH

#include <empoweragentproto/networklib.hh>

namespace Empower {
namespace Agent {

/// @brief Manages the network communication with an agent, including
///        what's needed to send and receive the data of encoded
///        messages.
class IO {
  public:
    ~IO();

    /// @name Setup
    /// @{

    /// @brief Set the port to use for listening or to use for
    ///        connecting. Takes effect on the next call to
    ///        `openListeningSocket()` or to `openSocket()`.
    ///        Default port is `2210`.
    IO &port(std::uint16_t port) {
        mPort = port;
        return *this;
    }

    std::uint16_t port() const { return mPort; }

    /// @brief Set the address to use for listening or to use for
    ///        connecting. Takes effect on the next call to
    ///        `openListeningSocket()` or to `openSocket()`.
    ///        Default address is any address for listening, and
    ///        `127.0.0.1` when connecting.
    IO &address(NetworkLib::IPv4Address address) {
        mAddress = address;
        return *this;
    }

    NetworkLib::IPv4Address address() const { return mAddress; }

    IO &delay(int msec) {
        mDelay_msec = msec;
        return *this;
    }

    int delay() const { return mDelay_msec; }

    /// @}

    /// @name Incoming connections
    /// @{

    /// @brief (Re)open a listening TCP socket on the given port.
    ///        Don't wait for a connection (use `isDataAvailable()` to
    ///        wait and accept a connection attempt.
    ///
    ///        Raises exceptions on errors.
    ///
    void openListeningSocket();

    /// @brief Wait for an incoming connection and accept it.
    void acceptConnectionIfNeeded();

    /// @}

    /// @name Outgoing connections (for C++ clients)
    /// @{

    /// @brief (Re)open a TCP connection (client) to the given address and
    ///        port.
    ///
    /// @return True if the connection was opened.
    bool openSocket();

    /// @}

    /// @name Incoming and outgoing connections
    /// @{

    /// @brief Close every opened connection (incoming or
    ///        outgoing). Don't complain if there isn't a open
    ///        connection.
    void closeConnection() noexcept;

    /// @brief Tells if the network connection is currently closed
    ///        (works for both directions).
    bool isConnectionClosed() const { return mConnectionFD == -1; }

    /// @}

    /// @brief Return a NetworkLib::BufferWritableView associated to a
    ///        newly allocated buffer that is suitable to hold a whole
    ///        message.
    ///
    /// The lifecycle of the BufferWritableView is managed automatically via
    /// a std::shared_ptr<T>
    ///
    /// Note: the buffer size is slighly less than the maximum theoretical
    ///       size for a message (64KiB), to play nice with memory
    ///       allocators.
    static NetworkLib::BufferWritableView makeMessageBuffer();

    /// @name Receive data
    /// @{

    /// @brief Wait for a whole message and read (receive) it all,
    ///        using the buffer specified by the given
    ///        NetworkLib::BufferWritableView as a input buffer.
    ///        Return only when either an entire message has been read
    ///        in or there are unrecoverable read errors.
    ///
    /// @return A NetworkLib::BufferView with the message, or an empty
    ///         BufferView.
    NetworkLib::BufferView
    readMessage(NetworkLib::BufferWritableView &inputBuffer);

    /// @brief Wait up to the configured delay for for data to be
    ///        available to read, or for a remote connection to take
    ///        place (in the latter case, the remote connection is
    ///        accepted via `acceptConnectionIfNeeded()`, and the we
    ///        wait again for data to read).
    ///
    /// @return False if the timeout expired.
    bool isDataAvailable();

    /// @brief Wait for the configured delay
    void sleep() const;

    /// @}

    /// @name Send Data
    /// @{

    /// @brief Write (send) the data of the message encoded in the
    ///        given NetworkLib::BufferView. Return only when an
    ///        entire message has been written out.
    ///
    /// @return the number of bytes written (`0` == no message written or
    ///         EOF).
    std::size_t writeMessage(const NetworkLib::BufferView &messageBuffer);

    /// @}

  private:
    NetworkLib::IPv4Address mAddress = {0, 0, 0, 0};
    std::uint16_t mPort = 2210;

    // Default delay/timeout is 1500 milliseconds
    int mDelay_msec = 1500;

    int mConnectionFD = -1;
    int mListeningSocketFD = -1;
};

} // namespace Agent
} // namespace Empower

#endif
