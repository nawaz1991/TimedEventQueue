cmake_minimum_required(VERSION 3.24)
project(myTimedEventQueue)

set(CMAKE_CXX_STANDARD 17)
find_package(Threads)
add_executable(${PROJECT_NAME} myTimedEventQueue.cpp)

target_link_libraries(${PROJECT_NAME}
        Threads::Threads
        )