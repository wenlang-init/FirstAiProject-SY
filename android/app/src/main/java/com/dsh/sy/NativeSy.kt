package com.dsh.sy

/**
 * JNI 进度回调。
 * 由 native 工作线程调用，返回 true 继续、false 请求取消。
 * 注意：必须使用具体的 [ProgressCallback] 类，JNI 通过 GetMethodID 查找 onProgress。
 */
interface ProgressCb {
    fun onProgress(percent: Int): Boolean
}

class ProgressCallback(private val block: (Int) -> Boolean) : ProgressCb {
    override fun onProgress(percent: Int): Boolean = block(percent)
}

object NativeSy {

    init {
        System.loadLibrary("sy")
    }

    /** 请求取消当前 native 任务 */
    external fun nativeCancel()

    /**
     * 文本兽音编码
     * @param mode 1=编码1(unicode≤16bit) 2=编码2 3=编码3
     * @return 编码后的 UTF-8 字节；失败返回 null
     */
    external fun beastEncode(src: ByteArray, mode: Int, key: String): ByteArray?

    /** 文本兽音解码 */
    external fun beastDecode(src: ByteArray, mode: Int): ByteArray?

    /** 文件兽音编码（流式）。fd 来自 ParcelFileDescriptor */
    external fun beastFileEncode(inFd: Int, outFd: Int, key: String, cb: ProgressCb?): Boolean

    /** 文件兽音解码（流式） */
    external fun beastFileDecode(inFd: Int, outFd: Int, cb: ProgressCb?): Boolean

    /** 文件异或加解密 */
    external fun xorFile(inFd: Int, outFd: Int, encrypt: Boolean, cb: ProgressCb?): Boolean

    /** 数据哈希（仅 native 算法） */
    external fun hashData(data: ByteArray, alg: Int): String?

    /** 文件哈希（流式 + 进度 + 取消） */
    external fun hashFd(fd: Int, alg: Int, cb: ProgressCb?): String?
}
