cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

PROJECT(AiObject-Yolov3)


set(SNPE_INC ${PROJECT_SOURCE_DIR}/inc/snpe/include/zdl)
set(SOURCE ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_ROOT "/home/tc-eb5/EdgeAPP")
set(OpenCV_DIR "${PROJECT_ROOT}/thirdparty/opencv-4.2.0/lib/cmake/opencv4")

message(STATUS "    project source path: ${SOURCE}")
message(STATUS "    project current path: ${PROJECT_ROOT}")
message(STATUS "    cmake current path: ${CMAKE_CURRENT_SOURCE_DIR}")

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

#include(FindPkgConfig)

set(GSTREAMER_INCLUDE_DIRS "/usr/include/gstreamer-1.0;")
set(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "/usr/lib/aarch64-linux-gnu/gstreamer-1.0/include;")
set(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "/usr/include/glib-2.0;")
set(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIRS} "/usr/lib/aarch64-linux-gnu/glib-2.0/include;")
set(GSTREAMER_LIBS  "gstapp-1.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "gstbase-1.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "gstrtsp-1.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "gstreamer-1.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "gstsdp-1.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "gio-2.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "gobject-2.0;")
set(GSTREAMER_LIBS ${GSTREAMER_LIBS} "glib-2.0;")
set(GSTREAMER_LIBRARY_DIRS  "/usr/lib/aarch64-linux-gnu;")

message(STATUS "GST DEBUG:###########################")
message(STATUS "    libraries: ${GSTREAMER_LIBS}")
message(STATUS "    include path: ${GSTREAMER_INCLUDE_DIRS}")
message(STATUS "    library path: ${GSTREAMER_LIBRARY_DIRS}")


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/
    ${OpenCV_INCLUDE_DIRS}/
    ${GSTREAMER_INCLUDE_DIRS}/
    ${GSTREAMER_APP_INCLUDE_DIRS}/
    ${GSTREAMER_RTSP_INCLUDE_DIRS}/	
	${PROJECT_SOURCE_DIR}/inc 
    ${SNPE_INC}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_directories(
    ${OpenCV_LIBRARY_DIRS}/
    ${GSTREAMER_LIBRARY_DIRS}
	${PROJECT_SOURCE_DIR}/libs
)

add_definitions(-DUSE_MODEL_FILE)

add_executable(${PROJECT_NAME}
    ${SOURCE}/time_utility.cpp
    # ${SOURCE}/YoloClassification.cpp
    # ${SOURCE}/TS_YoloClassification.cpp
    ${SOURCE}/yolov3_detection.cpp
    ${SOURCE}/TsCamera-rtmp.cpp
)

target_link_libraries(${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/libs/aarch/libSNPE.so
    # ${PARAM_CONF_LIBS}	
    ${OpenCV_LIBS}
    ${GSTREAMER_LIBS}
    z
)

set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/)
set(CMAKE_CXX_FLAGS "-std=c++0x  -lm -pthread")


install (TARGETS AiObject-Yolov3 DESTINATION usr/local/bin)
install (FILES README.txt DESTINATION usr/local/bin)
