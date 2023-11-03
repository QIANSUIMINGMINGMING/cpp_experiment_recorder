cmake_minimum_required(VERSION 3.0.0)

project(app)

set(CMAKE_CXX_FLAGS "-std=c++17 -Wall -W -Werror -fPIC -pthread")

set(SOURCE_FILES main.cpp)
add_executable(app ${SOURCE_FILES})

# <------------ add redis-plus-plus dependency here -------------->
find_path(REDIS_PLUS_PLUS_HEADER sw)      # NOTE: this should be *sw* NOT *redis++*
target_include_directories(app PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(app ${REDIS_PLUS_PLUS_LIB})

# <------------ add hiredis dependency here --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(app PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(app ${HIREDIS_LIB})

