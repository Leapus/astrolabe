#pragma once

/*
*
* Just so that all of our console notices can go out through a single place, and thread-safe
*
*/

#include <iostream>
#include <mutex>

namespace leapus{

class console final{

protected:
    static std::mutex m_mutex;

public:
    static void out(const std::string &line);
    static void err(const std::string &line);

    friend void out(const std::string &);
    friend void err(const std::string &);
};

}
