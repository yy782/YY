
#include "ConfigCenter.h"

#include <string>
#include <cstdlib>
#include <algorithm>
#include <memory>
namespace yy
{
namespace net
{
static std::string makeKey(const std::string& section, const std::string& name) {
    std::string key = section + "." + name;
    // Convert to lower case to make section/name lookups case-insensitive
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    return key;
}

std::string Conf::get(const std::string& section, const std::string& name, const std::string& default_value) {
    std::string key = makeKey(section, name);
    auto p = values_.find(key);
    return p == values_.end() ? default_value : p->second.back();
}

std::list<std::string> Conf::getStrings(const std::string& section, const std::string& name) {
    std::string key = makeKey(section, name);
    auto p = values_.find(key);
    return p == values_.end() ? std::list<std::string>() : p->second;
}

long Conf::getInteger(const std::string& section, const std::string& name, long default_value) {
    std::string valstr = get(section, name, "");
    const char *value = valstr.c_str();
    char *end;
    // This parses "1234" (decimal) and also "0x4D2" (hex)
    long n = strtol(value, &end, 0);
    return end > value ? n : default_value;
}

double Conf::getReal(const std::string& section, const std::string& name, double default_value) {
    std::string valstr = get(section, name, "");
    const char *value = valstr.c_str();
    char *end;
    double n = strtod(value, &end);
    return end > value ? n : default_value;
}

bool Conf::getBoolean(const std::string& section, const std::string& name, bool default_value) {
    std::string valstr = get(section, name, "");
    // Convert to lower case to make std::string comparisons case-insensitive
    std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
    if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
        return true;
    else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
        return false;
    else
        return default_value;
}

namespace {
struct LineScanner {
    char *p;
    int err;
    LineScanner(char *ln) : p(ln), err(0) {}
    LineScanner &skipSpaces() {
        while (!err && *p && isspace(*p)) {
            p++;
        }
        return *this;
    }
    static std::string rstrip(char *s, char *e) {
        while (e > s && isspace(e[-1])) {
            e--;
        }
        return {s, e};
    }
    int peekChar() {
        skipSpaces();
        return *p;
    }
    LineScanner &skip(int i) {
        p += i;
        return *this;
    }
    LineScanner &match(char c) {
        skipSpaces();
        err = *p++ != c;
        return *this;
    }
    std::string consumeTill(char c) {
        skipSpaces();
        char *e = p;
        while (!err && *e && *e != c) {
            e++;
        }
        if (*e != c) {
            err = 1;
            return "";
        }
        char *s = p;
        p = e;
        return rstrip(s, e);
    }
    std::string consumeTillEnd() {
        skipSpaces();
        char *e = p;
        int wasspace = 0;
        while (!err && *e && *e != ';' && *e != '#') {
            if (wasspace) {
                break;
            }
            wasspace = isspace(*e);
            e++;
        }
        char *s = p;
        p = e;
        return rstrip(s, e);
    }
};
}  // namespace

int Conf::parse(const std::string &filename) {
    this->filename_ = filename;
    FILE *file = fopen(this->filename_.c_str(), "r");   
    if (!file)
        return -1;
    std::unique_ptr<FILE, decltype(fclose) *> release2(file, fclose);
    static const int MAX_LINE = 16 * 1024;
    char *ln = new char[MAX_LINE];
    std::unique_ptr<char[]> release1(ln);
    int lineno = 0;
    std::string section, key;
    int err = 0;
    while (!err && fgets(ln, MAX_LINE, file) != nullptr) {
        lineno++;
        LineScanner ls(ln);
        int c = ls.peekChar();
        if (c == ';' || c == '#' || c == '\0') {
            continue;
        } else if (c == '[') {
            section = ls.skip(1).consumeTill(']');
            err = ls.match(']').err;
            key = "";
        } else if (isspace(ln[0])) {
            /* Non-black line with leading whitespace, treat as continuation
               of previous name's value (as per Python ConfigParser). */
            if (!key.empty()) {
                values_[makeKey(section, key)].push_back(ls.consumeTill('\0'));
            } else {
                err = 1;
            }
        } else {
            LineScanner lsc = ls;
            key = ls.consumeTill('=');
            if (ls.peekChar() == '=') {
                ls.skip(1);
            } else {
                ls = lsc;
                key = ls.consumeTill(':');
                err = ls.match(':').err;
            }
            std::string value = ls.consumeTillEnd();
            values_[makeKey(section, key)].push_back(value);
        }
    }
    return err ? lineno : 0;
}     
}    
}