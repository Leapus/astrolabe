//#include <google/protobuf/dynamic_message.h>

#include <fstream>
#include "astrolib/osmfile.hpp"


using namespace google::protobuf;
using namespace leapus::osm;

int main(int argc, char *argv[]){

    osm_file file( argv[1] );
    for( auto &v : file ){
        std::cout << v.first.type() << std::endl;
    }

    //auto it=file.begin();
    return 0;
}
