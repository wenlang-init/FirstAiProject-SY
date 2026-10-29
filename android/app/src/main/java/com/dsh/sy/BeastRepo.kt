package com.dsh.sy

/**
 * 兽音编解码业务层。
 * 密令规则与桌面版一致：QString::toUcs4() 后必须是 4 个互不相同的码点
 * （emoji 等组合字符会被算作多个码点，因此不可用）。
 */
object BeastRepo {

    /** 返回 null 表示合法；否则返回错误提示 */
    fun checkKey(key: String): String? {
        val cps = key.codePoints().toArray()
        if (cps.size != 4) {
            return "秘钥为4个不同的字(注:不能是❤️这样的组合字符),$key"
        }
        if (cps.distinct().size != 4) {
            return "秘钥为4个不同的字(注:不能是❤️这样的组合字符),$key"
        }
        return null
    }

    fun encode(text: String, mode: Int, key: String): ByteArray? =
        NativeSy.beastEncode(text.toByteArray(Charsets.UTF_8), mode, key)

    fun decode(text: String, mode: Int): ByteArray? =
        NativeSy.beastDecode(text.toByteArray(Charsets.UTF_8), mode)
}
