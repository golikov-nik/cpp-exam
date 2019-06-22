cmake_minimum_required(VERSION 2.8)

include_directories(${PROJECT_SOURCE_DIR})

add_executable(vector_testing
               vector_testing.cpp
               counted.h
               counted.cpp
               fault_injection.h
               fault_injection.cpp
               gtest/gtest-all.cc
               gtest/gtest.h
               gtest/gtest_main.cc
               vector.h
               basic_vector.h)

add_executable(list_testing
               list.cpp
               counted.h
               counted.cpp
               fault_injection.cpp
               fault_injection.h
               gtest/gtest-all.cc
               gtest/gtest.h
               gtest/gtest_main.cc
               list.h)

#add_executable(main main.cpp vector.h basic_vector.h list.h)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 -pedantic")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address,undefined -D_GLIBCXX_DEBUG")
endif()

target_link_libraries(vector_testing -lpthread)
target_link_libraries(list_testing -lpthread)
