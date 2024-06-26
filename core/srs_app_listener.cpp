//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#include <srs_app_listener.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
using namespace std;

#include <srs_core_autofree.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_buffer.hpp>

// set the max packet size.
#define SRS_UDP_MAX_PACKET_SIZE 65535

// sleep in srs_utime_t for udp recv packet.
#define SrsUdpPacketRecvCycleInterval 0

ISrsUdpHandler::ISrsUdpHandler()
{
}

ISrsUdpHandler::~ISrsUdpHandler()
{
}

srs_error_t ISrsUdpHandler::on_stfd_change(srs_netfd_t /*fd*/)
{
    return srs_success;
}

void ISrsUdpHandler::set_stfd(srs_netfd_t /*fd*/)
{
}

ISrsTcpHandler::ISrsTcpHandler()
{
}

ISrsTcpHandler::~ISrsTcpHandler()
{
}

SrsUdpListener::SrsUdpListener(ISrsUdpHandler* h, string i, int p)
{
    handler = h;
    ip = i;
    port = p;
    lfd = NULL;
    
    nb_buf = SRS_UDP_MAX_PACKET_SIZE;
    buf = new char[nb_buf];
    
    trd = new SrsDummyCoroutine();
    cid = _srs_context->generate_id();
}

SrsUdpListener::~SrsUdpListener()
{
    srs_freep(trd);
    srs_close_stfd(lfd);
    srs_freepa(buf);
}

int SrsUdpListener::fd()
{
    return srs_netfd_fileno(lfd);
}

srs_netfd_t SrsUdpListener::stfd()
{
    return lfd;
}

void SrsUdpListener::set_socket_buffer()
{
    int default_sndbuf = 0;
    // TODO: FIXME: Config it.
    int expect_sndbuf = 1024*1024*10; // 10M
    int actual_sndbuf = expect_sndbuf;
    int r0_sndbuf = 0;
    if (true) {
        socklen_t opt_len = sizeof(default_sndbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&default_sndbuf, &opt_len);

        if ((r0_sndbuf = setsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&actual_sndbuf, sizeof(actual_sndbuf))) < 0) {
            srs_warn("set SO_SNDBUF failed, expect=%d, r0=%d", expect_sndbuf, r0_sndbuf);
        }

        opt_len = sizeof(actual_sndbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_SNDBUF, (void*)&actual_sndbuf, &opt_len);
    }

    int default_rcvbuf = 0;
    // TODO: FIXME: Config it.
    int expect_rcvbuf = 1024*1024*10; // 10M
    int actual_rcvbuf = expect_rcvbuf;
    int r0_rcvbuf = 0;
    if (true) {
        socklen_t opt_len = sizeof(default_rcvbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&default_rcvbuf, &opt_len);

        if ((r0_rcvbuf = setsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&actual_rcvbuf, sizeof(actual_rcvbuf))) < 0) {
            srs_warn("set SO_RCVBUF failed, expect=%d, r0=%d", expect_rcvbuf, r0_rcvbuf);
        }

        opt_len = sizeof(actual_rcvbuf);
        // TODO: FIXME: check err
        getsockopt(fd(), SOL_SOCKET, SO_RCVBUF, (void*)&actual_rcvbuf, &opt_len);
    }

    srs_trace("UDP #%d LISTEN at %s:%d, SO_SNDBUF(default=%d, expect=%d, actual=%d, r0=%d), SO_RCVBUF(default=%d, expect=%d, actual=%d, r0=%d)",
        srs_netfd_fileno(lfd), ip.c_str(), port, default_sndbuf, expect_sndbuf, actual_sndbuf, r0_sndbuf, default_rcvbuf, expect_rcvbuf, actual_rcvbuf, r0_rcvbuf);
}


srs_error_t SrsUdpListener::listen()
{
    srs_error_t err = srs_success;

    if ((err = srs_udp_listen(ip, port, &lfd)) != srs_success) {
        return srs_error_wrap(err, "listen %s:%d", ip.c_str(), port);
    }

    set_socket_buffer();

    handler->set_stfd(lfd);
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("udp", this, cid);
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start thread");
    }
    
    return err;
}

srs_error_t SrsUdpListener::cycle()
{
    srs_error_t err = srs_success;

    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "udp listener");
        }

        int nread = 0;
        sockaddr_storage from;
        int nb_from = sizeof(from);
        if ((nread = srs_recvfrom(lfd, buf, nb_buf, (sockaddr*)&from, &nb_from, SRS_UTIME_NO_TIMEOUT)) <= 0) {
            return srs_error_new(ERROR_SOCKET_READ, "udp read, nread=%d", nread);
        }

        if ((err = handler->on_udp_packet((const sockaddr*)&from, nb_from, buf, nread)) != srs_success) {
            return srs_error_wrap(err, "handle packet %d bytes", nread);
        }
        
        if (SrsUdpPacketRecvCycleInterval > 0) {
            srs_usleep(SrsUdpPacketRecvCycleInterval);
        }
    }
    
    return err;
}

SrsTcpListener::SrsTcpListener(ISrsTcpHandler* h, string i, int p)
{
    handler = h;
    ip = i;
    port = p;

    lfd = NULL;
    
    trd = new SrsDummyCoroutine();
}

SrsTcpListener::~SrsTcpListener()
{
    srs_freep(trd);
    srs_close_stfd(lfd);
}

int SrsTcpListener::fd()
{
    return srs_netfd_fileno(lfd);;
}

srs_error_t SrsTcpListener::listen()
{
    srs_error_t err = srs_success;

    if ((err = srs_tcp_listen(ip, port, &lfd)) != srs_success) {
        return srs_error_wrap(err, "listen at %s:%d", ip.c_str(), port);
    }
    
    srs_freep(trd);
    trd = new SrsSTCoroutine("tcp", this);
    ((SrsSTCoroutine*)trd)->set_stack_size(1 << 18); //fix it
    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start coroutine");
    }
    
    return err;
}

srs_error_t SrsTcpListener::cycle()
{
    srs_error_t err = srs_success;
    
    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "tcp listener");
        }
        
        srs_netfd_t fd = srs_accept(lfd, NULL, NULL, SRS_UTIME_NO_TIMEOUT);
        if(fd == NULL){
            return srs_error_new(ERROR_SOCKET_ACCEPT, "accept at fd=%d", srs_netfd_fileno(lfd));
        }
        
        if ((err = srs_fd_closeexec(srs_netfd_fileno(fd))) != srs_success) {
            return srs_error_wrap(err, "set closeexec");
        }
        
        if ((err = handler->on_tcp_client(fd)) != srs_success) {
            return srs_error_wrap(err, "handle fd=%d", srs_netfd_fileno(fd));
        }
    }
    
    return err;
}

SrsUdpMuxSocket::SrsUdpMuxSocket(srs_netfd_t fd)
{
    nn_msgs_for_yield_ = 0;
    nb_buf = SRS_UDP_MAX_PACKET_SIZE;
    buf = new char[nb_buf];
    nread = 0;

    lfd = fd;

    fromlen = 0;
    peer_port = 0;

    fast_id_ = 0;
    address_changed_ = false;
    cache_buffer_ = new SrsBuffer(buf, nb_buf);
}

SrsUdpMuxSocket::~SrsUdpMuxSocket()
{
    srs_freepa(buf);
    srs_freep(cache_buffer_);
}


srs_error_t SrsUdpMuxSocket::update_from_sockaddr(std::string peer_ip, int port)
{
    srs_error_t err = srs_success;
  
  /*  char sport[8];
    snprintf(sport, sizeof(sport), "%d", port);
    
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    addrinfo* r  = NULL;
    SrsAutoFree(addrinfo, r);
    if(getaddrinfo(peer_ip.c_str(), sport, (const addrinfo*)&hints, &r)) {
        return srs_error_new(ERROR_SYSTEM_IP_INVALID, "get address info");
    }

    memset(&from, 0, sizeof(from));

    memcpy(&from, r, sizeof(struct sockaddr_in));

*/
    struct sockaddr_in sock_addr;
 
    bzero(&sock_addr,sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = inet_addr(peer_ip.c_str());
    memset(&from, 0, sizeof(from));
    memcpy(&from, &sock_addr, sizeof(struct sockaddr_in));
    fromlen =sizeof(struct sockaddr_in);
    address_changed_ = true;

    return err;
}

int SrsUdpMuxSocket::recvfrom(srs_utime_t timeout)
{
    fromlen = sizeof(from);
    nread = srs_recvfrom(lfd, buf, nb_buf, (sockaddr*)&from, &fromlen, timeout);
    if (nread <= 0) {
        return nread;
    }

    // Reset the fast cache buffer size.
    cache_buffer_->set_size(nread);
    cache_buffer_->skip(-1 * cache_buffer_->pos());

    // Drop UDP health check packet of Aliyun SLB.
    //      Healthcheck udp check
    // @see https://help.aliyun.com/document_detail/27595.html
    if (nread == 21 && buf[0] == 0x48 && buf[1] == 0x65 && buf[2] == 0x61 && buf[3] == 0x6c
        && buf[19] == 0x63 && buf[20] == 0x6b) {
        return 0;
    }

    // Parse address from cache.
    if (from.ss_family == AF_INET) {
        sockaddr_in* addr = (sockaddr_in*)&from;
        fast_id_ = uint64_t(addr->sin_port)<<48 | uint64_t(addr->sin_addr.s_addr);
    }

    // We will regenerate the peer_ip, peer_port and peer_id.
    address_changed_ = true;

    return nread;
}

srs_error_t SrsUdpMuxSocket::sendto(void* data, int size, srs_utime_t timeout)
{
    srs_error_t err = srs_success;

    int nb_write = srs_sendto(lfd, data, size, (sockaddr*)&from, fromlen, timeout);

    if (nb_write <= 0) {
        if (nb_write < 0 && errno == ETIME) {
            return srs_error_new(ERROR_SOCKET_TIMEOUT, "sendto timeout %d ms", srsu2msi(timeout));
        }   
    
        return srs_error_new(ERROR_SOCKET_WRITE, "sendto");
    }

    // Yield to another coroutines.
    // @see https://github.com/ossrs/srs/issues/2194#issuecomment-777542162
    if (++nn_msgs_for_yield_ > 20) {
        nn_msgs_for_yield_ = 0;
        srs_thread_yield();
    }

    return err;
}

srs_netfd_t SrsUdpMuxSocket::stfd()
{
    return lfd;
}

sockaddr_in* SrsUdpMuxSocket::peer_addr()
{
    return (sockaddr_in*)&from;
}

socklen_t SrsUdpMuxSocket::peer_addrlen()
{
    return (socklen_t)fromlen;
}

char* SrsUdpMuxSocket::data()
{
    return buf;
}

int SrsUdpMuxSocket::size()
{
    return nread;
}

std::string SrsUdpMuxSocket::get_peer_ip() const
{
    return peer_ip;
}

int SrsUdpMuxSocket::get_peer_port() const
{
    return peer_port;
}

int SrsUdpMuxSocket::get_local_port() const
{
    return local_port_;
}

std::string SrsUdpMuxSocket::peer_id()
{
    if (address_changed_) {
        address_changed_ = false;

        // Parse address from cache.
        bool parsed = false;
        if (from.ss_family == AF_INET) {
            sockaddr_in* addr = (sockaddr_in*)&from;

            // Load from fast cache, previous ip.
            std::map<uint32_t, string>::iterator it = cache_.find(addr->sin_addr.s_addr);
            if (it == cache_.end()) {
                peer_ip = inet_ntoa(addr->sin_addr);
                cache_[addr->sin_addr.s_addr] = peer_ip;
            } else {
                peer_ip = it->second;
            }

            peer_port = ntohs(addr->sin_port);
            parsed = true;
        }

        if (!parsed) {
            // TODO: FIXME: Maybe we should not covert to string for each packet.
            char address_string[64];
            char port_string[16];
            if (getnameinfo((sockaddr*)&from, fromlen,
                           (char*)&address_string, sizeof(address_string),
                           (char*)&port_string, sizeof(port_string),
                           NI_NUMERICHOST|NI_NUMERICSERV)) {
                return "";
            }

            peer_ip = std::string(address_string);
            peer_port = atoi(port_string);
        }

        // Build the peer id.
        static char id_buf[128];
        int len = snprintf(id_buf, sizeof(id_buf), "%s:%d", peer_ip.c_str(), peer_port);
        peer_id_ = string(id_buf, len);

    }

    return peer_id_;
}

uint64_t SrsUdpMuxSocket::fast_id()
{
    return fast_id_;
}

SrsBuffer* SrsUdpMuxSocket::buffer()
{
    return cache_buffer_;
}

SrsUdpMuxSocket* SrsUdpMuxSocket::copy_sendonly()
{
    SrsUdpMuxSocket* sendonly = new SrsUdpMuxSocket(lfd);

    // Don't copy buffer
    srs_freepa(sendonly->buf);
    sendonly->nb_buf    = 0;
    sendonly->nread     = 0;
    sendonly->lfd       = lfd;
    sendonly->from      = from;
    sendonly->fromlen   = fromlen;
    sendonly->peer_ip   = peer_ip;
    sendonly->peer_port = peer_port;

    // Copy the fast id.
    sendonly->peer_id_ = peer_id_;
    sendonly->fast_id_ = fast_id_;
    sendonly->address_changed_ = address_changed_;

    return sendonly;
}
