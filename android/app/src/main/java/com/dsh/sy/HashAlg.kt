package com.dsh.sy

/**
 * 23 种哈希算法，index 与桌面版 Qt 的 comboBox 顺序严格一致。
 * [native] = true 表示走 JNI（Qt 同款 C 源码）；
 * [native] = false 表示走 java.security.MessageDigest（标准算法，平台原生实现）。
 */
enum class HashAlg(val index: Int, val label: String, val native: Boolean, val javaName: String?) {
    MD4(0, "MD4", true, null),
    MD5(1, "MD5", false, "MD5"),
    SHA_1(2, "SHA-1", false, "SHA-1"),
    SHA_224(3, "SHA-224", false, "SHA-224"),
    SHA_256(4, "SHA-256", false, "SHA-256"),
    SHA_384(5, "SHA-384", false, "SHA-384"),
    SHA_512(6, "SHA-512", false, "SHA-512"),
    SHA3_224(7, "SHA3-224", true, null),
    SHA3_256(8, "SHA3-256", true, null),
    SHA3_384(9, "SHA3-384", true, null),
    SHA3_512(10, "SHA3-512", true, null),
    KECCAK_224(11, "Keccak-224", true, null),
    KECCAK_256(12, "Keccak-256", true, null),
    KECCAK_384(13, "Keccak-384", true, null),
    KECCAK_512(14, "Keccak-512", true, null),
    BLAKE2B_160(15, "BLAKE2b-160", true, null),
    BLAKE2B_256(16, "BLAKE2b-256", true, null),
    BLAKE2B_384(17, "BLAKE2b-384", true, null),
    BLAKE2B_512(18, "BLAKE2b-512", true, null),
    BLAKE2S_128(19, "BLAKE2s-128", true, null),
    BLAKE2S_160(20, "BLAKE2s-160", true, null),
    BLAKE2S_224(21, "BLAKE2s-224", true, null),
    BLAKE2S_256(22, "BLAKE2s-256", true, null);

    companion object {
        fun of(index: Int): HashAlg = entries.first { it.index == index }
        val labels: List<String> get() = entries.map { it.label }
    }
}
