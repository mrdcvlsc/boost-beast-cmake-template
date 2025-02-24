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
    src/client-sync.cpp src/certificates.cpp src/http_client_sync.cpp
)

# ================== async target =================

add_executable("${PROJECT_NAME}-async"
    src/client-async.cpp src/certificates.cpp src/http_client_async.cpp
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

# === download mozila ca-certificate from curl website ===

include(ExternalProject)

ExternalProject_Add(download_cert
    URL https://curl.se/ca/cacert.pem
    DOWNLOAD_NO_EXTRACT TRUE  # no need to extract
    CONFIGURE_COMMAND ""      # no configure step
    BUILD_COMMAND ""          # no build step
    INSTALL_COMMAND ${CMAKE_COMMAND} -E copy
        <DOWNLOAD_DIR>/cacert.pem
        ${CMAKE_BINARY_DIR}/certs/cacert.pem
)

add_custom_command(
    TARGET download_cert PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/certs
)

add_dependencies(download_cert "${PROJECT_NAME}-async" "${PROJECT_NAME}-sync")

# === Create deployment package ===

set(DEPLOY_DIR ${CMAKE_BINARY_DIR}/${PROJECT_NAME})

add_custom_target(package_project
    # create deployment directory
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}
    
    # copy binaries
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/bin
        ${DEPLOY_DIR}/bin
    
    # copy certificates
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_BINARY_DIR}/certs
        ${DEPLOY_DIR}/certs
    
    # create ZIP archive (the command differs based on platform)
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.zip" --format=zip
        "${PROJECT_NAME}"
    
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Packaging ${PROJECT_NAME} into ZIP archive"
)

# Make sure packaging happens after all binaries are built and certs are downloaded
add_dependencies(package_project 
    "${PROJECT_NAME}-async"
    "${PROJECT_NAME}-sync"
    download_cert
)