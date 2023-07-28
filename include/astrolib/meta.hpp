#pragma once

/*
* Metaprogramming stuff
*/


namespace leapus::meta{

//An empty class which can be passed around to represent a type
//without passing any actual content, and is hopefully readily optimized out
template<typename T>
struct type{
    using t=T;
};


template<typename Func>
struct guard_base{
    using function_type=Func;

protected:
    function_type p_func;

public:
    guard_base(const function_type &func):
        p_func(func){}

};

// Do some arbitrary thing as soon as the object goes out of scope or is destructed
template<typename Func, typename Data=void>
struct guard:protected guard_base<Func>{
    using data_type=Data;
    using typename guard_base<Func>::function_type;
private:
    data_type m_data;

public:
    guard(const data_type &d, const function_type &func):
        guard_base<Func>(func),
        m_data(d){}

    ~guard(){
        m_func(m_data);
    }
};

template<typename Func>
struct guard<Func, void>:protected guard_base<Func>{

    using base_type = guard_base<Func>;
    using typename base_type::function_type;
    using base_type::base_type;

    ~guard(){
        this->p_func();
    }
};

template<typename Data, typename Func>
guard(const Data &d, const Func &) -> guard<Data, Func>;

template<typename Func>
guard(const Func &) -> guard<Func>;

}
