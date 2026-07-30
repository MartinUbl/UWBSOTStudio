CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

SET(PROJECT_NAME "vsot_sensing")

SET(CMAKE_CXX_STANDARD 20)

PROJECT(${PROJECT_NAME})

SET(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/")

INCLUDE_DIRECTORIES("${SHARED_DIR}")

FILE(GLOB_RECURSE src src/*.cpp src/*.hpp src/*.h src/*.c)
FILE(GLOB_RECURSE src_shared "${SHARED_DIR}/uwbsot/*.cpp" "${SHARED_DIR}/uwbsot/*.hpp" "${SHARED_DIR}/uwbsot/*.h" "${SHARED_DIR}/uwbsot/*.c")
FILE(GLOB_RECURSE src_shared_net "${SHARED_DIR}/vsotnet/*.cpp" "${SHARED_DIR}/vsotnet/*.hpp" "${SHARED_DIR}/vsotnet/*.h" "${SHARED_DIR}/vsotnet/*.c")

SOURCE_GROUP(src FILES ${src})
SOURCE_GROUP(shared FILES ${src_shared})
SOURCE_GROUP(shared_net FILES ${src_shared_net})

ADD_LIBRARY(${PROJECT_NAME} SHARED ${src} ${src_shared} ${src_shared_net})

IF(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} Ws2_32)
ENDIF()

CONFIGURE_PLUGIN(${PROJECT_NAME})
