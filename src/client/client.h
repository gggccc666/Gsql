//1.ifdef头文件作用

//2.包含的/common的protocol.h是什么
//Client 和 Server 之间的通信协议
//定义了消息的格式、类型和序列化方式。

//3.什么时候介入异常？
//

//4.static constexpr const char*
//static表示类本身 constexpr表常量在编译期求值
//static const string不行？
//只有整数类型和枚举类型的 static const 成员可以在类内初始化。
//操作string需要内存地址

//5.传参用const string&
//效果更好，引用不用拷贝

//6.static login
//用static修饰的函数实际上放在类内外都是可以的，他只是个工具函数
//如果不用static修饰，那么displaylogin必须创建authmanager对象才能调用login
//而login是个工具函数，天然适合static

//7.account==DEFAULT_USER为何合法？
#ifndef GSQL_CLIENT_H
#define GSQL_CLIENT_H

#include<string>
#include "common/protocol.h"

namespace gsql{
    class AuthManager{
        private:
            static constexpr const char* DEFAULT_USER="root";
            static constexpr const char* DEFAULT_PASSWORD="123456";

        public:
            bool static login(const std::string& account,const std::string& password );

    };
    class PrintInterractor{
        private:
        public:
            void static displayWelcome();
            void static displayResultset();
            void static displayHelp();
            void static displayTips();
            bool static doLoginWithRetry(int max_attempts);
    };
    class SqlCollector {
        public:
    // 从终端收集一条完整 SQL（以分号结束）
    // 返回 "exit" / "quit" 表示退出
    // 返回 "" 表示内部已处理元命令，需继续读取
           static std::string  collectSQL();

        private:
            static bool isMetaCommand(const std::string& line);
            static void handleMetaCommand(const std::string& line);
};
    class GsqlClient{
        private:
        public:
            void run();
    };
}

#endif