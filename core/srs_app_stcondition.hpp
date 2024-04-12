#pragma once

#include "srs_kernel_utility.hpp"
#include "srs_app_ringbuf.hpp"

template<typename T, unsigned int N=1024>
class StCondition
{
public:
    StCondition() {
        notify_ringbuf_ = new SrsRingBuffer<T>(N);
    };
    ~StCondition() {};

    void notify() {
        T* notify_pkg = new T();
        notify_ringbuf_->Write_Packet_RingBuffer(notify_pkg);
    }

    int wait() {
        T* notify_pkg = nullptr;
        while (notify_ringbuf_->Read_Packet_RingBuffer(&notify_pkg) != E_RET_BUF_OK)
        {
            srs_usleep(10 * 1000);
            srs_thread_yield();
        }
        delete notify_pkg;
        return 0;
    }

    int wait(srs_utime_t timeout) {
        srs_utime_t us_beg = srs_get_system_startup_time();
        T* notify_pkg = nullptr;
        while (notify_ringbuf_->Read_Packet_RingBuffer(&notify_pkg) != E_RET_BUF_OK)
        {
            if( srs_get_system_startup_time() - us_beg >= timeout ) {
                return -1;
            }
            srs_usleep(10 * 1000);
            srs_thread_yield();
        }
        delete notify_pkg;
        return 0;
        
    }
    
private:
    SrsRingBuffer<T> *notify_ringbuf_;
};


