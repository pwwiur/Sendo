cmake_minimum_required(VERSION 3.14)
project(Sendo)

set(CMAKE_CXX_STANDARD 14)

add_executable(Sendo main.cpp Server.h dist/SendoException.h dist/Lexical.h dist/Semantic.h dist/Request.h dist/Executor.h dist/Gadget.h dist/Config.h)

target_compile_options (Sendo PRIVATE -lmysqlcppconn)


find_package(Boost COMPONENTS thread system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(Sendo ${Boost_LIBRARIES})

target_link_libraries(Sendo mysqlcppconn)
