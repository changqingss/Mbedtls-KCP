package com.switchbot.doorbell

class Utils {
    companion object {
        @JvmStatic
        public fun hexStringToBytes(hex: String?): ByteArray {
            if (hex == null || hex.isEmpty()) return ByteArray(0)
            require(hex.length % 2 == 0) { "Hex string length must be even" }
            val result = ByteArray(hex.length / 2)
            var i = 0
            while (i < hex.length) {
                val high = hex[i].digitToIntOrNull(16)
                    ?: throw IllegalArgumentException("Invalid hex char: ${hex[i]}")
                val low = hex[i + 1].digitToIntOrNull(16)
                    ?: throw IllegalArgumentException("Invalid hex char: ${hex[i+1]}")
                result[i / 2] = ((high shl 4) or low).toByte()
                i += 2
            }
            return result
        }
    }
}