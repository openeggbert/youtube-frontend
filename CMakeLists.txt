cmake_minimum_required(VERSION 3.25.1)
project(youtube_frontend)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
find_package(CURL REQUIRED)

add_compile_options(-Wall -Wextra -Wpedantic)

add_executable(youtube_frontend
        src/Main.cpp
        src/YoutubeComment.cpp
        src/YoutubeVideo.cpp
        src/ArgType.cpp
        src/Args.cpp
        src/Utils.cpp
        src/YoutubeVideoHtml.cpp
        src/AssetsStyle.cpp
        src/AssetsScript.cpp
        src/PageShell.cpp
)

target_include_directories(youtube_frontend PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)

target_link_libraries(youtube_frontend PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        ${OpenCV_LIBS}
        avformat avcodec avutil
        ${CURL_LIBRARIES}
)

# Unit tests for the pure/self-contained pieces (Utils, Args, YoutubeComment
# threading, YoutubeVideo sort order). Uses a small hand-rolled harness
# (tests/TestFramework.*) instead of pulling in an external framework, since
# none is installed in this environment and the test surface is small.
enable_testing()

add_executable(youtube_frontend_tests
        tests/main.cpp
        tests/TestFramework.cpp
        tests/UtilsTest.cpp
        tests/ArgsTest.cpp
        tests/YoutubeCommentTest.cpp
        tests/YoutubeVideoTest.cpp
        src/YoutubeComment.cpp
        src/YoutubeVideo.cpp
        src/ArgType.cpp
        src/Args.cpp
        src/Utils.cpp
)

target_include_directories(youtube_frontend_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)

target_link_libraries(youtube_frontend_tests PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        ${OpenCV_LIBS}
        avformat avcodec avutil
        ${CURL_LIBRARIES}
)

add_test(NAME unit_tests COMMAND youtube_frontend_tests)
