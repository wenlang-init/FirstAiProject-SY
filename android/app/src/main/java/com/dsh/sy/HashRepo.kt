package com.dsh.sy

import java.io.InputStream
import java.security.MessageDigest

object HashRepo {

    fun ByteArray.toHexString(): String = joinToString("") { "%02x".format(it) }

    /** 文本哈希 */
    fun ofText(data: ByteArray, alg: HashAlg): String {
        return if (alg.native) {
            NativeSy.hashData(data, alg.index) ?: error("native hash failed")
        } else {
            MessageDigest.getInstance(alg.javaName!!).digest(data).toHexString()
        }
    }

    /**
     * 流哈希（带进度、可取消）。
     * @param onProgress 返回 false 取消；返回 null 表示被取消
     */
    fun ofStream(
        openStream: () -> InputStream,
        totalSize: Long,
        alg: HashAlg,
        onProgress: (Int) -> Boolean
    ): String? {
        if (alg.native) {
            // native 分支由调用方直接传 fd，见 HashFragment
            error("use hashFd() for native algorithms")
        }
        val md = MessageDigest.getInstance(alg.javaName!!)
        val buf = ByteArray(1024 * 64)
        var read = 0L
        var lastTick = 0L
        openStream().use { input ->
            while (true) {
                val n = input.read(buf)
                if (n <= 0) break
                md.update(buf, 0, n)
                read += n
                val now = System.currentTimeMillis()
                if (now - lastTick > 1000) {
                    lastTick = now
                    val pct = if (totalSize > 0) (read * 100 / totalSize).toInt() else 0
                    if (!onProgress(pct)) return null
                }
            }
        }
        return md.digest().toHexString()
    }
}
