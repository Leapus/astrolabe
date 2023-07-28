#pragma once

/*
*
* A random access file abstraction that returns pointer objects into requested segments of a file.
* This could get rather complicated identifying usage for potentially overlapping segments,
* however, the basic implementation will be done the Easy Way™ by simply returning pointers into
* the entire memory-mapped file, and since most modern machines have 64-bit address spaces these days,
* the operating system can worry about all of the caching, paging, and actual I/O.
*
*/

#include <cstddef>
#include "astrolib/exception.hpp"

namespace leapus::io{

class io_exception:public exception::exception{
public:
    using exception::exception;
};

template<typename Ptr>
class random_access_file{
public:
    using pointer_type=Ptr;
    using size_type=::size_t;
    using pos_type=::size_t;

    virtual inline ~random_access_file(){};
    virtual pointer_type get( pos_type position, size_type size ) const = 0;

    template<typename T>
    const T *get(pos_type position, const leapus::meta::type<const T> &){
        return static_cast<const T *>(static_cast<const void *>( get(position, sizeof(T)) ));
    }
};

}



