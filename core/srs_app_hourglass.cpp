//
// Copyright (c) 2013-2021 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#include <srs_app_hourglass.hpp>

#include <algorithm>
using namespace std;

#include <srs_kernel_error.hpp>
#include <srs_kernel_log.hpp>
#include <srs_kernel_utility.hpp>

ISrsHourGlass::ISrsHourGlass()
{
}

ISrsHourGlass::~ISrsHourGlass()
{
}

SrsHourGlass::SrsHourGlass(string label, ISrsHourGlass* h, srs_utime_t resolution)
{
    label_ = label;
    handler = h;
    _resolution = resolution;
    total_elapse = 0;
    trd = new SrsSTCoroutine("timer-" + label, this, _srs_context->get_id());
}

SrsHourGlass::~SrsHourGlass()
{
    srs_freep(trd);
}

srs_error_t SrsHourGlass::start()
{
    srs_error_t err = srs_success;

    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start timer");
    }

    return err;
}

void SrsHourGlass::stop()
{
    trd->stop();
}

srs_error_t SrsHourGlass::tick(srs_utime_t interval)
{
    return tick(0, interval);
}

srs_error_t SrsHourGlass::tick(int event, srs_utime_t interval)
{
    srs_error_t err = srs_success;
    
    if (_resolution > 0 && (interval % _resolution) != 0) {
        return srs_error_new(ERROR_SYSTEM_HOURGLASS_RESOLUTION,
            "invalid interval=%dms, resolution=%dms", srsu2msi(interval), srsu2msi(_resolution));
    }
    
    ticks[event] = interval;
    
    return err;
}

void SrsHourGlass::untick(int event)
{
    map<int, srs_utime_t>::iterator it = ticks.find(event);
    if (it != ticks.end()) {
        ticks.erase(it);
    }
}

srs_error_t SrsHourGlass::cycle()
{
    srs_error_t err = srs_success;

    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "quit");
        }
    
        map<int, srs_utime_t>::iterator it;
        for (it = ticks.begin(); it != ticks.end(); ++it) {
            int event = it->first;
            srs_utime_t interval = it->second;

            if (interval == 0 || (total_elapse % interval) == 0) {

                if ((err = handler->notify(event, interval, total_elapse)) != srs_success) {
                    return srs_error_wrap(err, "notify");
                }
            }
        }

        // TODO: FIXME: Maybe we should use wallclock.
        total_elapse += _resolution;
        srs_usleep(_resolution);
    }
    
    return err;
}

ISrsDynamicTimer::ISrsDynamicTimer()
{
}

ISrsDynamicTimer::~ISrsDynamicTimer()
{
}

SrsDynamicTimer::SrsDynamicTimer(string label, ISrsDynamicTimer* h, srs_utime_t resolution)
{
    label_ = label;
    handler = h;
    _resolution = resolution;
    trd = new SrsSTCoroutine("timer-" + label, this, _srs_context->get_id());
}

SrsDynamicTimer::~SrsDynamicTimer()
{
    srs_freep(trd);
}

srs_error_t SrsDynamicTimer::start()
{
    srs_error_t err = srs_success;

    if ((err = trd->start()) != srs_success) {
        return srs_error_wrap(err, "start timer");
    }

    return err;
}

void SrsDynamicTimer::stop()
{
    trd->stop();
}

void SrsDynamicTimer::tick(int event, srs_utime_t expired_time)
{
    ticks[event] = expired_time;
}

void SrsDynamicTimer::untick(int event)
{
    map<int, srs_utime_t>::iterator it = ticks.find(event);
    if (it != ticks.end()) {
        ticks.erase(it);
    }
}

srs_error_t SrsDynamicTimer::cycle()
{
    srs_error_t err = srs_success;

    while (true) {
        if ((err = trd->pull()) != srs_success) {
            return srs_error_wrap(err, "quit");
        }

        srs_utime_t now_time = srs_update_system_time();
    
        map<int, srs_utime_t>::iterator it;
        for (it = ticks.begin(); it != ticks.end(); ++it) {
            int event = it->first;
            srs_utime_t expired_time = it->second;

            if (expired_time > 0 && now_time >= expired_time) {
                // Timeout, and mark it never expired, unless tick it again.
                it->second = -1;

                if ((err = handler->notify(event, now_time)) != srs_success) {
                    return srs_error_wrap(err, "notify");
                }
            }
        }

        srs_usleep(_resolution);
    }
    
    return err;
}

ISrsFastTimer::ISrsFastTimer()
{
}

ISrsFastTimer::~ISrsFastTimer()
{
}

SrsFastTimer::SrsFastTimer(std::string label, srs_utime_t interval)
{
    interval_ = interval;
    trd_ = new SrsSTCoroutine(label, this, _srs_context->get_id());
}

SrsFastTimer::~SrsFastTimer()
{
    srs_freep(trd_);
}

srs_error_t SrsFastTimer::start()
{
    srs_error_t err = srs_success;

    if ((err = trd_->start()) != srs_success) {
        return srs_error_wrap(err, "start timer");
    }

    return err;
}

void SrsFastTimer::subscribe(ISrsFastTimer* timer)
{
    if (std::find(handlers_.begin(), handlers_.end(), timer) == handlers_.end()) {
        handlers_.push_back(timer);
    }
}

void SrsFastTimer::unsubscribe(ISrsFastTimer* timer)
{
    vector<ISrsFastTimer*>::iterator it = std::find(handlers_.begin(), handlers_.end(), timer);
    if (it != handlers_.end()) {
        handlers_.erase(it);
    }
}

srs_error_t SrsFastTimer::cycle()
{
    srs_error_t err = srs_success;

    while (true) {
        if ((err = trd_->pull()) != srs_success) {
            return srs_error_wrap(err, "quit");
        }

        for (int i = 0; i < (int)handlers_.size(); i++) {
            ISrsFastTimer* timer = handlers_.at(i);

            if ((err = timer->on_timer(interval_)) != srs_success) {
                srs_freep(err); // Ignore any error for shared timer.
            }
        }
        srs_usleep(interval_);
    }

    return err;
}

