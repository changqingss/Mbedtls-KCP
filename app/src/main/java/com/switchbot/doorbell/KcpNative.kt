package com.switchbot.doorbell

import android.nfc.Tag
import android.util.Log
import kotlin.math.log

class KcpNative(conv: Int, ip: String, port: Int) {

    companion object {
        private val TAG: String="KcpNative"
        init {
            System.loadLibrary("kcp")         // ikcp.c 编译出的库
            System.loadLibrary("kcpwrapper")  // JNI 封装库
        }
    }

//    127.0.0.1

    // nativePtr 用于存储 C++ 层 KcpContext 指针
//    var nativePtr: Long = 0
    private var nativePtr: Long = createKcp(conv, ip, port)

    // -----------------------------
    // JNI 方法
    // -----------------------------
    external fun createKcp(conv: Int): Long
    external fun createKcp(conv: Int, ip: String, port: Int): Long
    external fun send(data: ByteArray)
    external fun recv(): ByteArray?
    external fun update(current: Int)
    external fun check(current: Int): Int
    external fun release()

    init {
        nativePtr =  createKcp(conv, ip, port)
        Log.d(TAG, "KcpNative created, nativePtr = $nativePtr ")
    }
}


