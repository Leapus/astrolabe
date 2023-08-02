#pragma once

/*
*
* Stuff for accessing OSM world map files in Google protobuf/pbf format
*
*/

#include <utility>
#include <filesystem>
#include "protobuf/fileformat.pb.h"
#include "protobuf/osmformat.pb.h"
#include "astrolib/pbffile.hpp"

namespace leapus::osm{

class osm_file;

/* 
       Walk the blobs present in the file, which are compressible blocks
       into which the file is broken so that you can decompress just the section you want.
       There's not much interesting in the file header blobs except for the bounding boxes,
       which are defined as optional, so we'd might as well calculate our own since we're
       spacially indexing anyway.

       Note that blob disk-size is discouraged by OSM documentation from being greater than 32kiB,
       and prohibited from exceeding 64kiB. For the uncompressed data that rule is 16/32Mib, though
       a compression ratio of 512:1 sounds awfully (delusionally) optimistic. Anyway, for the purpose
       of contemplating order-of-magnitude, you're looking at a typical data size of about 128kiB-
       512kiB because compression seldom does better than 2:1, and rarely like 4:1. They seem sized 
       roughly suitably for assigning as a worker thread task, which was probably a design goal.
*/
template<class File=osm_file>
class blob_iterator{
public:

    using const_iterator = blob_iterator<const File>;
    using non_const_iterator=blob_iterator<std::remove_const_t<File>>;
    friend const_iterator;
    friend non_const_iterator;

    using file_type=File;
    using pos_type=typename file_type::pos_type;
    using size_type=typename file_type::size_type;
    using value_type=std::pair<OSMPBF::BlobHeader, OSMPBF::Blob>;
    using header_size_type=typename file_type::blob_header_size_type;

    file_type &m_file;

    //m_pos points to the start of the present blob's size int regardless
    //Once the blob header is read, m_blob_pos is the position of the blob itself
    pos_type m_pos;
    mutable pos_type m_blob_pos=0;
    mutable value_type m_data;
    mutable bool m_blob_populated=false;

protected:
    void populate_header() const{
        if( !m_blob_pos ){
            m_blob_pos = m_file.read_blob_header(m_pos, m_data.first);
            m_file.readahead( m_blob_pos, 1024*1024 );
            //|| (::abort(), true );
        }
    }

    void populate_blob() const{
        populate_header();
        if(!m_blob_populated){
            m_file.read(m_blob_pos, m_data.first.datasize(), m_data.second);
            m_blob_populated=true;
        }
    }

public:
    blob_iterator( file_type &file, pos_type initial_pos ):
        m_file(file),
        m_pos( initial_pos){
    }
    
    bool operator==( const const_iterator &rhs) const{
        return &m_file==&rhs.m_file && m_pos == rhs.m_pos;
    }

    bool operator==( const non_const_iterator &rhs) const{
        return &m_file==&rhs.m_file && m_pos == rhs.m_pos;
    }

    bool operator!=( const const_iterator &rhs) const{
        return !(*this==rhs);
    }

    bool operator!=( const non_const_iterator &rhs) const{
        return !(*this==rhs);
    }

    value_type &operator*() const{
        populate_blob();
        return m_data;
    }

    value_type *operator->() const{
        populate_blob();
        return &m_data;
    }

    blob_iterator &operator++() {
        populate_header();
        m_pos = m_blob_pos + m_data.first.datasize();
        m_blob_populated=false;
        m_blob_pos=0;
        return *this;
    }
};

//We have no need to ever write these, so they are treated read-only
//We do write indices into separate files, sometimes using the same data types, though
class osm_file:public leapus::pbf::protobuf_file{

    using base_type=leapus::pbf::protobuf_file;
    OSMPBF::HeaderBlock m_header;

public:
    using blob_header_size_type=::int32_t;
    using blob_iterator_type=blob_iterator<osm_file>;
    using const_blob_iterator_type=blob_iterator<const osm_file>;

    osm_file();
    osm_file( const std::filesystem::path & );

    osm_file &operator=( osm_file && ) = default;

    //Reads a raw int32 in network byte order and converts it to host order
    blob_header_size_type read_raw_int32(pos_type pos) const;

    //This is a special case because it's preceded by a raw int32 in network byte-order
    //that specifies the blob header's serialized length. Returns the next file position
    //following the header.
    pos_type read_blob_header( pos_type pos, OSMPBF::BlobHeader &target ) const;

    blob_iterator_type begin();
    blob_iterator_type end();
    const_blob_iterator_type begin() const;
    const_blob_iterator_type end() const;
};

}
