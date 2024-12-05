set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Where to look for the target environment
set(CMAKE_FIND_ROOT_PATH /usr/arm-linux-gnueabihf)

# Search for programs only in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers only in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Raspberry Pi Zero W specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -march=armv6zk")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mcpu=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -march=armv6zk")

# Optimization flags
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
