#pragma once

/*
*
* Utilities for parsing command-line arguments
*
*/


namespace leapus{


//A convenient way of walking the argument list which will throw if read out-of-bounds
class arg_iterator{
    int m_argc, m_argp=0;
    const char **m_argv;

private:
    int range_check(int pos) const;

public:
    arg_iterator(int argc, const char *argv[]);

    const char *operator*() const;
    const char *operator [](int x) const;
    arg_iterator &operator++();
    arg_iterator &operator--();
};

class args{
    int m_argc;
    const char **m_argv;

public:
    arg_iterator begin() const;
    arg_iterator end() const;

    //Is the argument a short-form single-dash switch (-v, etc)
    static bool is_short_switch(const char *);

    //Is the argument a long-form double-dash switch (--version, etc)
    static bool is_long_switch(const char *);
};

}