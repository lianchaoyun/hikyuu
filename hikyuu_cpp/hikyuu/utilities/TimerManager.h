/*
 *  Copyright(C) 2021 hikyuu.org
 *
 *  Create on: 2021-01-08
 *     Author: fasiondog
 */

#pragma once

#include <forward_list>
#include "../datetime/Datetime.h"
#include "../Log.h"
#include "thread/ThreadPool.h"

namespace hku {

/**
 * 定时管理与调度
 * @ingroup Utilities
 */
class TimerManager {
public:
    TimerManager(const TimerManager&) = delete;
    TimerManager(TimerManager&) = delete;
    TimerManager(TimerManager&&) = delete;
    TimerManager& operator=(const TimerManager&) = delete;
    TimerManager& operator=(TimerManager&) = delete;
    TimerManager& operator=(TimerManager&&) = delete;

    /**
     * 构造函数，此时尚未启动运行，需调用 start 方法显示启动调度
     */
    TimerManager() : m_current_timer_id(-1), m_stop(true) {}

    /** 析构函数 */
    ~TimerManager() {
        stop();
        m_tg->stop();
    }

    /** 启动调度, 可在停止后重新启动 */
    void start() {
        if (m_stop) {
            m_stop = false;
            if (!m_tg) {
                m_tg = std::make_unique<ThreadPool>();
            }

            /*
             * 根据已有 timer 重建执行队列，并删除已无效的 timer
             */

            std::forward_list<int> invalid_timers;  // 记录已无效的 timer
            std::unique_lock<std::mutex> lock(m_mutex);
            for (auto iter = m_timers.begin(); iter != m_timers.end(); ++iter) {
                int time_id = iter->first;
                Timer* timer = iter->second;
                Datetime now = Datetime::now();

                // 记录已失效的 timer id
                if (timer->m_repeat_num <= 0 || (timer->m_end_date != Datetime::max() &&
                                                 timer->m_end_date + timer->m_end_time <= now)) {
                    invalid_timers.push_front(time_id);
                    continue;
                }

                IntervalS s;
                s.m_timer_id = time_id;
                s.m_time_point = now + timer->m_duration;
                if (timer->m_start_time != timer->m_end_time) {
                    Datetime point_date = s.m_time_point.startOfDay();
                    TimeDelta point = s.m_time_point - point_date;
                    if (point < timer->m_start_time) {
                        s.m_time_point = point_date + timer->m_start_time;
                    } else if (point > timer->m_end_time) {
                        s.m_time_point = point_date + timer->m_start_time + TimeDelta(1);
                    } else {
                        TimeDelta gap = point - timer->m_start_time;
                        if (gap % timer->m_duration != TimeDelta()) {
                            int x = int(gap / timer->m_duration) + 1;
                            s.m_time_point =
                              point_date + timer->m_start_time + timer->m_duration * double(x);
                        }
                    }
                }

                m_queue.push(s);
            }

            // 清除已无效的 timer
            for (auto id : invalid_timers) {
                removeTimer(id);
            }

            lock.unlock();
            m_cond.notify_all();

            std::thread([this]() { detectThread(); }).detach();
        }
    }

    /** 终止调度 */
    void stop() {
        if (!m_stop) {
            m_stop = true;
            std::unique_lock<std::mutex> lock(m_mutex);
            std::priority_queue<IntervalS> queue;
            m_queue.swap(queue);
            lock.unlock();
            m_cond.notify_all();
        }
    }

    /**
     * 增加计划任务, 添加失败时抛出异常
     * @tparam F 任务类型
     * @tparam Args 任务参数
     * @param start_date 允许运行的起始日期
     * @param end_date 允许运行的结束日期
     * @param start_time 允许运行的起始时间
     * @param end_time 允许运行的结束时间
     * @param repeat_num 重复次数，必须大于0，等于std::numeric_limits<int>::max()时表示无限循环
     * @param delay 间隔时间，需大于 TimeDelta(0)
     * @param f 待执行的延迟任务
     * @param args 任务具体参数
     */
    template <typename F, typename... Args>
    void addFunc(Datetime start_date, Datetime end_date, TimeDelta start_time, TimeDelta end_time,
                 int repeat_num, TimeDelta duration, F&& f, Args&&... args) {
        HKU_CHECK(!start_date.isNull(), "Invalid start_date!");
        HKU_CHECK(!end_date.isNull(), "Invalid end_date!");
        HKU_CHECK(end_date > start_date, "end_date({}) need > start_date({})!", end_date,
                  start_date);
        HKU_CHECK(start_time > TimeDelta(0) && start_time <= TimeDelta(0, 23, 59, 59, 999, 999),
                  "Invalid start_time: {}", start_time.repr());
        HKU_CHECK(end_time > TimeDelta(0) && end_time <= TimeDelta(0, 23, 59, 59, 999, 999),
                  "Invalid end_time: {}", end_time.repr());
        HKU_CHECK(end_time >= start_time, "end_time({}) need >= start_time({})!", end_time,
                  start_time);
        HKU_CHECK(repeat_num > 0, "Invalid repeat_num: {}", repeat_num);
        HKU_CHECK(duration > TimeDelta(0), "Invalid duration: {}", duration.repr());

        _addFunc(start_date, end_date, start_time, end_time, repeat_num, duration,
                 std::forward<F>(f), std::forward<Args>(args)...);
    }

    /**
     * 增加重复定时任务，添加失败时抛出异常
     * @tparam F 任务类型
     * @tparam Args 任务参数
     * @param repeat_num 重复次数，必须大于0，等于std::numeric_limits<int>::max()时表示无限循环
     * @param delay 间隔时间，需大于 TimeDelta(0)
     * @param f 待执行的延迟任务
     * @param args 任务具体参数
     * @return true 成功 | false 失败
     */
    template <typename F, typename... Args>
    void addDurationFunc(int repeat_num, TimeDelta duration, F&& f, Args&&... args) {
        HKU_CHECK(repeat_num > 0, "Invalid repeat_num: {}, must > 0", repeat_num);
        HKU_CHECK(duration > TimeDelta(), "Invalid duration: {}, must > TimeDelta(0)!",
                  duration.repr());
        _addFunc(Datetime::min(), Datetime::max(), TimeDelta(), TimeDelta(), repeat_num, duration,
                 std::forward<F>(f), std::forward<Args>(args)...);
    }

    /**
     * 增加延迟运行任务（只执行一次）, 添加失败时抛出异常
     * @tparam F 任务类型
     * @tparam Args 任务参数
     * @param delay 延迟时间，需大于 TimeDelta(0)
     * @param f 待执行的延迟任务
     * @param args 任务具体参数
     */
    template <typename F, typename... Args>
    void addDelayFunc(TimeDelta delay, F&& f, Args&&... args) {
        HKU_CHECK(delay > TimeDelta(), "Invalid delay: {}, must > TimeDelta(0)!");
        _addFunc(Datetime::min(), Datetime::max(), TimeDelta(), TimeDelta(), 1, delay,
                 std::forward<F>(f), std::forward<Args>(args)...);
    }

    /**
     * 在指定时刻执行任务（只执行一次）, 添加失败时抛出异常
     * @tparam F 任务类型
     * @tparam Args 任务参数
     * @param time_point 指定的运行时刻
     */
    template <typename F, typename... Args>
    void addFuncAtPoint(Datetime time_point, F&& f, Args&&... args) {
        HKU_CHECK(!time_point.isNull(), "Invalid time_point");
        TimeDelta delay(0, 0, 0, 0, 100);
        Datetime run_point = time_point - delay;
        Datetime date = run_point.startOfDay();
        TimeDelta time = run_point - date;
        _addFunc(date, Datetime::max(), time, TimeDelta(0, 23, 59, 59, 999, 999), 1, delay,
                 std::forward<F>(f), std::forward<Args>(args)...);
    }

private:
    void removeTimer(int id) {
        delete m_timers[id];
        m_timers.erase(id);
    }

    void detectThread() {
        while (!m_stop) {
            Datetime now = Datetime::now();
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_queue.empty()) {
                m_cond.wait(lock);
                continue;
            }

            IntervalS s = m_queue.top();
            TimeDelta diff = s.m_time_point - now;
            if (diff > TimeDelta()) {
                m_cond.wait_for(lock, std::chrono::duration<int64_t, std::micro>(diff.ticks()));
                continue;
            }

            m_queue.pop();
            auto timer_iter = m_timers.find(s.m_timer_id);
            if (timer_iter == m_timers.end()) {
                continue;
            }

            auto timer = timer_iter->second;
            m_tg->submit(timer->m_func);

            if (timer->m_repeat_num != std::numeric_limits<int>::max()) {
                timer->m_repeat_num--;
            }

            if (timer->m_repeat_num <= 0) {
                removeTimer(s.m_timer_id);
                continue;
            }

            s.m_time_point = s.m_time_point + timer->m_duration;
            if (timer->m_end_date != Datetime::max() &&
                s.m_time_point > timer->m_end_date + timer->m_end_time) {
                removeTimer(s.m_timer_id);
                continue;
            }

            Datetime today = now.startOfDay();
            if (timer->m_start_time != timer->m_end_time &&
                s.m_time_point > today + timer->m_end_time) {
                s.m_time_point = today + timer->m_start_time + TimeDelta(1);
            }

            m_queue.push(s);
        }
    }

    // 分配 timer_id
    int getNewTimerId() {
        int max_int = std::numeric_limits<int>::max();
        HKU_WARN_IF_RETURN(m_timers.size() >= max_int, -1, "Timer queue is full!");

        if (m_current_timer_id >= max_int) {
            m_current_timer_id = 0;
        } else {
            m_current_timer_id++;
        }

        while (true) {
            if (m_timers.find(m_current_timer_id) != m_timers.end()) {
                if (m_current_timer_id >= max_int) {
                    m_current_timer_id = 0;
                } else {
                    m_current_timer_id++;
                }
            } else {
                break;
            }
        }
        return m_current_timer_id;
    }

private:
    class Timer {
    public:
        void operator()() {
            m_func();
        }

        Datetime m_start_date = Datetime::min().startOfDay();  // 允许执行的起始日期（包含该日期）
        Datetime m_end_date = Datetime::max().startOfDay();  // 允许执行的终止日期（包含该日期）
        TimeDelta m_start_time;  // 允许执行的当日起始时间（包含该时间）
        TimeDelta m_end_time;    // 允许执行的当日结束时间（包含该时间）
        TimeDelta m_duration;    // 延迟时长或间隔时长
        int m_repeat_num = 1;    // 重复执行次数，max标识无限循环
        std::function<void()> m_func;
    };

    struct IntervalS {
        Datetime m_time_point;  // 执行的精确时间点
        int m_timer_id = -1;    // 对应的 Timer, 负数无效
        bool operator<(const IntervalS& other) const {
            return m_time_point > other.m_time_point;
        }
    };

    template <typename F, typename... Args>
    void _addFunc(Datetime start_date, Datetime end_date, TimeDelta start_time, TimeDelta end_time,
                  int repeat_num, TimeDelta duration, F&& f, Args&&... args) {
        Timer* t = new Timer;
        t->m_start_date = start_date;
        t->m_end_date = end_date;
        t->m_start_time = start_time;
        t->m_end_time = end_time;
        t->m_repeat_num = repeat_num;
        t->m_duration = duration;
        t->m_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        IntervalS s;
        s.m_time_point = Datetime::now() + duration;

        std::unique_lock<std::mutex> lock(m_mutex);
        int id = getNewTimerId();
        if (id < 0) {
            delete t;
            lock.unlock();
            HKU_THROW("Failed to get new id, maybe too timers!");
        }

        m_timers[id] = t;
        s.m_timer_id = id;
        m_queue.push(s);
        lock.unlock();
        m_cond.notify_all();
    }

private:
    std::priority_queue<IntervalS> m_queue;
    std::atomic_bool m_stop;
    std::mutex m_mutex;
    std::condition_variable m_cond;

    std::unordered_map<int, Timer*> m_timers;
    int m_current_timer_id;
    std::unique_ptr<ThreadPool> m_tg;
};  // namespace hku

}  // namespace hku