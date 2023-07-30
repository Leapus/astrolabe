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
#include "astrolib/meta.hpp"
#include "astrolib/exception.hpp"

namespace leapus::io{

class io_exception:public exception::exception{
public:
    using exception::exception;
};

template< template<typename> class Ptr=meta::bare_pointer>
class random_access_file{

public:

    template<typename T>
    using pointer_type=Ptr<T>;

    template<typename T>
    using const_pointer_type=Ptr<const T>;

    using size_type=::size_t;
    using pos_type=::size_t;

public:
    virtual inline ~random_access_file(){};

    //Reading off the end of a const file will throw
    virtual inline const_pointer_type<char> read( pos_type position, size_type size ) const = 0;
    //    throw std::runtime_error("unimplemented");
    //}

    //Reading off the end of a non-const file will grow the file and may invalidate
    //all addresses into the file, which is why we support smart pointers.
    //However, since this data will be almost always write-once and read-only,
    //we won't make any effort to make file expansion thread-safe. For now, it's only
    //the map indexer which will write PBF files, and it can do that in one thread
    //without races.
    virtual pointer_type<char> read( pos_type position, size_type size ) = 0;

    template<typename T>
    pointer_type<T> read(pos_type position, const leapus::meta::type<T> &){
        return  { (T *)this->read(position, sizeof(T)) };
    }

    template<typename T>
    const_pointer_type<T> read(pos_type position, const leapus::meta::type<const T> &) const{
        return  { (const T *)this->read(position, sizeof(const T)) };
    }

    //Offer a read-ahead hint. It might be ignored and be a NOP.
    virtual inline bool readahead(pos_type pos, size_type sz){ return false; };
    virtual size_type size() const = 0;
};

}
