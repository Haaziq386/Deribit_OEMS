cmake_minimum_required(VERSION 3.10)
project(DeribitOrderManagement)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find required packages
find_package(CURL REQUIRED)
find_package(websocketpp REQUIRED CONFIG) # Ensure CONFIG mode is used
find_package(Boost REQUIRED COMPONENTS system thread) # Ensure Boost::system and Boost::thread are available
find_package(nlohmann_json REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include) # Your project's include directory
include_directories(${Boost_INCLUDE_DIRS})         # Boost headers

include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v3.6.0
  GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(simdjson)
# Source files
file(GLOB SOURCES "src/*.cpp") # Include all .cpp files in src/

# Define executable
add_executable(deribit_order_management ${SOURCES})

# Link libraries
target_link_libraries(deribit_order_management PRIVATE 
    CURL::libcurl 
    websocketpp::websocketpp 
    Boost::system 
    Boost::thread 
    nlohmann_json::nlohmann_json
    simdjson
    ${OPENSSL_LIBRARIES}
)

# compiler options for better debugging and warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(deribit_order_management PRIVATE -Wall -Wextra -pedantic)
endif()