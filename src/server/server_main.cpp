#include "server/server.h"
int main() {
    gsql::GsqlServer server;
    server.run(8888);  // 监听 8888 端口
    return 0;
}