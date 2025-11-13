#include <jni.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "ikcp/ikcp.h"
#include "wo_aes.h"
#include <android/log.h>
#define LOG_TAG "KCP_NATIVE"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

static ikcpcb* kcp = nullptr;
static int udp_fd = -1;
static struct sockaddr_in remote_addr;

#define AES_KEY_SIZE 16


static aes_128_cbc_encrypo_t g_last_enc;
static bool g_has_aes_data = false;

// ✅ 提供给 repeater.c 调用
extern "C" void set_aes_key_iv(const unsigned char *key, const unsigned char *iv) {
    memcpy(g_last_enc.key, key, AES_KEY_SIZE);
    memcpy(g_last_enc.iv, iv, AES_KEY_SIZE);
    g_has_aes_data = true;
//    LOGD("AES key/iv updated from repeater.c key=%s iv=%s", key, iv);
    LOGD("AES key/iv updated from repeater.c key=%.*s iv=%.*s",
         AES_KEY_SIZE, key,
         AES_KEY_SIZE, iv);

}

// 输出回调函数（KCP 内部调用）
int udp_output(const char *buf, int len, ikcpcb *kcp, void *user)
{
    LOGD("udp_output send len=%d", len);
    if (udp_fd < 0) return -1;
    LOGD("udp_output send 2222222 ");
    if (udp_fd < 0) return -1;
    int ret = sendto(udp_fd, buf, len, 0, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
    if (ret < 0) {
        LOGD("sendto failed: %d", ret);
    }
    return 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_switchbot_doorbell_KcpClient_initKcp(JNIEnv *env, jobject thiz, jstring remote_ip, jint remote_port, jint conv)
{
    const char *ip = env->GetStringUTFChars(remote_ip, 0);

    LOGD("initKcp: remote_ip=%s port=%d conv=%d", ip, remote_port, conv);

    // 创建 UDP socket
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        LOGD("socket create failed errno=%d (%s)", errno, strerror(errno));
        env->ReleaseStringUTFChars(remote_ip, ip);
        return;
    }

    // 绑定本地端口
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(43210);  // 本地监听端口，可调整
    local_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(udp_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        LOGD("bind failed errno=%d (%s)", errno, strerror(errno));
    } else {
        LOGD("bind success on port 8888");
    }

    // 设置远程地址
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    inet_pton(AF_INET, ip, &remote_addr.sin_addr);

    // 初始化 KCP
    kcp = ikcp_create(conv, NULL);
    kcp->output = udp_output;

    ikcp_nodelay(kcp, 1, 10, 2, 1);
    ikcp_wndsize(kcp, 128, 128);

    env->ReleaseStringUTFChars(remote_ip, ip);
    LOGD("initKcp done.");
}

extern "C" JNIEXPORT void JNICALL
Java_com_switchbot_doorbell_KcpClient_updateKcp(JNIEnv *env, jobject thiz, jlong current)
{
    if (kcp) ikcp_update(kcp, (IUINT32)current);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_switchbot_doorbell_KcpClient_sendData(JNIEnv *env, jobject thiz, jbyteArray data)
{
    if (!kcp) return -1;
    jbyte *buf = env->GetByteArrayElements(data, nullptr);
    jsize len = env->GetArrayLength(data);
    int ret = ikcp_send(kcp, (const char *)buf, len);
    env->ReleaseByteArrayElements(data, buf, 0);
    return ret;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_switchbot_doorbell_KcpClient_receiveData(JNIEnv *env, jobject thiz)
{
    if (!kcp || udp_fd < 0) return nullptr;

    char buffer[1500];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    int n = recvfrom(udp_fd, buffer, sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*)&from, &fromlen);
    if (n > 0) {
        ikcp_input(kcp, buffer, n);
    }

    char kcp_buf[1500];
    int recv_len = ikcp_recv(kcp, kcp_buf, sizeof(kcp_buf));
    if (recv_len > 0) {
        jbyteArray result = env->NewByteArray(recv_len);
        env->SetByteArrayRegion(result, 0, recv_len, (jbyte*)kcp_buf);
        return result;
    }
    return nullptr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_switchbot_doorbell_TcpClient_getLastAesKey(JNIEnv *env, jobject thiz)
{
    if (!g_has_aes_data) {
        LOGD("No AES key available.");
        return nullptr;
    }
    jbyteArray keyArray = env->NewByteArray(AES_KEY_SIZE);
    env->SetByteArrayRegion(keyArray, 0, AES_KEY_SIZE, (jbyte*)g_last_enc.key);
    return keyArray;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_switchbot_doorbell_TcpClient_getLastAesIv(JNIEnv *env, jobject thiz)
{
    if (!g_has_aes_data) {
        LOGD("No AES iv available.");
        return nullptr;
    }
    jbyteArray ivArray = env->NewByteArray(AES_KEY_SIZE);
    env->SetByteArrayRegion(ivArray, 0, AES_KEY_SIZE, (jbyte*)g_last_enc.iv);
    return ivArray;
}



extern "C" JNIEXPORT void JNICALL
Java_com_switchbot_doorbell_KcpClient_releaseKcp(JNIEnv *env, jobject thiz)
{
    if (kcp) {
        ikcp_release(kcp);
        kcp = nullptr;
    }
    if (udp_fd >= 0) {
        close(udp_fd);
        udp_fd = -1;
    }
}
