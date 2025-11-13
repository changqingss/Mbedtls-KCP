#include <jni.h>
#include <string>
#include <android/log.h>
#include "tcp_client.h"

#define LOG_TAG "KCP_NATIVE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// 辅助函数：把 jlong 转成 PTcpClient
static inline PTcpClient toClient(jlong handle) {
    return reinterpret_cast<PTcpClient>(handle);
}

// extern "C" 必须包裹所有 JNI 方法
extern "C" {

// 初始化 TCP 客户端，返回 native 指针
JNIEXPORT jlong JNICALL
Java_com_switchbot_doorbell_TcpClient_initTcpClient(JNIEnv *env, jobject thiz, jstring serverIp, jint port) {
    LOGD("init send len=%s ",serverIp);
    const char *ip = env->GetStringUTFChars(serverIp, nullptr);
    if (!ip) return 0;

    PTcpClient client = tcp_client_init(ip, port);

    env->ReleaseStringUTFChars(serverIp, ip);

    return reinterpret_cast<jlong>(client);
}

// 连接服务器
JNIEXPORT jint JNICALL
Java_com_switchbot_doorbell_TcpClient_connect(JNIEnv *env, jobject thiz, jlong handle) {
    PTcpClient client = toClient(handle);
    if (!client) return -1;
    return tcp_client_connect(client);
}

// 发送数据
JNIEXPORT jint JNICALL
Java_com_switchbot_doorbell_TcpClient_sendData(JNIEnv *env, jobject thiz, jlong handle, jbyteArray data) {
    PTcpClient client = toClient(handle);
    if (!client) return -1;

    jsize len = env->GetArrayLength(data);
    jbyte* bytes = env->GetByteArrayElements(data, nullptr);
    if (!bytes) return -1;

    int ret = tcp_client_send_data(client, reinterpret_cast<const char*>(bytes), len);

    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
    return ret;
}

// 接收数据
JNIEXPORT jint JNICALL
Java_com_switchbot_doorbell_TcpClient_recvData(JNIEnv *env, jobject thiz, jlong handle, jbyteArray buffer) {
    PTcpClient client = toClient(handle);
    if (!client) return -1;

    jsize buf_len = env->GetArrayLength(buffer);
    jbyte* buf = env->GetByteArrayElements(buffer, nullptr);
    if (!buf) return -1;

    int ret = tcp_client_recv_data(client, reinterpret_cast<char*>(buf), buf_len, buf_len);

    env->ReleaseByteArrayElements(buffer, buf, 0);
    return ret;
}

// 释放客户端资源
JNIEXPORT void JNICALL
Java_com_switchbot_doorbell_TcpClient_release(JNIEnv *env, jobject thiz, jlong handle) {
    PTcpClient client = toClient(handle);
    if (client) {
        tcp_client_deinit(client);
    }
}

// 发送 hello 包
JNIEXPORT jint JNICALL
Java_com_switchbot_doorbell_TcpClient_sendHello(JNIEnv *env, jobject thiz,
                                                jstring serverIp,
                                                jbyteArray mac,
                                                jbyteArray master,
                                                jbyteArray aesKey) {
    const char *ip = env->GetStringUTFChars(serverIp, nullptr);
    if (!ip) return -1;

    jbyte *macBytes = env->GetByteArrayElements(mac, nullptr);
    jbyte *masterBytes = env->GetByteArrayElements(master, nullptr);
    jbyte *keyBytes = env->GetByteArrayElements(aesKey, nullptr);

    if (!macBytes || !masterBytes || !keyBytes) {
        if (ip) env->ReleaseStringUTFChars(serverIp, ip);
        if (macBytes) env->ReleaseByteArrayElements(mac, macBytes, JNI_ABORT);
        if (masterBytes) env->ReleaseByteArrayElements(master, masterBytes, JNI_ABORT);
        if (keyBytes) env->ReleaseByteArrayElements(aesKey, keyBytes, JNI_ABORT);
        return -1;
    }

    // 调用底层 C 函数
    int ret = tcp_client_send_hello(
            ip,
            reinterpret_cast<const char*>(macBytes),
            reinterpret_cast<const char*>(masterBytes),
            reinterpret_cast<const char*>(keyBytes)
    );

    // 释放 JNI 资源
    env->ReleaseStringUTFChars(serverIp, ip);
    env->ReleaseByteArrayElements(mac, macBytes, JNI_ABORT);
    env->ReleaseByteArrayElements(master, masterBytes, JNI_ABORT);
    env->ReleaseByteArrayElements(aesKey, keyBytes, JNI_ABORT);

    LOGD("tcp_client_send_hello result=%d", ret);
    return ret;
}


} // extern "C"