#include "astrolib/osmfile.hpp"


using namespace leapus::meta;
using namespace leapus::osm;
using namespace leapus::io;
using namespace google::protobuf;


osm_file::osm_file( const std::filesystem::path &path):
    base_type(path){
    
    //read_blob_header(0, m_header_header);
}

::int32 osm_file::read_raw_int32(pos_type pos) const{
    return meta::endian::convert_endian(
        *random_access_file<>::read(pos, type<const ::int32_t>{} ));

}

osm_file::pos_type osm_file::read_blob_header(pos_type pos, OSMPBF::BlobHeader &target) const{
    auto sz=read_raw_int32(pos);
    pos+=sizeof(sz);
    //target.ParseFromArray( this->read( pos, sz), sz );
    this->read(pos,sz, target);
    return pos + sz;
}

osm_file::blob_iterator_type osm_file::begin(){
    return { *this, 0 };
}

osm_file::blob_iterator_type osm_file::end(){
    return { *this, size() };
}

osm_file::const_blob_iterator_type osm_file::begin() const{
        return { *this, 0 };
}

osm_file::const_blob_iterator_type osm_file::end() const{
    return { *this, size() };
}

