/*
 * sy_bridge.cpp —— JNI 桥接层（com.dsh.sy.NativeSy）
 *
 * 对应桌面版 Qt 的 ShouYinW / FTDE / FuncHelper 中的业务调用。
 * 注意：Android 11+ 无法拿到任意文件的真实路径，所有文件类接口统一接收
 *       ParcelFileDescriptor 的 fd，native 侧 dup() 后用 fdopen 读写。
 */
#include <jni.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>

extern "C" {
#include "core/autf8.h"
#include "core/beastSound.h"
#include "core/edcrypt.h"
}

#include "hash/syhash.h"

#define SY_PKG "com/dsh/sy/"

/* ---------------- 全局状态 ---------------- */
static JavaVM            *g_vm     = nullptr;
static jobject           g_cb      = nullptr;   /* ProgressCb 全局引用 */
static std::atomic<int>   g_cancel{ 0 };
static std::mutex         g_fileMutex;          /* 兽音流式 API 有内部静态状态，必须串行 */

/* ---------------- 进度回调 ---------------- */
/*
 * Kotlin 侧接口：interface ProgressCb { fun onProgress(percent: Int): Boolean }
 *   返回 true  = 继续
 *   返回 false = 请求取消
 */
static void emitProgress(int percent)
{
    if (!g_vm || !g_cb)
        return;

    JNIEnv *env = nullptr;
    bool    attached = false;

    if (g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        /*
         * 注意：AttachCurrentThread 的签名在 Android NDK 与桌面 JDK 上不同：
         *   Android : jint AttachCurrentThread(JNIEnv **p_env, void *thr_args)
         *   JDK     : jint AttachCurrentThread(void **penv,   void *args)
         */
#ifdef __ANDROID__
        if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK)
#else
        if (g_vm->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr) != JNI_OK)
#endif
            return;
        attached = true;
    }

    jclass cls = env->GetObjectClass(g_cb);

    if (cls) {
        jmethodID mid = env->GetMethodID(cls, "onProgress", "(I)Z");

        if (mid) {
            jboolean cont = env->CallBooleanMethod(g_cb, mid, (jint)percent);

            if (cont == JNI_FALSE)
                g_cancel.store(1);
        }
        env->DeleteLocalRef(cls);
    }

    if (attached)
        g_vm->DetachCurrentThread();
}

/* 每秒最多回调一次，与桌面版 QTime 节流逻辑一致 */
static std::chrono::steady_clock::time_point g_lastTick;

static int shouldTick(long cur, long total)
{
    (void)cur; (void)total;
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastTick).count() >= 1000) {
        g_lastTick = now;
        return 1;
    }
    return 0;
}

/* ---------------- 工具函数 ---------------- */
static FILE *openFd(int fd, const char *mode)
{
    int d = dup(fd);

    if (d < 0)
        return nullptr;

    FILE *f = fdopen(d, mode);

    if (!f)
        close(d);
    return f;
}

/*
 * 解析 4 字密令：jstring(UTF-16) -> 4 个 Unicode 码点
 * 与桌面版 QString::toUcs4() 等价，正确处理代理对（emoji）
 */
static bool parseKey(JNIEnv *env, jstring key, unsigned long ucs4[4])
{
    if (!key)
        return false;

    const jchar *chars = env->GetStringChars(key, nullptr);

    if (!chars)
        return false;

    const jsize len = env->GetStringLength(key);
    std::vector<unsigned long> cps;

    for (jsize i = 0; i < len;) {
        unsigned long cp;

        if ((chars[i] >= 0xD800) && (chars[i] <= 0xDBFF) && ((i + 1) < len) &&
            (chars[i + 1] >= 0xDC00) && (chars[i + 1] <= 0xDFFF)) {
            cp = 0x10000UL + (((unsigned long)(chars[i] - 0xD800) << 10) |
                              (unsigned long)(chars[i + 1] - 0xDC00));
            i += 2;
        } else {
            cp = (unsigned long)chars[i];
            i += 1;
        }
        cps.push_back(cp);
    }
    env->ReleaseStringChars(key, chars);

    if (cps.size() != 4)
        return false;

    for (size_t i = 0; i < 4; i++)
        for (size_t j = i + 1; j < 4; j++)
            if (cps[i] == cps[j])
                return false;

    for (int i = 0; i < 4; i++)
        ucs4[i] = cps[i];
    return true;
}

/* 码点数组 -> 兽音字典（UTF-8），与桌面版 UTF8_putc 逻辑一致 */
static bool buildDict(const unsigned long ucs4[4], std::string dictqb[4], const char *dict[4])
{
    for (int i = 0; i < 4; i++) {
        char utf8[7];
        int  cnt = UTF8_putc((unsigned char *)utf8, 6, ucs4[i]);

        if (cnt < 1)
            return false;

        dictqb[i].assign(utf8, (size_t)cnt);
        dictqb[i].push_back('\0');
        dict[i] = dictqb[i].c_str();
    }
    return true;
}

/* ---------------- JNI 实现 ---------------- */
extern "C" {

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *)
{
    g_vm = vm;
    return JNI_VERSION_1_6;
}

/* 取消当前任务 */
JNIEXPORT void JNICALL Java_com_dsh_sy_NativeSy_nativeCancel(JNIEnv *, jclass)
{
    g_cancel.store(1);
}

/*
 * 文本兽音编码
 * mode: 1 = 2byte（兼容公用编码）, 2 = 4byte, 3 = 通用（4 倍膨胀）
 * 返回 null 表示失败（密令非法 / 含不支持的字符）
 */
JNIEXPORT jbyteArray JNICALL Java_com_dsh_sy_NativeSy_beastEncode(
    JNIEnv *env, jclass, jbyteArray src, jint mode, jstring key)
{
    unsigned long ucs4[4];

    if (!parseKey(env, key, ucs4))
        return nullptr;

    if (!src)
        return nullptr;

    const jsize srcLen = env->GetArrayLength(src);

    if (srcLen <= 0)
        return nullptr;

    jbyte *srcBuf = env->GetByteArrayElements(src, nullptr);

    if (!srcBuf)
        return nullptr;

    std::string dictqb[4];
    const char *dict[4];

    if (!buildDict(ucs4, dictqb, dict)) {
        env->ReleaseByteArrayElements(src, srcBuf, JNI_ABORT);
        return nullptr;
    }

    char *p = nullptr;

    switch (mode) {
    case 1:
    {
        /* 2byte 模式要求字典字符的 unicode <= 0xFFFF */
        for (int i = 0; i < 4; i++) {
            unsigned long val;

            if (UTF8_getc((const unsigned char *)dict[i], (int)strlen(dict[i]), &val) < 0) {
                env->ReleaseByteArrayElements(src, srcBuf, JNI_ABORT);
                return nullptr;
            }
            if (val > 0xFFFF) {
                env->ReleaseByteArrayElements(src, srcBuf, JNI_ABORT);
                return nullptr;
            }
        }
        p = toBeastSound_2byte((const unsigned char *)srcBuf, (int)srcLen, dict);
        break;
    }

    case 2:
        p = toBeastSound_4byte((const unsigned char *)srcBuf, (int)srcLen, dict);
        break;

    case 3:
        p = toBeastSound((const char *)srcBuf, (int)srcLen, dict);
        break;

    default:
        break;
    }
    env->ReleaseByteArrayElements(src, srcBuf, JNI_ABORT);

    if (!p)
        return nullptr;

    const size_t n = strlen(p);
    jbyteArray  out = env->NewByteArray((jsize)n);

    if (out)
        env->SetByteArrayRegion(out, 0, (jsize)n, (const jbyte *)p);
    free(p);
    return out;
}

/* 文本兽音解码 */
JNIEXPORT jbyteArray JNICALL Java_com_dsh_sy_NativeSy_beastDecode(
    JNIEnv *env, jclass, jbyteArray src, jint mode)
{
    if (!src)
        return nullptr;

    const jsize srcLen = env->GetArrayLength(src);

    if (srcLen <= 0)
        return nullptr;

    jbyte *srcBuf = env->GetByteArrayElements(src, nullptr);

    if (!srcBuf)
        return nullptr;

    char  *p    = nullptr;
    size_t n    = 0;
    int    size = 0;

    switch (mode) {
    case 1:
        p = fromBeastSound_2byte((const unsigned char *)srcBuf, (int)srcLen);
        if (p) n = strlen(p);
        break;

    case 2:
        p = fromBeastSound_4byte((const unsigned char *)srcBuf, (int)srcLen);
        if (p) n = strlen(p);
        break;

    case 3:
        p = fromBeastSound((const char *)srcBuf, (int)srcLen, &size);
        if (p) n = (size_t)size;
        break;

    default:
        break;
    }
    env->ReleaseByteArrayElements(src, srcBuf, JNI_ABORT);

    if (!p)
        return nullptr;

    jbyteArray out = env->NewByteArray((jsize)n);

    if (out)
        env->SetByteArrayRegion(out, 0, (jsize)n, (const jbyte *)p);
    free(p);
    return out;
}

/* 文件兽音编码（流式）。inFd/outFd 来自 ParcelFileDescriptor */
JNIEXPORT jboolean JNICALL Java_com_dsh_sy_NativeSy_beastFileEncode(
    JNIEnv *env, jclass, jint inFd, jint outFd, jstring key, jobject cb)
{
    std::lock_guard<std::mutex> lk(g_fileMutex);

    unsigned long ucs4[4];

    if (!parseKey(env, key, ucs4))
        return JNI_FALSE;

    std::string dictqb[4];
    const char *dict[4];

    if (!buildDict(ucs4, dictqb, dict))
        return JNI_FALSE;

    FILE *fin  = openFd(inFd, "rb");
    FILE *fout = openFd(outFd, "wb");

    if (!fin || !fout) {
        if (fin)  fclose(fin);
        if (fout) fclose(fout);
        return JNI_FALSE;
    }

    jobject old = g_cb;

    if (cb)
        g_cb = env->NewGlobalRef(cb);
    else
        g_cb = nullptr;
    g_cancel.store(0);
    g_lastTick = std::chrono::steady_clock::now();

    fseek(fin, 0, SEEK_END);
    const long fileSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char  *p = toBeastSoundStreamStart(dict);
    jboolean ok = JNI_TRUE;

    if (p) {
        fwrite(p, 1, strlen(p), fout);
        free(p);
    } else {
        ok = JNI_FALSE;
    }

    char  buff[1024];
    long  ret = 0;

    while (ok && (ret < fileSize)) {
        if (g_cancel.load()) {
            ok = JNI_FALSE;
            break;
        }
        size_t got = fread(buff, 1, sizeof(buff), fin);

        if (got == 0)
            break;

        p = toBeastSoundStream(buff, (int)got, dict);

        if (!p) {
            ok = JNI_FALSE;
            break;
        }
        fwrite(p, 1, strlen(p), fout);
        free(p);
        ret += (long)got;

        if (fileSize > 0)
            if (shouldTick(ret, fileSize))
                emitProgress((int)(ret * 100 / fileSize));
    }

    fclose(fin);
    fclose(fout);

    if (g_cb) {
        env->DeleteGlobalRef(g_cb);
        g_cb = nullptr;
    }
    (void)old;
    return ok;
}

/* 文件兽音解码（流式） */
JNIEXPORT jboolean JNICALL Java_com_dsh_sy_NativeSy_beastFileDecode(
    JNIEnv *env, jclass, jint inFd, jint outFd, jobject cb)
{
    std::lock_guard<std::mutex> lk(g_fileMutex);

    FILE *fin  = openFd(inFd, "rb");
    FILE *fout = openFd(outFd, "wb");

    if (!fin || !fout) {
        if (fin)  fclose(fin);
        if (fout) fclose(fout);
        return JNI_FALSE;
    }

    if (cb)
        g_cb = env->NewGlobalRef(cb);
    else
        g_cb = nullptr;
    g_cancel.store(0);
    g_lastTick = std::chrono::steady_clock::now();

    fseek(fin, 0, SEEK_END);
    const long fileSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    unsigned long dict_unicode[4];
    const int     readsize = 1024;
    char          buff[1024 * 2];
    long          ret        = 0;
    int           finish_len = 0;
    int           index      = 0;
    bool          isstart    = false;
    jboolean      ok         = JNI_TRUE;

    while (ok && (ret < fileSize)) {
        if (g_cancel.load()) {
            ok = JNI_FALSE;
            break;
        }

        if (finish_len >= readsize) {
            ok = JNI_FALSE;
            break;
        }
        size_t got = fread(buff + finish_len, 1, (size_t)(readsize - finish_len), fin);

        if (got == 0)
            break;

        ret += (long)got;
        int datasize = (int)got + finish_len;

        if (!isstart) {
            int finishsize = fromBeastSoundStreamStart(buff, datasize, &dict_unicode);

            if (finishsize < 0) {
                ok = JNI_FALSE;
                break;
            }
            memmove(buff, buff + finishsize, (size_t)(datasize - finishsize));
            datasize -= finishsize;
            isstart = true;
        }

        int   finishsize = 0;
        int   dstsize    = 0;
        char *p = fromBeastSoundStream(buff, datasize, dict_unicode, &dstsize, &finishsize, &index);

        if (!p) {
            ok = JNI_FALSE;
            break;
        }
        fwrite(p, 1, (size_t)dstsize, fout);
        free(p);

        finish_len = datasize - finishsize;
        memmove(buff, buff + finishsize, (size_t)(datasize - finishsize));

        if (fileSize > 0)
            if (shouldTick(ret, fileSize))
                emitProgress((int)(ret * 100 / fileSize));
    }

    fclose(fin);
    fclose(fout);

    if (g_cb) {
        env->DeleteGlobalRef(g_cb);
        g_cb = nullptr;
    }
    return ok;
}

/* 文件 XOR 加解密（encryptbs / decryptbs） */
JNIEXPORT jboolean JNICALL Java_com_dsh_sy_NativeSy_xorFile(
    JNIEnv *env, jclass, jint inFd, jint outFd, jboolean encrypt, jobject cb)
{
    FILE *fin  = openFd(inFd, "rb");
    FILE *fout = openFd(outFd, "wb");

    if (!fin || !fout) {
        if (fin)  fclose(fin);
        if (fout) fclose(fout);
        return JNI_FALSE;
    }

    if (cb)
        g_cb = env->NewGlobalRef(cb);
    else
        g_cb = nullptr;
    g_cancel.store(0);
    g_lastTick = std::chrono::steady_clock::now();

    fseek(fin, 0, SEEK_END);
    const long fileSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char     buff[1024 * 2];
    char     key  = 0;
    long     cnt  = 0;
    jboolean ok   = JNI_TRUE;

    while (ok && (cnt < fileSize)) {
        if (g_cancel.load()) {
            ok = JNI_FALSE;
            break;
        }
        size_t got = fread(buff, 1, sizeof(buff), fin);

        if (got == 0)
            break;

        if (encrypt == JNI_TRUE)
            encryptbs(buff, (int)got, &key);
        else
            decryptbs(buff, (int)got, &key);

        fwrite(buff, 1, got, fout);
        cnt += (long)got;

        if (fileSize > 0)
            if (shouldTick(cnt, fileSize))
                emitProgress((int)(cnt * 100 / fileSize));
    }

    fclose(fin);
    fclose(fout);

    if (g_cb) {
        env->DeleteGlobalRef(g_cb);
        g_cb = nullptr;
    }
    return ok;
}

/* 数据哈希（仅用于 native 算法；MD5/SHA1/SHA2 由 Kotlin MessageDigest 计算） */
JNIEXPORT jstring JNICALL Java_com_dsh_sy_NativeSy_hashData(
    JNIEnv *env, jclass, jbyteArray data, jint alg)
{
    if (!data || !syhash_is_native((int)alg))
        return nullptr;

    const jsize len = env->GetArrayLength(data);

    jbyte *buf = env->GetByteArrayElements(data, nullptr);

    if (!buf)
        return nullptr;

    syhash_ctx_t *ctx = syhash_new((int)alg);

    if (!ctx) {
        env->ReleaseByteArrayElements(data, buf, JNI_ABORT);
        return nullptr;
    }
    syhash_update(ctx, buf, (size_t)len);
    env->ReleaseByteArrayElements(data, buf, JNI_ABORT);

    char *hex = syhash_final_hex(ctx);
    syhash_free(ctx);

    if (!hex)
        return nullptr;

    jstring out = env->NewStringUTF(hex);
    free(hex);
    return out;
}

/* 文件哈希（流式 + 进度 + 取消） */
JNIEXPORT jstring JNICALL Java_com_dsh_sy_NativeSy_hashFd(
    JNIEnv *env, jclass, jint fd, jint alg, jobject cb)
{
    if (!syhash_is_native((int)alg))
        return nullptr;

    FILE *fin = openFd(fd, "rb");

    if (!fin)
        return nullptr;

    if (cb)
        g_cb = env->NewGlobalRef(cb);
    else
        g_cb = nullptr;
    g_cancel.store(0);
    g_lastTick = std::chrono::steady_clock::now();

    fseek(fin, 0, SEEK_END);
    const long fileSize = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    syhash_ctx_t *ctx = syhash_new((int)alg);

    if (!ctx) {
        fclose(fin);
        if (g_cb) {
            env->DeleteGlobalRef(g_cb);
            g_cb = nullptr;
        }
        return nullptr;
    }

    char  buff[1024 * 64];
    long  cnt = 0;
    int   cancelled = 0;

    while (cnt < fileSize) {
        if (g_cancel.load()) {
            cancelled = 1;
            break;
        }
        size_t got = fread(buff, 1, sizeof(buff), fin);

        if (got == 0)
            break;

        syhash_update(ctx, buff, got);
        cnt += (long)got;

        if (fileSize > 0)
            if (shouldTick(cnt, fileSize))
                emitProgress((int)(cnt * 100 / fileSize));
    }

    fclose(fin);

    char *hex = cancelled ? nullptr : syhash_final_hex(ctx);
    syhash_free(ctx);

    if (g_cb) {
        env->DeleteGlobalRef(g_cb);
        g_cb = nullptr;
    }

    if (!hex)
        return nullptr;

    jstring out = env->NewStringUTF(hex);
    free(hex);
    return out;
}

} /* extern "C" */
