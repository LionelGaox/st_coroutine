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
protected:
    char* buf;
    int nb_buf;
public:
    UdpTestWork(string i, int p) {
        ip = i;
        port = p;

        nb_buf = 65535;
        buf = new char[nb_buf];

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

            int nwirte = 0;
            sockaddr_in to_addr;
            to_addr.sin_family = AF_INET;
            to_addr.sin_addr.s_addr = inet_addr(ip.c_str());

            if(num%2==0) {
                to_addr.sin_port = htons(14000);
            } else {
                to_addr.sin_port = htons(15000);
            }

            string sendMsg = "16000 Send idx : " + to_string(num);
            srs_error( "UdpTestWork send [%s]",sendMsg.c_str() );

            if ((nwirte = srs_sendto(udp_->listener_->stfd(), (void*)sendMsg.c_str(), sendMsg.size(), (sockaddr*)&to_addr, sizeof(sockaddr_in), SRS_UTIME_NO_TIMEOUT)) <= 0) {
                return srs_error_new(ERROR_SOCKET_READ, "udp wirte, nwirte=%d", nwirte);
            }

            num++;

        }
        
        return err;
    }
};

void do_main()
{
    srs_error_t err = srs_success;

    _srs_log = new SrsFileLog();
    _srs_context = new SrsThreadContext();

    _srs_log->initialize();

    if ((err = srs_st_init()) != srs_success) {
        srs_error( "initialize st failed [%s]", srs_error_desc(err).c_str() );
        return;
    }

    srs_info("do_main 1");

    auto udp1 = UdpServer();
    if((err=udp1.listen("172.24.253.16", 14000)) != srs_success) {
        srs_error( "listen failed [%s]", srs_error_desc(err).c_str() );
    }

    auto udp2 = UdpServer();
    if((err=udp2.listen("172.24.253.16", 15000)) != srs_success) {
        srs_error( "listen failed [%s]", srs_error_desc(err).c_str() );
    }

    auto work = UdpTestWork("172.24.253.16", 16000);
    work.run();

    srs_info("do_main 2");
    while(true) {
        srs_usleep( 10 * 1000 * 1000);
    }


}

#include <st.h>

int main(int argc, const char* argv[])
{
    // test t;
    // cout << "aaaa:" << t.a << endl;
    // srs_thread_yield();
    
    do_main();

    srs_trace("demo exit");

    return 0;
}