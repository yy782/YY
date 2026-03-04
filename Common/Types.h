#ifndef _YY_TYPES_H_
#define _YY_TYPES_H_

#include <cstring>
#include <type_traits>
#include <functional>

namespace yy
{   

inline void memZero(void* p, size_t n)
{
  memset(p, 0, n);
}

#define _1 std::placeholders::_1
#define _2 std::placeholders::_2
#define _3 std::placeholders::_3

}
#endif