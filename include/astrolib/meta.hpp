#pragma once

/*
* Metaprogramming stuff
*/

#include <cstdint>

namespace leapus::meta{

//An empty class which can be passed around to represent a type
//without passing any actual content, and is hopefully readily optimized out
template<typename T>
struct type{
    using t=T;
};


template<typename Func>
struct guard_base{
    using function_type=Func;

protected:
    function_type p_func;

public:
    guard_base(const function_type &func):
        p_func(func){}

};

// Do some arbitrary thing as soon as the object goes out of scope or is destructed
template<typename Func, typename Data=void>
struct guard:protected guard_base<Func>{
    using data_type=Data;
    using typename guard_base<Func>::function_type;
private:
    data_type m_data;

public:
    guard(const data_type &d, const function_type &func):
        guard_base<Func>(func),
        m_data(d){}

    ~guard(){
        m_func(m_data);
    }
};

template<typename Func>
struct guard<Func, void>:protected guard_base<Func>{

    using base_type = guard_base<Func>;
    using typename base_type::function_type;
    using base_type::base_type;

    ~guard(){
        this->p_func();
    }
};

template<typename Data, typename Func>
guard(const Data &d, const Func &) -> guard<Data, Func>;

template<typename Func>
guard(const Func &) -> guard<Func>;


//::type is U, unless T is const, then it is const U
template<typename T, typename U>
struct copy_const{
    using type=U;
};

template<typename T, typename U>
struct copy_const<const T, U>{
    using type=const U;
};

//
// Realistically, this is probably dumb because if this were done in a loop, 
// it would probably get unrolled but whatever.
// Also, C++23 has native support for this, but I'm not cutting edge enough for that,
// and neither is Debian.
//
template<::size_t len, ::size_t i=len-1>
struct reverse_bytes{

protected:
    static void reverse_impl( const char *from, char  *to ){
        to[len-i-1]=from[i];
    }

public:
    static void reverse( const char *from, char *to ){
        reverse_impl(from, to);
        reverse_bytes<len, i-1>::reverse(from,to);
    }
};

template<::size_t len>
struct reverse_bytes<len, 0>:public reverse_bytes<len, 1>{
    static void reverse(const char *from, char *to){
        reverse_bytes<len,1>::reverse_impl(from, to);
    }
};

template<typename T>
struct reverse_endian{
    static T reverse(const T &v){
        T result;
        reverse_bytes<sizeof(T)>::reverse( (const char *)&v, (char *)&result );
        return result;        
    }
};

struct endian{
    static inline bool is_big(){
        static constexpr char x[]={ 0, 1 };
        static const ::int16_t y= *(const ::int16_t *)x;
        return y==1;
    }

    static inline bool is_little(){ return !is_big(); }

    /*
    *
    * Network byte order is assumed to be big-endian.
    * If the host is little-endian (PCs, for example, always are), perform a swap,
    * otherwise, NOP.
    */
    template<typename T>
    static T convert_endian(T v){
        return is_little() ? reverse_endian<T>::reverse(v) : v; 
    }

};

// So that you can specify a dumb pointer where a smart pointer would otherwise go
template<typename T>
using bare_pointer = T *;

}

