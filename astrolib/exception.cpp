#include "astrolib/exception.hpp"


using namespace leapus::exception;

exception::exception(const std::string &msg):
    std::runtime_error(msg){}

    