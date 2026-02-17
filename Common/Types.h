#ifndef _YY_TYPES_H_
#define _YY_TYPES_H_

#include <cstring>
#include <type_traits>

namespace yy
{
template<typename To, typename From>
inline To safe_static_cast(From from){
    return static_cast<To>(from);
}
   

inline void memZero(void* p, size_t n)
{
  memset(p, 0, n);
}


typedef size_t byte_size;

}
#endif