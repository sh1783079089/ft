#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tracker.h"

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static uint32_t rotr32(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22))
#define EP1(x) (rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25))
#define SIG0(x) (rotr32(x, 7) ^ rotr32(x, 18) ^ ((x) >> 3))
#define SIG1(x) (rotr32(x, 17) ^ rotr32(x, 19) ^ ((x) >> 10))

typedef struct {
    uint8_t buf[64];
    uint32_t state[8];
    uint64_t count;
} sha256_ctx;

static void sha256_init(sha256_ctx* ctx) {
    ctx->count = 0;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

static void sha256_transform(sha256_ctx* ctx, const uint8_t* data) {
    uint32_t a, b, c, d, e, f, g, h, t1, t2, m[64];
    int i, j;

    for (i = 0, j = 0; i < 16; i++, j += 4) {
        m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) |
               ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
    }
    for (; i < 64; i++) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

static void sha256_update(sha256_ctx* ctx, const uint8_t* data, size_t len) {
    size_t i;

    for (i = 0; i < len; i++) {
        ctx->buf[ctx->count % 64] = data[i];
        ctx->count++;
        if (ctx->count % 64 == 0) {
            sha256_transform(ctx, ctx->buf);
        }
    }
}

static void sha256_final(sha256_ctx* ctx, uint8_t digest[32]) {
    uint64_t bit_len;
    int i;

    i = (int)(ctx->count % 64);
    ctx->buf[i] = (uint8_t)0x80;
    i++;
    while (i < 64) {
        if (i == 56) break;
        ctx->buf[i] = 0x00;
        i++;
    }

    if (i > 56) {
        while (i < 64) {
            ctx->buf[i] = 0x00;
            i++;
        }
        sha256_transform(ctx, ctx->buf);
        i = 0;
        while (i < 56) {
            ctx->buf[i] = 0x00;
            i++;
        }
    }

    bit_len = ctx->count * 8;
    ctx->buf[56] = (uint8_t)(bit_len >> 56);
    ctx->buf[57] = (uint8_t)(bit_len >> 48);
    ctx->buf[58] = (uint8_t)(bit_len >> 40);
    ctx->buf[59] = (uint8_t)(bit_len >> 32);
    ctx->buf[60] = (uint8_t)(bit_len >> 24);
    ctx->buf[61] = (uint8_t)(bit_len >> 16);
    ctx->buf[62] = (uint8_t)(bit_len >> 8);
    ctx->buf[63] = (uint8_t)(bit_len);
    sha256_transform(ctx, ctx->buf);

    for (i = 0; i < 8; i++) {
        digest[i * 4] = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

int path_analysis(char* path, char* tracked_file,
                  char* working_dir) {  // 目前要求绝对路径
    int i, gap = 0, j, length = strlen(path);
    for (i = 0; i < length; i++) {
        if (path[i] == '"') {
            for (j = i; j < length; j++) {
                path[j] = path[j + 1];
            }
            i--;
            length--;
        }
        if (path[i] == '/') path[i] = '\\';
        if (path[i] == '\\') gap = i;
    }
    if (path[gap] != '\\') return 0;
    for (i = 0; i < gap; i++) working_dir[i] = path[i];
    working_dir[i] = '\0';
    for (i = gap + 1; i < length; i++) tracked_file[i - gap - 1] = path[i];
    tracked_file[i - gap - 1] = '\0';
    return 1;
}

void hash_data(const unsigned char* data, long size, char hash[HASH_LEN]) {
    sha256_ctx ctx;
    uint8_t digest[32];
    int i;

    sha256_init(&ctx);
    sha256_update(&ctx, data, (size_t)size);
    sha256_final(&ctx, digest);

    for (i = 0; i < 32; i++) {
        snprintf(hash + i * 2, 3, "%02x", digest[i]);
    }
}

int storage_text_is_safe(const char* text) {
    return text != NULL && strchr(text, '|') == NULL;
}

int choice_check() {
    int choice;
    int ch;
    printf("---------------------------------\n");
    while (1) {
        printf("请输入：");
        if (scanf("%d", &choice) == 1) {
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
            printf("---------------------------------\n");
            return choice;
        }
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        if (feof(stdin)) {
            printf("错误：输入流已关闭，程序现在退出！\n");
            exit(10);
        }
        if (ferror(stdin)) {
            printf("错误: 输入流读取失败！程序现在退出！\n");
            exit(11);
        }
        printf("输入不合法！");
    }
}

const char* static_short_hash(const char* hash) {
    static char buf[HASH_DISPLAY_LEN + 1] = "";
    if (hash == NULL) return "";
    strncpy(buf, hash, HASH_DISPLAY_LEN);
    buf[HASH_DISPLAY_LEN] = '\0';
    return buf;
}
