#ifndef YY_STRING_VIEW_H_
#define YY_STRING_VIEW_H_
#include <cstring>
#include <string>
#include <vector>

class string_view
{
public:
    string_view() : pb_("") { pe_ = pb_; }
    string_view(const char *b, const char *e) : pb_(b), pe_(e) {}
    string_view(const char *d, size_t n) : pb_(d), pe_(d + n) {}
    string_view(const std::string &s) : pb_(s.data()), pe_(s.data() + s.size()) {}
    string_view(const char *s) : pb_(s), pe_(s + strlen(s)) {}

    const char *data() const { return pb_; }
    char* data() { return const_cast<char*>(pb_); }
    const char *begin() const { return pb_; }
    const char *end() const { return pe_; }
    char front() { return *pb_; }
    char back() { return pe_[-1]; }
    size_t size() const { return pe_ - pb_; }
    void resize(size_t sz) { pe_ = pb_ + sz; }
    inline bool empty() const { return pe_ == pb_; }
    void clear() { pe_ = pb_ = ""; }

    // return the eated data
    string_view eatWord();
    string_view eatLine();
    string_view eat(int sz)
    {
        string_view s(pb_, sz);
        pb_ += sz;
        return s;
    }
    string_view sub(int boff, int eoff = 0) const
    {
        string_view s(*this);
        s.pb_ += boff;
        s.pe_ += eoff;
        return s;
    }
    string_view &trimSpace();

    inline char operator[](size_t n) const { return pb_[n]; }

    std::string toString() const { return std::string(pb_, pe_); }
    // Three-way comparison.  Returns value:
    int compare(const string_view &b) const;

    // Return true if "x" is a prefix of "*this"
    bool starts_with(const string_view &x) const { return (size() >= x.size() && memcmp(pb_, x.pb_, x.size()) == 0); }

    bool end_with(const string_view &x) const { return (size() >= x.size() && memcmp(pe_ - x.size(), x.pb_, x.size()) == 0); }
    operator std::string() const { return std::string(pb_, pe_); }
    std::vector<string_view> split(char ch) const;

private:
    const char *pb_;
    const char *pe_;
};

inline string_view string_view::eatWord()
{
    const char *b = pb_;
    while (b < pe_ && isspace(*b))
    {
        b++;
    }
    const char *e = b;
    while (e < pe_ && !isspace(*e))
    {
        e++;
    }
    pb_ = e;
    return string_view(b, e - b);
}

inline string_view string_view::eatLine()
{
    const char *p = pb_;
    while (pb_ < pe_ && *pb_ != '\n' && *pb_ != '\r')
    {
        pb_++;
    }
    return string_view(p, pb_ - p);
}

inline string_view &string_view::trimSpace()
{
    while (pb_ < pe_ && isspace(*pb_))
        pb_++;
    while (pb_ < pe_ && isspace(pe_[-1]))
        pe_--;
    return *this;
}

inline bool operator<(const string_view &x, const string_view &y)
{
    return x.compare(y) < 0;
}

inline bool operator==(const string_view &x, const string_view &y)
{
    return ((x.size() == y.size()) && (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const string_view &x, const string_view &y)
{
    return !(x == y);
}

inline int string_view::compare(const string_view &b) const
{
    size_t sz = size(), bsz = b.size();
    const size_t min_len = (sz < bsz) ? sz : bsz;
    int r = memcmp(pb_, b.pb_, min_len);
    if (r == 0)
    {
        if (sz < bsz)
            r = -1;
        else if (sz > bsz)
            r = +1;
    }
    return r;
}

inline std::vector<string_view> string_view::split(char ch) const
{
    std::vector<string_view> r;
    const char *pb = pb_;
    for (const char *p = pb_; p < pe_; p++)
    {
        if (*p == ch)
        {
            r.push_back(string_view(pb, p));
            pb = p + 1;
        }
    }
    if (pe_ != pb_)
        r.push_back(string_view(pb, pe_));
    return r;
}
#endif