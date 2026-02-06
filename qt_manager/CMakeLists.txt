cmake_minimum_required(VERSION 3.20)
project(qt_manager)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 寻找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Multimedia)

# 探测平台 (同 ctp_core)
if(WIN32)
    set(PLATFORM "win64")
else()
    set(PLATFORM "linux64")
endif()

include_directories("${CMAKE_SOURCE_DIR}/../shared")
include_directories("src")

# 寻找 ZMQ 和 JSON
# vcpkg 安装的 ZeroMQ 包名为 ZeroMQ
find_package(ZeroMQ QUIET)
if(NOT ZeroMQ_FOUND)
    find_package(cppzmq QUIET)
endif()

if(NOT ZeroMQ_FOUND AND NOT cppzmq_FOUND)
    if(UNIX)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(ZMQ REQUIRED libzmq)
        set(ZMQ_LIBS ${ZMQ_LIBRARIES})
        include_directories(${ZMQ_INCLUDE_DIRS})
    else()
        message(FATAL_ERROR "Could not find ZeroMQ. Please install via vcpkg: vcpkg install zeromq:x64-windows")
    endif()
else()
    # vcpkg 提供的 ZeroMQ 目标
    if(TARGET libzmq OR TARGET libzmq-static)
        set(ZMQ_LIBS libzmq)
    elseif(TARGET cppzmq)
        set(ZMQ_LIBS cppzmq)
    else()
        set(ZMQ_LIBS ZeroMQ::libzmq)
    endif()
endif()

find_package(nlohmann_json REQUIRED)

# 资源文件处理
qt_add_resources(RESOURCES qml/qml.qrc resources/resources.qrc)

set(APP_ICON_RESOURCE "")
if(WIN32)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.rc")
endif()

add_executable(${PROJECT_NAME} WIN32
    src/main.cpp
    src/network/ZmqWorker.cpp
    src/models/MarketModel.cpp
    src/models/PositionModel.cpp
    src/models/AccountInfo.cpp
    src/models/OrderController.cpp
    src/models/OrderModel.cpp
    src/models/TradeModel.cpp
    ${RESOURCES}
    ${APP_ICON_RESOURCE}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    ${ZMQ_LIBS}
    nlohmann_json::nlohmann_json
)
