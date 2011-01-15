#include "system.h"
#include "rpmio_internal.h"
#include "rpmbuild.h"
#include "psm.h"

#include "checkFiles.h"

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
    void check(const char *path)
    {
	Package pkg;
	for (pkg = spec->packages; pkg; pkg = pkg->next)
	    if (fiSearch(pkg->cpioList, path) >= 0)
		return;
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
    int cmp(const void *sptr1, const void *sptr2)
    {
	const char *s1 = *(const char **) sptr1;
	const char *s2 = *(const char **) sptr2;
	return strcmp(s1, s2);
    }
    if (uv) {
	rpmlog(RPMLOG_WARNING, "Installed (but unpackaged) file(s) found:\n");
	qsort(uv, uc, sizeof(*uv), cmp);
	int i;
	for (i = 0; i < uc; i++)
	    rpmlog(RPMLOG_INFO, "    %s\n", uv[i]);
	free(uv);
    }
    return rc;
}

int checkFiles(Spec spec)
{
    int rc = checkUnpackaged(spec);
    if (rc && rpmExpandNumeric("%{?_unpackaged_files_terminate_build}")) {
	rpmlog(RPMLOG_ERR, "File list check failed, terminating build\n");
	return rc;
    }
    return 0;
}

// ex: set ts=8 sts=4 sw=4 noet:
