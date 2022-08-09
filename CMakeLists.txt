cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project("Freenect2cam")

set(CMAKE_CXX_FLAGS "-std=c++11")

find_package(freenect2 REQUIRED)
find_package(OpenCV REQUIRED)

include_directories("/usr/include/libusb-1.0/")

INCLUDE_DIRECTORIES(
  ${freenect2_INCLUDE_DIR}
  ${OpenCV_INCLUDE_DIRS}
)

add_executable(freenect2cam ./src/main.cpp
                            ./src/video_device.cpp)

target_link_libraries(freenect2cam PRIVATE ${freenect2_LIBRARIES} ${OpenCV_LIBS})
target_include_directories(freenect2cam PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)