cmake_minimum_required(VERSION 3.13)
project(AnonAI)

# Cross compilation settings for Raspberry Pi Zero W
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Optimization flags for Pi Zero W
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ffast-math -fno-exceptions -fno-rtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdata-sections -ffunction-sections -Wl,--gc-sections")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(nlohmann_json 3.9.1 REQUIRED)
find_package(Threads REQUIRED)

# Add executables
add_executable(anon anon.cpp)
add_executable(neural_network neural_network.cpp)
# Add pwnagotchi executable
add_executable(pwnagotchi pwnagotchi.cpp)

# Link libraries
target_link_libraries(anon PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(neural_network PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(pwnagotchi PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
target_compile_features(pwnagotchi PRIVATE cxx_std_17)
