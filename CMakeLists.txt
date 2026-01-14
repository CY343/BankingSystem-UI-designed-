cmake_minimum_required(VERSION 3.19)

# Force vcpkg toolchain even if Qt Creator tries to set it to "<UNSET>"
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE
   OR CMAKE_TOOLCHAIN_FILE STREQUAL ""
   OR CMAKE_TOOLCHAIN_FILE STREQUAL "<UNSET>")
  set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE FILEPATH "vcpkg toolchain" FORCE)
endif()

project(BankingUI VERSION 0.1 LANGUAGES CXX)

# ----------------------------
# C++ Standard
# ----------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output folders (Qt Creator/Ninja single-config uses Debug/Release via CMAKE_BUILD_TYPE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")

# ----------------------------
# Qt6 Path (only if not provided by Kit)
# ----------------------------
if(NOT CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Qt6 install path")
endif()

# ----------------------------
# Enable Qt auto features
# ----------------------------
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ----------------------------
# Find Qt6
# ----------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport LinguistTools Sql)


# ----------------------------
# OpenSSL
# ----------------------------
find_package(OpenSSL REQUIRED)

# ----------------------------
# Boost
# ----------------------------
set(BOOST_ROOT "C:/Libraries/boost_1_88_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")  

find_package(Boost 1.88 REQUIRED COMPONENTS filesystem system)

message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")

# ----------------------------
# MySQL Connector/C++
# ----------------------------
set(MYSQLCPPCONN_ROOT "C:/Users/13478/MYSQLC/mysql-connector-c++-9.4.0-winx64")

set(MYSQLCPPCONN_INCLUDE_DIRS
    "${MYSQLCPPCONN_ROOT}/include"
    "${MYSQLCPPCONN_ROOT}/include/jdbc"
)

set(MYSQLCPPCONN_LIB "${MYSQLCPPCONN_ROOT}/lib64/vs14/mysqlcppconn.lib")

# IMPORTANT: you confirmed libmysql.dll is here:
set(MYSQL_SERVER_LIB_DIR "C:/Program Files/MySQL/MySQL Server 8.0-new/lib")
set(MYSQL_SERVER_DLL "${MYSQL_SERVER_LIB_DIR}/libmysql.dll")

# ----------------------------
# Sources / UI
# ----------------------------
set(SOURCES
    src/main.cpp

    src/LoginWindow.cpp
    src/managerwindow.cpp
    src/BankAccount.cpp
    src/BankingExceptions.cpp
    src/Card.cpp
    src/CardGenerator.cpp
    src/CheckingAccount.cpp
    src/ConnectionPool.cpp
    src/CreditCard.cpp
    src/Customers.cpp
    src/DataManager.cpp
    src/DebitCard.cpp
    src/SavingAccount.cpp
    src/SecurityHelper.cpp
    src/Services.cpp
    src/Transaction.cpp
    src/seedData.cpp
    src/mainwindow.cpp
)

set(HEADERS
    include/LoginWindow.hpp
    include/managerwindow.h
    include/seedData.hpp
    include/mainwindow.h
    include/Toast.h
)

set(UIS
    ui/LoginWindow.ui
    ui/managerwindow.ui
    ui/mainwindow.ui
    ui/mainwindow_incomplete.ui
)


# ----------------------------
# Create executable
# ----------------------------
qt_add_executable(BankingUI
    MANUAL_FINALIZATION
    ${SOURCES}
    ${HEADERS}
    ${UIS}
    resources.qrc
    include/appFlow.h
)


set_target_properties(BankingUI PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui"
)

# ----------------------------
# Include dirs
# ----------------------------
target_include_directories(BankingUI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}         
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${MYSQLCPPCONN_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)

# ----------------------------
# Link libs
# ----------------------------
target_link_libraries(BankingUI PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::Sql
    ${MYSQLCPPCONN_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    Boost::filesystem
    Boost::system
)

# ----------------------------
# Finalize Qt target
# ----------------------------
qt_finalize_executable(BankingUI)

set_target_properties(BankingUI PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# ----------------------------
# Debug output
# ----------------------------
message(STATUS "UI files: ${UIS}")
message(STATUS "MySQL include dirs: ${MYSQLCPPCONN_INCLUDE_DIRS}")
message(STATUS "MySQL lib: ${MYSQLCPPCONN_LIB}")
message(STATUS "OpenSSL include dirs: ${OPENSSL_INCLUDE_DIRS}")
message(STATUS "MySQL server dll: ${MYSQL_SERVER_DLL}")

# -------------------------------------------------------------
# DLL COPY (POST BUILD)
# -------------------------------------------------------------
function(copy_dll target_name source_dir dll_name)
    add_custom_command(
        TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${source_dir}/${dll_name}"
        "$<TARGET_FILE_DIR:${target_name}>"
    )
endfunction()

# 1) MySQL Connector/C++ DLL
set(MYSQL_DLL_DIR "${MYSQLCPPCONN_ROOT}/lib64")
copy_dll(BankingUI "${MYSQL_DLL_DIR}" "mysqlcppconn-10-vs14.dll")

# 1b) MySQL client DLL (libmysql.dll) - make it conditional so builds don't hard-fail
if(EXISTS "${MYSQL_SERVER_DLL}")
    add_custom_command(
        TARGET BankingUI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${MYSQL_SERVER_DLL}"
        "$<TARGET_FILE_DIR:BankingUI>"
    )
else()
    message(WARNING "libmysql.dll not found at: ${MYSQL_SERVER_DLL} (Skipping copy)")
endif()

# 2) OpenSSL DLLs (from vcpkg)
if(DEFINED VCPKG_INSTALLED_DIR)
  set(VCPKG_BIN_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/bin")
  set(VCPKG_DBG_BIN_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/debug/bin")

  add_custom_command(TARGET BankingUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<$<CONFIG:Debug>:${VCPKG_DBG_BIN_DIR}/libcrypto-3-x64.dll>
      $<$<NOT:$<CONFIG:Debug>>:${VCPKG_BIN_DIR}/libcrypto-3-x64.dll>
      $<TARGET_FILE_DIR:BankingUI>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<$<CONFIG:Debug>:${VCPKG_DBG_BIN_DIR}/libssl-3-x64.dll>
      $<$<NOT:$<CONFIG:Debug>>:${VCPKG_BIN_DIR}/libssl-3-x64.dll>
      $<TARGET_FILE_DIR:BankingUI>
  )
endif()


if(WIN32)
  set(WINDEPLOYQT "C:/Qt/6.9.1/msvc2022_64/bin/windeployqt.exe")

  add_custom_command(TARGET BankingUI POST_BUILD
    COMMAND ${WINDEPLOYQT}
      --debug
      --no-translations
      --no-system-d3d-compiler
      $<TARGET_FILE:BankingUI>
    COMMENT "Running windeployqt..."
    VERBATIM
  )
endif()

