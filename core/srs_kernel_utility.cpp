//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#include <srs_kernel_utility.hpp>

#ifndef _WIN32
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#endif

#ifdef OS_QNX
#include <sys/socket.h>
typedef uint8_t u_int8_t;
#endif

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <vector>
#include <algorithm>
using namespace std;

#include <srs_core_autofree.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_error.hpp>
#include <srs_kernel_buffer.hpp>

// this value must:
// equals to (SRS_SYS_CYCLE_INTERVAL*SRS_SYS_TIME_RESOLUTION_MS_TIMES)*1000
// @see SRS_SYS_TIME_RESOLUTION_MS_TIMES
#define SYS_TIME_RESOLUTION_US 300*1000

srs_utime_t _srs_system_time_us_cache = 0;
srs_utime_t _srs_system_time_startup_time = 0;

srs_utime_t srs_get_system_time()
{
    if (_srs_system_time_us_cache <= 0) {
        srs_update_system_time();
    }
    
    return _srs_system_time_us_cache;
}

srs_utime_t srs_get_system_startup_time()
{
    if (_srs_system_time_startup_time <= 0) {
        srs_update_system_time();
    }

    return _srs_system_time_startup_time;
}

// For utest to mock it.
#ifndef SRS_OSX
srs_gettimeofday_t _srs_gettimeofday = (srs_gettimeofday_t)::gettimeofday;
#endif

srs_utime_t srs_update_system_time()
{
    timeval now;
    
    if (_srs_gettimeofday(&now, NULL) < 0) {
        srs_warn("gettimeofday failed, ignore");
        return -1;
    }
    
    // we must convert the tv_sec/tv_usec to int64_t.
    int64_t now_us = ((int64_t)now.tv_sec) * 1000 * 1000 + (int64_t)now.tv_usec;
    
    // for some ARM os, the starttime maybe invalid,
    // for example, on the cubieboard2, the srs_startup_time is 1262304014640,
    // while now is 1403842979210 in ms, diff is 141538964570 ms, 1638 days
    // it's impossible, and maybe the problem of startup time is invalid.
    // use date +%s to get system time is 1403844851.
    // so we use relative time.
    if (_srs_system_time_us_cache <= 0) {
        _srs_system_time_startup_time = _srs_system_time_us_cache = now_us;
        return _srs_system_time_us_cache;
    }
    
    // use relative time.
    int64_t diff = now_us - _srs_system_time_us_cache;
    diff = srs_max(0, diff);
    if (diff < 0 || diff > 1000 * SYS_TIME_RESOLUTION_US) {
        srs_warn("clock jump, history=%lldus, now=%lldus, diff=%lldus", _srs_system_time_us_cache, now_us, diff);
        _srs_system_time_startup_time += diff;
    }
    
    _srs_system_time_us_cache = now_us;
    srs_info("clock updated, startup=%lldus, now=%lldus", _srs_system_time_startup_time, _srs_system_time_us_cache);
    
    return _srs_system_time_us_cache;
}

// TODO: FIXME: Replace by ST dns resolve.
string srs_dns_resolve(string host, int& family)
{
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    
    addrinfo* r = NULL;
    SrsAutoFree(addrinfo, r);
    if(getaddrinfo(host.c_str(), NULL, &hints, &r)) {
        return "";
    }
    
    char shost[64];
    memset(shost, 0, sizeof(shost));
    if (getnameinfo(r->ai_addr, r->ai_addrlen, shost, sizeof(shost), NULL, 0, NI_NUMERICHOST)) {
        return "";
    }

   family = r->ai_family;
   return string(shost);
}

void srs_parse_hostport(string hostport, string& host, int& port)
{
    // No host or port.
    if (hostport.empty()) {
        return;
    }

    size_t pos = string::npos;

    // Host only for ipv4.
    if ((pos = hostport.rfind(":")) == string::npos) {
        host = hostport;
        return;
    }

    // For ipv4(only one colon), host:port.
    if (hostport.find(":") == pos) {
        host = hostport.substr(0, pos);
        string p = hostport.substr(pos + 1);
        if (!p.empty() && p != "0") {
            port = ::atoi(p.c_str());
        }
        return;
    }

    // Host only for ipv6.
    if (hostport.at(0) != '[' || (pos = hostport.rfind("]:")) == string::npos) {
        host = hostport;
        return;
    }

    // For ipv6, [host]:port.
    host = hostport.substr(1, pos - 1);
    string p = hostport.substr(pos + 2);
    if (!p.empty() && p != "0") {
        port = ::atoi(p.c_str());
    }
}

string srs_any_address_for_listener()
{
    bool ipv4_active = false;
    bool ipv6_active = false;

    if (true) {
        int fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(fd != -1) {
            ipv4_active = true;
            close(fd);
        }
    }
    if (true) {
        int fd = socket(AF_INET6, SOCK_DGRAM, 0);
        if(fd != -1) {
            ipv6_active = true;
            close(fd);
        }
    }

    if (ipv6_active && !ipv4_active) {
        return SRS_CONSTS_LOOPBACK6;
    }
    return SRS_CONSTS_LOOPBACK;
}

void srs_parse_endpoint(string hostport, string& ip, int& port)
{
    const size_t pos = hostport.rfind(":");   // Look for ":" from the end, to work with IPv6.
    if (pos != std::string::npos) {
        if ((pos >= 1) && (hostport[0] == '[') && (hostport[pos - 1] == ']')) {
            // Handle IPv6 in RFC 2732 format, e.g. [3ffe:dead:beef::1]:1935
            ip = hostport.substr(1, pos - 2);
        } else {
            // Handle IP address
            ip = hostport.substr(0, pos);
        }
        
        const string sport = hostport.substr(pos + 1);
        port = ::atoi(sport.c_str());
    } else {
        ip = srs_any_address_for_listener();
        port = ::atoi(hostport.c_str());
    }
}

bool srs_check_ip_addr_valid(string ip)
{
    unsigned char buf[sizeof(struct in6_addr)];

    // check ipv4
    int ret = inet_pton(AF_INET, ip.data(), buf);
    if (ret > 0) {
        return true;
    }
        
    ret = inet_pton(AF_INET6, ip.data(), buf);
    if (ret > 0) {
        return true;
    }
        
    return false;
}

string srs_int2str(int64_t value)
{
    // len(max int64_t) is 20, plus one "+-."
    char tmp[22];
    snprintf(tmp, 22, "%ld", value);
    return tmp;
}

string srs_float2str(double value)
{
    // len(max int64_t) is 20, plus one "+-."
    char tmp[22];
    snprintf(tmp, 22, "%.2f", value);
    return tmp;
}

string srs_bool2switch(bool v) {
    return v? "on" : "off";
}

bool srs_is_little_endian()
{
    // convert to network(big-endian) order, if not equals,
    // the system is little-endian, so need to convert the int64
    static int little_endian_check = -1;
    
    if(little_endian_check == -1) {
        union {
            int32_t i;
            int8_t c;
        } little_check_union;
        
        little_check_union.i = 0x01;
        little_endian_check = little_check_union.c;
    }
    
    return (little_endian_check == 1);
}

string srs_string_replace(string str, string old_str, string new_str)
{
    std::string ret = str;
    
    if (old_str == new_str) {
        return ret;
    }
    
    size_t pos = 0;
    while ((pos = ret.find(old_str, pos)) != std::string::npos) {
        ret = ret.replace(pos, old_str.length(), new_str);
        pos += new_str.length();
    }
    
    return ret;
}

string srs_string_trim_end(string str, string trim_chars)
{
    std::string ret = str;
    
    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);
        
        while (!ret.empty() && ret.at(ret.length() - 1) == ch) {
            ret.erase(ret.end() - 1);
            
            // ok, matched, should reset the search
            i = -1;
        }
    }
    
    return ret;
}

string srs_string_trim_start(string str, string trim_chars)
{
    std::string ret = str;
    
    for (int i = 0; i < (int)trim_chars.length(); i++) {
        char ch = trim_chars.at(i);
        
        while (!ret.empty() && ret.at(0) == ch) {
            ret.erase(ret.begin());
            
            // ok, matched, should reset the search
            i = -1;
        }
    }
    
    return ret;
}

string srs_string_remove(string str, string remove_chars)
{
    std::string ret = str;
    
    for (int i = 0; i < (int)remove_chars.length(); i++) {
        char ch = remove_chars.at(i);
        
        for (std::string::iterator it = ret.begin(); it != ret.end();) {
            if (ch == *it) {
                it = ret.erase(it);
                
                // ok, matched, should reset the search
                i = -1;
            } else {
                ++it;
            }
        }
    }
    
    return ret;
}

string srs_erase_first_substr(string str, string erase_string)
{
	std::string ret = str;

	size_t pos = ret.find(erase_string);

	if (pos != std::string::npos) {
		ret.erase(pos, erase_string.length());
	}
    
	return ret;
}

string srs_erase_last_substr(string str, string erase_string)
{
	std::string ret = str;

	size_t pos = ret.rfind(erase_string);

	if (pos != std::string::npos) {
		ret.erase(pos, erase_string.length());
	}
    
	return ret;
}

bool srs_string_ends_with(string str, string flag)
{
    const size_t pos = str.rfind(flag);
    return (pos != string::npos) && (pos == str.length() - flag.length());
}

bool srs_string_ends_with(string str, string flag0, string flag1)
{
    return srs_string_ends_with(str, flag0) || srs_string_ends_with(str, flag1);
}

bool srs_string_ends_with(string str, string flag0, string flag1, string flag2)
{
    return srs_string_ends_with(str, flag0) || srs_string_ends_with(str, flag1) || srs_string_ends_with(str, flag2);
}

bool srs_string_ends_with(string str, string flag0, string flag1, string flag2, string flag3)
{
    return srs_string_ends_with(str, flag0) || srs_string_ends_with(str, flag1) || srs_string_ends_with(str, flag2) || srs_string_ends_with(str, flag3);
}

bool srs_string_starts_with(string str, string flag)
{
    return str.find(flag) == 0;
}

bool srs_string_starts_with(string str, string flag0, string flag1)
{
    return srs_string_starts_with(str, flag0) || srs_string_starts_with(str, flag1);
}

bool srs_string_starts_with(string str, string flag0, string flag1, string flag2)
{
    return srs_string_starts_with(str, flag0, flag1) || srs_string_starts_with(str, flag2);
}

bool srs_string_starts_with(string str, string flag0, string flag1, string flag2, string flag3)
{
    return srs_string_starts_with(str, flag0, flag1, flag2) || srs_string_starts_with(str, flag3);
}

bool srs_string_contains(string str, string flag)
{
    return str.find(flag) != string::npos;
}

bool srs_string_contains(string str, string flag0, string flag1)
{
    return str.find(flag0) != string::npos || str.find(flag1) != string::npos;
}

bool srs_string_contains(string str, string flag0, string flag1, string flag2)
{
    return str.find(flag0) != string::npos || str.find(flag1) != string::npos || str.find(flag2) != string::npos;
}

int srs_string_count(string str, string flag)
{
    int nn = 0;
    for (int i = 0; i < (int)flag.length(); i++) {
        char ch = flag.at(i);
        nn += std::count(str.begin(), str.end(), ch);
    }
    return nn;
}


vector<string> srs_string_split(string s, string seperator)
{
    vector<string> result;
    if(seperator.empty()){
        result.push_back(s);
        return result;
    }
    
    size_t posBegin = 0;
    size_t posSeperator = s.find(seperator);
    while (posSeperator != string::npos) {
        result.push_back(s.substr(posBegin, posSeperator - posBegin));
        posBegin = posSeperator + seperator.length(); // next byte of seperator
        posSeperator = s.find(seperator, posBegin);
    }
    // push the last element
    result.push_back(s.substr(posBegin));
    return result;
}

string srs_string_min_match(string str, vector<string> seperators)
{
    string match;
    
    if (seperators.empty()) {
        return str;
    }
    
    size_t min_pos = string::npos;
    for (vector<string>::iterator it = seperators.begin(); it != seperators.end(); ++it) {
        string seperator = *it;
        
        size_t pos = str.find(seperator);
        if (pos == string::npos) {
            continue;
        }
        
        if (min_pos == string::npos || pos < min_pos) {
            min_pos = pos;
            match = seperator;
        }
    }
    
    return match;
}

vector<string> srs_string_split(string str, vector<string> seperators)
{
    vector<string> arr;
    
    size_t pos = string::npos;
    string s = str;
    
    while (true) {
        string seperator = srs_string_min_match(s, seperators);
        if (seperator.empty()) {
            break;
        }
        
        if ((pos = s.find(seperator)) == string::npos) {
            break;
        }

        arr.push_back(s.substr(0, pos));
        s = s.substr(pos + seperator.length());
    }
    
    if (!s.empty()) {
        arr.push_back(s);
    }
    
    return arr;
}

int srs_do_create_dir_recursively(string dir)
{
    int ret = ERROR_SUCCESS;
    
    // stat current dir, if exists, return error.
    if (srs_path_exists(dir)) {
        return ERROR_SYSTEM_DIR_EXISTS;
    }
    
    // create parent first.
    size_t pos;
    if ((pos = dir.rfind("/")) != std::string::npos) {
        std::string parent = dir.substr(0, pos);
        ret = srs_do_create_dir_recursively(parent);
        // return for error.
        if (ret != ERROR_SUCCESS && ret != ERROR_SYSTEM_DIR_EXISTS) {
            return ret;
        }
        // parent exists, set to ok.
        ret = ERROR_SUCCESS;
    }
    
    // create curren dir.
    mode_t mode = S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IXOTH;
    if (::mkdir(dir.c_str(), mode) < 0) {
        if (errno == EEXIST) {
            return ERROR_SYSTEM_DIR_EXISTS;
        }
        
        ret = ERROR_SYSTEM_CREATE_DIR;
        srs_error("create dir %s failed. ret=%d", dir.c_str(), ret);
        return ret;
    }
    
    srs_info("create dir %s success.", dir.c_str());
    
    return ret;
}
    
bool srs_bytes_equals(void* pa, void* pb, int size)
{
    uint8_t* a = (uint8_t*)pa;
    uint8_t* b = (uint8_t*)pb;
    
    if (!a && !b) {
        return true;
    }
    
    if (!a || !b) {
        return false;
    }
    
    for(int i = 0; i < size; i++){
        if(a[i] != b[i]){
            return false;
        }
    }
    
    return true;
}

srs_error_t srs_create_dir_recursively(string dir)
{
    int ret = srs_do_create_dir_recursively(dir);
    
    if (ret == ERROR_SYSTEM_DIR_EXISTS || ret == ERROR_SUCCESS) {
        return srs_success;
    }
    
    return srs_error_new(ret, "create dir %s", dir.c_str());
}

bool srs_path_exists(std::string path)
{
    struct stat st;
    
    // stat current dir, if exists, return error.
    if (stat(path.c_str(), &st) == 0) {
        return true;
    }
    
    return false;
}

string srs_path_dirname(string path)
{
    std::string dirname = path;

    // No slash, it must be current dir.
    size_t pos = string::npos;
    if ((pos = dirname.rfind("/")) == string::npos) {
        return "./";
    }

    // Path under root.
    if (pos == 0) {
        return "/";
    }

    // Fetch the directory.
    dirname = dirname.substr(0, pos);
    return dirname;
}

string srs_path_basename(string path)
{
    std::string dirname = path;
    size_t pos = string::npos;
    
    if ((pos = dirname.rfind("/")) != string::npos) {
        // the basename("/") is "/"
        if (dirname.length() == 1) {
            return dirname;
        }
        dirname = dirname.substr(pos + 1);
    }
    
    return dirname;
}

string srs_path_filename(string path)
{
    std::string filename = path;
    size_t pos = string::npos;
    
    if ((pos = filename.rfind(".")) != string::npos) {
        return filename.substr(0, pos);
    }
    
    return filename;
}

string srs_path_filext(string path)
{
    size_t pos = string::npos;
    
    if ((pos = path.rfind(".")) != string::npos) {
        return path.substr(pos);
    }
    
    return "";
}


// We use the standard encoding:
//      var StdEncoding = NewEncoding(encodeStd)
// StdEncoding is the standard base64 encoding, as defined in RFC 4648.
namespace {
    char padding = '=';
    string encoder = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

#define SPACE_CHARS " \t\r\n"

int av_toupper(int c)
{
    if (c >= 'a' && c <= 'z') {
        c ^= 0x20;
    }
    return c;
}
    
// fromHexChar converts a hex character into its value and a success flag.
uint8_t srs_from_hex_char(uint8_t c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

char* srs_data_to_hex(char* des, const u_int8_t* src, int len)
{
    if(src == NULL || len == 0 || des == NULL){
        return NULL;
    }

    const char *hex_table = "0123456789ABCDEF";
    
    for (int i=0; i<len; i++) {
        des[i * 2]     = hex_table[src[i] >> 4];
        des[i * 2 + 1] = hex_table[src[i] & 0x0F];
    }  

    return des;
}

char* srs_data_to_hex_lowercase(char* des, const u_int8_t* src, int len)
{
    if(src == NULL || len == 0 || des == NULL){
        return NULL;
    }

    const char *hex_table = "0123456789abcdef";

    for (int i=0; i<len; i++) {
        des[i * 2]     = hex_table[src[i] >> 4];
        des[i * 2 + 1] = hex_table[src[i] & 0x0F];
    }

    return des;
}

int srs_hex_to_data(uint8_t* data, const char* p, int size)
{
    if (size <= 0 || (size%2) == 1) {
        return -1;
    }
    
    for (int i = 0; i < (int)size / 2; i++) {
        uint8_t a = srs_from_hex_char(p[i*2]);
        if (a == (uint8_t)-1) {
            return -1;
        }
        
        uint8_t b = srs_from_hex_char(p[i*2 + 1]);
        if (b == (uint8_t)-1) {
            return -1;
        }
        
        data[i] = (a << 4) | b;
    }
    
    return size / 2;
}


std::string srs_random_str(int len)
{
    static string random_table = "01234567890123456789012345678901234567890123456789abcdefghijklmnopqrstuvwxyz";

    string ret;
    ret.reserve(len);
    for (int i = 0; i < len; ++i) {
        ret.append(1, random_table[srs_random() % random_table.size()]);
    }

    return ret;
}

long srs_random()
{
    static bool _random_initialized = false;
    if (!_random_initialized) {
        _random_initialized = true;
        ::srandom((unsigned long)(srs_update_system_time() | (::getpid()<<13)));
    }

    return random();
}