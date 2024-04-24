#include <string>
#include <iostream>
#include <vector>

#include <arpa/inet.h>

#include "srs_service_st.hpp"
#include "srs_core.hpp"
#include "srs_kernel_error.hpp"
#include "srs_kernel_log.hpp"
#include "srs_app_log.hpp"
#include "srs_service_log.hpp"
#include "srs_app_listener.hpp"

#include "srs_app_hourglass.hpp"

using namespace std;

ISrsLog* _srs_log = NULL;
ISrsContext* _srs_context = NULL;

class UdpListener: public ISrsUdpHandler
{
public:
    UdpListener() {
        listener_ = NULL;
    }
    ~UdpListener() {
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
        srs_info("UdpListener listen success");
        return err;
    }

public:
    srs_error_t on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) {
        static int i = 0;
        srs_error_t err = srs_success;
        string msg(buf,nb_buf);
        string fromip = inet_ntoa(((sockaddr_in*)from)->sin_addr);
        int fromport = ntohs(((sockaddr_in*)from)->sin_port);
        srs_info("UdpListener[%s:%d] recv[%s:%d] : buff(%s)", ip_.c_str(), port_, fromip.c_str(), fromport,  msg.c_str());
        int nwirte = 0 ;

        sockaddr_in to_addr;
        to_addr.sin_family = AF_INET;
        to_addr.sin_port = ((sockaddr_in*)from)->sin_port;
        to_addr.sin_addr.s_addr = from->sa_data[2];

        string sendMsg = "udp server recv from udp client. count:[" + to_string(i++) + "]";
        if (( nwirte = srs_sendto(listener_->stfd(), (void*)sendMsg.c_str(), sendMsg.size(), (sockaddr*)&to_addr, sizeof(sockaddr_in), SRS_UTIME_NO_TIMEOUT)) <= 0) {
            srs_error("UdpListener send error. nwirte=%d", nwirte);
            return srs_error_new(ERROR_SOCKET_READ, "udp wirte, nwirte=%d", nwirte);
        }

        return err;
    }

public:
    SrsUdpListener* listener_;
    string ip_;
    int port_;
};

class UdpSendTimeTask : public ISrsCoroutineHandler, public ISrsUdpHandler
{
private:
    SrsCoroutine* trd;
    SrsContextId cid;
    SrsUdpListener* listener_;
    vector<pair<std::string, int>> list_server_;
    string ip_;
    int port_;

public:
    UdpSendTimeTask() {
        ip_ = "172.30.138.150";
        port_ = 20000;
        trd = new SrsDummyCoroutine();
        cid = _srs_context->generate_id();
    }

    ~UdpSendTimeTask() {
        srs_freep(trd);
    }

    void register_server(std::string ip, int port) {
        list_server_.push_back(make_pair(ip, port));
    }

// 协程
public:
    srs_error_t run() {
        srs_error_t err = srs_success;
        listener_ = new SrsUdpListener(this, ip_, port_);
        if((err=listener_->listen()) != srs_success) {
            srs_error( "UdpSendTimeTask listen failed [%s]", srs_error_desc(err).c_str() );
            return srs_error_wrap(err, "listen err");
        }

        srs_freep(trd);
        trd = new SrsSTCoroutine("UdpSendTimeTask", this, cid);
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

            for (int i = 0; i < list_server_.size(); i++) {
                srs_trace("Send udp to %s:%d", list_server_[i].first.c_str(), list_server_[i].second);

                sockaddr_in to_addr;
                to_addr.sin_family = AF_INET;
                to_addr.sin_port = htons(list_server_[i].second);
                to_addr.sin_addr.s_addr = inet_addr(list_server_[i].first.c_str());

                string sendMsg = "UdpSendTimeTask Send idx : " + to_string(num++);
                int nwirte = 0;
                if ((nwirte = srs_sendto(listener_->stfd(), (void*)sendMsg.c_str(), sendMsg.size(), (sockaddr*)&to_addr, sizeof(sockaddr_in), SRS_UTIME_NO_TIMEOUT)) <= 0) {
                    return srs_error_new(ERROR_SOCKET_READ, "udp wirte, nwirte=%d", nwirte);
                }
            }

            srs_usleep(1 * 1000 * 1000);
        }
        
        return err;
    }

public:
    srs_error_t on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) {
        static int i = 0;
        srs_error_t err = srs_success;
        string msg(buf,nb_buf);
        srs_info("UdpSendTimeTask  recv ");
        return err;
    }
};


bool init()
{
    _srs_log = new SrsFileLog();
    _srs_log->initialize();

    _srs_context = new SrsThreadContext();
    srs_error_t err = srs_st_init();
    if (err != srs_success) {
        srs_error( "initialize st failed [%s]", srs_error_desc(err).c_str() );
        return false;
    }
    return true;
}

int main(int argc, const char* argv[])
{
    if (!init()) {
        exit(-1);
    }

    srs_trace("udp listener starting");
    UdpListener udp_listener_1 = UdpListener();
    UdpListener udp_listener_2 = UdpListener();
    UdpListener udp_listener_3 = UdpListener();
    udp_listener_1.listen("172.30.138.150", 14000);
    udp_listener_2.listen("172.30.138.150", 15000);
    udp_listener_3.listen("172.30.138.150", 16000);
    srs_trace("udp listener started");

    srs_trace("udp sender starting");
    UdpSendTimeTask udp_sender = UdpSendTimeTask();
    udp_sender.register_server("172.30.138.150", 14000);
    udp_sender.register_server("172.30.138.150", 15000);
    udp_sender.register_server("172.30.138.150", 16000);

    udp_sender.run();
    srs_trace("udp sender started");

    srs_usleep(SRS_UTIME_NO_TIMEOUT);
    srs_trace("demo exit");
    return 0;
}
