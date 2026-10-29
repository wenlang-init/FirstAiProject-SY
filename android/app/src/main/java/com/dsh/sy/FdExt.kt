package com.dsh.sy

import android.content.ContentResolver
import android.database.Cursor
import android.net.Uri
import android.os.Build
import android.provider.DocumentsContract
import android.provider.OpenableColumns
import java.util.Locale

/** 以只读方式打开 uri，执行 block 期间 fd 保持有效，结束后自动关闭 */
inline fun <T> ContentResolver.withReadFd(uri: Uri, block: (Int) -> T): T =
    openFileDescriptor(uri, "r")!!.use { block(it.fd) }

/** 以只写方式打开 uri，执行 block 期间 fd 保持有效，结束后自动关闭 */
inline fun <T> ContentResolver.withWriteFd(uri: Uri, block: (Int) -> T): T =
    openFileDescriptor(uri, "w")!!.use { block(it.fd) }

/** 查询文件大小，失败返回 -1 */
fun ContentResolver.sizeOf(uri: Uri): Long =
    try {
        query(uri, null, null, null, null)?.use { c: Cursor ->
            val idx = c.getColumnIndex(OpenableColumns.SIZE)
            if (idx >= 0 && c.moveToFirst()) c.getLong(idx) else -1L
        } ?: -1L
    } catch (e: Exception) {
        -1L
    }

/** 查询文件显示名，失败返回 uri 本身 */
fun ContentResolver.nameOf(uri: Uri): String =
    try {
        query(uri, null, null, null, null)?.use { c: Cursor ->
            val idx = c.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (idx >= 0 && c.moveToFirst()) c.getString(idx) else uri.toString()
        } ?: uri.toString()
    } catch (e: Exception) {
        uri.toString()
    }

/**
 * 把耗时（毫秒）格式化成可读文本：`0.8 ms` / `3.45 s` / `1 分 5.20 秒`。
 * 用 Locale.US 固定小数点，避免系统语言把分隔符改成逗号。
 */
fun formatDuration(ms: Long): String = when {
    ms < 1L       -> "$ms ms"
    ms < 1_000L   -> String.format(Locale.US, "%.2f ms", ms.toDouble())
    ms < 60_000L  -> String.format(Locale.US, "%.2f s", ms / 1_000.0)
    else          -> String.format(Locale.US, "%d 分 %.2f 秒", ms / 60_000L, (ms % 60_000L) / 1_000.0)
}

/** 计时：执行 block，返回 (结果, 耗时毫秒) */
inline fun <T> timed(block: () -> T): Pair<T, Long> {
    val t0 = System.nanoTime()
    val r  = block()
    return r to (System.nanoTime() - t0) / 1_000_000L
}

/**
 * 查询文件在 SAF 中的显示路径（`Download/test.bin`）。
 * Android 11+ 拿不到真实绝对路径，这里用 DocumentsContract 回溯目录名拼出来；
 * 拿不到时退化为显示名。
 */
fun ContentResolver.pathOf(uri: Uri): String {
    val name = nameOf(uri)

    // 1) SAF 官方回溯（部分 provider 只返回文件名，此时继续走下面的兜底）
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        try {
            val segs = DocumentsContract.findDocumentPath(this, uri)?.path?.toMutableList()

            if (!segs.isNullOrEmpty()) {
                if (segs.last() != name) segs.add(name)
                val s = segs.joinToString("/")
                if (s.contains('/')) return s
            }
        } catch (e: Exception) {
            /* 继续兜底 */
        }
    }

    // 2) 外部存储 provider 的文档 id 自带相对路径：<rootId>:Download/a.bin
    val tail = uri.lastPathSegment

    if (!tail.isNullOrBlank() && tail.contains(':')) {
        val relPath = tail.substringAfter(':').trim('/')

        if (relPath.isNotEmpty()) {
            // 再补上存储卷名（内部存储 / SD 卡 / 共享文件 …），拿不到就只显示相对路径
            val vol = uri.authority?.let { rootTitleOf(it, tail.substringBefore(':')) }
            return if (vol.isNullOrEmpty()) relPath else "$vol/$relPath"
        }
    }

    // 3) 按 authority 补一个已知的根目录名
    val root = when (uri.authority) {
        "com.android.providers.downloads.documents" -> "Download"
        "com.android.providers.media.documents"     -> "Media"
        "com.android.externalstorage.documents"     -> "外部存储"
        else                                        -> null
    }

    return if (root != null) "$root/$name" else name
}

/** 查询存储卷（根）的显示名，如「内部存储」「MuMu 共享文件」；失败返回 null */
private fun ContentResolver.rootTitleOf(authority: String, rootId: String): String? =
    try {
        query(
            DocumentsContract.buildRootsUri(authority),
            arrayOf(DocumentsContract.Root.COLUMN_ROOT_ID, DocumentsContract.Root.COLUMN_TITLE),
            null, null, null
        )?.use { c ->
            val iId = c.getColumnIndex(DocumentsContract.Root.COLUMN_ROOT_ID)
            val iTi = c.getColumnIndex(DocumentsContract.Root.COLUMN_TITLE)
            var title: String? = null

            while (c.moveToNext()) {
                if (c.getString(iId) == rootId) {
                    title = c.getString(iTi)
                    break
                }
            }
            title
        }
    } catch (e: Exception) {
        null
    }

/** 吞吐：105.68 MB/s / 842.10 KB/s / 512 B/s */
fun formatThroughput(bytes: Long, ms: Long): String {
    val bps = bytes * 1_000.0 / ms

    return when {
        bps >= 1_048_576.0 -> String.format(Locale.US, "%.2f MB/s", bps / 1_048_576.0)
        bps >= 1_024.0     -> String.format(Locale.US, "%.2f KB/s", bps / 1_024.0)
        else               -> String.format(Locale.US, "%.0f B/s", bps)
    }
}

/** 统计行：`耗时 79.00 ms · 105.68 MB/s`（字节数或耗时为 0 时只显示耗时） */
fun formatCost(ms: Long, bytes: Long): String =
    if ((bytes > 0L) && (ms > 0L)) "耗时 ${formatDuration(ms)} · ${formatThroughput(bytes, ms)}"
    else "耗时 ${formatDuration(ms)}"
