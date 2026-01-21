cmake_minimum_required(VERSION 3.20)
project(qt_manager)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 寻找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# 探测平台 (同 ctp_core)
if(WIN32)
    set(PLATFORM "win64")
else()
    set(PLATFORM "linux64")
endif()

include_directories("${CMAKE_SOURCE_DIR}/../shared")
include_directories("src")

# 寻找 ZMQ 和 JSON
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

find_package(nlohmann_json REQUIRED)

# 资源文件处理
qt_add_resources(RESOURCES qml/qml.qrc)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/network/ZmqWorker.cpp
    src/models/MarketModel.cpp
    src/models/PositionModel.cpp
    src/models/AccountInfo.cpp
    src/models/OrderController.cpp
    src/models/OrderModel.cpp
    src/models/TradeModel.cpp
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    ${ZMQ_LIBS}
    nlohmann_json::nlohmann_json
)
