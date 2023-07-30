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

class mmap_file:public random_access_file<>{
    int m_fd;
    char *m_data=nullptr;
    size_t m_size;
    std::filesystem::path m_path; //For diagnostic messages

private:
    bool is_in_range(pos_type pos, size_type sz) const;
    void check_range(pos_type pos, size_type sz) const;

public:
    using base_type = random_access_file<>;
    using size_type=base_type::size_type;
    using pos_type=base_type::pos_type;
    //using pointer_type=base_type::pointer_type;
    //using const_pointer_type=base_type::const_pointer_type;

    mmap_file( const std::filesystem::path &path, bool writable=false );
    ~mmap_file() override;
    const_pointer_type<char> read( pos_type position, size_type size ) const override;
    pointer_type<char> read(pos_type position, size_type size) override;
    size_type size() const override;

    bool readahead(pos_type pos, size_type sz) override;
};

}

