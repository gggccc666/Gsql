#include<iostream>
#include<string>
#include<cstdlib>

#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>

#include"client.h"
#include "tcp_utils.h"

#include "../common/utils/string_utils.h"
namespace gsql{
    bool AuthManager::login(const std::string& account,const std::string& password){
        return account == DEFAULT_USER && password == DEFAULT_PASSWORD;
    }
    void PrintInterractor::displayWelcome(){
        std::cout << R"(
      ┌──────────────────────────────────┐
      │       Welcome to Gsql DB         │
      │       Toy Database v0.1          │
      └──────────────────────────────────┘
        )" << std::endl;
        std::cout<<"please login first."<<std::endl;
        std::cout<<"Gsql have default account \"root\" with password 123456. "<<std::endl;
    }
    void PrintInterractor::displayTips(){

        std::cout<<"type \"help\" to get help"<<std::endl;
        std::cout<<"type \"exit\" or \"quit\" to leave Gsql"<<std::endl;
        std::cout<<"type \"clear\"  to clear screen.\n"<<std::endl;
        std::cout<<"notice that your sql statement should end with a \";\""<<std::endl;
        std::cout<<"tips:Gsql's input algorithm is not so strong,so sentenses like \"insert into table_name() values(\"abc;def\")\"will cause bugs because of \";\""<<std::endl;
    }
    void PrintInterractor::displayHelp() {
        std::cout << "\n";
        std::cout << "═══════════════════════════════════════════════\n";
        std::cout << "  Gsql 支持的 SQL 语句\n";
        std::cout << "═══════════════════════════════════════════════\n";
        std::cout << "\n";
        std::cout << "  【DDL】\n";
        std::cout << "    CREATE DATABASE <库名>;\n";
        std::cout << "    DROP DATABASE <库名>;\n";
        std::cout << "    USE <库名>;\n";
        std::cout << "    CREATE TABLE <表名> (列名 类型 [PRIMARY], ...);\n";
        std::cout << "      类型: INT / STRING(256)\n";
        std::cout << "      PRIMARY 列自动建立 B+ 树索引\n";
        std::cout << "    DROP TABLE <表名>;\n";
        std::cout << "\n";
        std::cout << "  【DML】\n";
        std::cout << "    INSERT INTO <表名> VALUES (v1, v2, ...);\n";
        std::cout << "      字符串用双引号\n";
        std::cout << "    SELECT <列名/*> FROM <表名> [WHERE 列 =/</> 值];\n";
        std::cout << "      有索引自动使用索引\n";
        std::cout << "    DELETE FROM <表名> [WHERE 列 =/</> 值];\n";
        std::cout << "    UPDATE <表名> SET 列 = 值 [WHERE 列 =/</> 值];\n";
        std::cout << "\n";
        std::cout << "  【其他】\n";
        std::cout << "    help          显示此帮助\n";
        std::cout << "    clear         清屏\n";
        std::cout << "    exit / quit   退出\n";
        std::cout << "═══════════════════════════════════════════════\n";
        std::cout << "\n";
}

    bool PrintInterractor::doLoginWithRetry(int max_attempts) {
    for (int i = 0; i < max_attempts; i++) {
        std::string username, password;
        std::cout << "Account: ";
        std::getline(std::cin, username);
        std::cout << "Password: ";
        std::getline(std::cin, password);
        
        if (AuthManager::login(username, password)) {
            std::cout << "Login successfully!\n\n";
            return true;
        }
        
        int remaining = max_attempts - i - 1;
        if (remaining > 0) {
            std::cout << "Invalid credentials. " << remaining 
                      << " attempt(s) remaining.\n\n";
        }
    }
    std::cout << "Too many failed attempts. Exiting.\n";
    return false;
    }
    std::string SqlCollector::collectSQL(){
        std::string sql;
        std::string line;
        bool flag_first=true;
        while(true){
            std::cout<<(flag_first?"Gsql>":"    >");
            std::cout.flush();

            if(!std::getline(std::cin, line)){
                return "exit"; 
            }
            std::string subline=trim(line);
            
            if(subline.empty()){
                continue;
            }
            if(flag_first){
                if (subline == "exit" || subline == "quit" || subline == "exit;" || subline == "quit;" ) {
                    return subline;
                }

                if(isMetaCommand(subline)){
                    handleMetaCommand(subline);
                    return "";
                }
            }   

            if(flag_first){
                sql=subline;
                flag_first=false;
            }
            else sql+=" "+subline;

            if(subline.back()==';'){
                break;
            }
        }
        return sql;
    }
    bool SqlCollector::isMetaCommand(const std::string& line) {
        return line == "help" || line == "clear"|| line == "help;" || line == "clear;";
    }
    void SqlCollector::handleMetaCommand(const std::string& line) {
        if (line == "help"||line=="help;") {
           PrintInterractor::displayHelp();
        } 
        else if (line == "clear"||line == "clear;") {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        }
    }
    GsqlClient::GsqlClient() : sockfd_(-1), connected_(false) {}

GsqlClient::~GsqlClient() {
    disconnect();
}

// ---------- 连接 ----------
bool GsqlClient::connectToServer(const std::string& ip, int port) {
    // 1. 创建 socket
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd_ < 0) {
        std::cerr << "[Client] socket() failed" << std::endl;
        return false;
    }

    // 2. 服务器地址
    struct sockaddr_in serv_addr;
    std::memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "[Client] Invalid address: " << ip << std::endl;
        disconnect();
        return false;
    }

    // 3. connect
    if (::connect(sockfd_, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "[Client] connect() to " << ip << ":" << port << " failed" << std::endl;
        disconnect();
        return false;
    }

    connected_ = true;
    std::cout << "[Client] Connected to server " << ip << ":" << port << std::endl;
    return true;
}

// ---------- 断开 ----------
void GsqlClient::disconnect() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
    connected_ = false;
}

// ---------- 发送消息 ----------
bool GsqlClient::sendMessage(const Message& msg) {
    if (!connected_) return false;

    // 序列化
    std::vector<char> buffer;
    msg.serialize(buffer);

    // 发送全部字节
    if (!send_all(sockfd_, buffer.data(), buffer.size())) {
        std::cerr << "[Client] send failed" << std::endl;
        disconnect();
        return false;
    }
    return true;
}

// ---------- 接收消息 ----------
bool GsqlClient::recvMessage(Message& msg) {
    if (!connected_) return false;

    // 1. 先收 5 字节头部
    char header_buf[MessageHeader::HEADER_SIZE];
    ssize_t n = recv_all(sockfd_, header_buf, MessageHeader::HEADER_SIZE);
    if (n != static_cast<ssize_t>(MessageHeader::HEADER_SIZE)) {
        std::cerr << "[Client] recv header failed" << std::endl;
        disconnect();
        return false;
    }

    MessageHeader header = MessageHeader::deserialize(header_buf);

    // 2. 组装完整 buffer（头部 + 载荷）
    std::vector<char> full_msg(header_buf, header_buf + MessageHeader::HEADER_SIZE);
    if (header.payload_length > 0) {
        full_msg.resize(MessageHeader::HEADER_SIZE + header.payload_length);
        n = recv_all(sockfd_,
                     full_msg.data() + MessageHeader::HEADER_SIZE,
                     header.payload_length);
        if (n != static_cast<ssize_t>(header.payload_length)) {
            std::cerr << "[Client] recv payload failed" << std::endl;
            disconnect();
            return false;
        }
    }

    // 3. 反序列化
    msg = Message::deserialize(full_msg);
    return true;
}

// ---------- 服务器登录 ----------
bool GsqlClient::doServerLogin() {
    // 发送 LOGIN_REQUEST
    Message req = Message::createLoginRequest("root", "123456");
    if (!sendMessage(req)) return false;

    // 接收 LOGIN_RESPONSE
    Message resp;
    if (!recvMessage(resp)) return false;

    // 解析响应
    const char* p = resp.payload.c_str();
    if (std::strncmp(p, "OK", 2) == 0) {
        std::cout << "[Client] Server login OK" << std::endl;
        return true;
    }

    const char* info = p + 5;  // 跳过 "FAIL\0"
    std::cerr << "[Client] Server login failed: " << info << std::endl;
    return false;
}

    void GsqlClient::run(){
        PrintInterractor::displayWelcome();

        if(!PrintInterractor::doLoginWithRetry(5))return;

        if (!connectToServer("127.0.0.1", 8888)) {
        std::cerr << "[Client] Cannot connect to server. Exiting." << std::endl;
        return;
    }

    if (!doServerLogin()) return;

        PrintInterractor::displayTips();

        std::string sql;
        while(true){
            sql = SqlCollector::collectSQL();

        if (sql == "exit" || sql == "quit" || sql == "exit;" || sql == "quit;") {
            Message exit_msg = Message::createExit();
            sendMessage(exit_msg);  // 告诉服务器我要走了
            break;
        }
        if (sql.empty()) continue;

        // 发送 SQL 查询
        if (!sendMessage(Message::createSQLQuery(sql))) break;

        // 接收响应
        Message resp;
        if (!recvMessage(resp)) break;

        // 显示
        switch (resp.header.type) {
            case MessageType::RESULT_SET:
            case MessageType::OK_MSG:
                std::cout << resp.payload << std::endl;
                break;
            case MessageType::ERROR_MSG:
                std::cerr << "Error: " << resp.payload << std::endl;
                break;
            default:
                std::cout << resp.payload << std::endl;
                break;
        }
        }
          disconnect();
        std::cout << "Goodbye!" << std::endl;
    }
}