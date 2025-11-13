package com.switchbot.doorbell;

import android.util.Log;

public class TcpClient {
    private static final String TAG = "TcpClient";

    // native 方法声明
    // 每个 native 方法的签名必须与 JNI 函数一一对应

    public native long initTcpClient(String serverIp, int port);
    public native int connect(long nativePtr);
    public native int sendData(long nativePtr, byte[] data);
    public native int recvData(long nativePtr, byte[] buffer);
    public native void release(long nativePtr);
    private native int sendHello(String serverIp, byte[] mac, byte[] master, byte[] aesKey);

    public native byte[] getLastAesKey();
    public native byte[] getLastAesIv();

    // 保存 native 指针
    private long nativePtr;

    // 对外封装方法，方便调用
    public void init(String serverIp, int port) {
        Log.d(TAG, "init: serverIp "+serverIp+" port "+port);
        nativePtr = initTcpClient(serverIp, port);
    }

    public int bindDevice(String serverIp, byte[] mac, byte[] master, byte[] aesKey) {
        Log.d(TAG, "bindDevice: serverIp "+serverIp);
        int result = sendHello(serverIp, mac, master, aesKey);
        Log.d(TAG, "bindDevice: result "+result);
        return result;
    }

    public int connect() {
        return connect(nativePtr);
    }

    public int sendData(byte[] data) {
        return sendData(nativePtr, data);
    }

    public int recvData(byte[] buffer) {
        return recvData(nativePtr, buffer);
    }

    public void release() {
        release(nativePtr);
        nativePtr = 0;
    }
    public byte[] getAesKey(){
        return getLastAesKey();
    }
    public byte[] getAesIv(){
        return getLastAesIv();
    }

    static {
//        System.loadLibrary("tcp_jni"); // 加载你的 JNI 库
        System.loadLibrary("kcpwrapper");
    }
}
