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
		if (r1.pkg2 == r2.pkg1 && !Requires(r, r1.pkg1, r2.pkg2)) {
		    addRequires(r, r1.pkg1, r2.pkg2);
		    propagated++;
		}
		if (r2.pkg2 == r1.pkg1 && !Requires(r, r2.pkg1, r1.pkg2)) {
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

static
void pruneDebuginfoSrc(struct Req *r, Spec spec)
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
	    if (Requires(r, r1.pkg2, r2.pkg2))
		pruneSrc1(r1.pkg1, r2.pkg1);
	    // "else" guards against mutual deletions
	    else if (Requires(r, r2.pkg2, r1.pkg2))
		pruneSrc1(r2.pkg1, r1.pkg1);
	}
    }
}

int processInterdep(Spec spec)
{
    struct Req *r = makeRequires(spec);
    pruneDebuginfoSrc(r, spec);
    r = freeRequires(r);
    return 0;
}

// ex: set ts=8 sts=4 sw=4 noet:
