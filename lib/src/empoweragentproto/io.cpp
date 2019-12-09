#include <empoweragentproto/io.hh>
#include <empoweragentproto/protocol.hh>

// For read(2) and write(2)
#include <unistd.h>

// For socket(2), bind(2), listen(2)
#include <sys/socket.h>
#include <sys/types.h>

// To pause for some time while we wait for all the data
#include <chrono>
#include <thread>

// For std::strerror() and std::memset()
#include <cstring>

// For htons()
#include <arpa/inet.h>

// For std::max()
#include <algorithm>

namespace Empower {
namespace Agent {

IO::~IO() { closeConnection(); }

void IO::closeConnection() noexcept {

    if (mConnectionFD != -1) {
        close(mConnectionFD);
        mConnectionFD = -1;
    }

    if (mListeningSocketFD != -1) {
        close(mListeningSocketFD);
        mListeningSocketFD = -1;
    }
}

void IO::openListeningSocket() {

    closeConnection();

    // Create a TCP socket
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);

    if (socketFD == -1) {
        int savedErrno = errno;
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": call to socket(2) failed "
            << " (errno =" << savedErrno << ": " << std::strerror(savedErrno)
            << ')';
        throw std::runtime_error(err.str());
    }

    // Bind the listening address and port
    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(sockaddr_in));

    serverAddress.sin_family = AF_INET;

    if (mAddress == NetworkLib::IPv4Address(0, 0, 0, 0)) {
        serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        serverAddress.sin_addr.s_addr =
            htonl(static_cast<std::uint32_t>(mAddress));
    }
    serverAddress.sin_port = htons(mPort);

    if ((bind(socketFD, reinterpret_cast<sockaddr *>(&serverAddress),
              sizeof(serverAddress))) == -1) {

        int savedErrno = errno;
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": call to bind(2) failed "
            << " (errno =" << savedErrno << ": " << std::strerror(savedErrno)
            << ')';
        throw std::runtime_error(err.str());
    }

    // Listen...
    if ((listen(socketFD, 1)) == -1) {
        int savedErrno = errno;
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": call to listen(2) failed "
            << " (errno =" << savedErrno << ": " << std::strerror(savedErrno)
            << ')';
        throw std::runtime_error(err.str());
    }

    mListeningSocketFD = socketFD;
}

void IO::acceptConnectionIfNeeded() {
    if (mListeningSocketFD != -1 && mConnectionFD == -1) {

        // Wait for a connection and accept it.
        sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);

        int fd = accept(mListeningSocketFD,
                        reinterpret_cast<sockaddr *>(&clientAddress),
                        &clientAddressLen);
        if (fd == -1) {
            int savedErrno = errno;
            std::ostringstream err;
            err << NETWORKLIB_CURRENT_FUNCTION << ": call to accept(2) failed "
                << " (errno =" << savedErrno << ": "
                << std::strerror(savedErrno) << ')';
            throw std::runtime_error(err.str());
        }

        mConnectionFD = fd;
    }
}

bool IO::openSocket() {

    closeConnection();

    // Create a TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1) {
        int savedErrno = errno;
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": call to socket(2) failed "
            << " (errno =" << savedErrno << ": " << std::strerror(savedErrno)
            << ')';
        throw std::runtime_error(err.str());
    }

    // Specify the destination address and port
    sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(sockaddr_in));

    serverAddress.sin_family = AF_INET;

    if (mAddress == NetworkLib::IPv4Address(0, 0, 0, 0)) {
        NetworkLib::IPv4Address localhost(127, 0, 0, 1);
        serverAddress.sin_addr.s_addr =
            htonl(static_cast<std::uint32_t>(localhost));
    } else {
        serverAddress.sin_addr.s_addr =
            htonl(static_cast<std::uint32_t>(mAddress));
    }
    serverAddress.sin_port = htons(mPort);

    // Attempt to connect
    if ((connect(sockfd, reinterpret_cast<sockaddr *>(&serverAddress),
                 sizeof(serverAddress))) == -1) {

        int savedErrno = errno;

        // connect(2) failed. In any case, close the socket.
        close(sockfd);
        sockfd = 0;

        if (savedErrno == ECONNREFUSED || savedErrno == EINTR ||
            savedErrno == ETIMEDOUT) {
            // connect(2) failed in a (supposedly) recoverable way.
            return false;
        } else {
            // connect(2) failed for something more serious
            std::ostringstream err;
            err << NETWORKLIB_CURRENT_FUNCTION
                << ": call to connect(2) (address " << mAddress << ", port "
                << mPort << ") failed "
                << "(errno =" << savedErrno << ": " << std::strerror(savedErrno)
                << ')';
            throw std::runtime_error(err.str());
        }
    }

    mConnectionFD = sockfd;
    return true;
}

NetworkLib::BufferWritableView IO::makeMessageBuffer() {

    // Standard size (in bytes) for a message buffer.
    //
    // The encoded size of a single message cannot exceeed this size
    // (note that the protocol uses a std::uint16_t in the message
    // header for the message size, therefore a message can't be
    // longer than 2^16 bytes (65536) anyway.
    //
    // Here we use a value which is slightly less than 2^16 to play
    // nice with memory allocators.
    static const std::size_t messageBufferStandardSizeBytes = 65500;

    // The buffer is allocated on the heap
    auto pd = std::make_shared<
        NetworkLib::PacketBufferArrayBased<messageBufferStandardSizeBytes>>();

    // Cast the result to a generic NetworkLib::PacketBuffer
    auto pb = std::static_pointer_cast<NetworkLib::PacketBuffer>(pd);

    // Return a BufferWritableView associated to the buffer.
    return NetworkLib::BufferWritableView(pb);
}

NetworkLib::BufferView
IO::readMessage(NetworkLib::BufferWritableView &readBuffer) {

    using namespace ReferenceProtocolStructs;

    // Refuse to read if there's no active connection
    if (mConnectionFD == -1) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": no connection";
        throw std::runtime_error(err.str());
    }

    // How many milliseconds we should sleep when waiting for further
    // data to read a whole message.
    static const int millisecondsToSleep = 100;

    // Check we have room at least to read the preamble of the common
    // header of messages.
    if (readBuffer.size() < Preamble::size) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": buffer too small (size is "
            << readBuffer.size() << ", at least " << Preamble::size
            << " required)";

        throw std::runtime_error(err.str());
    }

    unsigned char *rawBuffer = readBuffer.getUnderlyingWritableBufferPtr();

    // Attempt to read Protocol::Preamble::size bytes
    ssize_t bytesRead = 0;
    ssize_t rc;

    do {
        rc = read(mConnectionFD, rawBuffer + bytesRead,
                  Preamble::size - bytesRead);

        if (rc == -1) {
            int savedErrno = errno;

            if (savedErrno == EAGAIN || savedErrno == EWOULDBLOCK ||
                savedErrno == EINTR) {
                // Nothing to read at the moment (or we were interrupted by a
                // signal. Sleep for a while and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } else if (savedErrno == ECONNABORTED || savedErrno == ECONNRESET) {
                // End-of-file
                closeConnection();
                return NetworkLib::BufferView();

            } else {
                // Something serious happened
                std::ostringstream err;
                err << NETWORKLIB_CURRENT_FUNCTION << ": error reading from fd"
                    << mConnectionFD << " (errno = " << savedErrno << ": "
                    << std::strerror(savedErrno) << ')';

                closeConnection();
                throw std::runtime_error(err.str());
            }
        } else if (rc == 0) {
            // End-of-file
            closeConnection();
            return NetworkLib::BufferView();
        }

        // Otherwise, the result is the number of bytes read.
        // Add it to the total number of bytes read.
        bytesRead += rc;

    } while (bytesRead < Preamble::size);

    // At this point, we read Protocol::Preamble::size bytes. Now try
    // to read the rest of the common header and the rest of the
    // message.

    std::uint8_t version =
        readBuffer.getUint8At_nocheck(Preamble::versionOffset);
    ssize_t messageLength = static_cast<ssize_t>(
        readBuffer.getUint32At_nocheck(Preamble::lengthOffset));

    // Check we have enough room to read the whole message.
    if (readBuffer.size() < static_cast<std::size_t>(messageLength)) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": buffer too small (size is "
            << readBuffer.size() << ", messageLength is " << messageLength
            << ")";

        // Here we could just skip `messageLength - bytesRead` bytes,
        // and ignore the current message. But we prefer to make some
        // noise, close down the connection and throwing an error
        // (after all, we either received junk data, or our reading
        // buffer is undersized).
        closeConnection();
        throw std::runtime_error(err.str());
    }

    // Read in the rest of the common header and the payload of the
    // message.
    do {
        rc = read(mConnectionFD, rawBuffer + bytesRead,
                  messageLength - bytesRead);

        if (rc == -1) {
            int savedErrno = errno;

            if (savedErrno == EAGAIN || savedErrno == EWOULDBLOCK ||
                savedErrno == EINTR) {
                // Nothing to read at the moment (or we were interrupted by a
                // signal. Sleep for a while and retry
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(millisecondsToSleep));

            } else if (savedErrno == ECONNABORTED || savedErrno == ECONNRESET) {
                // End-of-file
                closeConnection();
                return NetworkLib::BufferView();
            } else {
                // Something serious happened
                std::ostringstream err;
                err << NETWORKLIB_CURRENT_FUNCTION << ": error reading fd "
                    << mConnectionFD << " (errno = " << savedErrno << ": "
                    << std::strerror(savedErrno) << ')';

                closeConnection();
                throw std::runtime_error(err.str());
            }

        } else if (rc == 0) {
            // End-of-file
            closeConnection();
            return NetworkLib::BufferView();
        }

        // Otherwise, the result is the number of bytes read.
        // Add it to the total number of bytes read.
        bytesRead += rc;
    } while (bytesRead < messageLength);

    // At this point, we read in all the `messageLength` bytes of the message.

    // Check that this is protocol version 2.
    if (version != 2) {
        // Just silently skip this message and return a size of 0.
        return NetworkLib::BufferView();
    }

    // Sanity check. Redundant, in theory, but it doesn't hurt.
    if (bytesRead != messageLength) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": error reading fd "
            << mConnectionFD << " (bytesRead is " << bytesRead
            << ", messageLength is " << messageLength << ')';

        throw std::runtime_error(err.str());
    }

    // Return a bufferview on the message
    return readBuffer.getSub(0, bytesRead);
}

bool IO::isDataAvailable() {
    // Refuse to read if there's neither an active connection nor a
    // listening socket.
    if (mConnectionFD == -1 && mListeningSocketFD == -1) {
        return false;
    }

    // Use the delay as a timeout (in milliseconds), and split it into seconds
    // and *microseconds*
    timeval tv;
    tv.tv_sec = mDelay_msec / 1000;
    tv.tv_usec = (mDelay_msec % 1000) * 1000;

    // Prepare the sets of file descriptors to monitor...
    fd_set readFDs;
    fd_set errorFDs;

    FD_ZERO(&readFDs);
    FD_ZERO(&errorFDs);

    if (mListeningSocketFD != -1) {
        FD_SET(mListeningSocketFD, &readFDs);
        FD_SET(mListeningSocketFD, &errorFDs);
    }

    if (mConnectionFD != -1) {
        FD_SET(mConnectionFD, &readFDs);
        FD_SET(mConnectionFD, &errorFDs);
    }

    // Note: we rely on the fact that a closed socket is -1
    int nfds = std::max(mListeningSocketFD, mConnectionFD) + 1;

    int rc = select(nfds, &readFDs, NULL, &errorFDs, &tv);

    if (rc == 0) {
        // Timeout expired
        return false;
    } else if (rc == -1) {
        // Some error occurred.
        int savedErrno = errno;
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": call to select(2) failed "
            << " (errno =" << savedErrno << ": " << std::strerror(savedErrno)
            << ')';
        throw std::runtime_error(err.str());
    }

    if (mListeningSocketFD != -1 && mConnectionFD == -1) {
        // There's a connection to accept
        acceptConnectionIfNeeded();

        if (mConnectionFD != -1) {
            // If now there's a connection, Wait again for data.
            return isDataAvailable();
        }
    }

    // There's something now to read.
    return true;
}

void IO::sleep() const {
    timeval tv;
    tv.tv_sec = mDelay_msec / 1000;
    tv.tv_usec = (mDelay_msec % 1000) * 1000;
    select(0, NULL, NULL, NULL, &tv);
}

std::size_t IO::writeMessage(const NetworkLib::BufferView &messageBuffer) {
    using namespace ReferenceProtocolStructs;

    // Refuse to read if there's no active connection
    if (mConnectionFD == -1) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": no connection";
        throw std::runtime_error(err.str());
    }

    // How many milliseconds we should sleep when waiting before
    // writing further data.
    static const int millisecondsToSleep = 100;

    // Attempt to write the message data.
    ssize_t bytesWritten = 0;
    ssize_t rc;
    ssize_t messageLength = static_cast<ssize_t>(
        messageBuffer.getUint32At_nocheck(Preamble::lengthOffset));
    const unsigned char *rawBuffer = messageBuffer.getUnderlyingBufferPtr();

    do {
        rc = write(mConnectionFD, rawBuffer + bytesWritten,
                   messageLength - bytesWritten);

        if (rc == -1) {
            int savedErrno = errno;

            if (savedErrno == EAGAIN || savedErrno == EWOULDBLOCK ||
                savedErrno == EINTR) {
                // Nothing to read at the moment (or we were interrupted by a
                // signal. Sleep for a while and retry
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(millisecondsToSleep));
            } else {
                // Something serious happened
                std::ostringstream err;
                err << NETWORKLIB_CURRENT_FUNCTION << ": error writing fd "
                    << mConnectionFD << " (errno = " << savedErrno << ")";

                closeConnection();
                throw std::runtime_error(err.str());
            }
        } else if (rc == 0) {
            // Something weird happened.
            closeConnection();
            return 0;
        }

        // Otherwise, the result is the number of bytes written out.
        // Add it to the total number of bytes written.
        bytesWritten += rc;

    } while (bytesWritten < messageLength);

    // Sanity check. Redundant, in theory, but it doesn't hurt.
    if (bytesWritten != messageLength) {
        std::ostringstream err;
        err << NETWORKLIB_CURRENT_FUNCTION << ": error writing fd "
            << mConnectionFD << " (bytesWritten is " << bytesWritten
            << ", messageLength is " << messageLength << ')';

        throw std::runtime_error(err.str());
    }

    // Return the number of written bytes
    return static_cast<std::size_t>(bytesWritten);
}

} // namespace Agent
} // namespace Empower
