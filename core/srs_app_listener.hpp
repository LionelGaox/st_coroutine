#pragma once

#include <srs_core.hpp>

#include <sys/socket.h>
#include <netinet/in.h>

#include <map>
#include <string>

#include <srs_app_st.hpp>

struct sockaddr;

class SrsBuffer;
class SrsUdpMuxSocket;

// The udp packet handler.
class ISrsUdpHandler
{
public:
    ISrsUdpHandler();
    virtual ~ISrsUdpHandler();
public:
    // When fd changed, for instance, reload the listen port,
    // notify the handler and user can do something.
    virtual srs_error_t on_stfd_change(srs_netfd_t fd);
    
    virtual void set_stfd(srs_netfd_t fd);
public:
    // When udp listener got a udp packet, notice server to process it.
    // @param type, the client type, used to create concrete connection,
    //       for instance RTMP connection to serve client.
    // @param from, the udp packet from address.
    // @param buf, the udp packet bytes, user should copy if need to use.
    // @param nb_buf, the size of udp packet bytes.
    // @remark user should never use the buf, for it's a shared memory bytes.
    virtual srs_error_t on_udp_packet(const sockaddr* from, const int fromlen, char* buf, int nb_buf) = 0;
};

// The tcp connection handler.
class ISrsTcpHandler
{
public:
    ISrsTcpHandler();
    virtual ~ISrsTcpHandler();
public:
    // When got tcp client.
    virtual srs_error_t on_tcp_client(srs_netfd_t stfd) = 0;
};

// Bind udp port, start thread to recv packet and handler it.
class SrsUdpListener : public ISrsCoroutineHandler
{
protected:
    srs_netfd_t lfd;
    SrsCoroutine* trd;
    SrsContextId cid;
protected:
    char* buf;
    int nb_buf;
protected:
    ISrsUdpHandler* handler;
    std::string ip;
    int port;
public:
    SrsUdpListener(ISrsUdpHandler* h, std::string i, int p);
    virtual ~SrsUdpListener();
public:
    virtual int fd();
    virtual srs_netfd_t stfd();
private:
    void set_socket_buffer();
public:
    virtual srs_error_t listen();
// Interface ISrsReusableThreadHandler.
public:
    virtual srs_error_t cycle();
};

// Bind and listen tcp port, use handler to process the client.
class SrsTcpListener : public ISrsCoroutineHandler
{
private:
    srs_netfd_t lfd;
    SrsCoroutine* trd;
private:
    ISrsTcpHandler* handler;
    std::string ip;
    int port;
public:
    SrsTcpListener(ISrsTcpHandler* h, std::string i, int p);
    virtual ~SrsTcpListener();
public:
    virtual int fd();
public:
    virtual srs_error_t listen();
// Interface ISrsReusableThreadHandler.
public:
    virtual srs_error_t cycle();
};

// TODO: FIXME: Rename it. Refine it for performance issue.
class SrsUdpMuxSocket
{
private:
    // For sender yield only.
    uint32_t nn_msgs_for_yield_;
    std::map<uint32_t, std::string> cache_;
    SrsBuffer* cache_buffer_;
private:
    char* buf;
    int nb_buf;
    int nread;
    srs_netfd_t lfd;
    sockaddr_storage from;
    int fromlen;
private:
    std::string peer_ip;
    int peer_port;
    int local_port_;
private:
    // Cache for peer id.
    std::string peer_id_;
    // If the address changed, we should generate the peer_id.
    bool address_changed_;
    // For IPv4 client, we use 8 bytes int id to find it fastly.
    uint64_t fast_id_;
public:
    SrsUdpMuxSocket(srs_netfd_t fd);
    virtual ~SrsUdpMuxSocket();
public:
    int recvfrom(srs_utime_t timeout);
    srs_error_t sendto(void* data, int size, srs_utime_t timeout);
    srs_error_t update_from_sockaddr(std::string peer_ip, int port);

    srs_netfd_t stfd();
    sockaddr_in* peer_addr();
    socklen_t peer_addrlen();
    char* data();
    int size();
    std::string get_peer_ip() const;
    int get_peer_port() const;
    int get_local_port() const;
    void set_local_port(int port) { local_port_ = port;}
    std::string peer_id();
    uint64_t fast_id();
    SrsBuffer* buffer();
    SrsUdpMuxSocket* copy_sendonly();
};
