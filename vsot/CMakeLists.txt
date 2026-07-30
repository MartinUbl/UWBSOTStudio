CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

SET(PROJECT_NAME "VSOTEnv")

INCLUDE(FetchContent)

PROJECT(${PROJECT_NAME})

SET(CMAKE_CXX_STANDARD 20)

FILE(GLOB_RECURSE src src/*.cpp src/*.h src/*.hpp src/*.c)

SET(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/")

INCLUDE_DIRECTORIES("${SHARED_DIR}")

FILE(GLOB_RECURSE src src/*.cpp src/*.hpp src/*.h src/*.c)
FILE(GLOB_RECURSE src_shared "${SHARED_DIR}/*.cpp" "${SHARED_DIR}/*.hpp" "${SHARED_DIR}/*.h" "${SHARED_DIR}/*.c")

SOURCE_GROUP(src FILES ${src})
SOURCE_GROUP(shared FILES ${src_shared})

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.x
)

FetchContent_Declare(box2d
    GIT_REPOSITORY "https://github.com/erincatto/box2d.git"
    GIT_TAG "v3.1.1"
)

FetchContent_GetProperties(spdlog)
IF (NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    ADD_SUBDIRECTORY(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
ENDIF()

FetchContent_GetProperties(box2d)
IF (NOT box2d_POPULATED)
    FetchContent_Populate(box2d)
    ADD_SUBDIRECTORY(${box2d_SOURCE_DIR} ${box2d_BINARY_DIR})
ENDIF()

ADD_EXECUTABLE(${PROJECT_NAME} ${src} ${src_shared})

IF(WIN32)
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} Ws2_32)
ENDIF()
TARGET_LINK_LIBRARIES(${PROJECT_NAME} spdlog::spdlog box2d)

CONFIGURE_MAIN_EXECUTABLE(${PROJECT_NAME})
