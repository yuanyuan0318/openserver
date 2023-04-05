
CMAKE_MINIMUM_REQUIRED(VERSION 3.12.1)
PROJECT(openserver)

include_directories(src)
include_directories(test)

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
else()  
    link_libraries(pthread)
endif()


set(SRC 
	src/wepoll.h 
	src/wepoll.c 
	src/socket_os.h 
	src/socket_os.c 
	src/opensocket.h 
	src/opensocket.cpp
	src/openthread.h
	src/openthread.cpp
	src/openserver.h
	src/openserver.cpp
	src/opentime.h
	src/opentime.cpp
	src/openbuffer.h
	src/openbuffer.cpp
	src/openjson.h
	src/openjson.cpp
	src/opencsv.h
	src/opencsv.cpp
	src/openfsm.h
	src/openfsm.cpp
)

add_executable(helloworld ${SRC} test/helloworld.cpp)