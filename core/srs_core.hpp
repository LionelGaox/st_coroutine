#pragma once

// The time unit for timeout, interval or duration.
#include <srs_core_time.hpp>

#include <assert.h>
#define srs_assert(expression) assert(expression)

// To free the p and set to NULL.
// @remark The p must be a pointer T*.
#define srs_freep(p) \
    if (p) { \
        delete p; \
        p = NULL; \
    } \
    (void)0
// Please use the freepa(T[]) to free an array, otherwise the behavior is undefined.
#define srs_freepa(pa) \
    if (pa) { \
        delete[] pa; \
        pa = NULL; \
    } \
    (void)0

// Error predefined for all modules.
class SrsCplxError;
typedef SrsCplxError* srs_error_t;

#include <string>
// 上下文ID，就是个字符串
// The context ID, it default to a string object, we can also use other objects.
// @remark User can directly user string as SrsContextId, we user struct to ensure the context is an object.
class _SrsContextId
{
private:
    std::string v_;
public:
    _SrsContextId();
    _SrsContextId(const _SrsContextId& cp);
    _SrsContextId& operator=(const _SrsContextId& cp);
    virtual ~_SrsContextId();
public:
    const char* c_str() const;
    bool empty() const;
    // Compare the two context id. @see http://www.cplusplus.com/reference/string/string/compare/
    //      0	They compare equal
    //      <0	Either the value of the first character that does not match is lower in the compared string, or all compared characters match but the compared string is shorter.
    //      >0	Either the value of the first character that does not match is greater in the compared string, or all compared characters match but the compared string is longer.
    int compare(const _SrsContextId& to) const;
    // Set the value of context id.
    _SrsContextId& set_value(const std::string& v);
};
typedef _SrsContextId SrsContextId;

#define SRS_CONSTS_NULL_FILE "/dev/null"
#define SRS_CONSTS_LOCALHOST "127.0.0.1"
#define SRS_CONSTS_LOOPBACK "0.0.0.0"
#define SRS_CONSTS_LOOPBACK6 "::"
