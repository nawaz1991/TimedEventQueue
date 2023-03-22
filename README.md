Timed Event Queue
==============
`TimedEventQueue` is a C++17 template class for managing and scheduling events based on their associated timestamps. It
provides a simple and efficient way to handle time-based events in your C++ applications. This class is particularly
useful for scenarios such as scheduling tasks, timeouts, or any other event that needs to be triggered at a specific
time.

The `TimedEventQueue` class uses a separate worker thread to manage event expiration and invokes a user-provided
callback function when an event's timestamp expires. It is designed with thread-safety in mind, allowing you to add,
remove, and update events from multiple threads simultaneously.

### Features

- Schedule events with associated timestamps and values
- Efficiently handles event expiration using a separate worker thread
- Thread-safe implementation for adding, removing, and updating events
- User-provided callback function for custom event expiration behavior
- Supports C++17 standard

### Requirements

- C++17 compiler
- Standard C++ library

### Installation

Simply include the timeEventQueue.hpp header file in your project.

### Usage

To use `TimedEventQueue`, follow these steps:

1. Include the `timeEventQueue.hpp` header file in your project.
2. Derive your own class from `TimedEventQueue`, providing a template argument for the event value type.
3. Implement the `onTimestampExpire` function in your derived class, defining the custom behavior to be executed when an
   event expires.
4. Instantiate your derived class and use the member functions to add, remove, and update events.

### Step-by-step Example

An example program demonstrating the usage of `TimedEventQueue` can be found here:

#### Example Program

~~~cpp
#include "TimedEventQueue.hpp"
#include <iostream>

class MyTimedEventQueue : public TimedEventQueue<int> {
protected:
    void onTimestampExpire(const TIMESTAMP &timestamp, const int &value) override {
        std::cout << "Timestamp expired: "
                  << std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count()
                  << " with value: " << value << std::endl;
    }
};

int main() {
    MyTimedEventQueue myTimedEventQueue;

    auto now = TIME::now();
    myTimedEventQueue.addEvent(now + std::chrono::seconds(3), 1);
    myTimedEventQueue.addEvent(now + std::chrono::seconds(1), 2);
    myTimedEventQueue.addEvent(now + std::chrono::seconds(2), 3);
    myTimedEventQueue.addEvent(now + std::chrono::seconds(4), 4);
    myTimedEventQueue.removeEvent(2); // remove using value
    myTimedEventQueue.removeEvent(now + std::chrono::seconds(2)); // remove using timestamp
    myTimedEventQueue.updateValue(now + std::chrono::seconds(4), 5); //update using value
    myTimedEventQueue.updateTimestamp(now + std::chrono::seconds(10), 1); //update using timestamp
    std::this_thread::sleep_for(std::chrono::seconds(6)); // sleep for 6 seconds
    myTimedEventQueue.stop(); // finally stop and the entry with (updated value) 10 will not be printed as that event will occur after the stop

    return 0;
}
~~~

##### Compile

~~~shell
g++ -std=c++17 -pthread myTimedEventQueue.cpp -o myTimedEventQueue
~~~

##### Run

~~~shell
./myTimedEventQueue
~~~

### API Reference

The TimedEventQueue class provides the following public member functions:

- `addEvent(const TIMESTAMP &timestamp, const T &value)`: Adds an event with the specified timestamp and value to the
  queue.
- `removeEvent(const T &value)`: Removes the event with the specified value from the queue.
- `removeEvent(const TIMESTAMP &timestamp)`: Removes the event with the specified timestamp from the queue.
- `updateValue(const TIMESTAMP &timestamp, const T &value)`: Updates the value associated with the specified timestamp
  in the queue.
- `updateTimestamp(const TIMESTAMP &timestamp, const T &value)`: Updates the timestamp associated with the specified
  value in the queue.
- `stop()`: Stops the worker thread and cleans up resources.

### License

This project is open-source and available under the MIT License.

### Author

[Muhammad Nawaz](mailto:m.nawaz2003@gmail.com)
