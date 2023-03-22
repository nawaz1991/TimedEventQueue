/**
 * @file TimedEventQueue.hpp
 * @brief A C++ header file containing the TimedEventQueue class.
 *
 * The TimedEventQueue class provides a mechanism for managing and scheduling
 * events based on their associated timestamps. It allows users to add, remove,
 * and update events with specified timestamps and values. The class uses a
 * separate worker thread to manage event expiration and invokes a
 * user-provided callback function when an event's timestamp expires.
 */
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
/**
 * @typedef TIME
 * @brief A type alias for std::chrono::steady_clock, representing the clock used for scheduling events.
 */
using TIME = std::chrono::steady_clock;

/**
 * @typedef TIMESTAMP
 * @brief A type alias for std::chrono::time_point<std::chrono::steady_clock>, representing the timestamps used for events.
 */
using TIMESTAMP = std::chrono::time_point<TIME>;

/**
 * @def CENTENNIAL
 * @brief A macro that defines a time point 100 years in the future, used as a placeholder for an event that will never be triggered (practically).
 */
#define CENTENNIAL (TIME::now() + std::chrono::hours(100 * 365 * 24))

/**
 * @class TimedEventQueue
 * @tparam T The type of value to be associated with each event in the queue.
 *
 * @brief A class that manages a queue of timed events, allowing users to schedule, update, and cancel events.
 *
 * The TimedEventQueue class is a template class that allows users to add,
 * remove, and update events with specified timestamps and associated values.
 * It uses a separate worker thread to manage event expiration, and when an
 * event's timestamp expires, it invokes a user-provided callback function.
 * This class is designed to be thread-safe, ensuring that its operations can
 * be called from multiple threads without causing data races or other
 * concurrency issues.
 */
template<typename T>
class TimedEventQueue
{
private:
    std::map<TIMESTAMP, T>  _ts2Val;       ///< A map of timestamps to their associated values, used to efficiently find the next event to expire.
    std::map<T, TIMESTAMP>  _val2Ts;       ///< A reverse map of values to their associated timestamps, used to efficiently update or remove events by value.
    std::mutex              _mutex;        ///< A mutex used to protect concurrent access to the data members, ensuring thread safety.
    std::condition_variable _cv;           ///< A condition variable used to signal the internal worker thread when events are added, removed, or updated.
    std::atomic<bool>       _exit = false; ///< An atomic flag used to indicate whether the worker thread should exit, allowing for clean shutdown of the thread.
    std::thread             _thread;       ///< The worker thread that manages event expiration and calls the user-provided callback function.

    /**
     * @brief The internal function executed by the worker thread, which manages event expiration and calls the user-provided callback function.
     *
     * This function continuously checks for expired events in the queue and
     * calls the user-provided callback function when an event's timestamp
     * expires. It also handles waiting for the next event to expire and
     * responding to updates from other threads that modify the event queue.
     */
    void run()
    {
        while(!_exit.load())
        {
            std::unique_lock lock(_mutex);
            _cv.wait_until(lock, _ts2Val.begin()->first);

            if(_exit.load())
            {
                break;
            }

            auto current_time = TIME::now();
            while(_ts2Val.begin()->first <= current_time)
            {
                std::invoke(&TimedEventQueue<T>::onTimestampExpire, this, _ts2Val.begin()->first, _ts2Val.begin()->second);
                _val2Ts.erase(_ts2Val.begin()->second);
                _ts2Val.erase(_ts2Val.begin());
            }
        }
    }

protected:
    /**
     * @brief A pure virtual function that must be implemented by the user, called when an event's timestamp expires.
     *
     * This function is called by the worker thread when an event's timestamp expires. Users
     * should override this function to define custom behavior when an event
     * expires, such as performing a specific action or updating a data
     * structure. The function is provided with the expired timestamp and its
     * associated value.
     *
     * @param timestamp The expired timestamp.
     * @param value The value associated with the expired timestamp.
     */
    virtual void onTimestampExpire(const TIMESTAMP& timestamp, const T& value) = 0;

public:
    /**
     * @brief Constructs a new TimedEventQueue object, initializing the worker thread and adding a dummy event.
     *
     * The constructor creates a new TimedEventQueue object, initializing the
     * worker thread that manages event expiration. It also adds a dummy event
     * with a timestamp 100 years in the future to ensure that the worker
     * thread always has an event to wait for.
     */
    TimedEventQueue()
    {
        auto dummy_timestamp = CENTENNIAL;
        addEvent(dummy_timestamp, T());
        _thread = std::thread(&TimedEventQueue::run, this);
    }

    /**
     * @brief Destructs the TimedEventQueue object, stopping the worker thread and releasing resources.
     *
     * The destructor ensures that the worker thread is stopped and joined
     * before the object is destroyed, preventing potential issues with
     * accessing dangling resources. This is done by setting the _exit flag,
     * notifying the worker thread via the condition variable, and joining the
     * thread.
     */
    virtual ~TimedEventQueue() { stop(); }

    TimedEventQueue(const TimedEventQueue&) = delete;

    TimedEventQueue& operator=(const TimedEventQueue&) = delete;

    /**
     * @brief Adds an event with the specified timestamp and value to the queue.
     *
     * This function inserts a new event into the queue with the specified
     * timestamp and associated value. It is thread-safe and can be called from
     * multiple threads simultaneously. After adding the event, the function
     * notifies the worker thread to ensure that it is aware of the new event.
     *
     * @param timestamp The timestamp of the event.
     * @param value The value associated with the event.
     */
    void addEvent(const TIMESTAMP& timestamp, const T& value)
    {
        std::scoped_lock lock(_mutex);
        _ts2Val.emplace(timestamp, value);
        _val2Ts.emplace(value, timestamp);
        _cv.notify_one();
    }

    /**
     * @brief Removes the event with the specified value from the queue.
     *
     * This function removes the event with the specified value from the queue,
     * if it exists. It is thread-safe and can be called from multiple threads
     * simultaneously. After removing the event, the function notifies the
     * worker thread to ensure that it is aware of the change.
     *
     * @param value The value of the event to remove.
     */
    void removeEvent(const T& value)
    {
        std::scoped_lock lock(_mutex);
        if(auto itr = _val2Ts.find(value); itr != _val2Ts.end())
        {
            _ts2Val.erase(itr->second);
            _val2Ts.erase(itr);
        }
    }

    /**
     * @brief Removes the event with the specified timestamp from the queue.
     *
     * This function removes the event with the specified timestamp from the
     * queue, if it exists. It is thread-safe and can be called from multiple
     * threads simultaneously. After removing the event, the function notifies
     * the worker thread to ensure that it is aware of the change.
     *
     * @param timestamp The timestamp of the event to remove.
     */
    void removeEvent(const TIMESTAMP& timestamp)
    {
        std::scoped_lock lock(_mutex);
        if(auto itr = _ts2Val.find(timestamp); itr != _ts2Val.end())
        {
            _val2Ts.erase(itr->second);
            _ts2Val.erase(itr);
        }
    }

    /**
     * @brief Updates the value associated with the specified timestamp in the queue.
     *
     * This function updates the value associated with the specified timestamp
     * in the queue, if it exists. It is thread-safe and can be called from
     * multiple threads simultaneously. After updating the value, the function
     * notifies the worker thread to ensure that it is aware of the change.
     *
     * @param timestamp The timestamp of the event to update.
     * @param value The new value to associate with the timestamp.
     */
    void updateValue(const TIMESTAMP& timestamp, const T& value)
    {
        std::scoped_lock lock(_mutex);
        if(auto itr = _ts2Val.find(timestamp); _ts2Val.end() != itr)
        {
            auto nodeHandler  = _val2Ts.extract(itr->second);
            nodeHandler.key() = value;
            _val2Ts.insert(std::move(nodeHandler));
            itr->second = value;
        }
        _cv.notify_one();
    }

    /**
     * @brief Updates the timestamp associated with the specified value in the queue.
     *
     * This function updates the timestamp associated with the specified value
     * in the queue, if it exists. It is thread-safe and can be called from
     * multiple threads simultaneously. After updating the timestamp, the function
     * notifies the worker thread to ensure that it is aware of the change.
     *
     * @param timestamp The new timestamp to associate with the value.
     * @param value The value of the event to update.
     */
    void updateTimestamp(const TIMESTAMP& timestamp, const T& value)
    {
        std::scoped_lock lock(_mutex);
        if(auto itr = _val2Ts.find(value); _val2Ts.end() != itr)
        {
            auto nodeHandler  = _ts2Val.extract(itr->second);
            nodeHandler.key() = timestamp;
            _ts2Val.insert(std::move(nodeHandler));
            itr->second = timestamp;
        }
        _cv.notify_one();
    }

    /**
     * @brief Stops the worker thread and cleans up resources.
     *
     * This function stops the worker thread by setting the _exit flag, notifying
     * the worker thread via the condition variable, and joining the thread. It
     * should be called before the TimedEventQueue object is destroyed to ensure
     * that resources are properly cleaned up and no dangling references to the
     * object are accessed by the worker thread.
     */
    void stop()
    {
        if(_thread.joinable())
        {
            _exit.store(true);
            _cv.notify_one();
            _thread.join();
        }
    }
};
