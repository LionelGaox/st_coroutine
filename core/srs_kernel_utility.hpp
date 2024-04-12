//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_KERNEL_UTILITY_HPP
#define SRS_KERNEL_UTILITY_HPP

#include <srs_core.hpp>

#include <string>
#include <vector>


class SrsBuffer;
class SrsBitBuffer;

// Basic compare function.
#define srs_min(a, b) (((a) < (b))? (a) : (b))
#define srs_max(a, b) (((a) < (b))? (b) : (a))

// Get current system time in srs_utime_t, use cache to avoid performance problem
extern srs_utime_t srs_get_system_time();
extern srs_utime_t srs_get_system_startup_time();
// A daemon st-thread updates it.
extern srs_utime_t srs_update_system_time();

// The "ANY" address to listen, it's "0.0.0.0" for ipv4, and "::" for ipv6.
// @remark We prefer ipv4, only use ipv6 if ipv4 is disabled.
extern std::string srs_any_address_for_listener();

// The dns resolve utility, return the resolved ip address.
extern std::string srs_dns_resolve(std::string host, int& family);

// Split the host:port to host and port.
// @remark the hostport format in <host[:port]>, where port is optional.
extern void srs_parse_hostport(std::string hostport, std::string& host, int& port);

// Parse the endpoint to ip and port.
// @remark The hostport format in <[ip:]port>, where ip is default to "0.0.0.0".
extern void srs_parse_endpoint(std::string hostport, std::string& ip, int& port);

// Check whether the ip is valid.
extern bool srs_check_ip_addr_valid(std::string ip);

// Parse the int64 value to string.
extern std::string srs_int2str(int64_t value);
// Parse the float value to string, precise is 2.
extern std::string srs_float2str(double value);
// Convert bool to switch value, true to "on", false to "off".
extern std::string srs_bool2switch(bool v);

// Whether system is little endian
extern bool srs_is_little_endian();

// Replace old_str to new_str of str
extern std::string srs_string_replace(std::string str, std::string old_str, std::string new_str);
// Trim char in trim_chars of str
extern std::string srs_string_trim_end(std::string str, std::string trim_chars);
// Trim char in trim_chars of str
extern std::string srs_string_trim_start(std::string str, std::string trim_chars);
// Remove char in remove_chars of str
extern std::string srs_string_remove(std::string str, std::string remove_chars);
// Remove first substring from str
extern std::string srs_erase_first_substr(std::string str, std::string erase_string);
// Remove last substring from str
extern std::string srs_erase_last_substr(std::string str, std::string erase_string);
// Whether string end with
extern bool srs_string_ends_with(std::string str, std::string flag);
extern bool srs_string_ends_with(std::string str, std::string flag0, std::string flag1);
extern bool srs_string_ends_with(std::string str, std::string flag0, std::string flag1, std::string flag2);
extern bool srs_string_ends_with(std::string str, std::string flag0, std::string flag1, std::string flag2, std::string flag3);
// Whether string starts with
extern bool srs_string_starts_with(std::string str, std::string flag);
extern bool srs_string_starts_with(std::string str, std::string flag0, std::string flag1);
extern bool srs_string_starts_with(std::string str, std::string flag0, std::string flag1, std::string flag2);
extern bool srs_string_starts_with(std::string str, std::string flag0, std::string flag1, std::string flag2, std::string flag3);
// Whether string contains with
extern bool srs_string_contains(std::string str, std::string flag);
extern bool srs_string_contains(std::string str, std::string flag0, std::string flag1);
extern bool srs_string_contains(std::string str, std::string flag0, std::string flag1, std::string flag2);
// Count each char of flag in string
extern int srs_string_count(std::string str, std::string flag);
// Find the min match in str for flags.
extern std::string srs_string_min_match(std::string str, std::vector<std::string> flags);
// Split the string by seperator to array.
extern std::vector<std::string> srs_string_split(std::string s, std::string seperator);
extern std::vector<std::string> srs_string_split(std::string s, std::vector<std::string> seperators);

// Compare the memory in bytes.
// @return true if completely equal; otherwise, false.
extern bool srs_bytes_equals(void* pa, void* pb, int size);

// Create dir recursively
extern srs_error_t srs_create_dir_recursively(std::string dir);

// Whether path exists.
extern bool srs_path_exists(std::string path);
// Get the dirname of path, for instance, dirname("/live/livestream")="/live"
extern std::string srs_path_dirname(std::string path);
// Get the basename of path, for instance, basename("/live/livestream")="livestream"
extern std::string srs_path_basename(std::string path);
// Get the filename of path, for instance, filename("livestream.flv")="livestream"
extern std::string srs_path_filename(std::string path);
// Get the file extension of path, for instance, filext("live.flv")=".flv"
extern std::string srs_path_filext(std::string path);

// Calculate the output size needed to base64-encode x bytes to a null-terminated string.
#define SRS_AV_BASE64_SIZE(x) (((x)+2) / 3 * 4 + 1)

// Covert hex string to uint8 data, for example:
//      srs_hex_to_data(data, string("139056E5A0"))
//      which outputs the data in hex {0x13, 0x90, 0x56, 0xe5, 0xa0}.
extern int srs_hex_to_data(uint8_t* data, const char* p, int size);

// Convert data string to hex, for example:
//      srs_data_to_hex(des, {0xf3, 0x3f}, 2)
//      which outputs the des is string("F33F").
extern char* srs_data_to_hex(char* des, const uint8_t* src, int len);
// Output in lowercase, such as string("f33f").
extern char* srs_data_to_hex_lowercase(char* des, const uint8_t* src, int len);

// Generate random string [0-9a-z] in size of len bytes.
extern std::string srs_random_str(int len);

// Generate random value, use srandom(now_us) to init seed if not initialized.
extern long srs_random();

// For utest to mock it.
#include <sys/time.h>
#ifdef SRS_OSX
    #define _srs_gettimeofday gettimeofday
#else
    typedef int (*srs_gettimeofday_t) (struct timeval* tv, struct timezone* tz);
#endif

#endif

