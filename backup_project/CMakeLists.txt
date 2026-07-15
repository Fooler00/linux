cmake_minimum_required(VERSION 3.13)
project(backup_system CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 启用 HTTPS（传输加密）支持
add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

# 第三方头文件目录
include_directories(${CMAKE_SOURCE_DIR}/third_party)
# 公共头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 查找依赖
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(SQLite3 REQUIRED)

# ===== 核心库（独立，本地前端可直接链接调用）=====
add_library(backup_core STATIC
    src/core/Utils.cpp
    src/core/Metadata.cpp
    src/core/Filter.cpp
    src/core/Archive.cpp
    src/core/Crypto.cpp
    src/core/Incremental.cpp
    src/core/BackupEngine.cpp
    src/core/Scheduler.cpp
    src/core/UserManager.cpp
    src/core/TaskManager.cpp
    src/core/AuthManager.cpp
)
target_link_libraries(backup_core PUBLIC OpenSSL::SSL OpenSSL::Crypto Threads::Threads SQLite::SQLite3)
target_include_directories(backup_core PUBLIC include third_party)

# ===== 网盘接口库（含本地与远程实现）=====
add_library(backup_cloud STATIC
    src/cloud/LocalCloudStorage.cpp
    src/cloud/RemoteCloudStorage.cpp
    src/cloud/CloudConfig.cpp
)
target_link_libraries(backup_cloud PUBLIC backup_core)

# ===== HTTP 服务可执行文件 =====
add_executable(backup_server src/main.cpp src/server/BackupServer.cpp)
target_link_libraries(backup_server PRIVATE backup_cloud backup_core)

# ===== 本地前端示例（CLI，直接调用核心库，不依赖 HTTP）=====
add_executable(backup_cli src/cli.cpp)
target_link_libraries(backup_cli PRIVATE backup_core)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../Test"
        "${CMAKE_BINARY_DIR}/Test"
    )
endif()
