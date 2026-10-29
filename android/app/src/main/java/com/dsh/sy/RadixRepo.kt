package com.dsh.sy

/** 2–36 进制转换，等价于桌面版 ShouYinW::getBinary / getnumber */
object RadixRepo {

    fun parse(value: String, base: Int): ULong? {
        val s = value.trim()

        if (s.isEmpty()) return null
        if (base !in 2..36) return null

        return try {
            s.toULong(base)
        } catch (e: NumberFormatException) {
            null
        }
    }

    fun format(num: ULong, base: Int): String {
        if (base !in 2..36) return ""
        return if (base == 10) num.toString() else num.toString(base).uppercase()
    }
}
