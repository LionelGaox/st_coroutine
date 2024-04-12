#include <string>
#include <iostream>

#include <arpa/inet.h>

#include "core/srs_service_st.hpp"
#include "core/srs_core.hpp"
#include "core/srs_kernel_error.hpp"
#include "core/srs_kernel_log.hpp"
#include "core/srs_app_log.hpp"
#include "core/srs_service_log.hpp"
#include "core/srs_app_listener.hpp"

#include "core/srs_app_hourglass.hpp"

using namespace std;

ISrsLog* _srs_log = NULL;
ISrsContext* _srs_context = NULL;

class UdpServer: public ISrsUdpHandler
{
public:
    UdpServer() {
        listener_ = NULL;
    }
    ~UdpServer() {
        srs_freep(listener_);
    }

    srs_error_t listen(string ip, int port) {
        ip_ = ip;
        port_=port;
        srs_error_t err = srs_success;
        listener_ = new SrsUdpListener(this, ip, port);
        if ((err = listener_->listen()) != srs_success) {
            return srs_error_wrap(err, "listen %s:%d", ip.c_str(), port);
        }
        srs_info("UdpServer listen success");
        return err;
    }

public:
    srs_error_t on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) {
        srs_error_t err = srs_success;
        string msg(buf,nb_buf);
        srs_info("UdpServer on_udp_packet[%s:%d] : buff(%s)", ip_.c_str(), port_, msg.c_str());
        return err;
    }

public:
    SrsUdpListener* listener_;
    string ip_;
    int port_;
};

class UdpTestWork : public ISrsCoroutineHandler
{
protected:
    SrsCoroutine* trd;
    SrsContextId cid;
    UdpServer* udp_;
    std::string ip;
    int port;

public:
    UdpTestWork(string i, int p) {
        ip = i;
        port = p;

        trd = new SrsDummyCoroutine();
        cid = _srs_context->generate_id();
    }

    ~UdpTestWork() {
        srs_freep(trd);
    }

// 协程
public:
    srs_error_t run() {
        srs_error_t err = srs_success;
        udp_ = new UdpServer();
        if((err=udp_->listen(ip, port)) != srs_success) {
            srs_error( "UdpTestWork listen failed [%s]", srs_error_desc(err).c_str() );
            return srs_error_wrap(err, "listen err");
        }

        srs_freep(trd);
        trd = new SrsSTCoroutine("UdpTestWork", this, cid);
        if ((err = trd->start()) != srs_success) {
            return srs_error_wrap(err, "start thread");
        }

        return err;
    }

    srs_error_t cycle() {
        srs_error_t err = srs_success;

        int num = 1;

        while (true) {
            srs_usleep(1 * 1000 * 1000);

            if ((err = trd->pull()) != srs_success) {
                return srs_error_wrap(err, "udp worker");
            }

            // int nwirte = 0;
            // sockaddr_in to_addr;
            // to_addr.sin_family = AF_INET;
            // to_addr.sin_addr.s_addr = inet_addr(ip.c_str());

            // string sendMsg = "172.20.204.180 Send idx : " + to_string(num);

            // to_addr.sin_port = htons(9987);

            srs_error( "Send ........" );

            // if ((nwirte = srs_sendto(udp_->listener_->stfd(), (void*)sendMsg.c_str(), sendMsg.size(), (sockaddr*)&to_addr, sizeof(sockaddr_in), SRS_UTIME_NO_TIMEOUT)) <= 0) {
            //     return srs_error_new(ERROR_SOCKET_READ, "udp wirte, nwirte=%d", nwirte);
            // }

            num++;

        }
        
        return err;
    }
};

#include  <malloc.h>

class TimerTest : public ISrsFastTimer
{
public:
    TimerTest() {
        timer5s_ = new SrsFastTimer("hybrid", 1 * SRS_UTIME_SECONDS);
    };
    virtual ~TimerTest() {
        srs_freep(timer5s_);
    };
public:
    srs_error_t run() {
        srs_error_t err = srs_success;
        if ((err = timer5s_->start()) != srs_success) {
            return srs_error_wrap(err, "start timer");
        }
        timer5s_->subscribe(this);
        return err;
    }
    // Tick when timer is active.
    srs_error_t on_timer(srs_utime_t interval) {
        srs_error_t err = srs_success;
        // malloc_stats();
        struct mallinfo info = mallinfo();
        srs_trace("system bytes     =     %ld", info.arena + info.hblkhd);
        srs_trace("in use bytes     =     %ld", info.uordblks + info.hblkhd);
        return err;
    };
private:
    SrsFastTimer* timer5s_;
};

void do_main(const std::string& ip, int port1, int port2, int testWorkPort)
{
    _srs_log = new SrsFileLog();
    _srs_log->initialize();

    _srs_context = new SrsThreadContext();
    srs_error_t err = srs_st_init();
    if (err != srs_success) {
        srs_error( "initialize st failed [%s]", srs_error_desc(err).c_str() );
        return;
    }

    // UdpTestWork work(ip, testWorkPort);
    // err = work.run();
    // if (err != srs_success) {
    //     srs_error( "work run failed [%s]", srs_error_desc(err).c_str() );
    //     return;
    // }
    
    TimerTest tt;
    tt.run();

    srs_usleep(SRS_UTIME_NO_TIMEOUT);
    srs_info("do_main 2");
    // Wait for all server to quit.
}

class IAsynCallTask
{
public:
    IAsynCallTask();
    virtual ~IAsynCallTask();
public:
    virtual int call() = 0;
};

class AsynCallWorker : public ISrsCoroutineHandler
{
public:

};



int main_1(int argc, const char* argv[])
{

    do_main("127.0.0.1", 14000, 15000, 16000);

    srs_trace("demo exit");

    return 0;
}


// #include <iostream>
// #include <fstream>
// #include <vector>
// #include <boost/archive/text_oarchive.hpp>
// #include <boost/archive/text_iarchive.hpp>
// #include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// #include <boost/serialization/vector.hpp>

// int main_bak(int argc, const char* argv[])
// {
//     std::vector<int> numbers = {1, 2, 3, 4, 5};
//     std::vector<int> numbers_out;

//     // 序列化
//     std::ostringstream oss;
//     boost::archive::binary_oarchive oa(oss);
//     oa << BOOST_SERIALIZATION_NVP(numbers);
//     const std::string& str_in = oss.str();

//     // 反序列化
//     std::istringstream iss(str_in);
//     boost::archive::binary_iarchive ia(iss);
//     ia >> BOOST_SERIALIZATION_NVP(numbers_out);
//     // ifs.close();

//     // 打印反序列化后的容器内容
//     std::printf("sizeof[%ld] data[%x][%x][%x][%x]\n", str_in.size(), str_in.c_str()[0], str_in.c_str()[17], str_in.c_str()[34], str_in.c_str()[67]);

//     for (const auto& num : numbers_out) {
//         std::cout << num << " ";
//     }
//     std::cout << std::endl;

//     return 0;
// }

#include "srs_app_ringbuf.hpp"
#include "srs_app_stcondition.hpp"

class Packet
{
public:
    Packet() {};
    ~Packet() {};

    void set(int data) {
        data_ = data;
    }
    
    int get() {
        return data_;
    }

private:
    int data_;
};

StCondition<int> st_cond;

class Notifier  : public ISrsCoroutineHandler
{
protected:
    SrsCoroutine* trd;
    SrsContextId cid;

public:
    Notifier() {
        trd = new SrsDummyCoroutine();
        cid = _srs_context->generate_id();
    }

    ~Notifier() {
        srs_freep(trd);
    }

// 协程
public:
    srs_error_t run() {
        srs_error_t err = srs_success;
        srs_freep(trd);
        trd = new SrsSTCoroutine("Notifier", this, cid);
        if ((err = trd->start()) != srs_success) {
            return srs_error_wrap(err, "start thread");
        }
        return err;
    }

    srs_error_t cycle() {
        srs_error_t err = srs_success;
        int num = 1;
        while (true) {
            if ((err = trd->pull()) != srs_success) {
                return srs_error_wrap(err, "udp worker");
            }
            srs_usleep(2 * 1000 * 1000);
            st_cond.notify();
            srs_error("... notify %d", num++);
        }
        return err;
    }
};

class Waiter : public ISrsCoroutineHandler
{
protected:
    SrsCoroutine* trd;
    SrsContextId cid;

public:
    Waiter() {
        trd = new SrsDummyCoroutine();
        cid = _srs_context->generate_id();
    }

    ~Waiter() {
        srs_freep(trd);
    }

// 协程
public:
    srs_error_t run() {
        srs_error_t err = srs_success;
        srs_freep(trd);
        trd = new SrsSTCoroutine("Waiter", this, cid);
        if ((err = trd->start()) != srs_success) {
            return srs_error_wrap(err, "start thread");
        }
        return err;
    }

    srs_error_t cycle() {
        srs_error_t err = srs_success;
        int num = 1;
        while (true) {
            st_cond.wait();
            srs_error("... wait %d", num++);
        }
        return err;
    }
};

void run()
{
    _srs_log = new SrsFileLog();
    _srs_log->initialize();

    _srs_context = new SrsThreadContext();
    srs_error_t err = srs_st_init();
    if (err != srs_success) {
        srs_error( "initialize st failed [%s]", srs_error_desc(err).c_str() );
        return;
    }

    srs_info("start ...");
    
    TimerTest tt;
    tt.run();

    Waiter W;
    // Notifier N;

    W.run();
    // N.run();

    srs_usleep(SRS_UTIME_NO_TIMEOUT);
}

#include <thread>
int main(int argc, const char* argv[])
{
    thread t( []() {
        int i=1;
        while (true)
        {
            sleep(3);
            printf("sleep %d\n", i++);
            st_cond.notify();
        }
    });

    run();

    return 0;
}
