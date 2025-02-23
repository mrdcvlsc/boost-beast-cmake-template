cmake_minimum_required(VERSION 3.29)
project(main VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ================ find open ssl ==================

set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL REQUIRED)

# ================ download boost =================

include(FetchContent)

set(BOOST_INCLUDE_LIBRARIES beast asio system)
set(BOOST_ENABLE_CMAKE ON)
set(BUILD_SHARED_LIBS OFF)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

FetchContent_Declare(
  Boost
  URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
  URL_MD5 893b5203b862eb9bbd08553e24ff146a
  DOWNLOAD_EXTRACT_TIMESTAMP ON
  EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(Boost)

# ================== sync target ==================

add_executable("${PROJECT_NAME}-sync"
    src/client-sync.cpp
)

# ================== async target =================

add_executable("${PROJECT_NAME}-async"
    src/client-async.cpp
)

# ========= force msvc to use static lib ==========

if(MSVC)
    foreach(flag_var
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
    )
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

# ================== sync example =================

target_link_libraries("${PROJECT_NAME}-sync" PRIVATE OpenSSL::SSL OpenSSL::Crypto Boost::system Boost::beast)

target_include_directories("${PROJECT_NAME}-sync" PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# windows-specific configurations
if(WIN32)
    target_compile_definitions("${PROJECT_NAME}-sync" PRIVATE
        _WIN32_WINNT=0x0601  # target Windows 7 or later
        WIN32_LEAN_AND_MEAN  # exclude rarely used windows headers
    )
endif()

# ================== async example =================

target_link_libraries("${PROJECT_NAME}-async" PRIVATE OpenSSL::SSL OpenSSL::Crypto Boost::system Boost::beast)

target_include_directories("${PROJECT_NAME}-async" PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# windows-specific configurations
if(WIN32)
    target_compile_definitions("${PROJECT_NAME}-async" PRIVATE
        _WIN32_WINNT=0x0601  # target Windows 7 or later
        WIN32_LEAN_AND_MEAN  # exclude rarely used windows headers
    )
endif()