#ifndef YY_NET_CONFIG_CENTER_H_
#define YY_NET_CONFIG_CENTER_H_
#include <list>
#include <map>
#include <string>
#include <chrono>

namespace yy
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
// template<typename Rep, typename Period>
// std::chrono::duration<Rep, Period> Conf::getDuration(
//     const std::string& section, 
//     const std::string& name, 
//     std::chrono::duration<Rep, Period> default_value)
// {
//     std::string valstr = get(section, name, "");
//     if (valstr.empty()) 
//     {
//         return default_value;
//     }
    
    
//     std::string lower;
//     lower.reserve(valstr.size());
//     for (char c : valstr) 
//     {
//         if (!std::isspace(static_cast<char>(c))) 
//         {
//             lower.push_back(static_cast<char>(std::tolower(c)));
//         }
//     }
    
    
//     size_t pos = 0;
//     while (pos < lower.size() && 
//            (std::isdigit(lower[pos]) || lower[pos] == '.')) 
//     {
//         pos++;
//     }
    
//     std::string num_str = lower.substr(0, pos);
//     std::string unit = lower.substr(pos);
    
//     if (num_str.empty() || num_str == ".") 
//     {
//         return default_value;
//     }
    
//     char* end;
//     double num = std::strtod(num_str.c_str(), &end);
//     if (end == num_str.c_str())
//     {
//         return default_value;
//     }
    
//     using namespace std::chrono;
    
//     // 支持多种单位
//     if (unit.empty() || unit == "s" || unit == "sec" || unit == "second" || unit == "seconds") {
//         return duration_cast<std::chrono::duration<Rep, Period>>(duration<double>(num));
//     } else if (unit == "ms" || unit == "msec" || unit == "millisecond" || unit == "milliseconds") {
//         return duration_cast<std::chrono::duration<Rep, Period>>(duration<double, std::milli>(num));
//     } else if (unit == "us" || unit == "usec" || unit == "microsecond" || unit == "microseconds" || unit == "μs") {
//         return duration_cast<std::chrono::duration<Rep, Period>>(duration<double, std::micro>(num));
//     } else if (unit == "ns" || unit == "nsec" || unit == "nanosecond" || unit == "nanoseconds") {
//         return duration_cast<std::chrono::duration<Rep, Period>>(duration<double, std::nano>(num));
//     } else if (unit == "m" || unit == "min" || unit == "minute" || unit == "minutes") {
//         return duration_cast<std::chrono::duration<Rep, Period>>(duration<double, std::ratio<60>>(num));
//     } else if (unit == "h" || unit == "hr" || unit == "hour" || unit == "hours") {
//         return duration_cast<std::chrono::duration<Rep, Period>>(duration<double, std::ratio<3600>>(num));
//     }
    
//     return default_value;
// }
}

#endif
