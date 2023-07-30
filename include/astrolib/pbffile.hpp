#pragma once

/*
*
* A simple layer over file I/O to provide methods for accessing
* protobuf/pbf objects
*/

#include <filesystem>
#include "google/protobuf/message.h"
#include "astrolib/io/mmap_file.hpp"
#include "astrolib/meta.hpp"
#include "astrolib/exception.hpp"

namespace leapus::pbf{

using int32=google::protobuf::int32;

class pbf_parse_exception:public leapus::exception::exception{

public:
    pbf_parse_exception( const google::protobuf::Message &obj,
        const std::string &msg);
};

class protobuf_file:public leapus::io::mmap_file{
    using base_type=leapus::io::mmap_file;

public:
    protobuf_file( const std::filesystem::path & );
    void read( pos_type pos, size_type sz, ::google::protobuf::Message &target) const;
};

}

