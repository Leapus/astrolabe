#pragma once

/*
*
* A memory-mapped implementation of random_access.hpp
*
*/
#include <cassert>
#include <mutex>
#include <atomic>
#include <memory>
#include <filesystem>
#include "astrolib/meta.hpp"
#include "astrolib/io/random_access.hpp"

namespace leapus::io{

template<typename T>
class mmap_allocator;

class mmap_file:public random_access_file<>{
public:
    using base_type=random_access_file<>;
    using size_type=base_type::size_type;
    using pos_type=base_type::pos_type;

private:
    //Stuff that's default-movable
    struct {
        int m_fd=-1;
        char *m_data=nullptr;
        size_t m_map_size=0;
        std::filesystem::path m_path; //For diagnostic messages
    } m;

    //Has to be special-case moved
    std::atomic<size_t> m_size=0;

    //Not even moved given that a mutex is inherently fixed
    std::mutex m_mut_resize;

private:
    void init(bool writeable, size_type mapping_size);
    bool is_in_range(pos_type pos, size_type sz) const;
    void check_range(pos_type pos, size_type sz) const;

public:
    template<typename T>
    using allocator_type=mmap_allocator<T>;

    //using pointer_type=base_type::pointer_type;
    //using const_pointer_type=base_type::const_pointer_type;

    //Default will be zero mapping_size, which will instead default to mapping the current
    //size of the file on-disk. Zero is not a valid size for mmap anyway.
    //You'd might as well map an absurd block of memory since the address space on a typical 48-bit 
    //address space on a 64-bit machine is like 20TB these days, and it doesn't actually cost anything
    //until you write to it, unless you need terabytes of imaginary space for something else. The current
    //size of the OSM planet file is about 130GB so, pick some multiple of that and it should be
    //"all the RAM anyone should ever need®". Mapping a safe excess is way simpler than supporting runtime
    //mapping relocation.
    mmap_file( const std::filesystem::path &path, bool writable, size_t mapping_size=0);
    //mmap_file( const std::filesystem::path &path, bool writable);
    mmap_file(); //the null file
    ~mmap_file() override;

    //Can move, but not copy, because a copy could not trivially track its size in sync
    //Moving is not thread-safe
    mmap_file(const mmap_file &) = delete;  
    mmap_file(mmap_file &&rhs) = default;
    mmap_file &operator=( mmap_file &&rhs );

    operator bool() const { return m.m_data; }
    bool operator!() const { return ! this->operator bool(); }

    const_pointer_type<char> read( pos_type position, size_type size ) const override;
    pointer_type<char> read(pos_type position, size_type size) override;
    bool readahead(pos_type pos, size_type sz) const override;

    //The usable size, which for a memory-mapped file, is the size of the file backing the mapping.
    //If you map a smaller region than the file, then you will (hopefully) segfault beyond the mapping.
    //If you access a mapped region beyond the file, you get SIGBUS, which is like more a specific kind
    //of segfault. Anyway, it's buffer overflow either way.
    size_type size() const override;

    virtual pos_type grow(offset_type d);
    static mmap_file null_file;
};

template<typename T>
class mmap_allocator:public std::allocator<T>{
    mmap_file *m_file;
    using base_type=std::allocator<T>;

public:
    using typename base_type::pointer;
    using typename base_type::reference;
    using size_type=mmap_file::size_type;

    template<typename U>
    using rebind=mmap_allocator<U>;

    mmap_allocator( mmap_file &f=mmap_file::null_file ):
        m_file(&f){}

    template<typename U>
    mmap_allocator( const mmap_allocator<U> &other ):
        m_file(other.m_file){}

    mmap_allocator &operator=( const mmap_allocator &rhs ){
        m_file=rhs.m_file;
        return *this;
    }

    pointer address(reference x) const{
        return &x;
    }

    //hint is ignored because since there is no freeing and
    //there's only one place the allocation can happen plus [0, alignof(T)]
    pointer allocate(size_type n, std::allocator<void>::const_pointer hint=0){
        auto sz=sizeof(T) * n;
        auto chunksz=sz+alignof(T);
        auto pos=m_file->grow( chunksz );

        void *p=std::addressof(*m_file->read( pos, chunksz ));
        std::align( alignof(T), sz, p, chunksz);
        return { p };
    }

    //This is no-op and the space goes to waste
    void deallocate(pointer p, size_type n){
        assert(false);
    }

    size_type max_size() const noexcept{
        if( m_file->size() < sizeof(T) + alignof(T) )
            return 0;
        else
            return (m_file->size()-alignof(T)) / sizeof(T);
    }
};

}
