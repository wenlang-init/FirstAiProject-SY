/*
 * syhash.cpp —— 直接内联 Qt 3rdparty 同款实现，保证与桌面版结果一致
 *   md4         : Qt qtbase/src/3rdparty/md4   (public domain)
 *   sha3/keccak : Qt qtbase/src/3rdparty/sha3  (CC0)
 *   blake2      : Qt qtbase/src/3rdparty/blake2(CC0)
 */
#include "syhash.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ---------------- MD4 ---------------- */
#include "3rd/md4/md4.h"
#include "3rd/md4/md4.cpp"

/* ---------------- SHA3 / Keccak ---------------- */
extern "C" {
#include "3rd/sha3/KeccakSponge.h"
#include "3rd/sha3/KeccakNISTInterface.h"
}
#include "3rd/sha3/KeccakSponge.c"
#include "3rd/sha3/KeccakNISTInterface.c"
#include "3rd/sha3/KeccakF-1600-opt64.c"

/* ---------------- BLAKE2 ---------------- */
extern "C" {
#include "3rd/blake2/blake2.h"
}
#include "3rd/blake2/blake2b-ref.c"
#include "3rd/blake2/blake2s-ref.c"

struct syhash_ctx {
    int alg;
    int outlen;

    union {
        struct md4_context md4;
        hashState          sha3;
        blake2b_state      b2b;
        blake2s_state      b2s;
    } u;
};

/*
 * FIPS 202 §6.1：SHA-3 = Keccak(message || "01")。
 * 本实现取字节的**最高 2 位**、按低位优先解释，故后缀字节为 0b10'00'00'00 = 0x80。
 * Keccak（非 SHA3）不加后缀。与 Qt 的 sha3Finish() 完全一致。
 */
static const unsigned char kSha3FinalSuffix = 0x80;

static int alg_is_sha3(int alg)
{
    return (alg >= SY_HASH_SHA3_224 && alg <= SY_HASH_SHA3_512);
}

static int alg_is_keccak(int alg)
{
    return (alg >= SY_HASH_KECCAK_224 && alg <= SY_HASH_KECCAK_512);
}

static int alg_is_blake2b(int alg)
{
    return (alg >= SY_HASH_BLAKE2B_160 && alg <= SY_HASH_BLAKE2B_512);
}

static int alg_is_blake2s(int alg)
{
    return (alg >= SY_HASH_BLAKE2S_128 && alg <= SY_HASH_BLAKE2S_256);
}

extern "C" int syhash_size(int alg)
{
    switch (alg) {
    case SY_HASH_MD4:
    case SY_HASH_MD5:
        return 16;

    case SY_HASH_SHA1:
        return 20;

    case SY_HASH_SHA224:
    case SY_HASH_SHA3_224:
    case SY_HASH_KECCAK_224:
    case SY_HASH_BLAKE2S_224:
        return 224 / 8;

    case SY_HASH_SHA256:
    case SY_HASH_SHA3_256:
    case SY_HASH_KECCAK_256:
    case SY_HASH_BLAKE2B_256:
    case SY_HASH_BLAKE2S_256:
        return 256 / 8;

    case SY_HASH_SHA384:
    case SY_HASH_SHA3_384:
    case SY_HASH_KECCAK_384:
    case SY_HASH_BLAKE2B_384:
        return 384 / 8;

    case SY_HASH_SHA512:
    case SY_HASH_SHA3_512:
    case SY_HASH_KECCAK_512:
    case SY_HASH_BLAKE2B_512:
        return 512 / 8;

    case SY_HASH_BLAKE2B_160:
    case SY_HASH_BLAKE2S_160:
        return 160 / 8;

    case SY_HASH_BLAKE2S_128:
        return 128 / 8;

    default:
        return 0;
    }
}

extern "C" int syhash_is_native(int alg)
{
    /* MD5/SHA1/SHA224/256/384/512 交给 Java MessageDigest */
    return (alg == SY_HASH_MD4) || alg_is_sha3(alg) || alg_is_keccak(alg)
           || alg_is_blake2b(alg) || alg_is_blake2s(alg);
}

extern "C" syhash_ctx_t *syhash_new(int alg)
{
    if (!syhash_is_native(alg))
        return NULL;

    const int size = syhash_size(alg);

    if (size <= 0)
        return NULL;

    syhash_ctx_t *c = (syhash_ctx_t *)calloc(1, sizeof(syhash_ctx_t));

    if (!c)
        return NULL;

    c->alg    = alg;
    c->outlen = size;

    if (alg == SY_HASH_MD4) {
        md4_init(&c->u.md4);
    } else if (alg_is_sha3(alg) || alg_is_keccak(alg)) {
        Init(&c->u.sha3, size * 8);
    } else if (alg_is_blake2b(alg)) {
        blake2b_init(&c->u.b2b, (size_t)size);
    } else if (alg_is_blake2s(alg)) {
        blake2s_init(&c->u.b2s, (size_t)size);
    }
    return c;
}

extern "C" void syhash_update(syhash_ctx_t *ctx, const void *data, size_t len)
{
    if (!ctx || !data || (len == 0))
        return;

    const int alg = ctx->alg;

    if (alg == SY_HASH_MD4) {
        md4_update(&ctx->u.md4, (const unsigned char *)data, len);
    } else if (alg_is_sha3(alg) || alg_is_keccak(alg)) {
        Update(&ctx->u.sha3, (const BitSequence *)data, (DataLength)len * 8);
    } else if (alg_is_blake2b(alg)) {
        blake2b_update(&ctx->u.b2b, data, len);
    } else if (alg_is_blake2s(alg)) {
        blake2s_update(&ctx->u.b2s, data, len);
    }
}

extern "C" char *syhash_final_hex(syhash_ctx_t *ctx)
{
    if (!ctx)
        return NULL;

    unsigned char out[64];
    memset(out, 0, sizeof(out));

    const int alg = ctx->alg;
    const int n   = ctx->outlen;

    if (alg == SY_HASH_MD4) {
        md4_final(&ctx->u.md4, out);
    } else if (alg_is_sha3(alg)) {
        Update(&ctx->u.sha3, &kSha3FinalSuffix, 2);
        Final(&ctx->u.sha3, out);
    } else if (alg_is_keccak(alg)) {
        Final(&ctx->u.sha3, out);
    } else if (alg_is_blake2b(alg)) {
        blake2b_final(&ctx->u.b2b, out, (size_t)n);
    } else if (alg_is_blake2s(alg)) {
        blake2s_final(&ctx->u.b2s, out, (size_t)n);
    } else {
        return NULL;
    }

    static const char tab[] = "0123456789abcdef";
    char *hex = (char *)malloc((size_t)n * 2 + 1);

    if (!hex)
        return NULL;

    for (int i = 0; i < n; i++) {
        hex[i * 2]     = tab[(out[i] >> 4) & 0x0F];
        hex[i * 2 + 1] = tab[out[i] & 0x0F];
    }
    hex[n * 2] = '\0';
    return hex;
}

extern "C" void syhash_free(syhash_ctx_t *ctx)
{
    free(ctx);
}
