cmake_minimum_required(VERSION 3.12)
project(SharedImage)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)

# Specify executable targets
add_executable(reader reader.cpp)
add_executable(show show.cpp)

add_executable(vreader vector_reader.cpp)
add_executable(vshow vector_show.cpp)

# Link libraries
target_link_libraries(reader PRIVATE ${OpenCV_LIBS} Boost::system)
target_link_libraries(show PRIVATE ${OpenCV_LIBS} Boost::system)

target_link_libraries(vreader PRIVATE ${OpenCV_LIBS} Boost::system)
target_link_libraries(vshow PRIVATE ${OpenCV_LIBS} Boost::system)

