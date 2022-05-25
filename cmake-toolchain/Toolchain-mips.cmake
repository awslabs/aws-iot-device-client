# this one is important
SET(S2N_NO_PQ 1)
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)
# manually set desired system architecture
SET(CMAKE_SYSTEM_PROCESSOR mips)

variable_watch(PKG_CONFIG_PATH)
set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config")

# specify the cross compiler
SET(CMAKE_C_COMPILER   /usr/bin/mips-linux-gnu-gcc)
SET(CMAKE_CXX_COMPILER /usr/bin/mips-linux-gnu-g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /usr/lib/mips-linux-gnu;/usr/mips-linux-gnu)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH) 

SET(CMAKE_CROSSCOMPILING 1)

SET(S2N_NO_PQ 1)
set(CMAKE_FIND_DEBUG_MODE TRUE)

#Fix some wonkiness in the S2N build
SET(LibCrypto_SHARED_LIBRARY /usr/lib/mips-linux-gnu/lib/libcrypto.so)
SET(LibCrypto_STATIC_LIBRARY /usr/lib/mips-linux-gnu/lib/libcrypto.a)

include_directories(/usr/include)
