#pragma once


/*
*
* A memory-mapped implementation of random_access.hpp
*
*/

#include <filesystem>
#include "astrolib/meta.hpp"
#include "astrolib/io/random_access.hpp"

namespace leapus::io{

class mmap_file:public random_access_file<const char *>{
    int m_fd;
    const char *m_data;
    size_t m_size;

public:
    using base_type = random_access_file<const char *>;
    using size_type=base_type::size_type;
    using pos_type=base_type::pos_type;
    using pointer_type=base_type::pointer_type;

    mmap_file( const std::filesystem::path &path );
    ~mmap_file() override;
    pointer_type get( pos_type position, size_type size ) const override;

};

}
