cmake_minimum_required(VERSION 3.1)
project(comms_webrtc VERSION 0.0)

# Needed by libdatachannel
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.orogen/config")
include(comms_webrtcBase)

# FIND_PACKAGE(KDL)
# FIND_PACKAGE(OCL)

