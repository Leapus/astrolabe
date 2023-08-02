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

static int open_file( const std::filesystem::path &path ){
    //Seems like the file has to be opened RW to get virtual memory paging. See below.
    int fd=::open( path.c_str(), O_LARGEFILE | O_RDWR | O_CREAT);

    //m_fd=::open( path.c_str(), O_LARGEFILE | 
    //    true ? O_RDWR : O_RDONLY );

    if(fd == -1)
        throw posix_io_exception("Error opening file", path);

    return fd;
}

static mmap_file::size_type get_size_on_disk( int fd){
    //mmap() itself doesn't need this handle held open, but we keep it
    //in case we decide to expand the file
    mmap_file::size_type sz = ::lseek64(fd, 0, SEEK_END);

    if(sz == -1)
        throw std::runtime_error("Error seeking file end");
    
    return sz;
} 

mmap_file::mmap_file( const std::filesystem::path &path, bool writeable, size_type mapping_size){
    try{
        m.m_fd=open_file(path);
        init(writeable, mapping_size);
    }
    catch(const std::runtime_error &err){
        throw posix_io_exception( err.what(), path );
    }
}

mmap_file::mmap_file(){}

//leapus::io::mmap_file::mmap_file( const std::filesystem::path &path, bool writeable, size_type mapping_size):
 //   m_path(path)

void mmap_file::init(bool writeable, size_type mapping_size){

    m_size=get_size_on_disk(m.m_fd);

    if(!mapping_size)
        mapping_size=m_size;

    //You have to use MAP_SHARED to get the magical paging goodness that behaves
    //like gigabytes of data are in memory when they're still mostly on the disk.
    //However, if the underlying file descriptor was not opened for writing,
    //you will get EACCESS even if you are not mapping for writing. Oh well.
    m.m_data = (char *)::mmap(NULL, mapping_size, PROT_READ |
        writeable ? PROT_WRITE : 0,
        MAP_SHARED, m.m_fd, 0);

    if(m.m_data == MAP_FAILED)
        throw std::runtime_error("Failed calling mmap() to memory-map file");

    m.m_map_size=mapping_size;
}

mmap_file::~mmap_file(){

    if(m.m_data && m.m_data != MAP_FAILED)
        ::munmap((void*)m.m_data, m.m_map_size);

    if(m.m_fd != -1)
        ::close(m.m_fd);
}

bool mmap_file::is_in_range(pos_type pos, size_type sz) const{
    auto extent=pos+sz-1;
    return extent < m_size && extent < m.m_map_size; 
}

void mmap_file::check_range(pos_type pos, size_type sz) const{
    if( !is_in_range(pos, sz) )
        throw std::range_error("File segment out of range: get(" + std::to_string(sz) + ", " + std::to_string(pos) + ")" );
}

mmap_file::const_pointer_type<char> mmap_file::read( pos_type pos, size_type size ) const{
    check_range(pos, size);
    return const_cast<const char *>(m.m_data)+pos;
}

static const char nullchar='\0';
mmap_file::pointer_type<char> mmap_file::read(pos_type pos, size_type sz){

    auto nsz=pos+sz;
    ::off_t r;

    //Grow the file if necessary
    if( nsz > m_size  ){
        r=::lseek( m.m_fd, nsz-1, SEEK_SET);
        if(r == -1)
            throw posix_io_exception("Error lseek()ing file position to expand it", m.m_path);

        if( r != nsz-1)
            throw  io_exception("seek() returned the wrong position extending file size: "s + m.m_path.string());

        int r2;        
        while((r2 = ::write( m.m_fd, &nullchar, 1)) == 0){}

        if(r2 == -1)
            throw posix_io_exception("Error writing new EOF to extend file size", m.m_path);

        m.m_data=(char *)::mremap( m.m_data, m_size, nsz, MREMAP_MAYMOVE );
        if(m.m_data == MAP_FAILED)
            throw posix_io_exception("Error memory-mapping file", m.m_path);
        m_size=nsz;
    }

    check_range(pos, sz);
    return m.m_data+pos;
}

mmap_file::size_type mmap_file::size() const{
    return m_size;
}

bool mmap_file::readahead( pos_type pos, size_type sz ) const{
    ::size_t ps= ::getpagesize();

    ::size_t p=(::size_t)m.m_data+pos;

    p = p & ~(ps-1);
    sz = (sz + ps - 1) & ~(ps-1); 
    return ::posix_madvise((void *)p, sz, POSIX_MADV_WILLNEED) == 0;
}

mmap_file::pos_type mmap_file::grow(offset_type d){

    size_type osz, nsz;

    //Spinlock
    while((osz=m_size.exchange(-1)) == -1 ){}

    nsz=osz+d;
    ::ftruncate64(m.m_fd, nsz);
    m_size=nsz;
    return osz;
}  

mmap_file mmap_file::null_file = {};

mmap_file &mmap_file::operator=( mmap_file &&rhs ){

    //Nullify RHS so that the memory and file handle don't get freed
    m = std::move(rhs.m);
    rhs.m.m_data=nullptr;
    rhs.m.m_fd=-1;

    m_size=rhs.m_size.load();
    return *this;
}

