// thread_pool.cpp
/*
 *  Copyright (c) 2007 Leigh Johnston.
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of Leigh Johnston nor the names of any
 *       other contributors to this software may be used to endorse or
 *       promote products derived from this software without specific prior
 *       written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <neolib/neolib.hpp>
#include <condition_variable>
#include <neolib/core/scoped.hpp>
#include <neolib/core/lifetime.hpp>
#include <neolib/task/thread.hpp>
#include <neolib/task/thread_pool.hpp>

namespace neolib
{
    class thread_pool_thread : public thread
    {
    public:
        typedef std::shared_ptr<i_task> task_pointer;
        typedef std::pair<task_pointer, int32_t> task_queue_entry;
        typedef std::deque<task_queue_entry> task_queue;
    public:
        struct no_active_task : std::logic_error { no_active_task() : std::logic_error("neolib::thread_pool_thread::no_active_task") {} };
        struct already_active : std::logic_error { already_active() : std::logic_error("neolib::thread_pool_thread::already_active") {} };
    public:
        thread_pool_thread(thread_pool& aThreadPool) : thread{ "neolib::thread_pool_thread" }, iThreadPool{ aThreadPool }, iPoolMutex{ aThreadPool.mutex() }, iStopped{ false }
        {
            start();
        }
        ~thread_pool_thread()
        {
        }
    public:
        virtual void exec(yield_type aYieldType = yield_type::NoYield)
        {
            while (!finished())
            {
                std::unique_lock<std::mutex> lk(iCondVarMutex);
                iConditionVariable.wait(lk, [this] { return iActiveTask != nullptr || iStopped; });
                lk.unlock();
                if (iStopped)
                    return;
                if (!iActiveTask->cancelled())
                    iActiveTask->run(aYieldType);
                std::scoped_lock<std::recursive_mutex> lk2(iPoolMutex);
                release();
                next_task();
            }
        }
    public:
        bool active() const
        {
            std::scoped_lock<std::mutex> lk(iCondVarMutex);
            return iActiveTask != nullptr;
        }
        bool idle() const
        {
            std::scoped_lock<std::recursive_mutex> lk(iPoolMutex);
            std::scoped_lock<std::mutex> lk2(iCondVarMutex);
            return iActiveTask == nullptr && iWaitingTasks.empty();
        }
        void add(task_pointer aTask, int32_t aPriority)
        {
            std::scoped_lock<std::recursive_mutex> lk(iPoolMutex);
            auto where = std::upper_bound(iWaitingTasks.begin(), iWaitingTasks.end(), task_queue_entry{ task_pointer{}, aPriority },
                [](const task_queue_entry& aLeft, const task_queue_entry& aRight)
            {
                return aLeft.second > aRight.second;
            });
            iWaitingTasks.emplace(where, aTask, aPriority);
            if (!active())
                next_task();
        }
        bool steal_work(thread_pool_thread& aIdleThread)
        {
            std::unique_lock<std::recursive_mutex> lk(iPoolMutex);
            if (!iWaitingTasks.empty())
            {
                auto newTask = iWaitingTasks.front();
                iWaitingTasks.pop_front();
                aIdleThread.add(newTask.first, newTask.second);
                return true;
            }
            return false;
        }
        void stop()
        {
            if (!iStopped)
            {
                {
                    std::scoped_lock<std::mutex> lk2(iCondVarMutex);
                    iStopped = true;
                }
                iConditionVariable.notify_one();
                wait();
            }
        }
    private:
        void next_task()
        {
            std::unique_lock<std::recursive_mutex> lk(iPoolMutex);
            if (active())
                throw already_active();
            if (iWaitingTasks.empty())
                iThreadPool.steal_work(*this);
            if (!iWaitingTasks.empty())
            {
                {
                    std::scoped_lock<std::mutex> lk2(iCondVarMutex);
                    iActiveTask = iWaitingTasks.front().first;
                    iWaitingTasks.pop_front();
                }
                iConditionVariable.notify_one();
                iThreadPool.thread_gone_busy();
            }
            else
                iThreadPool.thread_gone_idle();
        }
        void release()
        {
            task_pointer currentTask;
            {
                std::scoped_lock<std::mutex> lk(iCondVarMutex);
                if (iActiveTask == nullptr)
                    throw no_active_task();
                currentTask = iActiveTask;
                iActiveTask = nullptr;
            }
        }
    private:
        thread_pool& iThreadPool;
        std::recursive_mutex& iPoolMutex;
        mutable std::mutex iCondVarMutex;
        std::condition_variable iConditionVariable;
        task_queue iWaitingTasks;
        task_pointer iActiveTask;
        std::atomic<bool> iStopped;
    };

    thread_pool::thread_pool() : iIdle{ true }, iStopped { false }, iMaxThreads{ 0 }
    {
        reserve(std::thread::hardware_concurrency());
    }

    thread_pool::~thread_pool()
    {
        wait();
        for (auto& t : iThreads)
            static_cast<thread_pool_thread&>(*t).stop();
    }

    void thread_pool::reserve(std::size_t aMaxThreads)
    {
        std::scoped_lock<std::recursive_mutex> lk(iMutex);
        iMaxThreads = aMaxThreads;
        while (iThreads.size() < iMaxThreads)
            iThreads.push_back(std::make_unique<thread_pool_thread>(*this));
    }

    std::size_t thread_pool::active_threads() const
    {
        std::scoped_lock<std::recursive_mutex> lk(iMutex);
        std::size_t result = 0;
        for (auto& t : iThreads)
            if (static_cast<thread_pool_thread&>(*t).active())
                ++result;
        return result;
    }

    std::size_t thread_pool::available_threads() const
    {
        std::scoped_lock<std::recursive_mutex> lk(iMutex);
        return max_threads() - active_threads();
    }

    std::size_t thread_pool::total_threads() const
    {
        std::scoped_lock<std::recursive_mutex> lk(iMutex);
        std::size_t result = 0;
        for (auto& t : iThreads)
            if (!static_cast<thread_pool_thread&>(*t).finished())
                ++result;
        return result;
    }

    std::size_t thread_pool::max_threads() const
    {
        return iMaxThreads;
    }

    void thread_pool::start(i_task& aTask, int32_t aPriority)
    {
        start(task_pointer{ task_pointer{}, &aTask }, aPriority);
    }

    void thread_pool::start(task_pointer aTask, int32_t aPriority)
    {
        if (stopped())
            return;
        std::scoped_lock<std::recursive_mutex> lk(iMutex);
        if (iThreads.empty())
            throw no_threads();
        for (auto& t : iThreads)
        {
            auto& tpt = static_cast<thread_pool_thread&>(*t);
            if (!tpt.active())
            {
                tpt.add(aTask, aPriority);
                return;
            }
        }
        static_cast<thread_pool_thread&>(*iThreads[0]).add(aTask, aPriority);
    }

    bool thread_pool::try_start(i_task& aTask, int32_t aPriority)
    {
        if (stopped())
            return false;
        if (available_threads() == 0)
            return false;
        start(aTask, aPriority);
        return true;
    }

    bool thread_pool::try_start(task_pointer aTask, int32_t aPriority)
    {
        if (stopped())
            return false;
        if (available_threads() == 0)
            return false;
        start(aTask, aPriority);
        return true;
    }

    std::pair<std::future<void>, thread_pool::task_pointer> thread_pool::run(std::function<void()> aFunction, int32_t aPriority)
    {
        if (stopped())
            return {};
        auto newTask = std::make_shared<function_task<void>>(aFunction);
        start(newTask, aPriority);
        return std::make_pair(newTask->get_future(), newTask);
    }

    bool thread_pool::idle() const
    {
        return iIdle;
    }

    void thread_pool::update_idle()
    {
        std::scoped_lock<std::recursive_mutex> lk1(iMutex);
        std::optional<std::unique_lock<std::mutex>> lk2;
        for (auto& t : iThreads)
        {
            if (!static_cast<thread_pool_thread&>(*t).idle())
            {
                lk2.emplace(iWaitMutex);
                iIdle = false;
                return;
            }
        }
        lk2.emplace(iWaitMutex);
        iIdle = true;
    }

    bool thread_pool::busy() const
    {
        return !idle();
    }

    void thread_pool::wait() const
    {
        if (stopped() || idle())
            return;
        std::unique_lock<std::mutex> lk(iWaitMutex);
        iWaitConditionVariable.wait(lk, [this] { return stopped() || idle(); });
    }

    bool thread_pool::stopped() const
    {
        return iStopped;
    }

    void thread_pool::stop()
    {
        if (!stopped())
        {
            for (auto& t : iThreads)
            {
                static_cast<thread_pool_thread&>(*t).stop();
            }
            {
                std::unique_lock<std::mutex> lk(iWaitMutex);
                iStopped = true;
            }
            iWaitConditionVariable.notify_one();
        }
    }

    thread_pool& thread_pool::default_thread_pool()
    {
        static thread_pool sDefaultThreadPool;
        return sDefaultThreadPool;
    }

    std::recursive_mutex& thread_pool::mutex() const
    {
        return iMutex;
    }

    void thread_pool::steal_work(thread_pool_thread& aIdleThread)
    {
        std::scoped_lock<std::recursive_mutex> lk(iMutex);
        if (iThreads.empty())
            throw no_threads();
        for (auto& t : iThreads)
        {
            if (&*t == &aIdleThread)
                continue;
            auto& tpt = static_cast<thread_pool_thread&>(*t);
            if (tpt.steal_work(aIdleThread))
                return;
        }
    }

    void thread_pool::thread_gone_idle()
    {
        update_idle();
        iWaitConditionVariable.notify_one();
    }

    void thread_pool::thread_gone_busy()
    {
        update_idle();
    }
}
