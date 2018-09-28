// Copyright 2018 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "maybe_tls_crypto_socket.h"
#include "tls_crypto_socket.h"
#include "protocol_snooping.h"
#include <vespa/vespalib/data/smart_buffer.h>
#include <cassert>

namespace vespalib {

namespace {

class MyCryptoSocket : public CryptoSocket
{
private:
    static constexpr size_t SNOOP_SIZE = net::tls::snooping::min_header_bytes_to_observe();

    CryptoSocket::UP                &_self;
    SocketHandle                     _socket;
    std::shared_ptr<TlsCryptoEngine> _factory;
    SmartBuffer                      _buffer;

    bool is_blocked(ssize_t res, int error) const {
        return ((res < 0) && ((error == EWOULDBLOCK) || (error == EAGAIN)));
    }

    bool looksLikeTlsToMe(const char *buf) {
        return (net::tls::snooping::snoop_client_hello_header(buf) == net::tls::snooping::TlsSnoopingResult::ProbablyTls);
    }

public:
    MyCryptoSocket(CryptoSocket::UP &self, SocketHandle socket, std::shared_ptr<TlsCryptoEngine> tls_engine)
        : _self(self), _socket(std::move(socket)), _factory(std::move(tls_engine)), _buffer(4096)
    {
        assert(SNOOP_SIZE > 0); // we read before checking this
        assert(SNOOP_SIZE <= 8); // we promise this in our comment
    }
    int get_fd() const override { return _socket.get(); }
    HandshakeResult handshake() override {
        if (_factory) {
            auto dst = _buffer.reserve(SNOOP_SIZE);
            ssize_t res = _socket.read(dst.data, dst.size);
            if (res > 0) {
                _buffer.commit(res);
            } else if (!is_blocked(res, errno)) {
                return HandshakeResult::FAIL;
            }
            auto src = _buffer.obtain();
            if (src.size < SNOOP_SIZE) {
                return HandshakeResult::NEED_READ;                
            }
            if (looksLikeTlsToMe(src.data)) {
                CryptoSocket::UP &self = _self; // need copy due to self destruction
                auto tls_socket = _factory->create_tls_crypto_socket(std::move(_socket), true);
                tls_socket->inject_read_data(src.data, src.size);
                self = std::move(tls_socket);
                return self->handshake();
            } else {
                _factory.reset();
            }
        }
        return HandshakeResult::DONE;
    }
    size_t min_read_buffer_size() const override { return 1; }
    ssize_t read(char *buf, size_t len) override {
        int drain_result = drain(buf, len);
        if (drain_result != 0) {
            return drain_result;
        }
        return _socket.read(buf, len);
    }
    ssize_t drain(char *buf, size_t len) override {
        auto src = _buffer.obtain();
        size_t frame = std::min(len, src.size);
        if (frame > 0) {
            memcpy(buf, src.data, frame);
            _buffer.evict(frame);
        }
        return frame;
    }
    ssize_t write(const char *buf, size_t len) override { return _socket.write(buf, len); }
    ssize_t flush() override { return 0; }
};

} // namespace vespalib::<unnamed>

MaybeTlsCryptoSocket::MaybeTlsCryptoSocket(SocketHandle socket, std::shared_ptr<TlsCryptoEngine> tls_engine)
    : _socket(std::make_unique<MyCryptoSocket>(_socket, std::move(socket), std::move(tls_engine)))
{
}

} // namespace vespalib
