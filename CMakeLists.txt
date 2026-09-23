cmake_minimum_required(VERSION 3.16)
project(QtChatApp VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Core)

add_executable(QtChatApp
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/settingsdialog.h
    src/settingsdialog.cpp
    src/connectionmanager.h
    src/connectionmanager.cpp
)
target_link_libraries(QtChatApp PRIVATE Qt6::Widgets Qt6::Network)

add_executable(QtChatRelay
    src/relay_main.cpp
    src/relayserver.h
    src/relayserver.cpp
)
target_link_libraries(QtChatRelay PRIVATE Qt6::Core Qt6::Network)
