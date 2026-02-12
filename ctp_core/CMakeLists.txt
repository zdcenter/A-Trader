cmake_minimum_required(VERSION 3.20)
project(ctp_core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 探测平台
if(WIN32)
    set(PLATFORM "win64")
    set(CTP_LIBS thostmduserapi_se thosttraderapi_se)
else()
    set(PLATFORM "linux64")
    set(CTP_LIBS thostmduserapi_se thosttraderapi_se)
endif()

# 设置 CTP 路径
set(CTP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../shared/api/ctp/v6.72/${PLATFORM}")
set(CTP_LIB_DIR "${CMAKE_SOURCE_DIR}/../shared/api/ctp/v6.72/${PLATFORM}")

include_directories(${CTP_INCLUDE_DIR})
include_directories("${CMAKE_SOURCE_DIR}/../shared")
include_directories("include")

link_directories(${CTP_LIB_DIR})

# 依赖库寻址
find_package(zmq QUIET)
if(NOT zmq_FOUND)
    find_package(ZeroMQ QUIET)
endif()

if(NOT zmq_FOUND AND NOT ZeroMQ_FOUND)
    if(UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ZMQ REQUIRED libzmq)
        set(ZMQ_LIBS ${ZMQ_LIBRARIES})
        include_directories(${ZMQ_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Could not find ZeroMQ. Please install it.")
    endif()
else()
    set(ZMQ_LIBS libzmq)
endif()

# nlohmann_json 往往是 header-only
find_package(nlohmann_json REQUIRED)

# Postgres Support (libpqxx)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
include_directories(${PQXX_INCLUDE_DIRS})
link_directories(${PQXX_LIBRARY_DIRS})

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/network/Publisher.cpp
    src/network/CommandServer.cpp
    src/api/MdHandler.cpp
    src/api/TraderHandler.cpp
    src/strategy/ConditionEngine.cpp # Added
    src/storage/DBManager.cpp # Added
    src/position/PositionManager.cpp # Added
)

# 复制 config.json 到构建目录
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} 
    ${CTP_LIBS}
    ${ZMQ_LIBS}
    nlohmann_json::nlohmann_json
    ${PQXX_LIBRARIES}
)

# 复制 CTP 运行库到二进制目录 (跨平台处理)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CTP_LIB_DIR}/thostmduserapi_se.dll"
        "${CTP_LIB_DIR}/thosttraderapi_se.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
