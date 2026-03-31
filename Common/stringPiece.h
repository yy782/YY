/**
 * @file stringPiece.h
 * @brief 字符串视图类的定义
 * 
 * 本文件定义了stringPiece类，用于高效地表示和操作字符串，避免不必要的内存拷贝。
 */

#ifndef YY_STRING_VIEW_H_
#define YY_STRING_VIEW_H_
#include <cstring>
#include <string>
#include <vector>
#include <ostream>
namespace yy
{ 

/**
 * @brief 字符串视图类
 * 
 * stringPiece是一个轻量级的字符串表示类，它不拥有字符串内存，而是通过指针指向字符串的开始和结束位置，
 * 避免了不必要的内存拷贝，提高了字符串操作的效率。
 */
class stringPiece
{
public:
    /**
     * @brief 默认构造函数
     */
    stringPiece() : pb_("") { pe_ = pb_; }
    /**
     * @brief 构造函数
     * 
     * @param b 字符串开始指针
     * @param e 字符串结束指针
     */
    stringPiece(const char *b, const char *e) : pb_(b), pe_(e) {}
    /**
     * @brief 构造函数
     * 
     * @param d 字符串数据指针
     * @param n 字符串长度
     */
    stringPiece(const char *d, size_t n) : pb_(d), pe_(d + n) {}
    /**
     * @brief 构造函数
     * 
     * @param s 标准字符串
     */
    stringPiece(const std::string &s) : pb_(s.data()), pe_(s.data() + s.size()) {}
    /**
     * @brief 构造函数
     * 
     * @param s 以null结尾的字符串
     */
    stringPiece(const char *s) : pb_(s), pe_(s + strlen(s)) {}

    /**
     * @brief 赋值运算符
     * 
     * @param s 标准字符串
     * @return stringPiece& 引用自身
     */
    stringPiece& operator=(const std::string& s)
    {
        pb_ = s.data();
        pe_ = s.data() + s.size();
        return *this;
    }
    /**
     * @brief 获取字符串数据指针
     * 
     * @return const char* 字符串数据指针
     */
    const char *data() const { return pb_; }
    /**
     * @brief 获取字符串数据指针（非const版本）
     * 
     * @return char* 字符串数据指针
     */
    char* data() { return const_cast<char*>(pb_); }
    /**
     * @brief 获取字符串开始位置
     * 
     * @return const char* 字符串开始指针
     */
    const char *begin() const { return pb_; }
    /**
     * @brief 获取字符串结束位置
     * 
     * @return const char* 字符串结束指针
     */
    const char *end() const { return pe_; }
    /**
     * @brief 获取字符串第一个字符
     * 
     * @return char 第一个字符
     */
    char front() { return *pb_; }
    /**
     * @brief 获取字符串最后一个字符
     * 
     * @return char 最后一个字符
     */
    char back() { return pe_[-1]; }
    /**
     * @brief 获取字符串长度
     * 
     * @return size_t 字符串长度
     */
    size_t size() const { return pe_ - pb_; }
    /**
     * @brief 调整字符串大小
     * 
     * @param sz 新的字符串长度
     */
    void resize(size_t sz) { pe_ = pb_ + sz; }
    /**
     * @brief 检查字符串是否为空
     * 
     * @return bool 是否为空
     */
    inline bool empty() const { return pe_ == pb_; }
    /**
     * @brief 清空字符串
     */
    void clear() { pe_ = pb_ = ""; }

    /**
     * @brief 消费并返回一个单词
     * 
     * @return stringPiece 消费的单词
     */
    stringPiece eatWord();
    /**
     * @brief 消费并返回一行
     * 
     * @return stringPiece 消费的行
     */
    stringPiece eatLine();
    /**
     * @brief 消费指定长度的字符串
     * 
     * @param sz 消费的长度
     * @return stringPiece 消费的字符串
     */
    stringPiece eat(int sz)
    {
        stringPiece s(pb_, sz);
        pb_ += sz;
        return s;
    }
    /**
     * @brief 获取子字符串
     * 
     * @param boff 开始偏移量
     * @param eoff 结束偏移量
     * @return stringPiece 子字符串
     */
    stringPiece sub(int boff, int eoff = 0) const
    {
        stringPiece s(*this);
        s.pb_ += boff;
        s.pe_ += eoff;
        return s;
    }
    /**
     * @brief 去除字符串首尾的空白字符
     * 
     * @return stringPiece& 引用自身
     */
    stringPiece &trimSpace();

    /**
     * @brief 下标运算符
     * 
     * @param n 索引
     * @return char 指定位置的字符
     */
    inline char operator[](size_t n) const { return pb_[n]; }

    /**
     * @brief 转换为标准字符串
     * 
     * @return std::string 标准字符串
     */
    std::string toString() const { return std::string(pb_, pe_); }
    /**
     * @brief 比较两个字符串
     * 
     * @param b 要比较的字符串
     * @return int 比较结果
     */
    int compare(const stringPiece &b) const;
    /**
     * @brief 检查字符串是否以指定前缀开始
     * 
     * @param x 前缀
     * @return bool 是否以指定前缀开始
     */
    bool starts_with(const stringPiece &x) const { return (size() >= x.size() && memcmp(pb_, x.pb_, x.size()) == 0); }
    /**
     * @brief 检查字符串是否以指定后缀结束
     * 
     * @param x 后缀
     * @return bool 是否以指定后缀结束
     */
    bool end_with(const stringPiece &x) const { return (size() >= x.size() && memcmp(pe_ - x.size(), x.pb_, x.size()) == 0); }
    /**
     * @brief 转换为标准字符串（隐式转换）
     * 
     * @return std::string 标准字符串
     */
    operator std::string() const { return std::string(pb_, pe_); }
    /**
     * @brief 分割字符串
     * 
     * @param ch 分隔符
     * @return std::vector<stringPiece> 分割后的字符串列表
     */
    std::vector<stringPiece> split(char ch) const;

private:
    const char *pb_; ///< 字符串开始指针
    const char *pe_; ///< 字符串结束指针
};

inline stringPiece stringPiece::eatWord()
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
    return stringPiece(b, e - b);
}

inline stringPiece stringPiece::eatLine()
{
    const char *p = pb_;
    while (pb_ < pe_ && *pb_ != '\n' && *pb_ != '\r')
    {
        pb_++;
    }
    return stringPiece(p, pb_ - p);
}
inline stringPiece &stringPiece::trimSpace()
{
    while (pb_ < pe_ && isspace(*pb_))
        pb_++;
    while (pb_ < pe_ && isspace(pe_[-1]))
        pe_--;
    return *this;
}
inline bool operator<(const stringPiece &x, const stringPiece &y)
{
    return x.compare(y) < 0;
}


inline bool operator==(const stringPiece &x, const stringPiece &y)
{
    return ((x.size() == y.size()) && (memcmp(x.data(), y.data(), x.size()) == 0));
}

inline bool operator!=(const stringPiece &x, const stringPiece &y)
{
    return !(x == y);
}

inline int stringPiece::compare(const stringPiece &b) const
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


inline std::vector<stringPiece> stringPiece::split(char ch) const
{
    std::vector<stringPiece> r;
    const char *pb = pb_;
    for (const char *p = pb_; p < pe_; p++)
    {
        if (*p == ch)
        {
            r.push_back(stringPiece(pb, p));
            pb = p + 1;
        }
    }
    if (pe_ != pb_)
        r.push_back(stringPiece(pb, pe_));
    return r;
}
}
#endif