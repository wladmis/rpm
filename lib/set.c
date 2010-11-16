/*
 * set.c - base62, golomb and set-string routines
 *
 * Copyright (C) 2010  Alexey Tourbin <at@altlinux.org>
 *
 * License: GPLv2+ or LGPL, see RPM COPYING
 */

#ifdef SELF_TEST
#undef NDEBUG
#include <stdio.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <assert.h>

/*
 * Base62 routines - encode bits with alnum characters.
 *
 * This is a base64-based base62 implementation.  Values 0..61 are encoded
 * with '0'..'9', 'a'..'z', and 'A'..'Z'.  However, 'Z' is special: it will
 * also encode 62 and 63.  To achieve this, 'Z' will occupy two high bits in
 * the next character.  Thus 'Z' can be interpreted as an escape character
 * (which indicates that the next character must be handled specially).
 * Note that setting high bits to "00", "01" or "10" cannot contribute
 * to another 'Z' (which would require high bits set to "11").  This is
 * how multiple escapes can be effectively avoided.
 */

/* Estimate base62 buffer size required to encode a given number of bits. */
static inline
int encode_base62_size(int bitc)
{
    /*
     * Four bits can make a character; the remaining bits can make
     * a character, too.  And the string should be null-terminated.
     */
    return (bitc >> 2) + 2;
}

static inline
void put_digit(char **base62, int c)
{
    assert(c >= 0 && c <= 61);

    if (c < 10)
	**base62 = c + '0';
    else if (c < 36)
	**base62 = c - 10 + 'a';
    else
	**base62 = c - 36 + 'A';

    (*base62)++;
}

/* Main base62 encoding routine: pack bitv into base62 string. */
static
int encode_base62(int bitc, const char *bitv, char *base62)
{
    char *base62_start = base62;
    int bits2 = 0; /* number of high bits set */
    int bits6 = 0; /* number of regular bits set */
    int num6b = 0; /* pending 6-bit number */
    while (bitc-- > 0) {
	assert(bits6 + bits2 < 6);
	num6b |= (*bitv++ << bits6++);
	if (bits6 + bits2 == 6) {
	    switch (num6b) {
	    case 61:
		/* escape */
		put_digit(&base62, 61);
		/* extra "00...." high bits (in the next character) */
		bits2 = 2;
		bits6 = 0;
		num6b = 0;
		break;
	    case 62:
		put_digit(&base62, 61);
		/* extra "01...." hight bits */
		bits2 = 2;
		bits6 = 0;
		num6b = 16;
		break;
	    case 63:
		put_digit(&base62, 61);
		/* extra "10...." hight bits */
		bits2 = 2;
		bits6 = 0;
		num6b = 32;
		break;
	    default:
		assert(num6b < 61);
		put_digit(&base62, num6b);
		bits2 = 0;
		bits6 = 0;
		num6b = 0;
		break;
	    }
	}
    }
    if (bits6 + bits2) {
	assert(num6b < 61);
	put_digit(&base62, num6b);
    }
    *base62 = '\0';
    return base62 - base62_start;
}

/* Estimate how many bits will result from decoding a base62 string. */
static inline
int decode_base62_size(const char *base62)
{
    int len = strlen(base62);
    /* Each character will fill at most 6 bits. */
    return (len << 2) + (len << 1);
}

static inline
int char_to_num(int c)
{
    if (c >= '0' && c <= '9')
	return c - '0';
    if (c >= 'a' && c <= 'z')
	return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
	return c - 'A' + 36;
    return -1;
}

static inline
void putbits(char **bitv, int n, int c)
{
    int i;

    for(i = 0; i < n; i++) {
	**bitv = (c >> i) & 1;
	(*bitv)++;
    }
}

/* Main base62 decoding routine: unpack base62 string into bitv. */
static
int decode_base62(const char *base62, char *bitv)
{
    char *bitv_start = bitv;
    int c;
    while ((c = *base62++)) {
	int num6b = char_to_num(c);
	if (num6b < 0)
	    return -1;
	if (num6b == 61) {
	    c = *base62++;
	    if (c == 0)
		return -2;
	    num6b = char_to_num(c);
	    if (num6b < 0)
		return -3;
	    switch (num6b & (16 + 32)) {
	    case 0:
		putbits(&bitv, 6, 61);
		break;
	    case 16:
		putbits(&bitv, 6, 62);
		break;
	    case 32:
		putbits(&bitv, 6, 63);
		break;
	    default:
		return -4;
		break;
	    }
	    putbits(&bitv, 4, num6b);
	}
	else {
	    putbits(&bitv, 6, num6b);
	}
    }
    return bitv - bitv_start;
}

#ifdef SELF_TEST
static
void test_base62(void)
{
    const char rnd_bitv[] = {
	1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 0, 0, 1,
	1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1,
	0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0,
	/* trigger some 'Z' */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    };
    const int rnd_bitc = sizeof rnd_bitv;
    /* encode */
    char base62[encode_base62_size(rnd_bitc)];
    int len = encode_base62(rnd_bitc, rnd_bitv, base62);
    assert(len > 0);
    assert(len == (int)strlen(base62));
    fprintf(stderr, "len=%d base62=%s\n", len, base62);
    /* The length cannot be shorter than 6 bits per symbol. */
    assert(len >= rnd_bitc / 6);
    /* Neither too long: each second character must fill at least 4 bits. */
    assert(len <= rnd_bitc / 2 / 4 + rnd_bitc / 2 / 6 + 1);
    /* decode */
    char bitv[decode_base62_size(base62)];
    int bitc = decode_base62(base62, bitv);
    fprintf(stderr, "rnd_bitc=%d bitc=%d\n", rnd_bitc, bitc);
    assert(bitc >= rnd_bitc);
    /* Decoded bits must match. */
    int i;
    for (i = 0; i < rnd_bitc; i++)
	assert(rnd_bitv[i] == bitv[i]);
    /* The remaining bits must be zero bits. */
    for (i = rnd_bitc; i < bitc; i++)
	assert(bitv[i] == 0);
    fprintf(stderr, "%s: base62 test OK\n", __FILE__);
}
#endif

/*
 * Golomb-Rice routines - compress integer values into bits.
 *
 * The idea is as follows.  Input values are assumed to be small integers.
 * Each value is split into two parts: an integer resulting from its higher
 * bits and an integer resulting from its lower bits (with the number of lower
 * bits specified by the Mshift parameter).  The frist integer is then stored
 * in unary coding (which is a variable-length sequence of '0' followed by a
 * terminating '1'); the second part is stored in normal binary coding (using
 * Mshift bits).
 *
 * The method is justified by the fact that, since most of the values are
 * small, their first parts will be short (typically 1..3 bits).  In particular,
 * the method is known to be optimal for uniformly distributed hash values,
 * after the values are sorted and delta-encoded.  See e.g.
 * Putze, F.; Sanders, P.; Singler, J. (2007),
 * "Cache-, Hash- and Space-Efficient Bloom Filters",
 * http://algo2.iti.uni-karlsruhe.de/singler/publications/cacheefficientbloomfilters-wea2007.pdf
 */

static inline
int log2i(int n)
{
    int m = 0;
    while (n >>= 1)
	m++;
    return m;
}

/* Calculate Mshift paramter for encoding. */
static
int encode_golomb_Mshift(int c, int bpp)
{
    /*
     * XXX Slightly better Mshift estimations are probably possible.
     * Recheck "Compression and coding algorithms" by Moffat & Turpin.
     */
    int Mshift = bpp - log2i(c) - 1;
    /* Adjust out-of-range values. */
    if (Mshift < 7)
	Mshift = 7;
    if (Mshift > 31)
	Mshift = 31;
    assert(Mshift < bpp);
    return Mshift;
}

/* Estimate how many bits can be filled up. */
static inline
int encode_golomb_size(int c, int Mshift)
{
    /*
     * XXX No precise estimation.  However, we do not expect unary-encoded bits
     * to take more than binary-encoded Mshift bits.
     */
    return (Mshift << 1) * c + 16;
}

/* Main golomb encoding routine: package integers into bits. */
static
int encode_golomb(int c, const unsigned *v, int Mshift, char *bitv)
{
    char *bitv_start = bitv;
    const unsigned mask = (1 << Mshift) - 1;
    while (c > 0) {
	c--;
	unsigned v0 = *v++;
	int i;
	/* first part: variable-length sequence */
	unsigned q = v0 >> Mshift;
	for (i = 0; i < (int)q; i++)
	    *bitv++ = 0;
	*bitv++ = 1;
	/* second part: lower Mshift bits */
	unsigned r = v0 & mask;
	for (i = 0; i < Mshift; i++)
	    *bitv++ = (r >> i) & 1;
    }
    return bitv - bitv_start;
}

/* Estimate how many values will emerge. */
static inline
int decode_golomb_size(int bitc, int Mshift)
{
    /*
     * Each (Mshift + 1) bits can make a value.
     * The remaining bits cannot make a value, though.
     */
    return bitc / (Mshift + 1);
}

/* Main golomb decoding routine: unpackage bits into values. */
static
int decode_golomb(int bitc, const char *bitv, int Mshift, unsigned *v)
{
    unsigned *v_start = v;
    /* next value */
    while (bitc > 0) {
	/* first part */
	unsigned q = 0;
	char bit = 0;
	while (bitc > 0) {
	    bitc--;
	    bit = *bitv++;
	    if (bit == 0)
		q++;
	    else
		break;
	}
	/* trailing zero bits in the input are okay */
	if (bitc == 0 && bit == 0)
	    break;
	/* otherwise, incomplete value is not okay */
	if (bitc < Mshift)
	    return -1;
	/* second part */
	unsigned r = 0;
	int i;
	for (i = 0; i < Mshift; i++) {
	    bitc--;
	    if (*bitv++)
		r |= (1 << i);
	}
	/* the value */
	*v++ = (q << Mshift) | r;
    }
    return v - v_start;
}

#ifdef SELF_TEST
static
void test_golomb(void)
{
    const unsigned rnd_v[] = {
	/* do re mi fa sol la si */
	1, 2, 3, 4, 5, 6, 7,
	/* koshka sela na taksi */
	7, 6, 5, 4, 3, 2, 1,
    };
    const int rnd_c = sizeof rnd_v / sizeof *rnd_v;
    int bpp = 10;
    int Mshift = encode_golomb_Mshift(rnd_c, bpp);
    fprintf(stderr, "rnd_c=%d bpp=%d Mshift=%d\n", rnd_c, bpp, Mshift);
    assert(Mshift > 0);
    assert(Mshift < bpp);
    /* encode */
    int alloc_bitc = encode_golomb_size(rnd_c, Mshift);
    assert(alloc_bitc > rnd_c);
    char bitv[alloc_bitc];
    int bitc = encode_golomb(rnd_c, rnd_v, Mshift, bitv);
    fprintf(stderr, "alloc_bitc=%d bitc=%d\n", alloc_bitc, bitc);
    assert(bitc > rnd_c);
    assert(bitc <= alloc_bitc);
    /* decode */
    int alloc_c = decode_golomb_size(bitc, Mshift);
    assert(alloc_c >= rnd_c);
    unsigned v[alloc_c];
    int c = decode_golomb(bitc, bitv, Mshift, v);
    fprintf(stderr, "rnd_c=%d alloc_c=%d c=%d\n", rnd_c, alloc_c, c);
    assert(alloc_c >= c);
    /* Decoded values must match. */
    assert(rnd_c == c);
    int i;
    for (i = 0; i < c; i++)
	assert(rnd_v[i] == v[i]);
    /* At the end of the day, did it save your money? */
    int golomb_bpp = bitc / c;
    fprintf(stderr, "bpp=%d golomb_bpp=%d\n", bpp, golomb_bpp);
    assert(golomb_bpp < bpp);
    fprintf(stderr, "%s: golomb test OK\n", __FILE__);
}
#endif

/*
 * Delta encoding routines - replace an increasing sequence of integer values
 * by the sequence of their differences.
 */

static
void encode_delta(int c, unsigned *v)
{
    assert(c > 0);
    unsigned int v0 = *v++;
    while (--c > 0) {
	*v -= v0;
	v0 += *v++;
    }
}

static
void decode_delta(int c, unsigned *v)
{
    assert(c > 0);
    unsigned int v0 = *v++;
    while (--c > 0) {
	*v += v0;
	v0 = *v++;
    }
}

#ifdef SELF_TEST
static
void test_delta(void)
{
    unsigned v[] = {
	1, 3, 7, 0
    };
    int c = 3;
    encode_delta(c, v);
    assert(v[0] == 1);
    assert(v[1] == 2);
    assert(v[2] == 4);
    assert(v[3] == 0);
    decode_delta(c, v);
    assert(v[0] == 1);
    assert(v[1] == 3);
    assert(v[2] == 7);
    assert(v[3] == 0);
    fprintf(stderr, "%s: delta test OK\n", __FILE__);
}
#endif

/*
 * Auxiliary routines.
 */

static
void maskv(int c, unsigned *v, unsigned mask)
{
    while (c-- > 0)
	*v++ &= mask;
}

static
int cmpv(const void *arg1, const void *arg2)
{
    unsigned v1 = *(unsigned *) arg1;
    unsigned v2 = *(unsigned *) arg2;
    if (v1 > v2)
	return 1;
    if (v1 < v2)
	return -1;
    return 0;
}

static
void sortv(int c, unsigned *v)
{
    qsort(v, c, sizeof *v, cmpv);
}

static
int uniqv(int c, unsigned *v)
{
    int i, j;
    for (i = 0, j = 0; i < c; i++) {
	while (i + 1 < c && v[i] == v[i+1])
	    i++;
	v[j++] = v[i];
    }
    assert(j <= c);
    return j;
}

#ifdef SELF_TEST
static
void test_aux(void)
{
    unsigned v[] = { 2, 3, 1, 2, 7, 6, 5 };
    int c = sizeof v / sizeof *v;
    maskv(c, v, 4 - 1);
    sortv(c, v);
    c = uniqv(c, v);
    assert(c == 3);
    assert(v[0] == 1);
    assert(v[1] == 2);
    assert(v[2] == 3);
    fprintf(stderr, "%s: aux test OK\n", __FILE__);
}
#endif

/*
 * Higher-level set-string routines - serialize integers into a set-string.
 *
 * A set-string looks like this: "set:bMxyz..."
 *
 * The "set:" prefix marks set-versions in rpm (to distinguish them between
 * regular rpm versions).  It is assumed to be stripped here.
 *
 * The next two characters (denoted 'b' and 'M') encode two small integers
 * in the range 7..32 using 'a'..'z'.  The first character encodes bpp.
 * Valid bpp range is 10..32.  The second character encodes Mshift.  Valid
 * Mshift range is 7..31.  Also, valid Mshift must be less than bpp.
 *
 * The rest ("xyz...") is a variable-length sequence of alnum characters.
 * It encodes a (sorted) set of (non-negative) integer values, as follows:
 * integers are delta-encoded, golomb-compressed and base62-serialized.
 */

static
int encode_set_size(int c, int bpp)
{
    int Mshift = encode_golomb_Mshift(c, bpp);
    int bitc = encode_golomb_size(c, Mshift);
    /* two leading characters are special */
    return 2 + encode_base62_size(bitc);
}

static
int encode_set(int c, unsigned *v, int bpp, char *base62)
{
    /* XXX v is non-const due to encode_delta */
    int Mshift = encode_golomb_Mshift(c, bpp);
    int bitc = encode_golomb_size(c, Mshift);
    char bitv[bitc];
    /* bpp */
    if (bpp < 10 || bpp > 32)
	return -1;
    *base62++ = bpp - 7 + 'a';
    /* golomb parameter */
    if (Mshift < 7 || Mshift > 31)
	return -2;
    *base62++ = Mshift - 7 + 'a';
    /* delta */
    encode_delta(c, v);
    /* golomb */
    bitc = encode_golomb(c, v, Mshift, bitv);
#ifdef SELF_TEST
    decode_delta(c, v);
#endif
    if (bitc < 0)
	return -3;
    /* base62 */
    int len = encode_base62(bitc, bitv, base62);
    if (len < 0)
	return -4;
    return 2 + len;
}

static
int decode_set_init(const char *str, int *pbpp, int *pMshift)
{
    /* 7..32 values encoded with 'a'..'z' */
    int bpp = *str++ + 7 - 'a';
    if (bpp < 10 || bpp > 32)
	return -1;
    /* golomb parameter */
    int Mshift = *str++ + 7 - 'a';
    if (Mshift < 7 || Mshift > 31)
	return -2;
    if (Mshift >= bpp)
	return -3;
    /* no empty sets for now */
    if (*str == '\0')
	return -4;
    *pbpp = bpp;
    *pMshift = Mshift;
    return 0;
}

static inline
int decode_set_size(const char *str, int Mshift)
{
    str += 2;
    int bitc = decode_base62_size(str);
    return decode_golomb_size(bitc, Mshift);
}

static
int decode_set(const char *str, int Mshift, unsigned *v)
{
    str += 2;
    /* base62 */
    char bitv[decode_base62_size(str)];
    int bitc = decode_base62(str, bitv);
    if (bitc < 0)
	return -1;
    /* golomb */
    int c = decode_golomb(bitc, bitv, Mshift, v);
    if (c < 0)
	return -2;
    /* delta */
    decode_delta(c, v);
    return c;
}

/* Special decode_set version with LRU caching. */
static
int cache_decode_set(const char *str, int Mshift, unsigned *v)
{
    struct cache_ent {
	struct cache_ent *next;
	const char *str;
	int c;
	unsigned v[1];
    };
    static __thread
    struct cache_ent *cache;
    /* lookup in the cache */
    struct cache_ent *cur = cache, *prev = NULL;
    int count = 0;
    while (cur) {
	if (strcmp(str, cur->str) == 0) {
	    /* hit, move to front */
	    if (cur != cache) {
		prev->next = cur->next;
		cur->next = cache;
		cache = cur;
	    }
	    memcpy(v, cur->v, cur->c * sizeof(*cur->v));
	    return cur->c;
	}
	count++;
	if (cur->next == NULL)
	    break;
	prev = cur;
	cur = cur->next;
    }
    /* miss, decode */
    int c = decode_set(str, Mshift, v);
    if (c <= 0)
	return c;
    /* truncate */
    int cache_size = 128;
    if (count >= cache_size) {
	free(cur);
	prev->next = NULL;
    }
    /* push to front */
    cur = malloc(sizeof(*cur) + strlen(str) + 1 + (c - 1) * sizeof(*v));
    if (cur == NULL)
	return c;
    cur->next = cache;
    cache = cur;
    cur->str = strcpy((char *)(cur->v + c), str);
    cur->c = c;
    memcpy(cur->v, v, c * sizeof(*v));
    return c;
}

static
int downsample_set(int c, unsigned *v, int bpp)
{
    unsigned mask = (1 << bpp) - 1;
    maskv(c, v, mask);
    sortv(c, v);
    return uniqv(c, v);
}

#ifdef SELF_TEST
static
void test_set(void)
{
    unsigned rnd_v[] = {
	0x020a, 0x07e5, 0x3305, 0x35f5,
	0x4980, 0x4c4f, 0x74ef, 0x7739,
	0x82ae, 0x8415, 0xa3e7, 0xb07e,
	0xb584, 0xb89f, 0xbb40, 0xf39e,
    };
    int rnd_c = sizeof rnd_v / sizeof *rnd_v;
    /* encode */
    int bpp = 16;
    char base62[encode_set_size(rnd_c, bpp)];
    int len = encode_set(rnd_c, rnd_v, bpp, base62);
    assert(len > 0);
    fprintf(stderr, "len=%d set=%s\n", len, base62);
    /* decode */
    int Mshift = bpp;
    int rc = decode_set_init(base62, &bpp, &Mshift);
    assert(rc == 0);
    assert(bpp == 16);
    assert(Mshift < bpp);
    int c = decode_set_size(base62, Mshift);
    assert(c >= rnd_c);
    unsigned v[c];
    c = decode_set(base62, Mshift, v);
    /* Decoded values must match. */
    assert(c == rnd_c);
    int i;
    for (i = 0; i < c; i++)
	assert(v[i] == rnd_v[i]);
    fprintf(stderr, "%s: set test OK\n", __FILE__);
}
#endif

/*
 * API routines start here.
 */

#include "set.h"

/* main API routine */
int rpmsetcmp(const char *str1, const char *str2)
{
	if (strncmp(str1, "set:", 4) == 0)
	    str1 += 4;
	if (strncmp(str2, "set:", 4) == 0)
	    str2 += 4;
	/* initialize decoding */
	int bpp1, Mshift1;
	int bpp2, Mshift2;
	if (decode_set_init(str1, &bpp1, &Mshift1) < 0)
	    return -3;
	if (decode_set_init(str2, &bpp2, &Mshift2) < 0)
	    return -4;
	/* make room for hash values */
	unsigned v1[decode_set_size(str1, Mshift1)];
	unsigned v2[decode_set_size(str2, Mshift2)];
	/* decode hash values
	   str1 comes on behalf of provides, decode with caching */
	int c1 = cache_decode_set(str1, Mshift1, v1);
	if (c1 < 0)
	    return -3;
	int c2 =       decode_set(str2, Mshift2, v2);
	if (c2 < 0)
	    return -4;
	/* adjust for comparison */
	if (bpp1 > bpp2) {
	    bpp1 = bpp2;
	    c1 = downsample_set(c1, v1, bpp1);
	}
	if (bpp2 > bpp1) {
	    bpp2 = bpp1;
	    c2 = downsample_set(c2, v2, bpp2);
	}
	/* compare */
	int ge = 1;
	int le = 1;
	int i1 = 0, i2 = 0;
	while (i1 < c1 && i2 < c2)
	    if (v1[i1] < v2[i2]) {
		le = 0;
		i1++;
	    }
	    else if (v1[i1] > v2[i2]) {
		ge = 0;
		i2++;
	    }
	    else {
		i1++;
		i2++;
	    }
	/* return */
	if (i1 < c1)
	    le = 0;
	if (i2 < c2)
	    ge = 0;
	if (le && ge)
	    return 0;
	if (ge)
	    return 1;
	if (le)
	    return -1;
	return -2;
}

/*
 * Simple API for creating set-versions.
 */

#include "system.h"
#include "rpmlib.h"

/* Internally, "struct set" is just a bag of strings and their hash values. */
struct set {
    int c;
    struct sv {
	const char *s;
	unsigned v;
    } *sv;
};

struct set *set_new()
{
    struct set *set = xmalloc(sizeof *set);
    set->c = 0;
    set->sv = NULL;
    return set;
}

void set_add(struct set *set, const char *sym)
{
    const int delta = 1024;
    if ((set->c & (delta - 1)) == 0)
	set->sv = xrealloc(set->sv, sizeof(*set->sv) * (set->c + delta));
    set->sv[set->c].s = xstrdup(sym);
    set->sv[set->c].v = 0;
    set->c++;
}

struct set *set_free(struct set *set)
{
    if (set) {
	int i;
	for (i = 0; i < set->c; i++)
	    set->sv[i].s = _free(set->sv[i].s);
	set->sv = _free(set->sv);
    }
    return NULL;
}

/* Jenkins' one-at-a-time hash */
static inline
unsigned int jenkins_hash(const char *str)
{
    unsigned int hash = 0x9e3779b9;
    const unsigned char *p = (const unsigned char *) str;
    while (*p) {
	hash += *p++;
	hash += (hash << 10);
	hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

static inline
int cmp_sv(const void *arg1, const void *arg2)
{
    struct sv *sv1 = (struct sv *) arg1;
    struct sv *sv2 = (struct sv *) arg2;
    if (sv1->v > sv2->v)
	return 1;
    if (sv2->v > sv1->v)
	return -1;
    return 0;
}

/* This routine does the whole job. */
const char *set_fini(struct set *set, int bpp)
{
    if (set->c < 1)
	return NULL;
    if (bpp < 10)
	return NULL;
    if (bpp > 32)
	return NULL;
    unsigned mask = (bpp < 32) ? (1u << bpp) - 1 : ~0u;
    /* hash sv strings */
    int i;
    for (i = 0; i < set->c; i++)
	set->sv[i].v = jenkins_hash(set->sv[i].s) & mask;
    /* sort by hash value */
    qsort(set->sv, set->c, sizeof *set->sv, cmp_sv);
    /* warn on hash collisions */
    for (i = 0; i < set->c - 1; i++) {
	if (set->sv[i].v != set->sv[i+1].v)
	    continue;
	if (strcmp(set->sv[i].s, set->sv[i+1].s) == 0)
	    continue;
	fprintf(stderr, "warning: hash collision: %s %s\n",
		set->sv[i].s, set->sv[i+1].s);
    }
    /* encode */
    unsigned v[set->c];
    for (i = 0; i < set->c; i++)
	v[i] = set->sv[i].v;
    int c = uniqv(set->c, v);
    char base62[encode_set_size(c, bpp)];
    int len = encode_set(c, v, bpp, base62);
    if (len < 0)
	return NULL;
    return xstrdup(base62);
}

#ifdef SELF_TEST
static
void test_api(void)
{
    struct set *set1 = set_new();
    set_add(set1, "mama");
    set_add(set1, "myla");
    set_add(set1, "ramu");
    const char *str10 = set_fini(set1, 16);
    fprintf(stderr, "set10=%s\n", str10);

    int cmp;
    struct set *set2 = set_new();
    set_add(set2, "myla");
    set_add(set2, "mama");
    const char *str20 = set_fini(set2, 16);
    fprintf(stderr, "set20=%s\n", str20);
    cmp = rpmsetcmp(str10, str20);
    assert(cmp == 1);

    set_add(set2, "ramu");
    const char *str21 = set_fini(set2, 16);
    fprintf(stderr, "set21=%s\n", str21);
    cmp = rpmsetcmp(str10, str21);
    assert(cmp == 0);

    set_add(set2, "baba");
    const char *str22 = set_fini(set2, 16);
    cmp = rpmsetcmp(str10, str22);
    assert(cmp == -1);

    set_add(set1, "deda");
    const char *str11 = set_fini(set1, 16);
    cmp = rpmsetcmp(str11, str22);
    assert(cmp == -2);

    set1 = set_free(set1);
    set2 = set_free(set2);
    str10 = _free(str10);
    str11 = _free(str11);
    str20 = _free(str20);
    str21 = _free(str21);
    str22 = _free(str22);

    fprintf(stderr, "%s: api test OK\n", __FILE__);
}
#endif

#ifdef SELF_TEST
int main(int argc, char **argv)
{
    test_base62();
    test_golomb();
    test_delta();
    test_aux();
    test_set();
    test_api();
    return 0;
}
#endif
/* ex: set ts=8 sts=4 sw=4 noet: */
