/*
*
* The posix or linux implementation of a memory-mapped file.
* Windows would need a different implementation if you were to use the Windows API
* and not some Unix-like layer.
*
*/

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "astrolib/meta.hpp"
#include "astrolib/io/mmap_file.hpp"

using namespace leapus::io;
using namespace leapus::meta;


class posix_io_exception:public io_exception{
public:
    posix_io_exception( const std::string &msg, const std::filesystem::path &path ):
        io_exception( msg + ": " + path.string() + ": " + ::strerror(errno)){}
};

leapus::io::mmap_file::mmap_file( const std::filesystem::path &path ){

    m_fd=::open( path.c_str(), O_LARGEFILE | O_RDONLY );

    //Remember to close the file handle which is not needed once mmap has it open (or on error, either way)
    leapus::meta::guard( [this](){ ::close(m_fd); } ); 

    if(m_fd == -1)
        throw posix_io_exception("Error opening file", path);
    
    m_size=::lseek64(m_fd, 0, SEEK_END);

    if(m_size == -1)
        throw posix_io_exception("Error seeking file end. Is it empty?", path);

    m_data = (const char *)::mmap(NULL, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0);

    if(m_data == MAP_FAILED)
        throw posix_io_exception("Failed calling mmap() to memory-map file", path);
}

mmap_file::~mmap_file(){
    ::munmap((void*)m_data, m_size);
}

const char *mmap_file::get( size_type size, pos_type pos ) const{

    if(pos+size >= m_size)
        throw std::range_error("File segment out of range: get(" + std::to_string(size) + ", " + std::to_string(pos) + ")" );

    return (const char *)m_data+pos;
}
