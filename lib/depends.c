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

/*@only@*/ char * printDepend(const char * depend, const char * key,
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
	    provideFlags[i] |= RPMSENSE_EQUAL; /* ALT21-139-g6cb9a9a */
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

    if (alSatisfiesDepend(&ts->addedPackages, keyName, keyEVR, keyFlags))
    {
	/* XXX here we do not discern between files and provides */
	rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (added provide)\n"),
			keyType, keyDepend+2);
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
