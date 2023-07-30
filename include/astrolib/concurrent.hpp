#pragma once

/*
*
* Concurrency and multithreading
*
*/

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace leapus::concurrent {


/*
*
* Brainy Smurf's book on lock-free philosophy states that lock-free code is about rushing
* something to completion fast enough to usually avoid contention, while maintaining a means
* to detect contention so as to keep retrying until success.
*
*/

/*

Lock-free queue

head<-I3<-I2<-I1<-tail

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
        std::atomic<list_link *>next=nullptr;              
    } m_tail;

    std::mutex m_sleep_mutex;
    std::condition_variable m_sleep_cond;
    std::atomic<list_link *> m_head=&m_tail;

    list_link *nap(){
        list_link *result;
        std::unique_lock lock(m_sleep_mutex);
        m_sleep_cond.wait(lock, [this, &result](){ return result=m_tail.next.load(); } );
        return result;
    }

    void wake(){
        std::lock_guard lock(m_sleep_mutex);
        m_sleep_cond.notify_all();
    }

public:
    void push_front(const value_type &v){

        list_link *nl= new list_link{ v };

        //be the first to point old-head-link at new-head-link
        //or keep trying if we just barely missed it due to contention
        do{
            list_link *h=m_head.load(); //grab a copy of the pointer to old head link

            //Update that to point to the new head link, while atomically ensuring no other thread beat us to it
            if(!h->next.compare_exchange_weak( nullptr, nl ))
                continue;

            //Now update the head pointer so that other threads won't be stuck spinning here
            //repeatedly finding that the old head link already points to a newer head.
            if( m_head.exchange(nl)  == &m_tail )
                wake();

            //Also, pushing into the empty queue is the only special case which actually messes with
            //locks in order to wake idle threads that went to sleep for lack of work.

            //Success
            break;
    
        } while( true );
    }

    value_type pop_back(){

        list_link *t, *n;
        do{
            //Fetch the next candidate for popping
            t=m_tail.next.load();

            //If the queue is empty, sleep the thread until not empty
            if( !t )
                t=nap();

            //Get the next item's next item
            n=t->next.load();

            //Atomically, if nobody else popped an item, set meta-next as next.
            //Or, if somebody updated the next-pointer while we were figuring out what ought to be next
            //then start over and try again.
        }while(! m_tail.next.compare_exchange_weak(t, n));

        value_type result=t->value;
        delete t;
        return result;
    }
};

}
