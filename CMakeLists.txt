cmake_minimum_required(VERSION 3.25.1)
project(youtube_frontend)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenSSL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
find_package(CURL REQUIRED)

add_executable(youtube_frontend
        src/Main.cpp
        src/YoutubeComment.cpp
        src/YoutubeVideo.cpp
        src/ArgType.cpp
        src/Args.cpp
        src/Utils.cpp
        src/YoutubeVideoHtml.cpp
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
