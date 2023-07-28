#pragma once

#include <string>
#include <stdexcept>

namespace leapus::exception{

    class exception:public std::runtime_error{
    public:
        exception(const std::string &desc);

    };

}