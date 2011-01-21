/*
 * checkFiles.c - check packaged file list.
 * Written by Alexey Tourbin <at@altlinux.org>.
 * License: GPLv2+.
 */
#include "system.h"
#include "rpmio_internal.h"
#include "rpmbuild.h"
#include "psm.h"

#include "checkFiles.h"

void fiIntersect(TFI_t fi1, TFI_t fi2, void (*cb)(char *f, int i1, int i2))
{
    if (fi1 == NULL) return;
    if (fi2 == NULL) return;
    int i1 = 0, i2 = 0;
    while (i1 < fi1->fc && i2 < fi2->fc) {
	char f1[PATH_MAX], f2[PATH_MAX];
	strcpy(f1, fi1->dnl[fi1->dil[i1]] + fi1->astriplen);
	strcpy(f2, fi2->dnl[fi2->dil[i2]] + fi2->astriplen);
	strcat(f1, fi1->bnl[i1]);
	strcat(f2, fi2->bnl[i2]);
	int cmp = strcmp(f1, f2);
	if (cmp < 0) {
	    i1++;
	    continue;
	}
	if (cmp > 0) {
	    i2++;
	    continue;
	}
	cb(f1, i1, i2);
	i1++;
	i2++;
    }
}

static
void checkPkgIntersect(Package pkg1, Package pkg2)
{
    TFI_t fi1 = pkg1->cpioList;
    TFI_t fi2 = pkg2->cpioList;
    if (fi1 == NULL) return;
    if (fi2 == NULL) return;
    int once = 0;
    void cb(char *f, int i1, int i2)
    {
	if (S_ISDIR(fi1->fmodes[i1]) && S_ISDIR(fi2->fmodes[i2]))
	    return;
	if (once++ == 0)
	    rpmlog(RPMLOG_WARNING,
		    "File(s) packaged into both %s-%s-%s and %s-%s-%s:\n",
		    fi1->name, fi1->version, fi1->release,
		    fi2->name, fi2->version, fi2->release);
	rpmlog(RPMLOG_INFO, "    %s\n", f);
    }
    fiIntersect(fi1, fi2, cb);
}

static
void checkIntersect(Spec spec)
{
    Package pkg1, pkg2;
    for (pkg1 = spec->packages; pkg1; pkg1 = pkg1->next)
	for (pkg2 = pkg1->next; pkg2; pkg2 = pkg2->next)
	    checkPkgIntersect(pkg1, pkg2);
}

static
int fiSearch(TFI_t fi, const char *path)
{
    if (fi == NULL || fi->fc == 0)
	return -1;
    int l = 0;
    int u = fi->fc;
    while (l < u) {
	int i = (l + u) / 2;
	const char *d = fi->dnl[fi->dil[i]];
	int dlen = strlen(d);
	int cmp = strncmp(path, d, dlen);
	if (cmp == 0) {
	    const char *b = fi->bnl[i];
	    cmp = strcmp(path + dlen, b);
	}
	if (cmp < 0)
	    u  = i;
	else if (cmp > 0)
	    l = i + 1;
	else
	    return i;
    }
    return -1;
}

#include <fts.h>

static
int checkUnpackaged(Spec spec)
{
    int rc = 0;
    int uc = 0;
    char **uv = NULL;
    int avcmp(const void *sptr1, const void *sptr2)
    {
	const char *s1 = *(const char **) sptr1;
	const char *s2 = *(const char **) sptr2;
	return strcmp(s1, s2);
    }
    if (spec->exclude)
	qsort(spec->exclude, spec->excludeCount, sizeof(char *), avcmp);
    void check(const char *path)
    {
	Package pkg;
	for (pkg = spec->packages; pkg; pkg = pkg->next)
	    if (fiSearch(pkg->cpioList, path) >= 0)
		return;
	if (spec->exclude) {
	    const char *key = path + strlen(spec->buildRootURL);
	    if (bsearch(&key, spec->exclude, spec->excludeCount,
			sizeof(char *), avcmp))
		return;
	}
	AUTO_REALLOC(uv, uc, 8);
	uv[uc++] = xstrdup(path + strlen(spec->buildRootURL));
	rc |= 1;
    }
    char *paths[] = { (char *) spec->buildRootURL, 0 };
    int options = FTS_COMFOLLOW | FTS_PHYSICAL;
    FTS *ftsp = fts_open(paths, options, NULL);
    FTSENT *fts;
    while ((fts = fts_read(ftsp)) != NULL) {
	// skip dotfiles in buildroot
	if (fts->fts_level == 1 && *fts->fts_name == '.') {
	    if (fts->fts_info == FTS_D)
		fts_set(ftsp, fts, FTS_SKIP);
	    continue;
	}
	switch (fts->fts_info) {
	// do not check for unpackaged directories
	case FTS_D:
	case FTS_DP:
	    break;
	case FTS_F:
	case FTS_SL:
	case FTS_SLNONE:
	case FTS_DEFAULT:
	    check(fts->fts_path);
	    break;
	default:
	    rpmlog(RPMLOG_WARNING, "%s: fts error\n", fts->fts_path);
	    rc |= 2;
	    break;
	}
    }
    if (uv) {
	rpmlog(RPMLOG_WARNING, "Installed (but unpackaged) file(s) found:\n");
	qsort(uv, uc, sizeof(*uv), avcmp);
	int i;
	for (i = 0; i < uc; i++)
	    rpmlog(RPMLOG_INFO, "    %s\n", uv[i]);
	free(uv);
    }
    return rc;
}

int checkFiles(Spec spec)
{
    checkIntersect(spec);
    int rc = checkUnpackaged(spec);
    if (rc && rpmExpandNumeric("%{?_unpackaged_files_terminate_build}")) {
	rpmlog(RPMLOG_ERR, "File list check failed, terminating build\n");
	return rc;
    }
    return 0;
}

// ex: set ts=8 sts=4 sw=4 noet:
