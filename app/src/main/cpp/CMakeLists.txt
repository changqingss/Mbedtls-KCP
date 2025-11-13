cmake_minimum_required(VERSION 3.18)
project(kcpwrapper LANGUAGES C CXX)

# -----------------------------
# 1️⃣ 查找 Android 日志库
# -----------------------------
find_library(log-lib log)

# -----------------------------
# 2️⃣ MBEDTLS 静态库路径
# -----------------------------
set(MBEDTLS_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/lib/arm64-v8a)

# -----------------------------
# 3️⃣ 编译统一的 JNI + KCP 库
# -----------------------------
add_library(
        kcpwrapper
        SHARED
        # JNI 层
        kcp_wrapper.cpp
        tcp_jni.cpp
        # 逻辑层
        ikcp/ikcp.c
        common_sock.c
        repeater_aes.c
        tcp_client.c
        wo_aes.c
        aes_key_gen.c
)

# -----------------------------
# 4️⃣ include 路径
# -----------------------------
target_include_directories(
        kcpwrapper
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/include
        ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# -----------------------------
# 5️⃣ 链接依赖库
# -----------------------------
target_link_libraries(
        kcpwrapper
        PRIVATE
        ${log-lib}
        atomic
        m
        ${MBEDTLS_LIB_DIR}/libmbedtls.a
        ${MBEDTLS_LIB_DIR}/libmbedcrypto.a
        ${MBEDTLS_LIB_DIR}/libmbedx509.a
)

# -----------------------------
# 6️⃣ C++ 标准
# -----------------------------
set_target_properties(
        kcpwrapper
        PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)
