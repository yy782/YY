
#include <cstring>

namespace yy
{
template<typename To, typename From>
inline To safe_static_cast(From from) {
    static_assert(std::is_pointer<From>::value, 
                  "From must be a pointer type");
    static_assert(std::is_pointer<To>::value,
                  "To must be a pointer type");
    return static_cast<To>(static_cast<void*>(from));
}
// @brief 这个safe_static_cast强行保证要转换的对象是指针，避免用
//          reinterpret_cast完成指针和非指针的转换，那是有问题的    

inline void memZero(void* p, size_t n)
{
  memset(p, 0, n);
}

}