cmake_minimum_required (VERSION 2.8.11)
project(tracer)

if(UNIX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

file(GLOB_RECURSE header "./*.h")

set(Sources trace.cpp DatabaseConnection.cpp ./sqlite/sqlite3.c)

add_library( tracer SHARED ${Sources} ${header} )
install(TARGETS tracer 
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	)
INSTALL(FILES trace.h
    DESTINATION "../include"
    )
