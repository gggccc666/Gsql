#ifndef GSQL_STRING_UTILS_H
#define GSQL_STRING_UTILS_H

#include <string>
namespace gsql{
    inline std::string trim(const std::string& s){
        auto start = s.find_first_not_of("\t\n\r");
        if(start==std::string::npos)return "";
        auto end  = s.find_first_not_of("\t\n\r");
        return s.substr(start,end-start+1);
    }
}
#endif