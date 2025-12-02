cmake_minimum_required(VERSION 3.23)
project(
  CrowQtClient
  VERSION 0.2.0
  DESCRIPTION "Upload images to Webserver"
  HOMEPAGE_URL "https://github.com/Zheng-Bote/crow_webserver_desktop-client"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_NAME "${PROJECT_NAME}")
set(PROG_LONGNAME "Crow QT Client")
set(PROG_CREATED "2025")
set(PROG_AUTHOR "ZHENG Bote")
set(PROG_ORGANIZATION_NAME "ZHENG Robert")
set(PROG_ORGANIZATION_DOMAIN "net.hase-zheng")
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
set(CMAKE_QTV "${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")

set(CMAKE_AUTOMOC ON)

add_subdirectory(configure)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(${PROJECT_NAME} main.cpp MainWindow.cpp MainWindow.h
                               img/logo-36x36.png)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
                                              Qt6::Network)
