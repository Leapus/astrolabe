#include <stdexcept>
#include "astrolib/argv.hpp"

using namespace leapus;

arg_iterator::arg_iterator(int argc, const char *argv[]):
    m_argc(argc),
    m_argv(argv){}

int arg_iterator::range_check(int pos) const{
     if(m_argp < 0)
        throw std::range_error("Tried to read a negative argv index");

    if(pos >= m_argc)
        throw std::range_error("Missing expected argument #" + std::to_string(pos));

    return pos;
}

const char *arg_iterator::operator*() const{
    return m_argv[ range_check(m_argp) ];
}

const char *arg_iterator::operator[](int x) const{
    return m_argv[ range_check(m_argp + x) ];
}

arg_iterator &arg_iterator::operator++(){
    ++m_argp;
    return *this;   
}

arg_iterator &arg_iterator::operator--(){
    return *this;   
}
