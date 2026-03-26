#ifndef YY_TYPES_H_
#define YY_TYPES_H_

#include <cstring>
#include <type_traits>
#include <functional>
#include <memory>
namespace yy
{   

inline void memZero(void* p, size_t n)
{
  memset(p, 0, n);
}
//typedef std::shared_ptr<std::string> SharedString;
#define _1 std::placeholders::_1
#define _2 std::placeholders::_2
#define _3 std::placeholders::_3

struct Base;

}
#endif