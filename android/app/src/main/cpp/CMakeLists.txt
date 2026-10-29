cmake_minimum_required(VERSION 3.22.1)
project("sy" C CXX)

set(CMAKE_C_STANDARD   99)
set(CMAKE_CXX_STANDARD 17)

# 兽音 / UTF-8 / 异或加解密：直接从桌面版移植，零改动
set(SY_CORE_SRC
        core/autf8.c
        core/beastSound.c
        core/edcrypt.c)

# 哈希封装（内部内联 Qt 同款 md4 / sha3 / blake2 源码）
set(SY_HASH_SRC
        hash/syhash.cpp)

add_library(sy SHARED
        sy_bridge.cpp
        ${SY_CORE_SRC}
        ${SY_HASH_SRC})

target_include_directories(sy PRIVATE
        core
        hash
        hash/3rd/md4
        hash/3rd/sha3
        hash/3rd/blake2)

# 需要 GBK 转换时打开，并把 unicode2gbktab.h 拷入 core/（会增加约 1.8MB）
# target_compile_definitions(sy PRIVATE SY_USE_GBK)

target_compile_options(sy PRIVATE
        -fno-strict-aliasing
        -Wno-unused-function
        -Wno-unused-variable)

target_link_libraries(sy log)
