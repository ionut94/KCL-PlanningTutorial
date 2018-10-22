/* -*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil -*- */

/*
 *  Main authors:
 *     Guido Tack <guido.tack@monash.edu>
 */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MINIZINC_TIMER_HH__
#define __MINIZINC_TIMER_HH__

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace MiniZinc {
  
  class Timer {
  protected:
#ifdef _WIN32
    LARGE_INTEGER time;
    LARGE_INTEGER freq;
#else
    timeval time;
#endif
  public:
    /// Construct timer
    Timer(void) {
#ifdef _WIN32
      QueryPerformanceFrequency(&freq);
      QueryPerformanceCounter(&time);
#else
      gettimeofday(&time, NULL);
#endif
    }
    /// Reset timer
    void reset(void) {
#ifdef _WIN32
      QueryPerformanceCounter(&time);
#else
      gettimeofday(&time, NULL);
#endif
    }
    /// Return milliseconds since timer was last reset
    double ms(void) const {
#ifdef _WIN32
      LARGE_INTEGER now;
      QueryPerformanceCounter(&now);
      return (static_cast<double>(now.QuadPart-time.QuadPart) / freq.QuadPart) * 1000.0;
#else
      timeval now;
      gettimeofday(&now, NULL);
      timeval diff;
      diff.tv_sec = now.tv_sec - time.tv_sec;
      diff.tv_usec = now.tv_usec - time.tv_usec;
      if (diff.tv_usec < 0) {
        diff.tv_sec--;
        diff.tv_usec += 1000000;
      }
      return static_cast<double>(diff.tv_sec) * 1000.0 + static_cast<double>(diff.tv_usec) / 1000.0;
#endif
    }
  };
  
}

#endif