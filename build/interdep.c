/*
 * interdep.c - inter-package analysis and optimizations based on
 * strict dependencies between subpackages (Requires: N = [E:]V-R).
 *
 * Written by Alexey Tourbin <at@altlinux.org>.
 * License: GPLv2+.
 */

#include "system.h"
#include "rpmbuild.h"
#include "interdep.h"

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
    return r;
}

static
struct Req *freeRequires(struct Req *r)
{
    r->v = _free(r->v);
    return _free(r);
}

int processInterdep(Spec spec)
{
    struct Req *r = makeRequires(spec);
    r = freeRequires(r);
    return 0;
}

// ex: set ts=8 sts=4 sw=4 noet:
