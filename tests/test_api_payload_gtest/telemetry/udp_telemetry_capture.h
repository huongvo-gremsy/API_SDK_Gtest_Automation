#ifndef PAYLOADSDK_TEST_UDP_TELEMETRY_CAPTURE_H_
#define PAYLOADSDK_TEST_UDP_TELEMETRY_CAPTURE_H_

#include "../common/payload_test_fixture.h"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace telemetry_test {

// Captures packets produced by the real PayloadSdkInterface encode/queue/write
// path. The SDK instance targets an ephemeral loopback UDP listener, so these
// tests do not inject synthetic navigation data into the connected payload.
class UdpTelemetryCapture {
public:
    UdpTelemetryCapture() {
        socket_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_ < 0) return;

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if (::bind(socket_, reinterpret_cast<sockaddr*>(&address),
                   sizeof(address)) != 0) {
            ::close(socket_);
            socket_ = -1;
            return;
        }

        socklen_t length = sizeof(address);
        if (::getsockname(socket_, reinterpret_cast<sockaddr*>(&address),
                          &length) != 0) {
            ::close(socket_);
            socket_ = -1;
            return;
        }

        T_ConnInfo connection{};
        connection.type = CONTROL_UDP;
        connection.device.udp.ip = loopbackAddress_;
        connection.device.udp.port = ntohs(address.sin_port);
        sdk_.reset(new PayloadSdkInterface(connection));
        started_ = sdk_->sdkInitConnection();
        drain();
    }

    ~UdpTelemetryCapture() {
        if (sdk_ && started_) sdk_->sdkQuit();
        sdk_.reset();
        if (socket_ >= 0) ::close(socket_);
    }

    bool ready() const { return socket_ >= 0 && started_ && sdk_; }

    bool sendAndCapture(uint32_t expectedMessageId,
                        const std::function<void(PayloadSdkInterface&)>& send,
                        mavlink_message_t& output, int timeoutMs = 3000) {
        if (!ready()) return false;
        drain();
        send(*sdk_);

        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(timeoutMs);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count();
            pollfd descriptor{socket_, POLLIN, 0};
            const int result = ::poll(&descriptor, 1,
                                      static_cast<int>(remaining));
            if (result <= 0) continue;

            uint8_t bytes[512] = {0};
            const ssize_t count = ::recv(socket_, bytes, sizeof(bytes), 0);
            if (count <= 0) continue;

            mavlink_message_t message{};
            mavlink_status_t status{};
            for (ssize_t i = 0; i < count; ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_3, bytes[i], &message,
                                       &status) &&
                    message.msgid == expectedMessageId) {
                    output = message;
                    return true;
                }
            }
        }
        return false;
    }

private:
    void drain() {
        if (socket_ < 0) return;
        uint8_t bytes[512];
        while (::recv(socket_, bytes, sizeof(bytes), MSG_DONTWAIT) > 0) {}
    }

    int socket_ = -1;
    bool started_ = false;
    char loopbackAddress_[16] = "127.0.0.1";
    std::unique_ptr<PayloadSdkInterface> sdk_;
};

class TelemetryPacketTest : public PayloadTest {
protected:
    void SetUp() override {
        capture_.reset(new UdpTelemetryCapture());
        ASSERT_TRUE(capture_->ready())
            << "Could not initialize loopback UDP telemetry capture.";
    }

    std::unique_ptr<UdpTelemetryCapture> capture_;
};

}  // namespace telemetry_test

#endif
