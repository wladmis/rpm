/*
 * interdep.c - inter-package analysis and optimizations based on
 * strict dependencies between subpackages (Requires: N = [E:]V-R).
 *
 * Written by Alexey Tourbin <at@altlinux.org>.
 * License: GPLv2+.
 */

#include "system.h"
#include "psm.h" // TFI_t
#include "rpmbuild.h"
#include "interdep.h"

static
const char *pkgName(Package pkg)
{
    TFI_t fi = pkg->cpioList;
    if (fi)
	return fi->name;
    const char *name;
    headerNVR(pkg->header, &name, NULL, NULL);
    return name;
}

static
const char *skipPrefixDash(const char *str, const char *prefix)
{
    int len = strlen(prefix);
    if (strncmp(str, prefix, len))
	return NULL;
    if (str[len] != '-')
	return NULL;
    return str + len + 1;
}

struct Req {
    int c;
    struct Pair {
	Package pkg1;
	Package pkg2;
    } *v;
};

static
int Requires(struct Req *r, Package pkg1, Package pkg2)
{
    int i;
    for (i = 0; i < r->c; i++)
	if (pkg1 == r->v[i].pkg1 && pkg2 == r->v[i].pkg2)
	    return 1;
    return 0;
}

static
void addRequires(struct Req *r, Package pkg1, Package pkg2)
{
    if (pkg1 == pkg2)
	return;
    if (Requires(r, pkg1, pkg2))
	return;
    AUTO_REALLOC(r->v, r->c, 8);
    r->v[r->c++] = (struct Pair) { pkg1, pkg2 };
}

static
void makeReq1(struct Req *r, Package pkg1, Package pkg2)
{
    int c = 0;
    const char **reqNv = NULL;
    const char **reqVv = NULL;
    const int *reqFv = NULL;
    const HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    int ok =
       hge(pkg1->header, RPMTAG_REQUIRENAME, NULL, (void **) &reqNv, &c) &&
       hge(pkg1->header, RPMTAG_REQUIREVERSION, NULL, (void **) &reqVv, NULL) &&
       hge(pkg1->header, RPMTAG_REQUIREFLAGS, NULL, (void **) &reqFv, NULL);
    if (!ok)
	return;
    const char *provN, *provV, *provR;
    headerNVR(pkg2->header, &provN, &provV, &provR);
    int i;
    for (i = 0; i < c; i++) {
	if (strcmp(reqNv[i], provN))
	    continue;
	if ((reqFv[i] & RPMSENSE_SENSEMASK) != RPMSENSE_EQUAL)
	    continue;
	const char *reqVR = reqVv[i];
	if (*reqVR == '\0')
	    continue;
	const char *reqR = skipPrefixDash(reqVR, provV);
	if (reqR == NULL) {
	    // XXX handle epoch properly?
	    const char *colon = strchr(reqVR, ':');
	    if (colon)
		reqR = skipPrefixDash(colon + 1, provV);
	}
	if (reqR == NULL)
	    continue;
	if (strcmp(reqR, provR))
	    continue;
	addRequires(r, pkg1, pkg2);
	break;
    }
    const HFD_t hfd = (HFD_t) headerFreeData;
    reqNv = hfd(reqNv, RPM_STRING_ARRAY_TYPE);
    reqVv = hfd(reqVv, RPM_STRING_ARRAY_TYPE);
}

static
struct Req *makeRequires(Spec spec)
{
    struct Req *r = xmalloc(sizeof *r);
    r->c = 0;
    r->v = NULL;
    Package pkg1, pkg2;
    for (pkg1 = spec->packages; pkg1; pkg1 = pkg1->next)
	for (pkg2 = pkg1->next; pkg2; pkg2 = pkg2->next) {
	    makeReq1(r, pkg1, pkg2);
	    makeReq1(r, pkg2, pkg1);
	}
    int propagated;
    do {
	propagated = 0;
	int i1, i2;
	for (i1 = 0; i1 < r->c; i1++)
	    for (i2 = i1; i2 < r->c; i2++) {
		struct Pair r1 = r->v[i1];
		struct Pair r2 = r->v[i2];
		if (r1.pkg2 == r2.pkg1 && r1.pkg1 != r2.pkg2 &&
		    !Requires(r, r1.pkg1, r2.pkg2))
		{
		    addRequires(r, r1.pkg1, r2.pkg2);
		    propagated++;
		}
		if (r2.pkg2 == r1.pkg1 && r2.pkg1 != r1.pkg2 &&
		    !Requires(r, r2.pkg1, r1.pkg2))
		{
		    addRequires(r, r2.pkg1, r1.pkg2);
		    propagated++;
		}
	    }
    }
    while (propagated);
    return r;
}

static
struct Req *freeRequires(struct Req *r)
{
    r->v = _free(r->v);
    return _free(r);
}

#include "checkFiles.h" // fiIntersect

static
void fiPrune(TFI_t fi, char pruned[])
{
    int *dil_;
    const char **bnl, **dnl;
    int bnc, dnc, dic;
    int ok =
       fi->hge(fi->h, RPMTAG_BASENAMES, NULL, (void **) &bnl, &bnc) &&
       fi->hge(fi->h, RPMTAG_DIRNAMES, NULL, (void **) &dnl, &dnc) &&
       fi->hge(fi->h, RPMTAG_DIRINDEXES, NULL, (void **) &dil_, &dic);
    assert(ok);
    assert(fi->fc == bnc);
    assert(bnc == dic);
    int i, j, k;
    // dil must be copied, cf. relocateFileList
    int dil[dic];
    for (i = 0; i < dic; i++)
	dil[i] = dil_[i];
    // mark used dirnames
    int dirused[dnc];
    bzero(dirused, dnc * sizeof *dirused);
    for (i = 0; i < bnc; i++)
	if (!pruned[i])
	    dirused[dil[i]]++;
    int propagated;
    do {
	propagated = 0;
	// for each unused dirname
	for (i = 0; i < dnc; i++) {
	    if (dirused[i])
		continue;
	    // find its corresponding parent_dir+name entry
	    for (j = 0; j < bnc; j++) {
		if (pruned[j])
		    continue;
		const char *D = dnl[i];
		const char *d = dnl[dil[j]];
		int dlen = strlen(d);
		if (strncmp(D, d, dlen))
		    continue;
		const char *b = bnl[j];
		int blen = strlen(b);
		if (strncmp(D + dlen, b, blen))
		    continue;
		if (strncmp(D + dlen + blen, "/", 2))
		    continue;
		// makr parent_dir+name for removal
		fprintf(stderr, "also prunning dir %s%s\n", d, b);
		pruned[j] = 1;
		// decrement parent_dir usage
		if (--dirused[dil[j]] == 0)
		    propagated++;
	    }
	}
    }
    while (propagated);
    // new count for bnc-like values
    int oldc = bnc;
    int newc = 0;
    for (i = 0; i < oldc; i++)
	if (!pruned[i])
	    newc++;
    // establish new dirnames and dirindexes
    for (i = 0, j = 0; i < dnc; i++) {
	if (!dirused[i])
	    continue;
	if (i == j)
	    goto skip;
	dnl[j] = dnl[i];
	for (k = 0; k < dic; k++)
	    if (dil[k] == i)
		dil[k] = j;
    skip:
	j++;
    }
    dnc = j;
    // handle bnl, dnl and dil
#define PruneV(v) \
    for (i = 0, j = 0; i < oldc; i++) \
	if (!pruned[i]) \
	    v[j++] = v[i]
    PruneV(bnl);
    PruneV(dil);
    PruneV(fi->bnl);
    PruneV(fi->dil);
    fi->hme(fi->h, RPMTAG_BASENAMES, RPM_STRING_ARRAY_TYPE, bnl, newc);
    fi->hme(fi->h, RPMTAG_DIRNAMES, RPM_STRING_ARRAY_TYPE, dnl, dnc);
    fi->hme(fi->h, RPMTAG_DIRINDEXES, RPM_INT32_TYPE, dil, newc);
    bnl = fi->hfd(bnl, RPM_STRING_ARRAY_TYPE);
    dnl = fi->hfd(dnl, RPM_STRING_ARRAY_TYPE);
    // prune header tags
    rpmTagType tagt;
    int tagc;
    const char **strv;
#define PruneStrTag(tag) \
    if (fi->hge(fi->h, tag, &tagt, (void **) &strv, &tagc)) { \
	assert(tagt == RPM_STRING_ARRAY_TYPE); \
	assert(tagc == oldc); \
	PruneV(strv); \
	fi->hme(fi->h, tag, RPM_STRING_ARRAY_TYPE, strv, newc); \
	fi->hfd(strv, RPM_STRING_ARRAY_TYPE); \
    }
    short *INT16p, INT16v[oldc];
    int *INT32p, INT32v[oldc];
#define PruneIntTag(INT, tag) \
    if (fi->hge(fi->h, tag, &tagt, (void **) &INT ## p, &tagc)) { \
	assert(tagt == RPM_ ## INT ## _TYPE); \
	assert(tagc == oldc); \
	for (i = 0; i < oldc; i++) \
	    INT ## v[i] = INT ## p[i]; \
	PruneV(INT ## v); \
	fi->hme(fi->h, tag, RPM_ ## INT ##_TYPE, INT ## v, newc); \
    }
#define PruneI16Tag(tag) PruneIntTag(INT16, tag)
#define PruneI32Tag(tag) PruneIntTag(INT32, tag)
    PruneI32Tag(RPMTAG_FILESIZES);
    PruneStrTag(RPMTAG_FILEUSERNAME);
    PruneStrTag(RPMTAG_FILEGROUPNAME);
    PruneI32Tag(RPMTAG_FILEMTIMES);
    PruneI16Tag(RPMTAG_FILEMODES);
    PruneI16Tag(RPMTAG_FILERDEVS);
    PruneI32Tag(RPMTAG_FILEDEVICES);
    PruneI32Tag(RPMTAG_FILEINODES);
    PruneStrTag(RPMTAG_FILELANGS);
    PruneStrTag(RPMTAG_FILEMD5S);
    PruneStrTag(RPMTAG_FILELINKTOS);
    PruneI32Tag(RPMTAG_FILEVERIFYFLAGS);
    PruneI32Tag(RPMTAG_FILEFLAGS);
    // update fi, cf. genCpioListAndHeader
    PruneV(fi->apath);
    PruneV(fi->actions);
    PruneV(fi->fmapflags);
    PruneV(fi->fuids);
    PruneV(fi->fgids);
    struct transactionFileInfo_s save_fi;
#define MV(a) save_fi.a = fi->a; fi->a = NULL
    MV(bnl); MV(dnl); MV(dil);
    MV(apath); MV(actions); MV(fmapflags); MV(fuids); MV(fgids);
    save_fi.h = fi->h;
    save_fi.astriplen = fi->astriplen;
    freeFi(fi);
    bzero(fi, sizeof *fi);
    fi->type = TR_ADDED;
    loadFi(save_fi.h, fi);
    assert(fi->fc == newc);
    fi->dnl = _free(fi->dnl);
    fi->bnl = _free(fi->bnl);
#undef MV
#define MV(a) fi->a = save_fi.a
    MV(bnl); MV(dnl); MV(dil);
    MV(apath); MV(actions); MV(fmapflags); MV(fuids); MV(fgids);
    fi->astriplen = save_fi.astriplen;
}

// prune src dups from pkg1 and add dependency on pkg2
static
void pruneSrc1(Package pkg1, Package pkg2)
{
    TFI_t fi1 = pkg1->cpioList;
    TFI_t fi2 = pkg2->cpioList;
    if (fi1 == NULL) return;
    if (fi2 == NULL) return;
    int npruned = 0;
    char pruned[fi1->fc];
    bzero(pruned, fi1->fc);
    void cb(char *f, int i1, int i2)
    {
	(void) i2;
	if (S_ISDIR(fi1->fmodes[i1]))
	    return;
	const char src[] = "/usr/src/debug/";
	if (strncmp(f, src, sizeof(src) - 1))
	    return;
	pruned[i1] = 1;
	npruned++;
    }
    fiIntersect(fi1, fi2, cb);
    if (npruned == 0)
	return;
    fprintf(stderr, "removing %d sources from %s and adding dependency on %s\n",
	    npruned, pkgName(pkg1), pkgName(pkg2));
    fiPrune(fi1, pruned);
    const char *name = pkgName(pkg2);
    const char *evr = headerSprintf(pkg2->header,
	    "%|epoch?{%{epoch}:}|%{version}-%{release}",
	    rpmTagTable, rpmHeaderFormats, NULL);
    assert(evr);
    int flags = RPMSENSE_EQUAL | RPMSENSE_FIND_REQUIRES;
    assert(pkg1->header == fi1->h);
    headerAddOrAppendEntry(fi1->h, RPMTAG_REQUIRENAME, RPM_STRING_ARRAY_TYPE, &name, 1);
    headerAddOrAppendEntry(fi1->h, RPMTAG_REQUIREVERSION, RPM_STRING_ARRAY_TYPE, &evr, 1);
    headerAddOrAppendEntry(fi1->h, RPMTAG_REQUIREFLAGS, RPM_INT32_TYPE, &flags, 1);
    evr = _free(evr);
}

static void
processDependentDebuginfo(struct Req *r, Spec spec,
    void (*cb)(Package pkg1, Package pkg2), int mutual)
{
    int i1, i2;
    struct Pair r1, r2;
    const char *Nd, *Np;
    const char *suf;
    // r1 = { pkg1-debuginfo, pkg1 }
    for (i1 = 0; i1 < r->c; i1++) {
	r1 = r->v[i1];
	Nd = pkgName(r1.pkg1);
	Np = pkgName(r1.pkg2);
	suf = skipPrefixDash(Nd, Np);
	if (suf == NULL || strcmp(suf, "debuginfo"))
	    continue;
	// r2 = { pkg2-debuginfo, pkg2 }
	for (i2 = i1; i2 < r->c; i2++) {
	    r2 = r->v[i2];
	    Nd = pkgName(r2.pkg1);
	    Np = pkgName(r2.pkg2);
	    suf = skipPrefixDash(Nd, Np);
	    if (suf == NULL || strcmp(suf, "debuginfo"))
		continue;
	    // (pkg1 <-> pkg2) => (pkg1-debuginfo <-> pkg2-debuginfo)
	    if (Requires(r, r1.pkg2, r2.pkg2)) {
		cb(r1.pkg1, r2.pkg1);
		if (!mutual)
		    continue;
	    }
	    if (Requires(r, r2.pkg2, r1.pkg2))
		cb(r2.pkg1, r1.pkg1);
	}
    }
}

static
void pruneDebuginfoSrc(struct Req *r, Spec spec)
{
    processDependentDebuginfo(r, spec, pruneSrc1, 0);
}

#include "al.h"

// if pkg1 implicitly requires pkg2, add strict dependency
static
void liftDeps1(Package pkg1, Package pkg2)
{
    int reqc = 0;
    const char **reqNv = NULL;
    const char **reqVv = NULL;
    const int *reqFv = NULL;
    const HGE_t hge = (HGE_t) headerGetEntryMinMemory;
    int ok =
       hge(pkg1->header, RPMTAG_REQUIRENAME, NULL, (void **) &reqNv, &reqc) &&
       hge(pkg1->header, RPMTAG_REQUIREVERSION, NULL, (void **) &reqVv, NULL) &&
       hge(pkg1->header, RPMTAG_REQUIREFLAGS, NULL, (void **) &reqFv, NULL);
    if (!ok)
	return;
    availableList proval = alloca(sizeof proval);
    alCreate(proval);
    alAddPackage(proval, pkg2->header, NULL, NULL, NULL);
    int i;
    struct availablePackage *ap = NULL;
    for (i = 0; i < reqc; i++) {
	ap = alSatisfiesDepend(proval, reqNv[i], reqVv[i], reqFv[i]);
	if (ap)
	    break;
    }
    const HFD_t hfd = (HFD_t) headerFreeData;
    reqNv = hfd(reqNv, RPM_STRING_ARRAY_TYPE);
    reqVv = hfd(reqVv, RPM_STRING_ARRAY_TYPE);
    if (ap == NULL)
	return;
    const char *name = pkgName(pkg2);
    const char *evr = headerSprintf(pkg2->header,
	    "%|epoch?{%{epoch}:}|%{version}-%{release}",
	    rpmTagTable, rpmHeaderFormats, NULL);
    assert(evr);
    int flags = RPMSENSE_EQUAL | RPMSENSE_FIND_REQUIRES;
    if (addReqProv(NULL, pkg1->header, flags, name, evr, 0) == 0)
	fprintf(stderr, "%s: adding strict dependency on %s\n",
		pkgName(pkg1), pkgName(pkg2));
    evr = _free(evr);
}

static
void liftDebuginfoDeps(struct Req *r, Spec spec)
{
    processDependentDebuginfo(r, spec, liftDeps1, 1);
}

int processInterdep(Spec spec)
{
    struct Req *r = makeRequires(spec);
    pruneDebuginfoSrc(r, spec);
    liftDebuginfoDeps(r, spec);
    r = freeRequires(r);
    return 0;
}

// ex: set ts=8 sts=4 sw=4 noet:
