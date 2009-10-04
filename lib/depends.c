/** \ingroup rpmdep
 * \file lib/depends.c
 */

#include "system.h"

#include "rpmlib.h"
#include "rpmmacro.h"		/* XXX for rpmExpand() */

#include "depends.h"
#include "al.h"
#include "rpmdb.h"		/* XXX response cache needs dbiOpen et al. */

#include "debug.h"

/*@access dbiIndex@*/		/* XXX compared with NULL */
/*@access dbiIndexSet@*/	/* XXX compared with NULL */
/*@access Header@*/		/* XXX compared with NULL */
/*@access FD_t@*/		/* XXX compared with NULL */
/*@access rpmdb@*/		/* XXX compared with NULL */
/*@access rpmdbMatchIterator@*/		/* XXX compared with NULL */
/*@access rpmTransactionSet@*/
/*@access rpmDependencyConflict@*/
/*@access availableList@*/

static int _cacheDependsRC = 1;

/**
 * Return formatted dependency string.
 * @param depend	type of dependency ("R" == Requires, "C" == Conflcts)
 * @param key		dependency name string
 * @param keyEVR	dependency [epoch:]version[-release] string
 * @param keyFlags	dependency logical range qualifiers
 * @return		formatted dependency (malloc'ed)
 */
static /*@only@*/ char * printDepend(const char * depend, const char * key,
		const char * keyEVR, int keyFlags)
	/*@*/
{
    char * tbuf, * t;
    size_t nb;

    nb = 0;
    if (depend)	nb += strlen(depend) + 1;
    if (key)	nb += strlen(key);
    if (keyFlags & RPMSENSE_SENSEMASK) {
	if (nb)	nb++;
	if (keyFlags & RPMSENSE_LESS)	nb++;
	if (keyFlags & RPMSENSE_GREATER) nb++;
	if (keyFlags & RPMSENSE_EQUAL)	nb++;
    }
    if (keyEVR && *keyEVR) {
	if (nb)	nb++;
	nb += strlen(keyEVR);
    }

    t = tbuf = xmalloc(nb + 1);
    if (depend) {
	while(*depend != '\0')	*t++ = *depend++;
	*t++ = ' ';
    }
    if (key)
	while(*key != '\0')	*t++ = *key++;
    if (keyFlags & RPMSENSE_SENSEMASK) {
	if (t != tbuf)	*t++ = ' ';
	if (keyFlags & RPMSENSE_LESS)	*t++ = '<';
	if (keyFlags & RPMSENSE_GREATER) *t++ = '>';
	if (keyFlags & RPMSENSE_EQUAL)	*t++ = '=';
    }
    if (keyEVR && *keyEVR) {
	if (t != tbuf)	*t++ = ' ';
	while(*keyEVR != '\0')	*t++ = *keyEVR++;
    }
    *t = '\0';
    return tbuf;
}

#ifdef	UNUSED
static /*@only@*/ const char *buildEVR(int_32 *e, const char *v, const char *r)
{
    const char *pEVR;
    char *p;

    pEVR = p = xmalloc(21 + strlen(v) + 1 + strlen(r) + 1);
    *p = '\0';
    if (e) {
	sprintf(p, "%d:", *e);
	while (*p)
	    p++;
    }
    (void) stpcpy( stpcpy( stpcpy(p, v) , "-") , r);
    return pEVR;
}
#endif

struct orderListIndex {
    int alIndex;
    int orIndex;
};

/* parseEVR() moved to rpmvercmp.c */

const char *rpmNAME = PACKAGE;
const char *rpmEVR = VERSION;
int rpmFLAGS = RPMSENSE_EQUAL;

int rpmRangesOverlap(const char * AName, const char * AEVR, int AFlags,
	const char * BName, const char * BEVR, int BFlags)
{
    const char *aDepend = printDepend(NULL, AName, AEVR, AFlags);
    const char *bDepend = printDepend(NULL, BName, BEVR, BFlags);
    char *aEVR, *bEVR;
    const char *aE, *aV, *aR, *bE, *bV, *bR;
    int result;
    int sense;

    /* Different names don't overlap. */
    if (strcmp(AName, BName)) {
	result = 0;
	goto exit;
    }

    /* Same name. If either A or B is an existence test, always overlap. */
    if (!((AFlags & RPMSENSE_SENSEMASK) && (BFlags & RPMSENSE_SENSEMASK))) {
	result = 1;
	goto exit;
    }

    if (!AEVR) AEVR = "";
    if (!BEVR) BEVR = "";

    /* Optimize: when both EVRs are non-existent or empty, always overlap. */
    if (!(*AEVR || *BEVR)) {
	result = 1;
	goto exit;
    }

    /* Both AEVR and BEVR exist. */
    aEVR = xstrdup(AEVR);
    parseEVR(aEVR, &aE, &aV, &aR);
    bEVR = xstrdup(BEVR);
    parseEVR(bEVR, &bE, &bV, &bR);
    /* rpmEVRcmp() is also shared; the code moved to rpmvercmp.c */
    sense = rpmEVRcmp(aE, aV, aR, aDepend, bE, bV, bR, bDepend);
    aEVR = _free(aEVR);
    bEVR = _free(bEVR);

    /* Detect overlap of {A,B} range. */
    result = 0;
    if (sense < 0 && ((AFlags & RPMSENSE_GREATER) || (BFlags & RPMSENSE_LESS))) {
	result = 1;
    } else if (sense > 0 && ((AFlags & RPMSENSE_LESS) || (BFlags & RPMSENSE_GREATER))) {
	result = 1;
    } else if (sense == 0 &&
	(((AFlags & RPMSENSE_EQUAL) && (BFlags & RPMSENSE_EQUAL)) ||
	 ((AFlags & RPMSENSE_LESS) && (BFlags & RPMSENSE_LESS)) ||
	 ((AFlags & RPMSENSE_GREATER) && (BFlags & RPMSENSE_GREATER)))) {
	result = 1;
    }

exit:
    rpmMessage(RPMMESS_DEBUG, _("  %s    A %s\tB %s\n"),
	(result ? _("YES") : _("NO ")), aDepend, bDepend);
    aDepend = _free(aDepend);
    bDepend = _free(bDepend);
    return result;
}

/*@-typeuse@*/
typedef int (*dbrecMatch_t) (Header h, const char *reqName, const char * reqEVR, int reqFlags);
/*@=typeuse@*/

static int rangeMatchesDepFlags (Header h,
		const char * reqName, const char * reqEVR, int reqFlags)
	/*@*/
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    HFD_t hfd = headerFreeData;
    rpmTagType pnt, pvt;
    const char ** provides;
    const char ** providesEVR;
    int_32 * provideFlags;
    int providesCount;
    int result;
    int i;

    if (!(reqFlags & RPMSENSE_SENSEMASK) || !reqEVR || !strlen(reqEVR))
	return 1;

    /* Get provides information from header */
    /*
     * Rpm prior to 3.0.3 does not have versioned provides.
     * If no provides version info is available, match any requires.
     */
    if (!hge(h, RPMTAG_PROVIDEVERSION, &pvt,
		(void **) &providesEVR, &providesCount))
	return 1;

    (void) hge(h, RPMTAG_PROVIDEFLAGS, NULL, (void **) &provideFlags, NULL);

    if (!hge(h, RPMTAG_PROVIDENAME, &pnt, (void **) &provides, &providesCount))
    {
	providesEVR = hfd(providesEVR, pvt);
	return 0;	/* XXX should never happen */
    }

    result = 0;
    for (i = 0; i < providesCount; i++) {

	/* Filter out provides that came along for the ride. */
	if (strcmp(provides[i], reqName))
	    continue;

	if (!(provideFlags[i] & RPMSENSE_SENSEMASK))
	    provideFlags[i] |= RPMSENSE_EQUAL;
	result = rpmRangesOverlap(provides[i], providesEVR[i], provideFlags[i],
			reqName, reqEVR, reqFlags);

	/* If this provide matches the require, we're done. */
	if (result)
	    break;
    }

    provides = hfd(provides, pnt);
    providesEVR = hfd(providesEVR, pvt);

    return result;
}

int headerMatchesDepFlags(Header h,
		const char * reqName, const char * reqEVR, int reqFlags)
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    const char *name, *version, *release;
    int_32 * epoch;
    const char *pkgEVR;
    char *p;
    int pkgFlags = RPMSENSE_EQUAL;

    if (!((reqFlags & RPMSENSE_SENSEMASK) && reqEVR && *reqEVR))
	return 1;

    /* Get package information from header */
    (void) headerNVR(h, &name, &version, &release);

    pkgEVR = p = alloca(21 + strlen(version) + 1 + strlen(release) + 1);
    *p = '\0';
    if (hge(h, RPMTAG_EPOCH, NULL, (void **) &epoch, NULL)) {
	sprintf(p, "%d:", *epoch);
	while (*p != '\0')
	    p++;
    }
    (void) stpcpy( stpcpy( stpcpy(p, version) , "-") , release);

    return rpmRangesOverlap(name, pkgEVR, pkgFlags, reqName, reqEVR, reqFlags);
}

rpmTransactionSet rpmtransCreateSet(rpmdb rpmdb, const char * rootDir)
{
    rpmTransactionSet ts;
    int rootLen;

    if (!rootDir) rootDir = "";

    ts = xcalloc(1, sizeof(*ts));
    ts->filesystemCount = 0;
    ts->filesystems = NULL;
    ts->di = NULL;
    /*@-assignexpose@*/
    ts->rpmdb = rpmdb;
    /*@=assignexpose@*/
    ts->scriptFd = NULL;
    ts->id = 0;

    ts->numRemovedPackages = 0;
    ts->removedPackages = NULL;

    /* This canonicalizes the root */
    rootLen = strlen(rootDir);
    if (!(rootLen && rootDir[rootLen - 1] == '/')) {
	char * t;

	t = alloca(rootLen + 2);
	*t = '\0';
	(void) stpcpy( stpcpy(t, rootDir), "/");
	rootDir = t;
    }

    ts->rootDir = xstrdup(rootDir);
    ts->currDir = NULL;
    ts->chrootDone = 0;

    alCreate(&ts->addedPackages);

    ts->orderCount = 0;
    ts->order = NULL;

    return ts;
}

/**
 * Compare removed package instances (qsort/bsearch).
 * @param a		1st instance address
 * @param b		2nd instance address
 * @return		result of comparison
 */
static int intcmp(const void * a, const void * b)	/*@*/
{
    const int * aptr = a;
    const int * bptr = b;
    int rc = (*aptr - *bptr);
    return rc;
}

/**
 * Add removed package instance to ordered transaction set.
 * @param ts		transaction set
 * @param dboffset	rpm database instance
 * @param depends	installed package of pair (or -1 on erase)
 * @return		0 on success
 */
static int removePackage(rpmTransactionSet ts, int dboffset, int depends)
	/*@modifies ts @*/
{

    /* Filter out duplicate erasures. */
    if (ts->numRemovedPackages > 0 && ts->removedPackages != NULL) {
	if (bsearch(&dboffset, ts->removedPackages, ts->numRemovedPackages,
			sizeof(int), intcmp) != NULL)
	    return 0;
    }

    AUTO_REALLOC(ts->removedPackages, ts->numRemovedPackages);
    ts->removedPackages[ts->numRemovedPackages++] = dboffset;
    qsort(ts->removedPackages, ts->numRemovedPackages, sizeof(int), intcmp);

    AUTO_REALLOC(ts->order, ts->orderCount);
    ts->order[ts->orderCount].type = TR_REMOVED;
    ts->order[ts->orderCount].u.removed.dboffset = dboffset;
    ts->order[ts->orderCount++].u.removed.dependsOnIndex = depends;

    return 0;
}

static int rpmDigestCompare(Header first, Header second)
{
    const char * one, * two;

    if (!headerGetEntry(first, RPMTAG_SHA1HEADER, NULL, (void **) &one, NULL))
	one = NULL;
    if (!headerGetEntry(second, RPMTAG_SHA1HEADER, NULL, (void **) &two, NULL))
	two = NULL;

    if (one && two)
	return strcmp(one, two);
    if (one && !two)
	return 1;
    if (!one && two)
	return -1;
    return 0;
}

int rpmtransAddPackage(rpmTransactionSet ts, Header h, FD_t fd,
			const void * key, int upgrade, rpmRelocation * relocs)
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    HFD_t hfd = headerFreeData;
    rpmTagType ont, ovt;
    /* this is an install followed by uninstalls */
    const char * name;
    int count;
    const char ** obsoletes;

    /*
     * FIXME: handling upgrades like this is *almost* okay. It doesn't
     * check to make sure we're upgrading to a newer version, and it
     * makes it difficult to generate a return code based on the number of
     * packages which failed.
     */
    struct availablePackage *alp =
	    alAddPackage(&ts->addedPackages, h, key, fd, relocs);
    int alNum = alp - ts->addedPackages.list;
    AUTO_REALLOC(ts->order, ts->orderCount);
    ts->order[ts->orderCount].type = TR_ADDED;
    ts->order[ts->orderCount++].u.addedIndex = alNum;

    if (!upgrade || ts->rpmdb == NULL)
	return 0;

    /* XXX binary rpms always have RPMTAG_SOURCERPM, source rpms do not */
    if (headerIsEntry(h, RPMTAG_SOURCEPACKAGE))
	return 0;

    (void) headerNVR(h, &name, NULL, NULL);

    {	rpmdbMatchIterator mi;
	Header h2;

	mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_NAME, name, 0);
	while((h2 = rpmdbNextIterator(mi)) != NULL) {
	    if (rpmDigestCompare(h, h2) || rpmVersionCompare(h, h2))
		(void) removePackage(ts, rpmdbGetIteratorOffset(mi), alNum);
	}
	mi = rpmdbFreeIterator(mi);
    }

    if (hge(h, RPMTAG_OBSOLETENAME, &ont, (void **) &obsoletes, &count)) {
	const char ** obsoletesEVR;
	int_32 * obsoletesFlags;
	int j;

	(void) hge(h, RPMTAG_OBSOLETEVERSION, &ovt, (void **) &obsoletesEVR,
			NULL);
	(void) hge(h, RPMTAG_OBSOLETEFLAGS, NULL, (void **) &obsoletesFlags,
			NULL);

	for (j = 0; j < count; j++) {

	    /* XXX avoid self-obsoleting packages. */
	    if (!strcmp(name, obsoletes[j]))
		continue;

	  { rpmdbMatchIterator mi;
	    Header h2;

	    mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_NAME, obsoletes[j], 0);

	    (void) rpmdbPruneIterator(mi,
		ts->removedPackages, ts->numRemovedPackages, 1);

	    while((h2 = rpmdbNextIterator(mi)) != NULL) {
		/*
		 * Rpm prior to 3.0.3 does not have versioned obsoletes.
		 * If no obsoletes version info is available, match all names.
		 */
		if (obsoletesEVR == NULL ||
		    headerMatchesDepFlags(h2,
			obsoletes[j], obsoletesEVR[j], obsoletesFlags[j]))
		{
		    (void) removePackage(ts, rpmdbGetIteratorOffset(mi), alNum);
		}
	    }
	    mi = rpmdbFreeIterator(mi);
	  }
	}

	obsoletesEVR = hfd(obsoletesEVR, ovt);
	obsoletes = hfd(obsoletes, ont);
    }

    return 0;
}

int rpmtransRemovePackage(rpmTransactionSet ts, int dboffset)
{
    return removePackage(ts, dboffset, -1);
}

rpmTransactionSet rpmtransFree(rpmTransactionSet ts)
{
    if (ts) {
	alFree(&ts->addedPackages);
	ts->di = _free(ts->di);
	ts->removedPackages = _free(ts->removedPackages);
	ts->order = _free(ts->order);
	if (ts->scriptFd != NULL)
	    ts->scriptFd =
		fdFree(ts->scriptFd, "rpmtransSetScriptFd (rpmtransFree");
	ts->rootDir = _free(ts->rootDir);
	ts->currDir = _free(ts->currDir);

	ts = _free(ts);
    }
    return NULL;
}

rpmDependencyConflict rpmdepFreeConflicts(rpmDependencyConflict conflicts,
		int numConflicts)
{
    int i;

    if (conflicts)
    for (i = 0; i < numConflicts; i++) {
	conflicts[i].byHeader = headerFree(conflicts[i].byHeader);
	conflicts[i].byName = _free(conflicts[i].byName);
	conflicts[i].byVersion = _free(conflicts[i].byVersion);
	conflicts[i].byRelease = _free(conflicts[i].byRelease);
	conflicts[i].needsName = _free(conflicts[i].needsName);
	conflicts[i].needsVersion = _free(conflicts[i].needsVersion);
    }

    return (conflicts = _free(conflicts));
}

/**
 * Check key for an unsatisfied dependency.
 * @todo Eliminate rpmrc provides.
 * @param ts		transaction set
 * @param keyType	type of dependency
 * @param keyDepend	dependency string representation
 * @param keyName	dependency name string
 * @param keyEVR	dependency [epoch:]version[-release] string
 * @param keyFlags	dependency logical range qualifiers
 * @return		0 if satisfied, 1 if not satisfied, 2 if error
 */
static int unsatisfiedDepend(rpmTransactionSet ts,
		const char * keyType, const char * keyDepend,
		const char * keyName, const char * keyEVR, int keyFlags)
	/*@modifies ts @*/
{
    rpmdbMatchIterator mi;
    Header h;
    int rc = 0;	/* assume dependency is satisfied */

    /*
     * Check if dbiOpen/dbiPut failed (e.g. permissions), we can't cache.
     */
    if (_cacheDependsRC) {
	dbiIndex dbi;
	dbi = dbiOpen(ts->rpmdb, RPMDBI_DEPENDS, 0);
	if (dbi == NULL)
	    _cacheDependsRC = 0;
	else {
	    DBC * dbcursor = NULL;
	    size_t keylen = strlen(keyDepend);
	    void * datap = NULL;
	    size_t datalen = 0;
	    int xx;
	    xx = dbiCopen(dbi, &dbcursor, 0);
	    /*@-mods@*/		/* FIX: keyDepends mod undocumented. */
	    xx = dbiGet(dbi, dbcursor, (void **)&keyDepend, &keylen, &datap, &datalen, 0);
	    /*@=mods@*/
	    if (xx == 0 && datap && datalen == 4) {
		memcpy(&rc, datap, datalen);
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s %-s (cached)\n"),
			keyType, keyDepend, (rc ? _("NO ") : _("YES")));
		xx = dbiCclose(dbi, NULL, 0);

		return rc;
	    }
	    xx = dbiCclose(dbi, dbcursor, 0);
	}
    }

#ifdef	DYING
  { static /*@observer@*/ const char noProvidesString[] = "nada";
    static /*@observer@*/ const char * rcProvidesString = noProvidesString;
    const char * start;
    int i;

    if (rcProvidesString == noProvidesString)
	rcProvidesString = rpmGetVar(RPMVAR_PROVIDES);

    if (rcProvidesString != NULL && !(keyFlags & RPMSENSE_SENSEMASK)) {
	i = strlen(keyName);
	/*@-observertrans -mayaliasunique@*/
	while ((start = strstr(rcProvidesString, keyName))) {
	/*@=observertrans =mayaliasunique@*/
	    if (xisspace(start[i]) || start[i] == '\0' || start[i] == ',') {
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (rpmrc provides)\n"),
			keyType, keyDepend+2);
		goto exit;
	    }
	    rcProvidesString = start + 1;
	}
    }
  }
#endif

    /*
     * New features in rpm packaging implicitly add versioned dependencies
     * on rpmlib provides. The dependencies look like "rpmlib(YaddaYadda)".
     * Check those dependencies now.
     */
    if (!strncmp(keyName, "rpmlib(", sizeof("rpmlib(")-1)) {
	if (rpmCheckRpmlibProvides(keyName, keyEVR, keyFlags)) {
	    rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (rpmlib provides)\n"),
			keyType, keyDepend+2);
	    goto exit;
	}
	goto unsatisfied;
    }

    if (alSatisfiesDepend(&ts->addedPackages, keyType, keyDepend,
		keyName, keyEVR, keyFlags))
    {
	goto exit;
    }

    /* XXX only the installer does not have the database open here. */
    if (ts->rpmdb != NULL) {
	if (*keyName == '/' && (keyFlags & RPMSENSE_SENSEMASK) == 0) {

	    mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_BASENAMES, keyName, 0);

	    (void) rpmdbPruneIterator(mi,
			ts->removedPackages, ts->numRemovedPackages, 1);

	    while ((h = rpmdbNextIterator(mi)) != NULL) {
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (db files)\n"),
			keyType, keyDepend+2);
		mi = rpmdbFreeIterator(mi);
		goto exit;
	    }
	    mi = rpmdbFreeIterator(mi);
	}

	mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_PROVIDENAME, keyName, 0);
	(void) rpmdbPruneIterator(mi,
			ts->removedPackages, ts->numRemovedPackages, 1);
	while ((h = rpmdbNextIterator(mi)) != NULL) {
	    if (rangeMatchesDepFlags(h, keyName, keyEVR, keyFlags)) {
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (db provides)\n"),
			keyType, keyDepend+2);
		mi = rpmdbFreeIterator(mi);
		goto exit;
	    }
	}
	mi = rpmdbFreeIterator(mi);

#ifndef DYING
	mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_NAME, keyName, 0);
	(void) rpmdbPruneIterator(mi,
			ts->removedPackages, ts->numRemovedPackages, 1);
	while ((h = rpmdbNextIterator(mi)) != NULL) {
	    if (rangeMatchesDepFlags(h, keyName, keyEVR, keyFlags)) {
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (db package)\n"),
			keyType, keyDepend+2);
		mi = rpmdbFreeIterator(mi);
		goto exit;
	    }
	}
	mi = rpmdbFreeIterator(mi);
#endif

    }

unsatisfied:
    rpmMessage(RPMMESS_DEBUG, _("%s: %-45s NO\n"), keyType, keyDepend+2);
    rc = 1;	/* dependency is unsatisfied */

exit:
    /*
     * If dbiOpen/dbiPut fails (e.g. permissions), we can't cache.
     */
    if (_cacheDependsRC) {
	dbiIndex dbi;
	dbi = dbiOpen(ts->rpmdb, RPMDBI_DEPENDS, 0);
	if (dbi == NULL) {
	    _cacheDependsRC = 0;
	} else {
	    DBC * dbcursor = NULL;
	    int xx;
	    xx = dbiCopen(dbi, &dbcursor, DBI_WRITECURSOR);
	    xx = dbiPut(dbi, dbcursor, keyDepend, strlen(keyDepend), &rc, sizeof(rc), 0);
	    if (xx)
		_cacheDependsRC = 0;
#if 0	/* XXX NOISY */
	    else
		rpmMessage(RPMMESS_DEBUG, _("%s: (%s, %s) added to Depends cache.\n"), keyType, keyDepend, (rc ? _("NO ") : _("YES")));
#endif
	    xx = dbiCclose(dbi, dbcursor, DBI_WRITECURSOR);
	}
    }
    return rc;
}

/**
 * Check header requires/conflicts against against installed+added packages.
 * @param ts		transaction set
 * @param psp		dependency problems
 * @param h		header to check
 * @param keyName	dependency name
 * @return		0 no problems found
 */
static int checkPackageDeps(rpmTransactionSet ts, problemsSet psp,
		Header h, const char * keyName)
	/*@modifies ts, h, psp */
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    HFD_t hfd = headerFreeData;
    rpmTagType rnt, rvt;
    rpmTagType cnt, cvt;
    const char * name, * version, * release;
    const char ** requires;
    const char ** requiresEVR = NULL;
    int_32 * requireFlags = NULL;
    int requiresCount = 0;
    const char ** conflicts;
    const char ** conflictsEVR = NULL;
    int_32 * conflictFlags = NULL;
    int conflictsCount = 0;
    rpmTagType type;
    int i, rc;
    int ourrc = 0;

    (void) headerNVR(h, &name, &version, &release);

    if (!hge(h, RPMTAG_REQUIRENAME, &rnt, (void **) &requires, &requiresCount))
    {
	requiresCount = 0;
	rvt = RPM_STRING_ARRAY_TYPE;
    } else {
	(void)hge(h, RPMTAG_REQUIREFLAGS, NULL, (void **) &requireFlags, NULL);
	(void)hge(h, RPMTAG_REQUIREVERSION, &rvt, (void **) &requiresEVR, NULL);
    }

    for (i = 0; i < requiresCount && !ourrc; i++) {
	const char * keyDepend;

	/* Filter out requires that came along for the ride. */
	if (keyName && strcmp(keyName, requires[i]))
	    continue;

	keyDepend = printDepend("R",
		requires[i], requiresEVR[i], requireFlags[i]);

	rc = unsatisfiedDepend(ts, " Requires", keyDepend,
		requires[i], requiresEVR[i], requireFlags[i]);

	switch (rc) {
	case 0:		/* requirements are satisfied. */
	    break;
	case 1:		/* requirements are not satisfied. */
	    rpmMessage(RPMMESS_DEBUG, _("package %s-%s-%s require not satisfied: %s\n"),
		    name, version, release, keyDepend+2);

	    AUTO_REALLOC(psp->problems, psp->num);

	    {	rpmDependencyConflict pp = psp->problems + psp->num;
		pp->byHeader = headerLink(h);
		pp->byName = xstrdup(name);
		pp->byVersion = xstrdup(version);
		pp->byRelease = xstrdup(release);
		pp->needsName = xstrdup(requires[i]);
		pp->needsVersion = xstrdup(requiresEVR[i]);
		pp->needsFlags = requireFlags[i];
		pp->sense = RPMDEP_SENSE_REQUIRES;
	    }

	    psp->num++;
	    break;
	case 2:		/* something went wrong! */
	default:
	    ourrc = 1;
	    break;
	}
	keyDepend = _free(keyDepend);
    }

    if (requiresCount) {
	requiresEVR = hfd(requiresEVR, rvt);
	requires = hfd(requires, rnt);
    }

    if (!hge(h, RPMTAG_CONFLICTNAME, &cnt, (void **)&conflicts, &conflictsCount))
    {
	conflictsCount = 0;
	cvt = RPM_STRING_ARRAY_TYPE;
    } else {
	(void) hge(h, RPMTAG_CONFLICTFLAGS, &type,
		(void **) &conflictFlags, &conflictsCount);
	(void) hge(h, RPMTAG_CONFLICTVERSION, &cvt,
		(void **) &conflictsEVR, &conflictsCount);
    }

    for (i = 0; i < conflictsCount && !ourrc; i++) {
	const char * keyDepend;

	/* Filter out conflicts that came along for the ride. */
	if (keyName && strcmp(keyName, conflicts[i]))
	    continue;

	keyDepend = printDepend("C", conflicts[i], conflictsEVR[i], conflictFlags[i]);

	rc = unsatisfiedDepend(ts, "Conflicts", keyDepend,
		conflicts[i], conflictsEVR[i], conflictFlags[i]);

	/* 1 == unsatisfied, 0 == satsisfied */
	switch (rc) {
	case 0:		/* conflicts exist. */
	    rpmMessage(RPMMESS_DEBUG, _("package %s conflicts: %s\n"),
		    name, keyDepend+2);

	    AUTO_REALLOC(psp->problems, psp->num);

	    {	rpmDependencyConflict pp = psp->problems + psp->num;
		pp->byHeader = headerLink(h);
		pp->byName = xstrdup(name);
		pp->byVersion = xstrdup(version);
		pp->byRelease = xstrdup(release);
		pp->needsName = xstrdup(conflicts[i]);
		pp->needsVersion = xstrdup(conflictsEVR[i]);
		pp->needsFlags = conflictFlags[i];
		pp->sense = RPMDEP_SENSE_CONFLICTS;
	    }

	    psp->num++;
	    break;
	case 1:		/* conflicts don't exist. */
	    break;
	case 2:		/* something went wrong! */
	default:
	    ourrc = 1;
	    break;
	}
	keyDepend = _free(keyDepend);
    }

    if (conflictsCount) {
	conflictsEVR = hfd(conflictsEVR, cvt);
	conflicts = hfd(conflicts, cnt);
    }

    return ourrc;
}

/**
 * Check dependency against installed packages.
 * Adding: check name/provides key against each conflict match,
 * Erasing: check name/provides/filename key against each requiredby match.
 * @param ts		transaction set
 * @param psp		dependency problems
 * @param key		dependency name
 * @param mi		rpm database iterator
 * @return		0 no problems found
 */
static int checkPackageSet(rpmTransactionSet ts, problemsSet psp,
		const char * key, /*@only@*/ /*@null@*/ rpmdbMatchIterator mi)
	/*@modifies ts, mi, psp @*/
{
    Header h;
    int rc = 0;

    (void) rpmdbPruneIterator(mi,
		ts->removedPackages, ts->numRemovedPackages, 1);
    while ((h = rpmdbNextIterator(mi)) != NULL) {
	if (checkPackageDeps(ts, psp, h, key)) {
	    rc = 1;
	    break;
	}
    }
    mi = rpmdbFreeIterator(mi);

    return rc;
}

/**
 * Erasing: check name/provides/filename key against requiredby matches.
 * @param ts		transaction set
 * @param psp		dependency problems
 * @param key		requires name
 * @return		0 no problems found
 */
static int checkDependentPackages(rpmTransactionSet ts,
			problemsSet psp, const char * key)
	/*@modifies ts, psp @*/
{
    rpmdbMatchIterator mi;
    mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_REQUIRENAME, key, 0);
    return checkPackageSet(ts, psp, key, mi);
}

/**
 * Adding: check name/provides key against conflicts matches.
 * @param ts		transaction set
 * @param psp		dependency problems
 * @param key		conflicts name
 * @return		0 no problems found
 */
static int checkDependentConflicts(rpmTransactionSet ts,
		problemsSet psp, const char * key)
	/*@modifies ts, psp @*/
{
    int rc = 0;

    if (ts->rpmdb) {	/* XXX is this necessary? */
	rpmdbMatchIterator mi;
	mi = rpmdbInitIterator(ts->rpmdb, RPMTAG_CONFLICTNAME, key, 0);
	rc = checkPackageSet(ts, psp, key, mi);
    }

    return rc;
}

struct badDeps_s {
/*@observer@*/ /*@null@*/ const char * pname;
/*@observer@*/ /*@null@*/ const char * qname;
};

#ifdef	DYING
static struct badDeps_s {
/*@observer@*/ /*@null@*/ const char * pname;
/*@observer@*/ /*@null@*/ const char * qname;
} badDeps[] = {
    { "libtermcap", "bash" },
    { "modutils", "vixie-cron" },
    { "ypbind", "yp-tools" },
    { "ghostscript-fonts", "ghostscript" },
    /* 7.2 only */
    { "libgnomeprint15", "gnome-print" },
    { "nautilus", "nautilus-mozilla" },
    /* 7.1 only */
    { "arts", "kdelibs-sound" },
    /* 7.0 only */
    { "pango-gtkbeta-devel", "pango-gtkbeta" },
    { "XFree86", "Mesa" },
    { "compat-glibc", "db2" },
    { "compat-glibc", "db1" },
    { "pam", "initscripts" },
    { "initscripts", "sysklogd" },
    /* 6.2 */
    { "egcs-c++", "libstdc++" },
    /* 6.1 */
    { "pilot-link-devel", "pilot-link" },
    /* 5.2 */
    { "pam", "pamconfig" },
    { NULL, NULL }
};
#else
static struct badDeps_s * badDeps = NULL;
#endif

/**
 * Check for dependency relations to be ignored.
 *
 * @param p	successor package (i.e. with Requires: )
 * @param q	predecessor package (i.e. with Provides: )
 * @return	1 if dependency is to be ignored.
 */
static int ignoreDep(const struct availablePackage * p,
		const struct availablePackage * q)
	/*@*/
{
    struct badDeps_s * bdp;
    static int _initialized = 0;
    const char ** av = NULL;
    int ac = 0;

    if (!_initialized) {
	char * s = rpmExpand("%{?_dependency_whiteout}", NULL);
	int i;

	if (s != NULL && *s != '\0'
	&& !(i = poptParseArgvString(s, &ac, (const char ***)&av))
	&& ac > 0 && av != NULL)
	{
	    bdp = badDeps = xcalloc(ac+1, sizeof(*badDeps));
	    for (i = 0; i < ac; i++, bdp++) {
		char * p, * q;

		if (av[i] == NULL)
		    break;
		p = xstrdup(av[i]);
		if ((q = strchr(p, '>')) != NULL)
		    *q++ = '\0';
		bdp->pname = p;
		bdp->qname = q;
		rpmMessage(RPMMESS_DEBUG,
			_("ignore package name relation(s) [%d]\t%s -> %s\n"),
			i, bdp->pname, bdp->qname);
	    }
	    bdp->pname = bdp->qname = NULL;
	}
	av = _free(av);
	s = _free(s);
	_initialized++;
    }

    if (badDeps != NULL)
    for (bdp = badDeps; bdp->pname != NULL && bdp->qname != NULL; bdp++) {
	if (!strcmp(p->name, bdp->pname) && !strcmp(q->name, bdp->qname))
	    return 1;
    }
    return 0;
}

/**
 * Recursively mark all nodes with their predecessors.
 * @param tsi		successor chain
 * @param q		predecessor
 */
static void markLoop(/*@special@*/ tsortInfo tsi,
		struct availablePackage * q)
	/*@uses tsi @*/
	/*@modifies internalState @*/
{
    struct availablePackage * p;

    while (tsi != NULL && (p = tsi->tsi_suc) != NULL) {
	tsi = tsi->tsi_next;
	if (p->tsi.tsi_pkg != NULL)
	    continue;
	p->tsi.tsi_pkg = q;
	if (p->tsi.tsi_next != NULL)
	    markLoop(p->tsi.tsi_next, p);
    }
}

static inline /*@observer@*/ const char * identifyDepend(int_32 f)
{
    if (isLegacyPreReq(f))
	return "PreReq:";
    f = _notpre(f);
    if (f & RPMSENSE_SCRIPT_PRE)
	return "Requires(pre):";
    if (f & RPMSENSE_SCRIPT_POST)
	return "Requires(post):";
    if (f & RPMSENSE_SCRIPT_PREUN)
	return "Requires(preun):";
    if (f & RPMSENSE_SCRIPT_POSTUN)
	return "Requires(postun):";
    if (f & RPMSENSE_SCRIPT_VERIFY)
	return "Requires(verify):";
    if (f & RPMSENSE_FIND_REQUIRES)
	return "Requires(auto):";
    return "Requires:";
}

/**
 * Find (and eliminate co-requisites) "q <- p" relation in dependency loop.
 * Search all successors of q for instance of p. Format the specific relation,
 * (e.g. p contains "Requires: q"). Unlink and free co-requisite (i.e.
 * pure Requires: dependencies) successor node(s).
 * @param q		sucessor (i.e. package required by p)
 * @param p		predecessor (i.e. package that "Requires: q")
 * @param zap		max. no. of co-requisites to remove (-1 is all)?
 * @retval nzaps	address of no. of relations removed
 * @return		(possibly NULL) formatted "q <- p" releation (malloc'ed)
 */
static /*@owned@*/ /*@null@*/ const char *
zapRelation(struct availablePackage * q, struct availablePackage * p,
		int zap, /*@in@*/ /*@out@*/ int * nzaps)
	/*@modifies q, p, *nzaps @*/
{
    tsortInfo tsi_prev;
    tsortInfo tsi;
    const char *dp = NULL;

    for (tsi_prev = &q->tsi, tsi = q->tsi.tsi_next;
	 tsi != NULL;
	/* XXX Note: the loop traverses "not found", break on "found". */
	/*@-nullderef@*/
	 tsi_prev = tsi, tsi = tsi->tsi_next)
	/*@=nullderef@*/
    {
	int j;

	if (tsi->tsi_suc != p)
	    continue;
	if (p->requires == NULL) continue;	/* XXX can't happen */
	if (p->requireFlags == NULL) continue;	/* XXX can't happen */
	if (p->requiresEVR == NULL) continue;	/* XXX can't happen */

	j = tsi->tsi_reqx;
	dp = printDepend( identifyDepend(p->requireFlags[j]),
		p->requires[j], p->requiresEVR[j], p->requireFlags[j]);

	/*
	 * Attempt to unravel a dependency loop by eliminating Requires's.
	 */
	if (zap && !(p->requireFlags[j] & RPMSENSE_PREREQ)) {
	    rpmMessage(RPMMESS_DEBUG,
			_("removing %s-%s-%s \"%s\" from tsort relations.\n"),
			p->name, p->version, p->release, dp);
	    p->tsi.tsi_count--;
	    if (tsi_prev) tsi_prev->tsi_next = tsi->tsi_next;
	    tsi->tsi_next = NULL;
	    tsi->tsi_suc = NULL;
	    tsi = _free(tsi);
	    if (nzaps)
		(*nzaps)++;
	    if (zap)
		zap--;
	}
	/* XXX Note: the loop traverses "not found", get out now! */
	break;
    }
    return dp;
}

/**
 * Record next "q <- p" relation (i.e. "p" requires "q").
 * @param ts		transaction set
 * @param p		predecessor (i.e. package that "Requires: q")
 * @param selected	boolean package selected array
 * @param j		relation index
 * @return		0 always
 */
static inline int addRelation( const rpmTransactionSet ts,
		struct availablePackage * p, unsigned char * selected, int j)
	/*@modifies p->tsi.tsi_u.count, p->depth, *selected @*/
{
    struct availablePackage * q;
    tsortInfo tsi;
    int matchNum;

    if (!p->requires || !p->requiresEVR || !p->requireFlags)
	return 0;

    q = alSatisfiesDepend(&ts->addedPackages, NULL, NULL,
		p->requires[j], p->requiresEVR[j], p->requireFlags[j]);

    /* Ordering depends only on added package relations. */
    if (q == NULL)
	return 0;

    /* Avoid rpmlib feature dependencies. */
    if (!strncmp(p->requires[j], "rpmlib(", sizeof("rpmlib(")-1))
	return 0;

    /* Avoid certain dependency relations. */
    if (ignoreDep(p, q))
	return 0;

    /* Avoid redundant relations. */
    /* XXX TODO: add control bit. */
    matchNum = q - ts->addedPackages.list;
    if (selected[matchNum] != 0)
	return 0;
    selected[matchNum] = 1;

    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
    p->tsi.tsi_count++;			/* bump p predecessor count */
    if (p->depth <= q->depth)		/* Save max. depth in dependency tree */
	p->depth = q->depth + 1;

    tsi = xmalloc(sizeof(*tsi));
    tsi->tsi_suc = p;
    tsi->tsi_reqx = j;
    tsi->tsi_next = q->tsi.tsi_next;
    q->tsi.tsi_next = tsi;
    q->tsi.tsi_qcnt++;			/* bump q successor count */
    return 0;
}

/**
 * Compare ordered list entries by index (qsort/bsearch).
 * @param one		1st ordered list entry
 * @param two		2nd ordered list entry
 * @return		result of comparison
 */
static int orderListIndexCmp(const void * one, const void * two)	/*@*/
{
    int a = ((const struct orderListIndex *)one)->alIndex;
    int b = ((const struct orderListIndex *)two)->alIndex;
    return (a - b);
}

/**
 * Add element to list sorting by initial successor count.
 * @param p		new element
 * @retval qp		address of first element
 * @retval rp		address of last element
 */
static void addQ(struct availablePackage * p,
		/*@in@*/ /*@out@*/ struct availablePackage ** qp,
		/*@in@*/ /*@out@*/ struct availablePackage ** rp)
	/*@modifies p->tsi, *qp, *rp @*/
{
    struct availablePackage *q, *qprev;

    /* Mark the package as queued. */
    p->tsi.tsi_reqx = 1;

    if ((*rp) == NULL) {	/* 1st element */
	(*rp) = (*qp) = p;
	return;
    }
    for (qprev = NULL, q = (*qp); q != NULL; qprev = q, q = q->tsi.tsi_suc) {
	if (q->tsi.tsi_qcnt <= p->tsi.tsi_qcnt)
	    break;
    }
    if (qprev == NULL) {	/* insert at beginning of list */
	p->tsi.tsi_suc = q;
	(*qp) = p;		/* new head */
    } else if (q == NULL) {	/* insert at end of list */
	qprev->tsi.tsi_suc = p;
	(*rp) = p;		/* new tail */
    } else {			/* insert between qprev and q */
	p->tsi.tsi_suc = q;
	qprev->tsi.tsi_suc = p;
    }
}

int rpmdepOrder(rpmTransactionSet ts)
{
    int npkgs = ts->addedPackages.size;
    int anaconda = 0;
    struct availablePackage * p;
    struct availablePackage * q;
    struct availablePackage * r;
    tsortInfo tsi;
    tsortInfo tsi_next;
    int * ordering = alloca(sizeof(*ordering) * (npkgs + 1));
    int orderingCount = 0;
    unsigned char * selected = alloca(sizeof(*selected) * (npkgs + 1));
    int loopcheck;
    transactionElement newOrder;
    int newOrderCount = 0;
    struct orderListIndex * orderList;
    int _printed = 0;
    int treex;
    int depth;
    int qlen;
    int i, j;

    /* T1. Initialize. */
    loopcheck = npkgs;

    /* Record all relations. */
    rpmMessage(RPMMESS_DEBUG, _("========== recording tsort relations\n"));
    if ((p = ts->addedPackages.list) != NULL)
    for (i = 0; i < npkgs; i++, p++) {
	int matchNum;

	if (p->requiresCount <= 0)
	    continue;

	memset(selected, 0, sizeof(*selected) * npkgs);

	/* Avoid narcisstic relations. */
	matchNum = p - ts->addedPackages.list;
	selected[matchNum] = 1;

	/* T2. Next "q <- p" relation. */

	/* First, do pre-requisites. */
	for (j = 0; j < p->requiresCount; j++) {

	    if (p->requireFlags == NULL) continue;	/* XXX can't happen */

	    /* Skip if not %pre/%post requires or legacy prereq. */

	    if (!( isInstallPreReq(p->requireFlags[j]) ||
		   isLegacyPreReq(p->requireFlags[j]) ))
		continue;

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
	    (void) addRelation(ts, p, selected, j);

	}

	/* Then do co-requisites. */
	for (j = 0; j < p->requiresCount; j++) {

	    if (p->requireFlags == NULL) continue;	/* XXX can't happen */

	    /* Skip if %pre/%post/%preun/%postun requires or legacy prereq. */

	    if (isErasePreReq(p->requireFlags[j]) ||
		 ( isInstallPreReq(p->requireFlags[j]) ||
		   isLegacyPreReq(p->requireFlags[j]) ))
		continue;

	    /* T3. Record next "q <- p" relation (i.e. "p" requires "q"). */
	    (void) addRelation(ts, p, selected, j);

	}
    }

    /* Save predecessor count and mark tree roots. */
    treex = 0;
    if ((p = ts->addedPackages.list) != NULL)
    for (i = 0; i < npkgs; i++, p++) {
	p->npreds = p->tsi.tsi_count;
	p->tree = (p->npreds == 0 ? treex++ : -1);
    }

    /* T4. Scan for zeroes. */
    rpmMessage(RPMMESS_DEBUG, _("========== tsorting packages (order, #predecessors, #succesors, tree, depth)\n"));

rescan:
    q = r = NULL;
    qlen = 0;
    if ((p = ts->addedPackages.list) != NULL)
    for (i = 0; i < npkgs; i++, p++) {

	/* Prefer packages in chainsaw or anaconda presentation order. */
	if (anaconda)
	    p->tsi.tsi_qcnt = (npkgs - i);

	if (p->tsi.tsi_count != 0)
	    continue;
	p->tsi.tsi_suc = NULL;
	addQ(p, &q, &r);
	qlen++;
    }

    /* T5. Output front of queue (T7. Remove from queue.) */
    for (; q != NULL; q = q->tsi.tsi_suc) {

	/* Mark the package as unqueued. */
	q->tsi.tsi_reqx = 0;

	rpmMessage(RPMMESS_DEBUG, "%5d%5d%5d%5d%5d %*s %s-%s-%s\n",
			orderingCount, q->npreds, q->tsi.tsi_qcnt,
			q->tree, q->depth,
			2*q->depth, "",
			q->name, q->version, q->release);

	treex = q->tree;
	depth = q->depth;
	q->degree = 0;

	ordering[orderingCount++] = q - ts->addedPackages.list;
	qlen--;
	loopcheck--;

	/* T6. Erase relations. */
	tsi_next = q->tsi.tsi_next;
	q->tsi.tsi_next = NULL;
	while ((tsi = tsi_next) != NULL) {
	    tsi_next = tsi->tsi_next;
	    tsi->tsi_next = NULL;
	    p = tsi->tsi_suc;
	    if (p && (--p->tsi.tsi_count) <= 0) {

		p->tree = treex;
		p->depth = depth + 1;
		p->parent = q;
		q->degree++;

		/* XXX TODO: add control bit. */
		p->tsi.tsi_suc = NULL;
		/*@-nullstate@*/	/* FIX: q->tsi.tsi_u.suc may be NULL */
		addQ(p, &q->tsi.tsi_suc, &r);
		/*@=nullstate@*/
		qlen++;
	    }
	    tsi = _free(tsi);
	}
	if (!_printed && loopcheck == qlen && q->tsi.tsi_suc != NULL) {
	    _printed++;
	    rpmMessage(RPMMESS_DEBUG,
		_("========== successors only (presentation order)\n"));

	    /* Relink the queue in presentation order. */
	    tsi = &q->tsi;
	    if ((p = ts->addedPackages.list) != NULL)
	    for (i = 0; i < npkgs; i++, p++) {
		/* Is this element in the queue? */
		if (p->tsi.tsi_reqx == 0)
		    /*@innercontinue@*/ continue;
		tsi->tsi_suc = p;
		tsi = &p->tsi;
	    }
	    tsi->tsi_suc = NULL;
	}
    }

    /* T8. End of process. Check for loops. */
    if (loopcheck != 0) {
	int nzaps;

	/* T9. Initialize predecessor chain. */
	nzaps = 0;
	if ((q = ts->addedPackages.list) != NULL)
	for (i = 0; i < npkgs; i++, q++) {
	    q->tsi.tsi_pkg = NULL;
	    q->tsi.tsi_reqx = 0;
	    /* Mark packages already sorted. */
	    if (q->tsi.tsi_count == 0)
		q->tsi.tsi_count = -1;
	}

	/* T10. Mark all packages with their predecessors. */
	if ((q = ts->addedPackages.list) != NULL)
	for (i = 0; i < npkgs; i++, q++) {
	    if ((tsi = q->tsi.tsi_next) == NULL)
		continue;
	    q->tsi.tsi_next = NULL;
	    markLoop(tsi, q);
	    q->tsi.tsi_next = tsi;
	}

	/* T11. Print all dependency loops. */
	if ((r = ts->addedPackages.list) != NULL)
	for (i = 0; i < npkgs; i++, r++) {
	    int printed;

	    printed = 0;

	    /* T12. Mark predecessor chain, looking for start of loop. */
	    for (q = r->tsi.tsi_pkg; q != NULL; q = q->tsi.tsi_pkg) {
		if (q->tsi.tsi_reqx)
		    /*@innerbreak@*/ break;
		q->tsi.tsi_reqx = 1;
	    }

	    /* T13. Print predecessor chain from start of loop. */
	    while ((p = q) != NULL && (q = p->tsi.tsi_pkg) != NULL) {
		const char * dp;
		char buf[4096];

		/* Unchain predecessor loop. */
		p->tsi.tsi_pkg = NULL;

		if (!printed) {
		    rpmMessage(RPMMESS_DEBUG, _("LOOP:\n"));
		    printed = 1;
		}

		/* Find (and destroy if co-requisite) "q <- p" relation. */
		dp = zapRelation(q, p, 1, &nzaps);

		/* Print next member of loop. */
		sprintf(buf, "%s-%s-%s", p->name, p->version, p->release);
		rpmMessage(RPMMESS_DEBUG, "    %-40s %s\n", buf,
			(dp ? dp : "not found!?!"));

		dp = _free(dp);
	    }

	    /* Walk (and erase) linear part of predecessor chain as well. */
	    for (p = r, q = r->tsi.tsi_pkg;
		 q != NULL;
		 p = q, q = q->tsi.tsi_pkg)
	    {
		/* Unchain linear part of predecessor loop. */
		p->tsi.tsi_pkg = NULL;
		p->tsi.tsi_reqx = 0;
	    }
	}

	/* If a relation was eliminated, then continue sorting. */
	/* XXX TODO: add control bit. */
	if (nzaps) {
	    rpmMessage(RPMMESS_DEBUG, _("========== continuing tsort ...\n"));
	    goto rescan;
	}
	return 1;
    }

    /*
     * The order ends up as installed packages followed by removed packages,
     * with removes for upgrades immediately following the installation of
     * the new package. This would be easier if we could sort the
     * addedPackages array, but we store indexes into it in various places.
     */
    orderList = xmalloc(npkgs * sizeof(*orderList));
    for (i = 0, j = 0; i < ts->orderCount; i++) {
	if (ts->order[i].type == TR_ADDED) {
	    orderList[j].alIndex = ts->order[i].u.addedIndex;
	    orderList[j].orIndex = i;
	    j++;
	}
    }
    assert(j <= npkgs);

    qsort(orderList, npkgs, sizeof(*orderList), orderListIndexCmp);

    newOrder = xmalloc(ts->orderCount * sizeof(*newOrder));
    for (i = 0, newOrderCount = 0; i < orderingCount; i++) {
	struct orderListIndex * needle, key;

	key.alIndex = ordering[i];
	needle = bsearch(&key, orderList, npkgs, sizeof(key),orderListIndexCmp);
	/* bsearch should never, ever fail */
	if (needle == NULL) continue;

	newOrder[newOrderCount++] = ts->order[needle->orIndex];
	for (j = needle->orIndex + 1; j < ts->orderCount; j++) {
	    if (ts->order[j].type == TR_REMOVED &&
		ts->order[j].u.removed.dependsOnIndex == needle->alIndex) {
		newOrder[newOrderCount++] = ts->order[j];
	    } else
		/*@innerbreak@*/ break;
	}
    }

    for (i = 0; i < ts->orderCount; i++) {
	if (ts->order[i].type == TR_REMOVED &&
	    ts->order[i].u.removed.dependsOnIndex == -1)  {
	    newOrder[newOrderCount++] = ts->order[i];
	}
    }
    assert(newOrderCount == ts->orderCount);

    ts->order = _free(ts->order);
    ts->order = newOrder;
    orderList = _free(orderList);

    return 0;
}

/**
 * Close a single database index.
 * @param db		rpm database
 * @param rpmtag	rpm tag
 * @return              0 on success
 */
static int rpmdbCloseDBI(/*@null@*/ rpmdb db, int rpmtag)
	/*@ modifies db, fileSystem @*/
{
    int dbix;
    int rc = 0;

    if (db == NULL || db->_dbi == NULL || dbiTags == NULL)
	return 0;

    for (dbix = 0; dbix < dbiTagsMax; dbix++) {
	if (dbiTags[dbix] != rpmtag)
	    continue;
	if (db->_dbi[dbix] != NULL) {
	    int xx;
	    /*@-unqualifiedtrans@*/		/* FIX: double indirection. */
	    xx = dbiClose(db->_dbi[dbix], 0);
	    if (xx && rc == 0) rc = xx;
	    db->_dbi[dbix] = NULL;
	    /*@=unqualifiedtrans@*/
	}
	break;
    }
    return rc;
}

int rpmdepCheck(rpmTransactionSet ts,
		rpmDependencyConflict * conflicts, int * numConflicts)
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    HFD_t hfd = headerFreeData;
    rpmdbMatchIterator mi = NULL;
    Header h = NULL;
    struct availablePackage * p;
    problemsSet ps;
    int npkgs;
    int i, j;
    int rc;

    npkgs = ts->addedPackages.size;

    ps = xcalloc(1, sizeof(*ps));
    ps->num = 0;
    ps->problems = NULL;

    *conflicts = NULL;
    *numConflicts = 0;

    /*
     * Look at all of the added packages and make sure their dependencies
     * are satisfied.
     */
    if ((p = ts->addedPackages.list) != NULL)
    for (i = 0; i < npkgs; i++, p++)
    {

        rpmMessage(RPMMESS_DEBUG,  "========== +++ %s-%s-%s\n" ,
		p->name, p->version, p->release);
	rc = checkPackageDeps(ts, ps, p->h, NULL);
	if (rc)
	    goto exit;

	/* Adding: check name against conflicts matches. */
	rc = checkDependentConflicts(ts, ps, p->name);
	if (rc)
	    goto exit;

	if (p->providesCount == 0 || p->provides == NULL)
	    continue;

	rc = 0;
	for (j = 0; j < p->providesCount; j++) {
	    /* Adding: check provides key against conflicts matches. */
	    if (!checkDependentConflicts(ts, ps, p->provides[j]))
		continue;
	    rc = 1;
	    /*@innerbreak@*/ break;
	}
	if (rc)
	    goto exit;
    }

    /*
     * Look at the removed packages and make sure they aren't critical.
     */
    if (ts->numRemovedPackages > 0) {
      mi = rpmdbInitIterator(ts->rpmdb, RPMDBI_PACKAGES, NULL, 0);
      (void) rpmdbAppendIterator(mi,
			ts->removedPackages, ts->numRemovedPackages);
      while ((h = rpmdbNextIterator(mi)) != NULL) {

	{   const char * name, * version, * release;
	    (void) headerNVR(h, &name, &version, &release);
	    rpmMessage(RPMMESS_DEBUG,  "========== --- %s-%s-%s\n" ,
		name, version, release);

	    /* Erasing: check name against requiredby matches. */
	    rc = checkDependentPackages(ts, ps, name);
	    if (rc)
		goto exit;
	}

	{   const char ** provides;
	    int providesCount;
	    rpmTagType pnt;

	    if (hge(h, RPMTAG_PROVIDENAME, &pnt, (void **) &provides,
				&providesCount))
	    {
		rc = 0;
		for (j = 0; j < providesCount; j++) {
		    /* Erasing: check provides against requiredby matches. */
		    if (!checkDependentPackages(ts, ps, provides[j]))
			continue;
		    rc = 1;
		    /*@innerbreak@*/ break;
		}
		provides = hfd(provides, pnt);
		if (rc)
		    goto exit;
	    }
	}

	{   const char ** baseNames, ** dirNames;
	    int_32 * dirIndexes;
	    rpmTagType dnt, bnt;
	    int fileCount;
	    char * fileName = NULL;
	    int fileAlloced = 0;
	    int len;

	    if (hge(h, RPMTAG_BASENAMES, &bnt, (void **) &baseNames, &fileCount))
	    {
		(void) hge(h, RPMTAG_DIRNAMES, &dnt, (void **) &dirNames, NULL);
		(void) hge(h, RPMTAG_DIRINDEXES, NULL, (void **) &dirIndexes,
				NULL);
		rc = 0;
		for (j = 0; j < fileCount; j++) {
		    len = strlen(baseNames[j]) + 1 + 
			  strlen(dirNames[dirIndexes[j]]);
		    if (len > fileAlloced) {
			fileAlloced = len * 2;
			fileName = xrealloc(fileName, fileAlloced);
		    }
		    *fileName = '\0';
		    (void) stpcpy( stpcpy(fileName, dirNames[dirIndexes[j]]) , baseNames[j]);
		    /* Erasing: check filename against requiredby matches. */
		    if (!checkDependentPackages(ts, ps, fileName))
			continue;
		    rc = 1;
		    /*@innerbreak@*/ break;
		}

		fileName = _free(fileName);
		baseNames = hfd(baseNames, bnt);
		dirNames = hfd(dirNames, dnt);
		if (rc)
		    goto exit;
	    }
	}

      }
      mi = rpmdbFreeIterator(mi);
    }

    if (ps->num) {
	*conflicts = ps->problems;
	ps->problems = NULL;
	*numConflicts = ps->num;
    }
    rc = 0;

exit:
    mi = rpmdbFreeIterator(mi);
    ps->problems = _free(ps->problems);
    ps = _free(ps);
    if (_cacheDependsRC)
	(void) rpmdbCloseDBI(ts->rpmdb, RPMDBI_DEPENDS);
    return rc;
}
