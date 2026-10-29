/*
 * syhash.h —— 哈希算法统一封装
 *
 * 算法索引与桌面版 Qt 程序 comboBox 顺序严格一致（见 ShouYinW::getHashValue）。
 * 说明：MD5 / SHA-1 / SHA-224 / SHA-256 / SHA-384 / SHA-512 由 Kotlin 侧
 *       java.security.MessageDigest 计算（标准算法、平台原生、零风险），
 *       其余 17 种（MD4 / SHA3-* / Keccak-* / BLAKE2b-* / BLAKE2s-*）
 *       由本模块用 Qt 同款 C 源码计算，保证与桌面版结果逐字节一致。
 */
#ifndef SYHASH_H
#define SYHASH_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SY_HASH_MD4          0
#define SY_HASH_MD5          1
#define SY_HASH_SHA1         2
#define SY_HASH_SHA224       3
#define SY_HASH_SHA256       4
#define SY_HASH_SHA384       5
#define SY_HASH_SHA512       6
#define SY_HASH_SHA3_224     7
#define SY_HASH_SHA3_256     8
#define SY_HASH_SHA3_384     9
#define SY_HASH_SHA3_512    10
#define SY_HASH_KECCAK_224  11
#define SY_HASH_KECCAK_256  12
#define SY_HASH_KECCAK_384  13
#define SY_HASH_KECCAK_512  14
#define SY_HASH_BLAKE2B_160 15
#define SY_HASH_BLAKE2B_256 16
#define SY_HASH_BLAKE2B_384 17
#define SY_HASH_BLAKE2B_512 18
#define SY_HASH_BLAKE2S_128 19
#define SY_HASH_BLAKE2S_160 20
#define SY_HASH_BLAKE2S_224 21
#define SY_HASH_BLAKE2S_256 22
#define SY_HASH_COUNT       23

/* 摘要字节数；非法 alg 返回 0 */
int syhash_size(int alg);

/* 1 = 由本模块(native)计算；0 = 由 Java MessageDigest 计算 */
int syhash_is_native(int alg);

typedef struct syhash_ctx syhash_ctx_t;

/* 创建上下文，失败返回 NULL（alg 不由 native 计算时返回 NULL） */
syhash_ctx_t *syhash_new(int alg);

void syhash_update(syhash_ctx_t *ctx, const void *data, size_t len);

/* 结束计算并返回小写十六进制字符串（malloc 分配，调用方 free） */
char *syhash_final_hex(syhash_ctx_t *ctx);

void syhash_free(syhash_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* SYHASH_H */
