#include <iostream>
#include <mutex>
#include "astrolib/console.hpp"

using namespace leapus;

std::mutex console::m_mutex={};

void console::out(const std::string &line){
    std::lock_guard guard(m_mutex);
    std::cout << line << std::endl;
}

void console::err(const std::string &line){
    std::lock_guard guard(m_mutex);
    std::cerr << line << std::endl;
}
