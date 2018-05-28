//==============================================================================
// TaskXn.h - Native Task
//
// Author        :
// Version       : 2.0.01 (Temmuz 2018)
// Compatibility : POSIX, GCC
//==============================================================================

#include <iostream>
#include <pthread.h>
#include "TaskXn.h"


TaskXn::TaskXn(std::string name,int priority)
    : mName(name)
    , mOverruns(0)
    , mIsPeriodic(false)
    , mWishToRun(true)
{
    runTask(priority);
}

TaskXn::TaskXn(std::string name,
       std::chrono::steady_clock::duration period,
       int priority)
    : mName(name)
    , mPeriod(period)
    , mOverruns(0)
    , mIsPeriodic(true)
    , mWishToRun(true)
{
    runTask(priority);
}

void TaskXn::runTask(int priority)
{
    mTask = std::thread(&TaskXn::taskFunction, this);
    // Give RT priority to task
    sched_param sch;
    sch.__sched_priority = priority;
    if(pthread_setschedparam(mTask.native_handle(), SCHED_FIFO, &sch) == -1){
        mWishToRun = false;
        mTask.join();
        //throw ZnmException(mName, "TaskXn::TaskXn,"
        //                          " pthread_setschedparam", errno );
    }
}

TaskXn::~TaskXn()
{
    mWishToRun = false;
    if(mTask.joinable())
        mTask.join();

}

unsigned TaskXn::overruns()
{
    return mOverruns;
}

void TaskXn::join()
{
    mTask.join();
}

void TaskXn::detach()
{
    mTask.detach();
}

std::chrono::duration<double> TaskXn::elapsedTimeSec()
{
    return std::chrono::steady_clock::now() - mStartTime;
}

int TaskXn::maxPriority()
{
    return sched_get_priority_max(SCHED_FIFO);
}

int TaskXn::minPriority()
{
    return sched_get_priority_min(SCHED_FIFO);
}

void TaskXn::requestPeriodicTaskTermination()
{
    mWishToRun = false;
}

void TaskXn::taskFunction()
{
    mStartTime = std::chrono::steady_clock::now();
    if(mIsPeriodic){
        auto nextStartTime = mStartTime + mPeriod;
        while(mWishToRun){
            run();
            // detect overrun
            if((nextStartTime - std::chrono::steady_clock::now()).count() < 0){
                ++mOverruns;
            }
            std::this_thread::sleep_until(nextStartTime);
            nextStartTime += mPeriod;
        }
    }
    else{
        run();
    }
}

