//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_APP_HOURGLASS_HPP
#define SRS_APP_HOURGLASS_HPP

#include <srs_core.hpp>

#include <srs_app_st.hpp>

#include <map>
#include <string>
#include <vector>

class SrsCoroutine;

// The handler for the tick.
class ISrsHourGlass
{
public:
    ISrsHourGlass();
    virtual ~ISrsHourGlass();
public:
    // When time is ticked, this function is called.
    virtual srs_error_t notify(int event, srs_utime_t interval, srs_utime_t tick) = 0;
};

// The hourglass(timer or SrsTimer) for special tasks,
// while these tasks are attached to some intervals, for example,
// there are N=3 tasks bellow:
//          1. A heartbeat every 3s.
//          2. A print message every 5s.
//          3. A notify backend every 7s.
// The hourglass will call back when ticks:
//          1. Got notify(event=1, time=3)
//          2. Got notify(event=2, time=5)
//          3. Got notify(event=1, time=6)
//          4. Got notify(event=3, time=7)
//          5. Got notify(event=1, time=9)
//          6. Got notify(event=2, time=10)
// It's a complex and high-performance timer.
//
// Usage:
//      SrsHourGlass* hg = new SrsHourGlass("nack", handler, 100 * SRS_UTIME_MILLISECONDS);
//
//      hg->tick(1, 300 * SRS_UTIME_MILLISECONDS);
//      hg->tick(2, 500 * SRS_UTIME_MILLISECONDS);
//      hg->tick(3, 700 * SRS_UTIME_MILLISECONDS);
//
//      // The hg will create a thread for timer.
//      hg->start();
class SrsHourGlass : public ISrsCoroutineHandler
{
private:
    std::string label_;
    SrsCoroutine* trd;
    ISrsHourGlass* handler;
    srs_utime_t _resolution;
    // The ticks:
    //      key: the event of tick.
    //      value: the interval of tick.
    std::map<int, srs_utime_t> ticks;
    // The total elapsed time,
    // for each cycle, we increase it with a resolution.
    srs_utime_t total_elapse;
public:
    // TODO: FIMXE: Refine to SrsHourGlass(std::string label);
    SrsHourGlass(std::string label, ISrsHourGlass* h, srs_utime_t resolution);
    virtual ~SrsHourGlass();
public:
    // Start or stop the hourglass.
    virtual srs_error_t start();
    virtual void stop();
public:
    // TODO: FIXME: Refine to tick with handler. Remove the tick(interval).
    // Add a pair of tick(event, interval).
    // @param event the event of tick, default is 0.
    // @param interval the interval in srs_utime_t of tick.
    virtual srs_error_t tick(srs_utime_t interval);
    virtual srs_error_t tick(int event, srs_utime_t interval);
    // Remove the tick by event.
    void untick(int event);
public:
    // Cycle the hourglass, which will sleep resolution every time.
    // and call handler when ticked.
    virtual srs_error_t cycle();
};

// The handler for the dynamic timer.
class ISrsDynamicTimer
{
public:
    ISrsDynamicTimer();
    virtual ~ISrsDynamicTimer();
public:
    // When time is ticked, this function is called.
    virtual srs_error_t notify(int event, srs_utime_t now_time) = 0;
};

// Dynamic Timer, similar with HourGlass, but the interval is dynamic
class SrsDynamicTimer : public ISrsCoroutineHandler
{
private:
    std::string label_;
    SrsCoroutine* trd;
    ISrsDynamicTimer* handler;
    srs_utime_t _resolution;
    // The ticks, key: event id, value: expired_time
    std::map<int, srs_utime_t> ticks;
public:
    SrsDynamicTimer(std::string label, ISrsDynamicTimer* h, srs_utime_t resolution);
    virtual ~SrsDynamicTimer();
public:
    virtual srs_error_t start();
    virtual void stop();
public:
    // insert or update expired_time of event.
    void tick(int event, srs_utime_t expired_time);
    // remove event.
    void untick(int event);
public:
    virtual srs_error_t cycle();
};

// The handler for fast timer.
class ISrsFastTimer
{
public:
    ISrsFastTimer();
    virtual ~ISrsFastTimer();
public:
    // Tick when timer is active.
    virtual srs_error_t on_timer(srs_utime_t interval) = 0;
};

// The fast timer, shared by objects, for high performance.
// For example, we should never start a timer for each connection or publisher or player,
// instead, we should start only one fast timer in server.
class SrsFastTimer : public ISrsCoroutineHandler
{
private:
    SrsCoroutine* trd_;
    srs_utime_t interval_;
    std::vector<ISrsFastTimer*> handlers_;
public:
    SrsFastTimer(std::string label, srs_utime_t interval);
    virtual ~SrsFastTimer();
public:
    srs_error_t start();
    void set_stack_size(int size){ ((SrsSTCoroutine*)trd_)->set_stack_size(size); }
public:
    void subscribe(ISrsFastTimer* timer);
    void unsubscribe(ISrsFastTimer* timer);
// Interface ISrsCoroutineHandler
private:
    // Cycle the hourglass, which will sleep resolution every time.
    // and call handler when ticked.
    virtual srs_error_t cycle();
};


#endif
