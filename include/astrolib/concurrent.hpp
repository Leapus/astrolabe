#pragma once

/*
*
* Concurrency and multithreading
*
*/

#include <exception>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>

#include "meta.hpp"

namespace leapus::concurrent {
/*
*
* Brainy Smurf's book on lock-free philosophy states that lock-free code is about rushing
* something to completion fast enough to usually avoid contention, while maintaining a means
* to detect contention so as to keep retrying until success.
*
*/

//Not strictly an exception in the error sense, which is considered naughty by C++ conventions,
//mostly due to perfromance, but who cares? Are you going to have a hundred thread pools to tear down?
struct interrupt_exception{};


/*
Lock-free queue

//m_head points to the last element added so that the next one can be linked from that one.
//m_tail points to the next element to be popped

//Two items
           <-m_head
nullptr<-I2<-I1<-m_tail

// With one item
           <-m_head
nullptr<-I1<-m_tail

//Or when empty
                  <-m_head
 nullptr <- m_tail

There actually is a lock, but only for sleeping the thread when the queue is empty,
since it would be dumb to spin the processor just because there is nothing to do in the present thread.

*/
template<typename T>
class lf_queue{
    using pointer_type = T*;
    using ref_type = T &;

public:
    using value_type = T;

private:
    struct list_link{
        value_type value;
        std::atomic<list_link *> next=nullptr;              
    } m_tail;

    std::mutex m_sleep_mutex;
    std::condition_variable m_sleep_cond;
    std::atomic<list_link *> m_head=&m_tail;
    std::atomic_bool m_interrupt=false;

    list_link *nap(){
        list_link *result;
        std::unique_lock lock(m_sleep_mutex);

        m_sleep_cond.wait(lock, [this, &result](){    
            if( result=m_tail.next.load() )
                return true; 

            if(m_interrupt)
                throw interrupt_exception();
            else
                return false;
        });

        return result;
    }

    void wake(){
        std::lock_guard lock(m_sleep_mutex);
        m_sleep_cond.notify_all();
    }

    void push_front_impl( list_link *nl ){

        list_link *null;

        //be the first to point old-head-link at new-head-link
        //or keep trying if we just barely missed it due to contention
        do{
            null=nullptr;
            list_link *h=m_head.load(); //grab a copy of the pointer to old head link
            //Update that to point to the new head link, while atomically ensuring no other thread beat us to it
            if(!h->next.compare_exchange_weak( null, nl ))
                continue;

            //Now update the head pointer so that other threads won't be stuck spinning here
            //repeatedly finding that the old head link already points to a newer head.
            if( m_head.exchange(nl)  == &m_tail ){

                //Racing appends will be appending at head, so that's not a problem
                //Racing pops will not be able to pop because m_tail points to null.
                //So, this is the only thread which will succeed updating the tail
                //between the update of head and tail.
                m_tail.next=nl;
                wake();
            }

            //Also, pushing into the empty queue is the only special case which actually messes with
            //locks in order to wake idle threads that went to sleep for lack of work.

            //Success
            break;
    
        } while( true );
    }

public:
    void push_front(const value_type &v){
        list_link *nl= new list_link{ v };
        push_front_impl(nl);
    }

    void push_front(value_type &&v){
        list_link *nl= new list_link{ std::move(v) };
        push_front_impl(nl);
    }

    //Throws interrupt_exception to help shut down a worker thread if the queue is empty and interrupt() has been called
    value_type pop_back(){

        list_link *t, *n;
        do{
            //Fetch the next candidate for popping
            t=m_tail.next.load();

            //If the queue is empty, sleep the thread until not empty (or throw if empty following interrupt())
            if( !t )
                t=nap();

            //Get the next item's next item
            n=t->next.load();

            //Atomically, if nobody else popped an item, set meta-next as next.
            //Or, if somebody updated the next-pointer while we were figuring out what ought to be next
            //then start over and try again.
        }while(! m_tail.next.compare_exchange_weak(t, n));

        //If n is null, then the list is empty and m_head has only m_tail left to update, 
        //so that's where it needs to be pointed
        

        value_type result=std::move(t->value);
        delete t;
        return result;
    }

    void interrupt(){ 
        m_interrupt=true;
        wake();
     }
};


//A pool of threads that pull tasks out of a shared queue
template<typename T, typename Q>
class ThreadPool{

public:
    using task_type=T;
    using queue_type=Q;

private:
    queue_type m_queue;
    void thread_proc(){
        task_type task;
        while(true){
            try{
                task=m_queue.pop_back();
            }
            catch( const interrupt_exception &){
                break;
            }

            try{
                task();
            }catch(...){
                exception_handler( std::current_exception() );
            }
        }   
    }

    std::vector<std::thread> m_threads;

protected:
    //If this throws, it will cause the thread to exit
    virtual void exception_handler(std::exception_ptr ep)=0;

public:
    ThreadPool(int thread_count = std::thread::hardware_concurrency()){
        while( m_threads.size() < thread_count ){
            m_threads.emplace_back(  std::thread{ [this](){ this->thread_proc();  } } );
        }
    }

    virtual ~ThreadPool(){
        shutdown();
    }

    void shutdown(){
        m_queue.interrupt();

        for(auto &t: m_threads)
            if(t.joinable()) t.join();
        
        m_threads.clear();
    }

    void push_front(const task_type &task){
        m_queue.push_front(task);
    }

    void push_front(task_type &&task){
        m_queue.push_front(std::move(task));
    }

    task_type pop_back(){
        return m_queue.pop_back();
    }
};

}
