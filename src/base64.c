#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "base64.h"

//static void bp (void) { puts("BP"); }

static const char E[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Compute offsets (+1) in E (also works as a truth table for is_base64().) */
static const size_t D[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  63, 0,  0,  0,  64,  /* +,/ */
    53, 54, 55, 56, 57, 58, 59, 60,  /* 0-7 */
    61, 62, 0,  0,  0,  0,  0,  0,   /* 8,9 */
    0,  1,  2,  3,  4,  5,  6,  7,   /* A-G */
    8,  9,  10, 11, 12, 13, 14, 15,  /* H-O */
    16, 17, 18, 19, 20, 21, 22, 23,  /* P-W */
    24, 25, 26, 0,  0,  0,  0,  0,   /* X-Z */
    0,  27, 28, 29, 30, 31, 32, 33,  /* a-g */
    34, 35, 36, 37, 38, 39, 40, 41,  /* h-o */
    42, 43, 44, 45, 46, 47, 48, 49,  /* p-w */
    50, 51, 52, 0,  0,  0,  0,  0    /* x-z */
};

static inline size_t val (int c);
static inline int is_base64 (int c);
static inline void chunk_encode (uint8_t in[3], char out[4], int len);
static inline void chunk_decode (char in[4], uint8_t out[3]);

/* IN: binary data, OUT: B64 string (possibly NUL-terminated). */
int base64_encode (const uint8_t *in, size_t in_sz, char *out, size_t out_sz)
{
    size_t i, len;
    uint8_t buf[3]; 
    char *pout;

    for (pout = out; in_sz; )
    {
        /* Get three bytes from 'in'. */
        for (len = 0, i = 0; i < 3; ++i)
        {
            if (in_sz && in_sz-- > 0)   /* Avoid wrapping around in_sz. */
            {
                buf[i] = *in++;
                ++len;
            }
            else
                buf[i] = '\0';
        }

        /* See if we've harvested enough data to call the block encoder. */
        if (len)
        {
            /* See if there's room at the output buffer to receive. */
            if (out_sz >= 4)
            {
                chunk_encode(buf, pout, len);
                out_sz -= 4;
                pout += 4;
            }
            else
                return -1;
        }
    }

    /* Possibly NUL-terminate out string. */
    if (out_sz)
        *pout = '\0';

    return 0;
}

int base64_decode (const char *in, size_t in_sz, uint8_t *out, size_t *out_sz)
{
    char buf[4];
    size_t i, len, pad, tot_sz = *out_sz;
    uint8_t *pout;

    for (pout = out; in_sz; )
    {
        for (pad = 0, len = 0, i = 0; i < 4; ++i)
        {
            if (in_sz && in_sz-- > 0)   /* Avoid wrapping around in_sz. */
            {
                buf[i] = *in++;

                if (buf[i] == '=')
                    ++pad;
                else if (!is_base64(buf[i]))    
                    return -1;

                ++len;
            }
            else
                buf[i] = '\0';
        }

        if (len)
        {
            size_t nlen = 3 - pad;

            if (tot_sz >= nlen)
            {
                chunk_decode(buf, pout);
                tot_sz -= nlen; /* Take care of subtracting pad bytes. */
                pout += nlen;
            } 
            else
                return -1;  /* Not enough space. */
        }
    }

    *out_sz -= tot_sz;

    return 0;
}

static inline int is_base64 (int c) 
{
    return D[c];
}

static inline size_t val (int c) 
{
    return (D[c] - 1);
}

static inline void chunk_encode (uint8_t in[3], char out[4], int len)
{
    out[0] = E[in[0] >> 2];
    out[1] = E[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
    out[2] = len > 1 ? E[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)] : '=';
    out[3] = len > 2 ? E[in[2] & 0x3f] : '=';
}

static inline void chunk_decode (char in[4], uint8_t out[3])
{   
    out[0] = (val(in[0]) << 2) | (val(in[1]) >> 4);
    out[1] = ((val(in[1]) & 0x0f) << 4) | (val(in[2]) >> 2);
    out[2] = ((val(in[2]) & 0x03) << 6) | val(in[3]);
}
