package com.switchbot.doorbell

import android.os.Handler
import android.os.Looper

class KcpManager(conv: Int, ip: String, port: Int) {
    private val kcp = KcpNative(conv, ip, port)
    private val handler = Handler(Looper.getMainLooper())

    private val updateRunnable = object : Runnable {
        override fun run() {
            val current = (System.currentTimeMillis() and 0xFFFFFFFF).toInt()
            kcp.update(current)
            kcp.recv()?.let { onReceive(it) }
            handler.postDelayed(this, 20)
        }
    }

    fun start() = handler.post(updateRunnable)
    fun stop() {
        handler.removeCallbacks(updateRunnable)
        kcp.release()
    }

    fun send(data: ByteArray) = kcp.send(data)

    private fun onReceive(data: ByteArray) {
        println("KCP 收到: ${String(data)}")
    }
}