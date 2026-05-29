#ifndef GSQL_TCP_UTILS_H
#define GSQL_TCP_UTILS_H

#include <sys/socket.h>
#include <unistd.h>
#include <cstddef>

namespace gsql {

// 保证收满 len 字节
// 返回实际收到的字节数，出错返回 -1
inline ssize_t recv_all(int fd, void* buf, size_t len) {
    size_t total = 0;
    char* p = static_cast<char*>(buf);
    while (total < len) {
        ssize_t n = recv(fd, p + total, len - total, 0);
        if (n <= 0) {
            return (n == 0 && total > 0) ? static_cast<ssize_t>(total) : -1;
        }
        total += n;
    }
    return static_cast<ssize_t>(total);
}

// 保证发完 len 字节
inline bool send_all(int fd, const void* buf, size_t len) {
    size_t total = 0;
    const char* p = static_cast<const char*>(buf);
    while (total < len) {
        ssize_t n = send(fd, p + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

} // namespace gsql

#endif