package com.dsh.sy

/**
 * 「生成代码 / 解码」——桌面版 src/usecode.cpp 的 Kotlin 等价实现。
 * 把文本用单字节异或混淆后，包进一段可直接编译运行的 C 源码。
 */
object UseCodeRepo {

    private const val KEYCORE = 0xC2

    /** 与 C 端 encrypt() 一致：逐字节异或 key，返回 key ^ KEYCORE */
    private fun encrypt(p: ByteArray, key: Int): Int {
        for (i in p.indices) {
            p[i] = ((p[i].toInt() and 0xFF) xor (key and 0xFF)).toByte()
        }
        return (key and 0xFF) xor KEYCORE
    }

    /** 与 C 端 decrypt() 一致 */
    private fun decrypt(p: ByteArray, key: Int) {
        val k = (key and 0xFF) xor KEYCORE
        for (i in p.indices) {
            p[i] = ((p[i].toInt() and 0xFF) xor k).toByte()
        }
    }

    fun getCode(text: String, randomKey: Boolean, keyHex: Int): String {
        val src = text.toByteArray(Charsets.UTF_8)

        if (src.isEmpty()) return ""

        val enkey = if (randomKey) (0..255).random() else (keyHex and 0xFF)
        val dekey = encrypt(src, enkey)

        val sb = StringBuilder()
        sb.append("#include <stdio.h>\n#include <string.h>\n")
        sb.append("void decrypt(unsigned char *p, unsigned int size, unsigned char key)\n{\n")
        sb.append("\tkey ^= 0x%02X;\n".format(KEYCORE))
        sb.append("\tfor(unsigned int i=0; i<size; i++)\n\t{\n\t\tp[i] ^= key;\n\t}\n}\n")
        sb.append("int main(){\n\t")
        sb.append("unsigned char encode[] = {\n\t")

        for (i in 0 until src.size - 1) {
            if ((i > 0) && (i % 10 == 0)) sb.append("\n\t")
            sb.append("0x%02x,".format(src[i].toInt() and 0xFF))
        }
        sb.append("0x%02x};\n".format(src[src.size - 1].toInt() and 0xFF))
        sb.append("\tunsigned char dekey = 0x%02X;\n".format(dekey and 0xFF))
        sb.append("\tdecrypt(encode,sizeof(encode),dekey);\n")
        sb.append("\tfor(int i=0;i<sizeof(encode);i++)printf(\"%%c\",encode[i]);printf(\"\\n\");\n")
        sb.append("\treturn 0;\n}\n\n")
        return sb.toString()
    }

    fun getDecode(text: String): String? {
        var index = text.indexOf("unsigned char dekey")

        if (index < 0) return null

        var indexEnd = text.indexOf(";", index)

        if ((indexEnd < 0) || (indexEnd - 4 < 0)) return null

        var hex = text.substring(indexEnd - 4, indexEnd).trim()

        if (hex.startsWith("0x", ignoreCase = true)) hex = hex.substring(2)

        val dekey = hex.toUIntOrNull(16) ?: return null

        index = text.indexOf("unsigned char encode")

        if (index < 0) return null

        index = text.indexOf("{", index)

        if (index < 1) return null

        indexEnd = text.indexOf("}", index)

        if (indexEnd < 2) return null

        val body = text.substring(index + 1, indexEnd)
            .replace("\n", "")
            .replace("\t", "")
            .replace("\r", "")
            .trim()

        if (body.isEmpty()) return null

        val bytes = ByteArray(body.split(",").size)
        var n = 0

        for (part in body.split(",")) {
            val v = part.trim().removePrefix("0x").removePrefix("0X").toUIntOrNull(16) ?: return null
            bytes[n++] = v.toByte()
        }

        val data = bytes.copyOf(n)
        decrypt(data, dekey.toInt())
        return String(data, Charsets.UTF_8)
    }
}
