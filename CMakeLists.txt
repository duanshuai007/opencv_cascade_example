cmake_minimum_required(VERSION 3.4)
project(MyTestOpencv36)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
find_package(OpenCV 3.4.0 REQUIRED)
include_directories( ${OpenCV_INCLUDE_DIRS})
include_directories(./)

set(SRC_DETECTION ./PlateDetection.cpp  ./util.h ./PlateDetection.h)

add_executable(TEST_Detection ${SRC_DETECTION} ./main.cpp)
target_link_libraries(TEST_Detection ${OpenCV_LIBS})
