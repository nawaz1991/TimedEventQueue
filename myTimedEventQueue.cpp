#include "TimedEventQueue.hpp"

#include <iostream>

class MyTimedEventQueue : public TimedEventQueue<int>
{
protected:
    void onTimestampExpire(const TIMESTAMP& timestamp, const int& value) override
    {
        std::cout << "Timestamp expired: " << std::chrono::duration_cast<std::chrono::seconds>(timestamp.time_since_epoch()).count() << " with value: " << value << std::endl;
    }
};

int main()
{
    MyTimedEventQueue myTimedEventQueue;

    auto now = TIME::now();
    myTimedEventQueue.addEvent(now + std::chrono::seconds(3), 1);
    myTimedEventQueue.addEvent(now + std::chrono::seconds(1), 2);
    myTimedEventQueue.addEvent(now + std::chrono::seconds(2), 3);
    myTimedEventQueue.addEvent(now + std::chrono::seconds(4), 4);
    myTimedEventQueue.removeEvent(2);                                                  // remove using value
    myTimedEventQueue.removeEvent(now + std::chrono::seconds(2));              // remove using timestamp
    myTimedEventQueue.updateValue(now + std::chrono::seconds(4), 5);      // update using value
    myTimedEventQueue.updateTimestamp(now + std::chrono::seconds(10), 1); // update using timestamp
    std::this_thread::sleep_for(std::chrono::seconds(6));                          // sleep for 6 seconds
    myTimedEventQueue.stop();                                                               // finally stop and the entry with (updated value) 10 will not be printed as that event will occur after the stop

    return 0;
}