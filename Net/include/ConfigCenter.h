#ifndef YY_NET_CONFIG_CENTER_H_
#define YY_NET_CONFIG_CENTER_H_
#include <list>
#include <map>
#include <string>
#include <chrono>

namespace yy
{
namespace net 
{
struct Conf 
{
    int parse(const std::string &filename);

    std::string get(const std::string& section, const std::string& name, const std::string& default_value="");

    long getInteger(const std::string& section, const std::string& name, long default_value=0);

    double getReal(const std::string& section, const std::string& name, double default_value=0.0);

    bool getBoolean(const std::string& section, const std::string& name, bool default_value=false);

    std::list<std::string> getStrings(const std::string& section, const std::string& name);

    //template<typename Rep = long long, typename Period = std::micro>
    //std::chrono::duration<Rep, Period> getDuration(const std::string& section, const std::string& name, 
    //                                                std::chrono::duration<Rep, Period> default_value = std::chrono::duration<Rep, Period>(0));

    std::map<std::string, std::list<std::string>> values_;
    std::string filename_;
};
}
}
#endif
