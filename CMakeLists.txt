cmake_minimum_required(VERSION 3.15)
project(main VERSION 1.0.0 LANGUAGES CXX)

# Specify C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

include(FetchContent)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(BUILD_CURL_EXE OFF)
set(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)
set(CURL_USE_STATIC_LIBS TRUE)
set(HTTP_ONLY ON)  # Minimize build time by enabling HTTP only

FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_5_0  # Adjust version as needed
)

set(CURL_DISABLE_INSTALL ON)

FetchContent_MakeAvailable(curl)

if(MSVC)
    # MSVC compiler flags
    add_compile_options(/W4 /MP)
else()
    # GCC/Clang compiler flags
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
    if(MINGW)
        add_definitions(-D_WIN32_WINNT=0x0601)
    endif()
endif()

add_executable(${PROJECT_NAME} 
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)