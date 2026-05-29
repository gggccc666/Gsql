//unit8_t?0x01

#ifndef GSQL_PROTOCOL_H
#define GSQL_PROTOCOL_H

#include <string>
#include <cstring>
#include <cstdint>
#include <vector>
#include <arpa/inet.h>

namespace gsql{

    enum class MessageType : uint8_t {
    LOGIN_REQUEST   = 0x01,
    LOGIN_RESPONSE  = 0x02,
    SQL_QUERY       = 0x03,
    RESULT_SET      = 0x04,
    ERROR_MSG       = 0x05,
    OK_MSG          = 0x06,
    EXIT            = 0x07,
};      
    struct MessageHeader {
    MessageType type;
    uint32_t payload_length;      // 载荷长度（网络字节序）

    static constexpr size_t HEADER_SIZE = 5;  // 1 + 4

    // 序列化头部到 buffer
    void serialize(std::vector<char>& buffer) const {
        buffer.push_back(static_cast<char>(type));
        uint32_t net_len = htonl(payload_length);
        const char* p = reinterpret_cast<const char*>(&net_len);
        buffer.insert(buffer.end(), p, p + sizeof(net_len));
    }

    // 从原始字节反序列化头部
    static MessageHeader deserialize(const char* data) {
        MessageHeader h;
        h.type = static_cast<MessageType>(data[0]);
        uint32_t net_len;
        std::memcpy(&net_len, data + 1, sizeof(net_len));
        h.payload_length = ntohl(net_len);
        return h;
    }
};

struct Message {
    MessageHeader header;
    std::string payload;

    static Message createLoginRequest(const std::string& user,
                                       const std::string& pass) {
        Message m;
        m.header.type = MessageType::LOGIN_REQUEST;
        m.payload = user + '\0' + pass;
        m.header.payload_length = static_cast<uint32_t>(m.payload.size());
        return m;
    }

    static Message createLoginResponse(bool ok, const std::string& info = "") {
        Message m;
        m.header.type = MessageType::LOGIN_RESPONSE;
        m.payload = (ok ? std::string("OK") : std::string("FAIL"))
                    + '\0' + info;
        m.header.payload_length = static_cast<uint32_t>(m.payload.size());
        return m;
    }

    static Message createSQLQuery(const std::string& sql) {
        Message m;
        m.header.type = MessageType::SQL_QUERY;
        m.payload = sql;
        m.header.payload_length = static_cast<uint32_t>(m.payload.size());
        return m;
    }

    static Message createResultSet(const std::string& result) {
        Message m;
        m.header.type = MessageType::RESULT_SET;
        m.payload = result;
        m.header.payload_length = static_cast<uint32_t>(m.payload.size());
        return m;
    }

    static Message createError(const std::string& err) {
        Message m;
        m.header.type = MessageType::ERROR_MSG;
        m.payload = err;
        m.header.payload_length = static_cast<uint32_t>(m.payload.size());
        return m;
    }

    static Message createOK(const std::string& info = "") {
        Message m;
        m.header.type = MessageType::OK_MSG;
        m.payload = info;
        m.header.payload_length = static_cast<uint32_t>(m.payload.size());
        return m;
    }

    static Message createExit() {
        Message m;
        m.header.type = MessageType::EXIT;
        m.header.payload_length = 0;
        return m;
    }

    // 序列化 / 反序列化

    void serialize(std::vector<char>& buffer) const {
        buffer.clear();
        header.serialize(buffer);
        if (!payload.empty()) {
            buffer.insert(buffer.end(), payload.begin(), payload.end());
        }
    }

    static Message deserialize(const std::vector<char>& buffer) {
        Message m;
        if (buffer.size() < MessageHeader::HEADER_SIZE) return m;

        m.header = MessageHeader::deserialize(buffer.data());
        size_t total = MessageHeader::HEADER_SIZE + m.header.payload_length;
        if (buffer.size() >= total) {
            m.payload.assign(buffer.begin() + MessageHeader::HEADER_SIZE,
                             buffer.begin() + total);
        }
        return m;
    }
};



}

#endif