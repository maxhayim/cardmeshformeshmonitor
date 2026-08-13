cmake_minimum_required(VERSION 3.16)
project(cardmesh CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)

add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/third_party)

add_library(cardmesh_core STATIC
    src/api/HttpClient.cpp
    src/api/MeshMonitorClient.cpp
    src/storage/Settings.cpp
    src/storage/Database.cpp
)

target_include_directories(cardmesh_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(cardmesh_core PUBLIC CURL::libcurl SQLite3::SQLite3 nlohmann_json)

add_executable(cardmesh src/main.cpp)
target_link_libraries(cardmesh PRIVATE cardmesh_core)

enable_testing()
add_subdirectory(tests)
