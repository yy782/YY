#ifndef YY_NONCOPYABLE_H_
#define YY_NONCOPYABLE_H_ 

namespace yy{
class noncopyable
{
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};    
}



#endif