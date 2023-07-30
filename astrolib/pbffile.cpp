#include "astrolib/pbffile.hpp"

using namespace leapus::pbf;
using namespace google::protobuf;

/*
* mmap() specific implementation.
* It doesn't cost anything to reference the entire file until it's necessary
* to retrieve the actual file blocks, so we will just pass the pbuf library
* the current position straight to the end of the file every time. It ensures
* that the entire object is pulled in in one operation, without really any
* overhead.
*/
void protobuf_file::read( pos_type pos, size_type sz, ::google::protobuf::Message &target) const{
    //auto sz = size() - pos - 1;
    //auto sz = target.ByteSizeLong();

    if(!target.ParseFromArray( mmap_file::read(pos, sz), sz ))
    //if(!target.ParseFromString( std::string(get(pos, sz),sz)))
    //if( !target.ParsePartialFromArray(  random_access_file<>::read(pos, sz), sz ) )
        throw pbf_parse_exception( target, "Failed parsing protobuf object at offset: " + 
            std::to_string(pos) );
}

protobuf_file::protobuf_file( const std::filesystem::path &path ):
    base_type(path){
}

static std::string make_parse_error_string( const Message &obj, const std::string &msg ){
    return msg + ": " + obj.GetTypeName() + ": " + obj.InitializationErrorString();
}

pbf_parse_exception::pbf_parse_exception( const Message &obj, const std::string &msg ):
    leapus::exception::exception(make_parse_error_string(obj,msg))
    {}
