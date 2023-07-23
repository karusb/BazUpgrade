FIND_PACKAGE(CURL)
IF(CURL_FOUND)
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
  SET(requiredlibs ${requiredlibs} ${CURL_LIBRARIES} )
ELSE(CURL_FOUND)
    include(ExternalProject)
    message(STATUS "Building libcurl enabled")

    set(CURL_FLAGS -DBUILD_CURL_EXE=OFF -DBUILD_SHARED_LIBS=OFF
        -DUSE_WIN32_LDAP=OFF -DHTTP_ONLY=ON -DCURL_ZLIB=OFF
        -DCURL_DISABLE_CRYPTO_AUTH=ON -DCMAKE_USE_LIBSSH2=OFF -DBUILD_TESTING=OFF )
    if (WIN32)
        set(CURL_FLAGS ${CURL_FLAGS} -DCURL_STATIC_CRT=ON )
    endif()

    if (APPLE)
        set(CURL_FLAGS ${CURL_FLAGS} -DCURL_USE_SECTRANSP=ON )
    elseif(WIN32)
        set(CURL_FLAGS ${CURL_FLAGS} -DCURL_USE_SCHANNEL=ON )
    else()
        set(CURL_FLAGS ${CURL_FLAGS} -DCURL_USE_OPENSSL=ON )
    endif()

    ExternalProject_Add(
        libcurl
        PREFIX libcurl
        GIT_REPOSITORY "https://github.com/curl/curl.git"
        GIT_TAG "curl-7_85_0"
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/third-party/libcurl
        CMAKE_ARGS -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
                    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                    -DCMAKE_C_FLAGS_DEBUG=${CMAKE_C_FLAGS_DEBUG}
                    -DCMAKE_C_FLAGS_MINSIZEREL=${CMAKE_C_FLAGS_MINSIZEREL}
                    -DCMAKE_C_FLAGS_RELEASE=${CMAKE_C_FLAGS_RELEASE}

                    -DCMAKE_C_FLAGS_RELWITHDEBINFO=${CMAKE_C_FLAGS_RELWITHDEBINFO}

                    -DCMAKE_CXX_STANDARD_LIBRARIES=${CMAKE_CXX_STANDARD_LIBRARIES}

                    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install
                    -DCMAKE_INSTALL_LIBDIR=lib
                    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                    -DANDROID_ABI=${ANDROID_ABI}
                    -DANDROID_STL=${ANDROID_STL} ${CURL_FLAGS}
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""
    )

    set(CURL_DEBUG_TARGET_NAME "libcurl-d")
    set(CURL_RELEASE_TARGET_NAME "libcurl")
    add_library(curl INTERFACE)
    add_definitions(-DCURL_STATICLIB) # Mandatory for building libcurl as static lib

    target_include_directories(curl INTERFACE
$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install/include>)

    # libcurl require ws2_32.lib for windows only
    if (WIN32)
        target_link_libraries(curl INTERFACE ws2_32.lib crypt32.lib)
    else(NOT APPLE)
        set(OPENSSL_USE_STATIC_LIBS TRUE)
        find_package(OpenSSL REQUIRED)
        target_link_libraries(curl INTERFACE OpenSSL::SSL OpenSSL::Crypto)
    endif()

    target_link_libraries(curl INTERFACE debug
${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install/lib/${CURL_DEBUG_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})
    target_link_libraries(curl INTERFACE optimized
${CMAKE_CURRENT_BINARY_DIR}/libcurl/libcurl_install/lib/${CURL_RELEASE_TARGET_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})

ENDIF(CURL_FOUND)

add_library(bazupgrade.curl STATIC)
set_target_properties(bazupgrade.curl PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.curl PUBLIC ${BazUpgradePath} ${CURL_INCLUDE_DIRS} curl)

target_link_libraries(bazupgrade.curl PUBLIC
    bazupgrade.interfaces
    curl
)
target_sources(bazupgrade.curl PRIVATE
    CurlContext.cpp
    CurlContext.hpp
    DataCallbacks.cpp
    DataCallbacks.hpp
    HttpClientCurl.cpp
    HttpClientCurl.hpp
)