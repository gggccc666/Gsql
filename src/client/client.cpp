#include<iostream>
#include<string>
#include<cstdlib>

#include<client.h>

#include "common/utils/string_utils.h"
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
        std::cout<<"type \"exit\" or \"quit\" to leave Gsql";
        std::cout<<"type \"clear\"  to clear screen.\n";
        std::cout<<"notice that your sql statement should end with a \";\""<<std::endl;
        std::cout<<"tips:Gsql's input is not so automatic,so like \"insert into table_name() values(\"abc;def\")\"will cause bugs because of \";\""<<std::endl;
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
                return ""; 
            }
            std::string subline=trim(line);

            if(subline.empty()){
                continue;
            }
            if(flag_first){
                if (subline == "exit" || subline == "quit") {
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
        return line == "help" || line == "clear";
    }
    void SqlCollector::handleMetaCommand(const std::string& line) {
        if (line == "help") {
           PrintInterractor::displayHelp();
        } 
        else if (line == "clear") {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        }
    }
    void GsqlClient::run(){
        PrintInterractor::displayWelcome();
        PrintInterractor::doLoginWithRetry(5);
        PrintInterractor::displayTips();
    }
}