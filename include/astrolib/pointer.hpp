#pragma once

/*
*
* Pointer related stuff
*
*/

#include <utility>
#include "meta.hpp"

namespace leapus::pointer{


//Probably long int in practice, and most processors don't actually have 64 bits of address space anyway. 48, often.
using offset_t =  decltype(std::declval<char *>() - std::declval<char *>());
using int_addr_t = offset_t;

template<typename T>
int_addr_t int_addressof(T &obj){
    //We're particular about constness because on some architectures const has a separate address space
    return (( meta::copy_cv_t<T, char *> )std::addressof(obj)) - ( meta::copy_cv_t<T, char *> )nullptr;
}

template<typename T>
T *ptr_from_addr(int_addr_t addr){
    return (T *)(((meta::copy_cv_t<T, char *>) nullptr)+addr);
}

template<typename T>
class relative_ptr{
    offset_t m_offset;
    
public:
    //Effectively points to address zero which is what NULL has traditionally meant in C-like settings
    //It's unlikely to be a valid address on most if any machines
    relative_ptr():
        m_offset( int_addressof((T *)nullptr) - int_addressof(this) ){}

    relative_ptr(::nullptr_t nul):
        relative_ptr(){}

    relative_ptr(T &obj):
        m_offset( int_addressof(obj) - int_addressof(*this) ){}

    relative_ptr(const relative_ptr &p):
        relative_ptr( *p ){}

    relative_ptr(relative_ptr &&) = delete;

    bool operator!() const{
        return int_addressof(*this) + m_offset != 0;
    }

    T *get() const{
        return ptr_from_addr<T>(int_addressof(*this) + m_offset);
    }

    bool operator==( const relative_ptr &rhs ) const{
        return get() == rhs.get();
    }

    T &operator[]( size_t i ) const{
        return get()[i];
    }

    T *operator->() const{
        return get();
    }

    T &operator*() const{
        return *get();
    }

    relative_ptr &operator++(){
        m_offset+=sizeof(T);
        return *this;
    }

    relative_ptr &operator--(){
        m_offset-=sizeof(T);
        return *this;
    }

    relative_ptr operator+( offset_t offs ){
        return { get() + offs }; 
    }

    relative_ptr operator-( offset_t offs){
        return { get() - offs };
    }

};


//Provide a new-like interface to an allocator
template<typename T, class Alloc, typename... Args>
typename Alloc::pointer_type new_with(Alloc &alloc, Args &&...args){
    auto p=alloc.allocate( 1 );
    alloc.construct(p, std::forward<Args>(args)... );
    return p;
}

};
