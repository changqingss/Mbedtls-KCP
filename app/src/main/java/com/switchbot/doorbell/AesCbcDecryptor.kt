package com.switchbot.doorbell

import android.util.Base64
import java.nio.charset.Charset
import javax.crypto.Cipher
import javax.crypto.spec.IvParameterSpec
import javax.crypto.spec.SecretKeySpec

object AesCbcDecryptor {

    /**
     * 使用 AES-128-CBC 解密
     *
     * @param data 待解密数据（ByteArray）
     * @param key  AES key (16 字节)
     * @param iv   AES IV (16 字节)
     * @return     解密后的字符串，默认 UTF-8 编码
     */
    fun decrypt(data: ByteArray, key: ByteArray, iv: ByteArray, charset: Charset = Charsets.UTF_8): String {
        require(key.size == 16) { "AES key must be 16 bytes" }
        require(iv.size == 16) { "AES IV must be 16 bytes" }

        val secretKey = SecretKeySpec(key, "AES")
        val ivSpec = IvParameterSpec(iv)

        val cipher = Cipher.getInstance("AES/CBC/PKCS5Padding")
        cipher.init(Cipher.DECRYPT_MODE, secretKey, ivSpec)

        val decryptedBytes = cipher.doFinal(data)
        return String(decryptedBytes, charset)
    }

    /**
     * 如果数据是 Base64 字符串形式
     */
    fun decryptBase64(base64Data: String, key: ByteArray, iv: ByteArray, charset: Charset = Charsets.UTF_8): String {
        val bytes = Base64.decode(base64Data, Base64.NO_WRAP)
        return decrypt(bytes, key, iv, charset)
    }

}
