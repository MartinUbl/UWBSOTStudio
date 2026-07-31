CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

PROJECT(UWBSOTStudio)

SET(CMAKE_CXX_STANDARD 20)

SET(APP_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")

FUNCTION(CONFIGURE_MAIN_EXECUTABLE target_name)
    SET_TARGET_PROPERTIES(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${APP_OUTPUT_DIR}"
    )
ENDFUNCTION()

FUNCTION(CONFIGURE_PLUGIN target_name)
    SET_TARGET_PROPERTIES(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${APP_OUTPUT_DIR}/plugins"
        LIBRARY_OUTPUT_DIRECTORY "${APP_OUTPUT_DIR}/plugins"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>"
    )
ENDFUNCTION()

INCLUDE(FetchContent)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
  GIT_SHALLOW 1
)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_SHALLOW 1
)

FetchContent_MakeAvailable(glfw imgui)

FIND_PACKAGE(OpenGL REQUIRED)

FILE(GLOB imgui_src "${imgui_SOURCE_DIR}/*.cpp" "${imgui_SOURCE_DIR}/*.h" "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp" "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h" "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp" "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h" "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib*")

INCLUDE_DIRECTORIES("${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends/" "${glfw_SOURCE_DIR}/include")

FILE(GLOB_RECURSE src_shared shared/uwbsot/*.cpp shared/uwbsot/*.h shared/uwbsot/*.hpp shared/uwbsot/*.c)

INCLUDE_DIRECTORIES("shared/")

SOURCE_GROUP(shared FILES ${src_shared})

FILE(GLOB_RECURSE src src/*.cpp src/*.h src/*.hpp src/*.c)

SOURCE_GROUP(src FILES ${src})

ADD_LIBRARY(imgui STATIC ${imgui_src})

ADD_EXECUTABLE(UWBSOTStudio ${src} ${src_shared})

TARGET_LINK_LIBRARIES(UWBSOTStudio imgui glfw OpenGL::GL)

CONFIGURE_MAIN_EXECUTABLE(UWBSOTStudio)

ADD_SUBDIRECTORY("plugins")
ADD_SUBDIRECTORY("vsot")
