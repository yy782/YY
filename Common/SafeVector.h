#ifndef YY_SAFEVECTOR_H
#define YY_SAFEVECTOR_H
namespace yy
{
    template<class T>
    class SafeVector:copyable
    {
    public:
        SafeVector()=default;
        template<typename U>
        void push_back(U&& t)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            vec_.push_back(std::forward<U>(t));
        }
        bool empty()
        {
            std::lock_guard<std::mutex> lock(mtx_);
            return vec_.empty();
        }
        typename std::vector<T>::iterator begin()////////////////////////////////////
        {
            std::lock_guard<std::mutex> lock(mtx_);
            return vec_.begin();
        }
        typename std::vector<T>::iterator end()
        {
            std::lock_guard<std::mutex> lock(mtx_);
            return vec_.end();
        }
        SafeVector getAndClear()
        {
            std::lock_guard<std::mutex> lock(mtx_);
            SafeVector temp;
            temp.swap(vec_);
            return temp;
        }
        void swap(std::vector<T>& other)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            vec_.swap(other);
        }
        SafeVector& operator=(const SafeVector& other)
        {
            if(this!=&other)
            {
                std::lock_guard<std::mutex> lock(mtx_);
                vec_=other.vec_;
            }
            return *this;
        }
        SafeVector(SafeVector&& other)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            vec_=std::move(other.vec_);
        }
    private:
        std::vector<T> vec_;
        std::mutex mtx_;
    };
}
#endif // YY_SAFEVECTOR_H