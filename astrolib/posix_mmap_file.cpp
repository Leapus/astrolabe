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
#include <string>
#include "astrolib/meta.hpp"
#include "astrolib/io/mmap_file.hpp"

using namespace std::string_literals;
using namespace leapus::io;
using namespace leapus::meta;

class posix_io_exception:public io_exception{
public:
    posix_io_exception( const std::string &msg, const std::filesystem::path &path ):
        io_exception( msg + ": " + path.string() + ": " + ::strerror(errno)){}
};

leapus::io::mmap_file::mmap_file( const std::filesystem::path &path, bool writeable):
    m_path(path){

    m_fd=-1;

    //Seems like the file has to be opened RW to get virtual memory paging. See below.
    m_fd=::open( path.c_str(), O_LARGEFILE | O_RDWR);

    //m_fd=::open( path.c_str(), O_LARGEFILE | 
    //    true ? O_RDWR : O_RDONLY );

    //mmap() itself doesn't need this handle held open, but we keep it
    //in case we decide to expand the file
    //leapus::meta::guard( [this](){ ::close(m_fd); } ); 

    if(m_fd == -1)
        throw posix_io_exception("Error opening file", path);
    
    m_size = ::lseek64(m_fd, 0, SEEK_END);

    if(m_size == -1)
        throw posix_io_exception("Error seeking file end. Is it empty?", path);

    //You have to use MAP_SHARED to get the magical paging goodness that behaves
    //like gigabytes of data are in memory when they're still mostly on the disk.
    //However, if the underlying file descriptor was not opened for writing,
    //you will get EACCESS even if you are not mapping for writing. Oh well.
    m_data = (char *)::mmap(NULL, m_size, PROT_READ |
        writeable ? PROT_WRITE : 0,
        MAP_SHARED, m_fd, 0);

    if(m_data == MAP_FAILED)
        throw posix_io_exception("Failed calling mmap() to memory-map file", path);
}

mmap_file::~mmap_file(){

    if(m_data && m_data != MAP_FAILED)
        ::munmap((void*)m_data, m_size);

    if(m_fd != -1)
        ::close(m_fd);
}

bool mmap_file::is_in_range(pos_type pos, size_type sz) const{
    return pos+sz-1 < m_size; 
}

void mmap_file::check_range(pos_type pos, size_type sz) const{
    if( !is_in_range(pos, sz) )
        throw std::range_error("File segment out of range: get(" + std::to_string(sz) + ", " + std::to_string(pos) + ")" );
}

const char *mmap_file::read( pos_type pos, size_type size ) const{
    check_range(pos, size);
    return m_data+pos;
}

static const char nullchar='\0';
char *mmap_file::read(pos_type pos, size_type sz){

    auto nsz=pos+sz;
    ::off_t r;

    //Grow the file if necessary
    if( nsz > m_size  ){
        r=::lseek( m_fd, nsz-1, SEEK_SET);
        if(r == -1)
            throw posix_io_exception("Error lseek()ing file position to expand it", m_path);

        if( r != nsz-1)
            throw  io_exception("seek() returned the wrong position extending file size: "s + m_path.string());

        int r2;        
        while((r2 = ::write( m_fd, &nullchar, 1)) == 0){}

        if(r2 == -1)
            throw posix_io_exception("Error writing new EOF to extend file size", m_path);

        m_data=(char *)::mremap( m_data, m_size, nsz, MREMAP_MAYMOVE );
        if(m_data == MAP_FAILED)
            throw posix_io_exception("Error memory-mapping file", m_path);
        m_size=nsz;
    }

    check_range(pos, sz);
    return m_data+pos;
}

mmap_file::size_type mmap_file::size() const{
    return m_size;
}


bool mmap_file::readahead( pos_type pos, size_type sz ){
    ::size_t ps= ::getpagesize();

    ::size_t p=(::size_t)m_data+pos;

    p = p & ~(ps-1);
    sz = (sz + ps - 1) & ~(ps-1); 
    return ::posix_madvise((void *)p, sz, POSIX_MADV_WILLNEED) == 0;
}