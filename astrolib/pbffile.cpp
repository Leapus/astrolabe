#include "astrolib/pbffile.hpp"

using namespace leapus::pbf;
using namespace google::protobuf;

void protobuf_file::read( pos_type pos, size_type sz, ::google::protobuf::Message &target) const{
    //auto sz = size() - pos - 1;
    //auto sz = target.ByteSizeLong();

    if(!target.ParseFromArray( std::addressof(*mmap_file::read(pos, sz)), sz))
    //if(!target.ParseFromString( std::string(get(pos, sz),sz)))
    //if( !target.ParsePartialFromArray(  random_access_file<>::read(pos, sz), sz ) )
        throw pbf_parse_exception( target, "Failed parsing protobuf object at offset: " + 
            std::to_string(pos) );
}

static std::string make_parse_error_string( const Message &obj, const std::string &msg ){
    return msg + ": " + obj.GetTypeName() + ": " + obj.InitializationErrorString();
}

pbf_parse_exception::pbf_parse_exception( const Message &obj, const std::string &msg ):
    leapus::exception::exception(make_parse_error_string(obj,msg))
    {}
