cmake_minimum_required(VERSION 3.10)

project(DearChat VERSION 1.0.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CPP_COMPILE_OPTIONS -Werror -Wall -Wextra -Wno-deprecated -Wno-unused-parameter -pedantic)

### EXTERNAL LIBRARIES ###

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    imgui
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.1.tar.gz
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.h
    ${imgui_SOURCE_DIR}/imgui_internal.h
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.h
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}/
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
)

set(IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# TODO: add fetchdeclare for libzmq and cppzmq
# https://chatgpt.com/share/1ed154ad-863d-4bc0-a315-21f066400477

## ZMQ (cppzmq) library
find_package(cppzmq)

## find packages for imgui
if (APPLE) 
    # install glfw3 via brew
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)

    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)

    include_directories(/usr/local/include /opt/local/include /opt/homebrew/include)
    link_directories(/usr/local/lib /opt/homebrew/lib)
    target_link_libraries(imgui PUBLIC ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})

elseif (UNIX AND NOT APPLE)
    # TODO: test build on linux machine
    # no clue if this will work

    find_package(OpenGL REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW3 REQUIRED glfw3)

    include_directories(${GLFW3_INCLUDE_DIRS})
    link_directories(${GLFW3_LIBRARY_DIRS})

    target_link_libraries(imgui PUBLIC ${GLFW3_LIBRARIES} ${OPENGL_LIBRARIES})
endif()


### HEADER ONLY LIBs ###
add_library(zpp_bits INTERFACE)
target_include_directories(zpp_bits INTERFACE src/lib/zpp_bits)

### LOCAL CODE FILES ###

## exchange
set(SERVER_SOURCE_FILES
    src/server/server.cpp
)

add_library(server_lib STATIC ${SERVER_SOURCE_FILES})
target_include_directories(server_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src/server
)
target_link_libraries(server_lib PUBLIC spdlog::spdlog cppzmq zpp_bits)

add_executable(server src/server/server.m.cpp)
target_link_libraries(server PRIVATE server_lib)

## client
set(CLIENT_SOURCE_FILES
    src/client/client.cpp
)

add_library(client_lib STATIC ${CLIENT_SOURCE_FILES})
target_include_directories(client_lib
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src/client
)
# TODO: remove orderbook_lib from client as only need the order.h class
target_link_libraries(client_lib PUBLIC spdlog::spdlog cppzmq zpp_bits imgui glfw3 OpenGL::GL)

add_executable(client src/client/client.m.cpp ${IMGUI_SRC})
target_link_libraries(client PRIVATE client_lib)


## CLIENT GUI

if (APPLE OR UNIX)

    set(CLIENT_GUI_SOURCE_FILES
        src/client/client.cpp
    )

    add_library(client_gui_lib STATIC ${CLIENT_GUI_SOURCE_FILES})
    target_include_directories(client_gui_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/client
    )
    target_link_libraries(client_gui_lib PUBLIC spdlog::spdlog client_lib)

    add_executable(client_gui src/client/clientgui.m.cpp ${IMGUI_SRC})
    target_link_libraries(client_gui PRIVATE client_gui_lib)

endif()
