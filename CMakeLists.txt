CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (StreamServer C)


FIND_PACKAGE (Threads REQUIRED)

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

ADD_EXECUTABLE (test_ring_buffer test_ring_buffer.c ring_buffer.c)
TARGET_LINK_LIBRARIES (test_ring_buffer ${CMAKE_THREAD_LIBS_INIT})

