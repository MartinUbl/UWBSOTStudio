CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

SET(PROJECT_NAME "table_control")

SET(CMAKE_CXX_STANDARD 20)

PROJECT(${PROJECT_NAME})

INCLUDE(FetchContent)

FetchContent_Declare(
  msot-ctllib
  GIT_REPOSITORY https://github.com/MartinUbl/ZCU-KIV-MiroSOT-DesktopControlLib.git
  GIT_TAG 18fed74c3ed48e689b85f79aa9b2a7957b42bf66
  GIT_SHALLOW 1
)

FetchContent_MakeAvailable(msot-ctllib)

SET(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/")

INCLUDE_DIRECTORIES("${SHARED_DIR}" "${msot-ctllib_SOURCE_DIR}/src/")

FILE(GLOB_RECURSE src src/*.cpp src/*.hpp src/*.h src/*.c)
FILE(GLOB_RECURSE src_shared "${SHARED_DIR}/uwbsot/*.cpp" "${SHARED_DIR}/uwbsot/*.hpp" "${SHARED_DIR}/uwbsot/*.h" "${SHARED_DIR}/uwbsot/*.c")

SOURCE_GROUP(src FILES ${src})
SOURCE_GROUP(shared FILES ${src_shared})

ADD_LIBRARY(${PROJECT_NAME} SHARED ${src} ${src_shared})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} msot-ctllib)

CONFIGURE_PLUGIN(${PROJECT_NAME})
