#include "server.h"
#include "../client/tcp_utils.h"   // 复用 recv_all / send_all

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

namespace gsql {

GsqlServer::GsqlServer() : listen_fd_(-1), running_(false) {}

GsqlServer::~GsqlServer() {
    stopListener();
}

// ==================== 监听相关 ====================

bool GsqlServer::startListener(int port) {
    // 1. 创建 socket
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::cerr << "[Server] socket() failed" << std::endl;
        return false;
    }

    // 允许端口复用（开发时有用，避免 TIME_WAIT 导致 bind 失败）
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. 绑定地址
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 监听所有网卡
    addr.sin_port = htons(port);

    if (bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[Server] bind() failed on port " << port << std::endl;
        stopListener();
        return false;
    }

    // 3. 监听
    if (listen(listen_fd_, 5) < 0) {
        std::cerr << "[Server] listen() failed" << std::endl;
        stopListener();
        return false;
    }

    running_ = true;
    std::cout << "[Server] Listening on port " << port << " ..." << std::endl;
    return true;
}

void GsqlServer::stopListener() {
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
    running_ = false;
}

// ==================== 消息收发 ====================

bool GsqlServer::sendMessage(int fd, const Message& msg) {
    std::vector<char> buffer;
    msg.serialize(buffer);

    if (!send_all(fd, buffer.data(), buffer.size())) {
        std::cerr << "[Server] send failed on fd=" << fd << std::endl;
        return false;
    }
    return true;
}

bool GsqlServer::recvMessage(int fd, Message& msg) {
    // 收头部 5 字节
    char header_buf[MessageHeader::HEADER_SIZE];
    ssize_t n = recv_all(fd, header_buf, MessageHeader::HEADER_SIZE);
    if (n != static_cast<ssize_t>(MessageHeader::HEADER_SIZE)) {
        if (n > 0) std::cerr << "[Server] incomplete header" << std::endl;
        return false;
    }

    MessageHeader header = MessageHeader::deserialize(header_buf);

    // 收载荷
    std::vector<char> full_msg(header_buf, header_buf + MessageHeader::HEADER_SIZE);
    if (header.payload_length > 0) {
        full_msg.resize(MessageHeader::HEADER_SIZE + header.payload_length);
        n = recv_all(fd,
                     full_msg.data() + MessageHeader::HEADER_SIZE,
                     header.payload_length);
        if (n != static_cast<ssize_t>(header.payload_length)) {
            std::cerr << "[Server] recv payload failed" << std::endl;
            return false;
        }
    }

    msg = Message::deserialize(full_msg);
    return true;
}

// ==================== 业务处理 ====================

bool GsqlServer::handleLogin(int fd, const Message& req) {
    // payload 格式: "root\0123456"
    size_t pos = req.payload.find('\0');
    std::string user = req.payload.substr(0, pos);
    std::string pass = req.payload.substr(pos + 1);

    // 简单验证
    if (user == "root" && pass == "123456") {
        Message resp = Message::createLoginResponse(true, "Welcome");
        sendMessage(fd, resp);
        return true;
    } else {
        Message resp = Message::createLoginResponse(false, "Invalid credentials");
        sendMessage(fd, resp);
        return false;
    }
}

void GsqlServer::handleSQL(int fd, const Message& req) {
    std::cout << "[Server] Received SQL: " << req.payload << std::endl;

    // TODO: 调用 interpreter → executor → manager → storage
    // 这里暂时返回一个假的成功消息
    Message resp = Message::createOK("Query OK");
    sendMessage(fd, resp);
}

// ==================== 处理单个客户端 ====================

void GsqlServer::handleClient(int client_fd) {
    std::cout << "[Server] New client connected, fd=" << client_fd << std::endl;

    // 第一步：等待登录
    Message login_req;
    if (!recvMessage(client_fd, login_req)) {
        std::cerr << "[Server] Failed to receive login request" << std::endl;
        ::close(client_fd);
        return;
    }

    if (login_req.header.type != MessageType::LOGIN_REQUEST) {
        std::cerr << "[Server] Expected LOGIN_REQUEST, got something else" << std::endl;
        ::close(client_fd);
        return;
    }

    if (!handleLogin(client_fd, login_req)) {
        std::cerr << "[Server] Login failed" << std::endl;
        ::close(client_fd);
        return;
    }

    // 第二步：循环处理 SQL
    while (true) {
        Message req;
        if (!recvMessage(client_fd, req)) {
            std::cout << "[Server] Client disconnected, fd=" << client_fd << std::endl;
            break;
        }

        switch (req.header.type) {
            case MessageType::SQL_QUERY:
                handleSQL(client_fd, req);
                break;

            case MessageType::EXIT:
                std::cout << "[Server] Client sent EXIT, fd=" << client_fd << std::endl;
                ::close(client_fd);
                return;   // 退出函数，不关闭外层 listen

            default:
                std::cerr << "[Server] Unknown message type" << std::endl;
                break;
        }
    }

    ::close(client_fd);
}

// ==================== 主循环 ====================

void GsqlServer::run(int port) {
    if (!startListener(port)) return;

    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // 阻塞等待客户端连接
        int client_fd = accept(listen_fd_,
                               (struct sockaddr*)&client_addr,
                               &client_len);
        if (client_fd < 0) {
            if (running_)
                std::cerr << "[Server] accept() failed" << std::endl;
            continue;
        }

        // 打印客户端信息
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
        std::cout << "[Server] Connection from " << ip_str
                  << ":" << ntohs(client_addr.sin_port) << std::endl;

        // 处理（单线程版本，处理完这个才接下一个）
        handleClient(client_fd);
    }

    stopListener();
}

} // namespace gsql