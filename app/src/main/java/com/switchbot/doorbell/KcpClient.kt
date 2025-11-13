package com.switchbot.doorbell

import android.util.Log
import java.nio.charset.StandardCharsets

object KcpClient {

init {
    System.loadLibrary("kcpwrapper")
}

private const val TAG = "KcpClient"

// ---- native 方法声明 ----
private external fun initKcp(remoteIp: String, remotePort: Int, conv: Int)
private external fun updateKcp(currentMs: Long)
private external fun sendData(data: ByteArray): Int
private external fun receiveData(): ByteArray?
private external fun releaseKcp()

// ---- KCP 运行状态 ----
@Volatile
private var running = false
private var recvThread: Thread? = null

// ---- 初始化（启动 KCP） ----
fun start(remoteIp: String, remotePort: Int, conv: Int) {
    if (running) return
            Log.d(TAG, "start: initKcp")
    initKcp(remoteIp, remotePort, conv)
    running = true

    recvThread = Thread {
        while (running) {
            try {
                updateKcp(System.currentTimeMillis())
                val recv = receiveData()
                if (recv != null && recv.isNotEmpty()) {
                    onReceive(recv)
                }
                Thread.sleep(30)
            } catch (e: Exception) {
                Log.e(TAG, "recvThread error", e)
            }
        }
    }.apply { name = "KcpRecvThread"; start() }
}

// ---- 停止（释放资源） ----
fun stop() {
    running = false
    recvThread?.let {
        try { it.join(100) } catch (_: InterruptedException) {}
        recvThread = null
    }
    releaseKcp()
}

// ---- 发送数据 ----
fun send(data: ByteArray) {
    if (!running) {
        Log.w(TAG, "KCP not started yet.")
        return
    }
    Log.d(TAG, "send: data $data")
    sendData(data)
}

// ---- 接收回调 ----
private val recvBuffer = mutableListOf<Byte>()
    private val FULL_AES_PACKAGE_SIZE = 56 * 4 // 224 字节完整 AES 包

    fun onReceive(data: ByteArray) {
        // 添加到缓冲区
        recvBuffer.addAll(data.toList())
        Log.d(TAG, "Buffer size=${recvBuffer.size}")

        // 只有当缓冲区达到完整包长度才解密
        while (recvBuffer.size >= FULL_AES_PACKAGE_SIZE) {
            val chunk = recvBuffer.take(FULL_AES_PACKAGE_SIZE).toByteArray()
            recvBuffer.subList(0, FULL_AES_PACKAGE_SIZE).clear() // 清除已处理数据

            try {
                val decrypted = AesCbcDecryptor.decrypt(
                    chunk,
                    "VQikblIrZXQ42Hng".toByteArray(Charsets.UTF_8),
                    "2FfVKcscXpylGLGT".toByteArray(Charsets.UTF_8)
                )
                Log.d(TAG, "Decrypted: $decrypted")
            } catch (e: Exception) {
                Log.e(TAG, "AES decryption error", e)
            }
        }
    }

}
