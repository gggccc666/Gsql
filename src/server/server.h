#ifndef GSQL_SERVER_H
#define GSQL_SERVER_H

#include <string>
#include "../common/protocol.h"

namespace gsql {

class GsqlServer {
private:
    int listen_fd_;
    bool running_;

    bool startListener(int port);
    void stopListener();

    // 处理单个客户端连接
    void handleClient(int client_fd);

    // 消息收发
    bool sendMessage(int fd, const Message& msg);
    bool recvMessage(int fd, Message& msg);

    // 业务处理
    bool handleLogin(int fd, const Message& req);
    void handleSQL(int fd, const Message& req);

public:
    GsqlServer();
    ~GsqlServer();
    void run(int port);
};

} // namespace gsql

#endif