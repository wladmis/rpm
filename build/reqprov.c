/** \ingroup rpmbuild
 * \file build/reqprov.c
 *  Add dependency tags to package header(s).
 */

#include "system.h"

#include "rpmbuild.h"
#include "debug.h"

typedef enum {
	DEP_UND = 0, /* uncomparable */
	DEP_STR = 1, /* stronger or same */
	DEP_WKR = -1 /* weaker or same */
} dep_compare_t;

static dep_compare_t compare_sense_flags (rpmTag tag, int cmp,
	rpmsenseFlags a, rpmsenseFlags b)
{
	if (cmp < 0)
	{
		/* Aevr < Bevr */
		switch (tag)
		{
			case RPMTAG_REQUIREFLAGS:
				if ((a == 0) && (b != 0))
					return DEP_WKR;
				if (!(a & RPMSENSE_GREATER) && (b & RPMSENSE_LESS))
					return DEP_STR;
				if (!(b & RPMSENSE_LESS) && (a & RPMSENSE_GREATER))
					return DEP_WKR;
				return DEP_UND;
			case RPMTAG_PROVIDEFLAGS:
				if ((a == 0) && (b != 0))
					return DEP_WKR;
				if (!(b & RPMSENSE_LESS) && (a & RPMSENSE_GREATER))
					return DEP_STR;
				if (!(a & RPMSENSE_GREATER) && (b & RPMSENSE_LESS))
					return DEP_WKR;
				return DEP_UND;
			default:
				if ((a == 0) && (b != 0))
					return DEP_STR;
				if (!(b & RPMSENSE_LESS) && (a & RPMSENSE_GREATER))
					return DEP_STR;
				if (!(a & RPMSENSE_GREATER) && (b & RPMSENSE_LESS))
					return DEP_WKR;
				return DEP_UND;
		}
	} else if (cmp > 0)
	{
		/* Aevr > Bevr */
		return -compare_sense_flags (tag, -cmp, b, a);
	} else /* cmp == 0 */
	{
		/* Aevr == Bevr */
		if ((a & b) == a)
			return (tag == RPMTAG_REQUIREFLAGS) ? DEP_STR : DEP_WKR;
		if ((a & b) == b)
			return (tag == RPMTAG_REQUIREFLAGS) ? DEP_WKR : DEP_STR;
		return DEP_UND;
	}
}

static dep_compare_t compare_deps (rpmTag tag,
	const char *Aevr, rpmsenseFlags Aflags,
	const char *Bevr, rpmsenseFlags Bflags)
{
	dep_compare_t rc = DEP_UND, cmp_rc;
	rpmsenseFlags Asense, Bsense;
	int sense;
	char *aEVR, *bEVR;
	const char *aE, *aV, *aR, *bE, *bV, *bR;

	/* filter out noise */
	Aflags &= ~(RPMSENSE_FIND_REQUIRES | RPMSENSE_FIND_PROVIDES | RPMSENSE_MULTILIB);
	Bflags &= ~(RPMSENSE_FIND_REQUIRES | RPMSENSE_FIND_PROVIDES | RPMSENSE_MULTILIB);

	Asense = Aflags & RPMSENSE_SENSEMASK;
	Bsense = Bflags & RPMSENSE_SENSEMASK;

	/* 0. sanity checks */
	switch (tag) {
		case RPMTAG_PROVIDEFLAGS:
		case RPMTAG_OBSOLETEFLAGS:
		case RPMTAG_CONFLICTFLAGS:
		case RPMTAG_REQUIREFLAGS:
			break;
		default:
			/* no way to optimize this case. */
			if (Aflags == Bflags && !strcmp (Aevr, Bevr))
				return DEP_STR;
			return DEP_UND;
	}
	if (
	    ((Asense & RPMSENSE_LESS) && (Asense & RPMSENSE_GREATER)) ||
	    ((Bsense & RPMSENSE_LESS) && (Bsense & RPMSENSE_GREATER)) ||
	    ((Asense == 0) ^ (Aevr[0] == 0)) ||
	    ((Bsense == 0) ^ (Bevr[0] == 0))
	   )
		return DEP_UND;

	/* 1. filter out essentialy differ versions. */
	if (
	    ((Asense & RPMSENSE_LESS) && (Bsense & RPMSENSE_GREATER)) ||
	    ((Bsense & RPMSENSE_LESS) && (Asense & RPMSENSE_GREATER))
	   )
		return DEP_UND;

	/* 2. filter out essentialy differ flags. */
	if ((Aflags & ~RPMSENSE_SENSEMASK) != (Bflags & ~RPMSENSE_SENSEMASK))
	{
		rpmsenseFlags Areq, Breq;

		/* additional check for REQUIREFLAGS */
		if (tag != RPMTAG_REQUIREFLAGS)
			return DEP_UND;

		/* 1a. filter out essentialy differ requires. */
		if ((Aflags & ~RPMSENSE_SENSEMASK & ~_ALL_REQUIRES_MASK) !=
		    (Bflags & ~RPMSENSE_SENSEMASK & ~_ALL_REQUIRES_MASK))
			return DEP_UND;

		Areq = Aflags & _ALL_REQUIRES_MASK;
		Breq = Bflags & _ALL_REQUIRES_MASK;

		/* 1b. Aflags contains Bflags? */
		if ((Areq & Breq) == Breq)
			rc = DEP_STR;

		/* 1c. Bflags contains Aflags? */
		else if ((Areq & Breq) == Areq)
			rc = DEP_WKR;

		else
			return DEP_UND;
	}

	/* 3. compare versions. */
	aEVR = xstrdup(Aevr);
	parseEVR(aEVR, &aE, &aV, &aR);
	bEVR = xstrdup(Bevr);
	parseEVR(bEVR, &bE, &bV, &bR);

	sense = rpmEVRcmp(aE, aV, aR, Aevr, bE, bV, bR, Bevr);
	aEVR = _free(aEVR);
	bEVR = _free(bEVR);

	/* 4. detect overlaps. */
	cmp_rc = compare_sense_flags (tag, sense, Asense, Bsense);
	if (cmp_rc == DEP_UND)
		return DEP_UND;

	if (rc == DEP_UND)
	{
		if (cmp_rc == DEP_WKR && compare_sense_flags (tag, -sense, Bsense, Asense) == DEP_WKR)
			/* A <= B && B <= A means A == B */
			return DEP_STR;
		return cmp_rc;
	}

	/* 5. compare expected with received. */
	if (rc != cmp_rc)
	{
		dep_compare_t cmp_rc2 = compare_sense_flags (tag, -sense, Bsense, Asense);
		if (cmp_rc2 != cmp_rc)
			return DEP_UND;
	}

	return rc;
}

int addReqProv(/*@unused@*/ Spec spec, Header h,
	       rpmsenseFlags depFlags, const char *depName, const char *depEVR,
		int index)
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    HFD_t hfd = headerFreeData;
    const char ** names;
    rpmTagType dnt;
    rpmTag nametag = 0;
    rpmTag versiontag = 0;
    rpmTag flagtag = 0;
    rpmTag indextag = 0;
    int len;
    rpmsenseFlags extra = RPMSENSE_ANY;
    int xx;
    
    if (depFlags & RPMSENSE_PROVIDES) {
	nametag = RPMTAG_PROVIDENAME;
	versiontag = RPMTAG_PROVIDEVERSION;
	flagtag = RPMTAG_PROVIDEFLAGS;
	extra = depFlags & RPMSENSE_FIND_PROVIDES;
    } else if (depFlags & RPMSENSE_OBSOLETES) {
	nametag = RPMTAG_OBSOLETENAME;
	versiontag = RPMTAG_OBSOLETEVERSION;
	flagtag = RPMTAG_OBSOLETEFLAGS;
    } else if (depFlags & RPMSENSE_CONFLICTS) {
	nametag = RPMTAG_CONFLICTNAME;
	versiontag = RPMTAG_CONFLICTVERSION;
	flagtag = RPMTAG_CONFLICTFLAGS;
    } else if (depFlags & RPMSENSE_PREREQ) {
	nametag = RPMTAG_REQUIRENAME;
	versiontag = RPMTAG_REQUIREVERSION;
	flagtag = RPMTAG_REQUIREFLAGS;
	extra = depFlags & _ALL_REQUIRES_MASK;
    } else if (depFlags & RPMSENSE_TRIGGER) {
	nametag = RPMTAG_TRIGGERNAME;
	versiontag = RPMTAG_TRIGGERVERSION;
	flagtag = RPMTAG_TRIGGERFLAGS;
	indextag = RPMTAG_TRIGGERINDEX;
	extra = depFlags & RPMSENSE_TRIGGER;
    } else {
	nametag = RPMTAG_REQUIRENAME;
	versiontag = RPMTAG_REQUIREVERSION;
	flagtag = RPMTAG_REQUIREFLAGS;
	extra = depFlags & _ALL_REQUIRES_MASK;
    }

    depFlags = (depFlags & (RPMSENSE_SENSEMASK | RPMSENSE_MULTILIB)) | extra;

    /*@-branchstate@*/
    if (depEVR == NULL)
	depEVR = "";
    /*@=branchstate@*/
    
    /* Check for duplicate dependencies. */
    if (hge(h, nametag, &dnt, (void **) &names, &len)) {
	const char ** versions = NULL;
	rpmTagType dvt = RPM_STRING_ARRAY_TYPE;
	int *flags = NULL;
	int *indexes = NULL;
	int duplicate = 0;

	if (flagtag) {
	    xx = hge(h, versiontag, &dvt, (void **) &versions, NULL);
	    xx = hge(h, flagtag, NULL, (void **) &flags, NULL);
	}
	if (indextag)
	    xx = hge(h, indextag, NULL, (void **) &indexes, NULL);

	while (len > 0) {
	    len--;
	    if (indextag && indexes && indexes[len] != index)
		continue;

	    if (strcmp(names[len], depName))
		continue;

	    if (flagtag && flags && versions) {
	    	dep_compare_t rc = compare_deps (flagtag, versions[len], flags[len], depEVR, depFlags);

#if 0
		fprintf (stderr, "DEBUG: name=%s, compare_deps=%d: tag=%d, AEVR=%s, Aflags=%#x, BEVR=%s, Bflags=%#x\n",
			depName, rc, flagtag, versions[len], flags[len], depEVR, depFlags);
#endif
	    	switch (rc)
		{
			case DEP_STR:
				break;
#ifdef	NOTYET
			case DEP_WKR:
				/* swap old and new values */
				break;
#endif
			default:
				continue;
		}
	    }

	    /* This is a duplicate dependency. */
	    duplicate = 1;

	    if (flagtag && isDependsMULTILIB(depFlags) &&
		!isDependsMULTILIB(flags[len]))
		    flags[len] |= RPMSENSE_MULTILIB;

	    break;
	}
	versions = hfd(versions, dvt);
	names = hfd(names, dnt);
	if (duplicate)
	    return 0;
    }

    /* Do not add provided requires. */
    if ((nametag == RPMTAG_REQUIRENAME) &&
        (flagtag == RPMTAG_REQUIREFLAGS) &&
        !(depFlags & _notpre(RPMSENSE_RPMLIB | RPMSENSE_KEYRING | RPMSENSE_SCRIPT_PRE | RPMSENSE_SCRIPT_POSTUN)) &&
        !isLegacyPreReq(depFlags) &&
        hge(h, RPMTAG_PROVIDENAME, &dnt, (void **) &names, &len)) {

	int skip = 0;
	int *flags = 0;
	const char ** versions = 0;
	rpmTagType dvt = RPM_STRING_ARRAY_TYPE;

	hge(h, RPMTAG_PROVIDEVERSION, &dvt, (void **) &versions, NULL);
	hge(h, RPMTAG_PROVIDEFLAGS, NULL, (void **) &flags, NULL);

	while (flags && versions && (len > 0)) {
	    --len;

	    if (strcmp (depName, names[len]))
		continue;
	    if (!(depFlags & RPMSENSE_SENSEMASK)) {
		skip = 1;
		break;
	    }
	    if (!(flags[len] & RPMSENSE_SENSEMASK))
		continue;
	    if (rpmRangesOverlap ("", versions[len], flags[len], "", depEVR, depFlags)) {
		skip = 1;
		break;
	    }
	}

	versions = hfd(versions, dvt);
	names = hfd(names, dnt);
	if (skip) return 0;
    }

    /* Add this dependency. */
    xx = headerAddOrAppendEntry(h, nametag, RPM_STRING_ARRAY_TYPE, &depName, 1);
    if (flagtag) {
	xx = headerAddOrAppendEntry(h, versiontag,
			       RPM_STRING_ARRAY_TYPE, &depEVR, 1);
	xx = headerAddOrAppendEntry(h, flagtag,
			       RPM_INT32_TYPE, &depFlags, 1);
    }
    if (indextag)
	xx = headerAddOrAppendEntry(h, indextag, RPM_INT32_TYPE, &index, 1);

    return 0;
}

int rpmlibNeedsFeature(Header h, const char * feature, const char * featureEVR)
{
    char * reqname = alloca(sizeof("rpmlib()") + strlen(feature));

    (void) stpcpy( stpcpy( stpcpy(reqname, "rpmlib("), feature), ")");

    /* XXX 1st arg is unused */
   return addReqProv(NULL, h, RPMSENSE_RPMLIB|(RPMSENSE_LESS|RPMSENSE_EQUAL),
	reqname, featureEVR, 0);
}
