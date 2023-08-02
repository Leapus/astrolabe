//#include <google/protobuf/dynamic_message.h>

#include <string>
#include <exception>
#include <fstream>
#include <functional>

#include "astrolib/console.hpp"
#include "astrolib/pbffile.hpp"
#include "astrolib/osmfile.hpp"
#include "astrolib/concurrent.hpp"

#include "astrolib/index.hpp"

using namespace std::string_literals;
using namespace google::protobuf;
using namespace leapus;
using namespace leapus::osm;
using namespace leapus::concurrent;
using namespace leapus::astrolib::index;

class worker_pool:public ThreadPool< std::function<void()>, lf_queue<std::function<void()>> >{
public:
    using ThreadPool::ThreadPool;

protected:
    void exception_handler( std::exception_ptr ptr) override{

        try{
            std::rethrow_exception(ptr);
        }
        catch( const std::exception &ex ){
            leapus::console::out("Worker thread exception: "s + ex.what());
        }
        catch(...){
            leapus::console::out( "Uncaught exception in worker thread" );
        }
    }
};

static void blob_handler( const index_config &config, osm_file::const_blob_iterator_type it){
    leapus::console::out( it->first.type() );
}

int main(int argc, char *argv[]){

    index_config config;
    worker_pool threads(0);
    //const osm_file in( argv[1] );
    pbf::protobuf_file out{ argv[2], true, (pbf::protobuf_file::size_type)130 * 1024 * 1024 * 1024 * 4 };

    //We go with a mapping size of four times the OSM planet file as of this writing
    //or about 520GB
    config.in_file = std::move( osm::osm_file{ argv[1] } );

    config.file_allocator={ out };

    //Walk the blobs in the thread and create an indexing task for each one
    for( auto it=meta::constify(config.in_file).begin(); it!=meta::constify(config.in_file).end(); ++it ){
        threads.push_front( [&config, it](){ blob_handler(config, it); } );
        //leapus::console::out( it->first.type() );
    }

    /*
    leapus::io::mmap_file file(argv[1]);
    auto p=file.read(0,1024*1024);

    std::string str;
    std::getline( std::cin, str);

    for(int i=0; i < 1024*1024; ++i){
        p[i]='X';
    }
    */

    return 0;
}
