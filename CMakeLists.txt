CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (StreamServer C)

SET (CMAKE_BUILD_TYPE DEBUG)

FIND_PACKAGE (Threads REQUIRED)

SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pthread")
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

FIND_LIBRARY (ZLOG NAMES zlog)
MESSAGE (STATUS "${ZLOG_INCLUDE_DIRS} ${ZLOG_LIBRARY_DIRS}")
INCLUDE_DIRECTORIES (${ZLOG_INCLUDE_DIRS})
LINK_DIRECTORIES (${ZLOG_LIBRARY_DIRS})



ADD_LIBRARY (thread_pool thread_pool.c)
ADD_LIBRARY (ring_buffer ring_buffer.c)
ADD_LIBRARY (file_loader file_loader.c)
ADD_LIBRARY (thread_pool_cluster thread_pool_cluster.c)
TARGET_LINK_LIBRARIES (thread_pool_cluster thread_pool)


ADD_EXECUTABLE (test_ring_buffer test_ring_buffer.c)
TARGET_LINK_LIBRARIES (test_ring_buffer ring_buffer thread_pool zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_thread_pool test_thread_pool.c)
TARGET_LINK_LIBRARIES (test_thread_pool thread_pool zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_file_loader test_file_loader.c)
TARGET_LINK_LIBRARIES (test_file_loader thread_pool file_loader zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_thread_pool_cluster test_thread_pool_cluster.c)
TARGET_LINK_LIBRARIES (test_thread_pool_cluster thread_pool thread_pool_cluster zlog ${CMAKE_THREAD_LIBS_INIT})
