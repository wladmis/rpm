/** \ingroup rpmbuild
 * \file build/reqprov.c
 *  Add dependency tags to package header(s).
 */

#include "system.h"

#include "rpmbuild.h"
#include "debug.h"

static int
deps_opt_enabled (void)
{
	static int enabled = 0, initialized = 0;

	if (!initialized)
	{
		initialized = 1;
		enabled = rpmExpandNumeric ("%{?_deps_optimization:%{_deps_optimization}}%{?!_deps_optimization:1}");
	}

	return enabled;
}

typedef enum {
	DEP_UN = 0,	/* uncomparable */
	DEP_ST = 1,	/* stronger */
	DEP_WK = -1,	/* weaker */
	DEP_EQ = 2	/* same */
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
					return DEP_WK;
				if (!(a & RPMSENSE_GREATER) && (b & RPMSENSE_LESS))
					return DEP_ST;
				if (!(b & RPMSENSE_LESS) && (a & RPMSENSE_GREATER))
					return DEP_WK;
				return DEP_UN;
			case RPMTAG_PROVIDEFLAGS:
				if ((a == 0) && (b != 0))
					return DEP_WK;
				if (!(b & RPMSENSE_LESS) && (a & RPMSENSE_GREATER))
					return DEP_ST;
				if (!(a & RPMSENSE_GREATER) && (b & RPMSENSE_LESS))
					return DEP_WK;
				return DEP_UN;
			default:
				if ((a == 0) && (b != 0))
					return DEP_ST;
				if (!(b & RPMSENSE_LESS) && (a & RPMSENSE_GREATER))
					return DEP_ST;
				if (!(a & RPMSENSE_GREATER) && (b & RPMSENSE_LESS))
					return DEP_WK;
				return DEP_UN;
		}
	} else if (cmp > 0)
	{
		/* Aevr > Bevr */
		return -compare_sense_flags (tag, -cmp, b, a);
	} else /* cmp == 0 */
	{
		/* Aevr == Bevr */
		if (a == b)
			return DEP_EQ;
		if ((a & b) == a)
			return (tag == RPMTAG_REQUIREFLAGS) ? DEP_ST : DEP_WK;
		if ((a & b) == b)
			return (tag == RPMTAG_REQUIREFLAGS) ? DEP_WK : DEP_ST;
		return DEP_UN;
	}
}

static dep_compare_t compare_deps (rpmTag tag,
	const char *Aevr, rpmsenseFlags Aflags,
	const char *Bevr, rpmsenseFlags Bflags)
{
	dep_compare_t rc = DEP_UN, cmp_rc;
	rpmsenseFlags Asense, Bsense;
	int sense;
	char *aEVR, *bEVR;
	const char *aE, *aV, *aR, *bE, *bV, *bR;

	/* 1. filter out noise */
	Aflags &= ~(RPMSENSE_FIND_REQUIRES | RPMSENSE_FIND_PROVIDES);
	Bflags &= ~(RPMSENSE_FIND_REQUIRES | RPMSENSE_FIND_PROVIDES);

	/* 2. identical? */
	if (Aflags == Bflags && !strcmp (Aevr, Bevr))
		return DEP_EQ;

	/* 3. whether dependency optimization is enabled? */
	if (!deps_opt_enabled ())
		return DEP_UN;

	Asense = Aflags & RPMSENSE_SENSEMASK;
	Bsense = Bflags & RPMSENSE_SENSEMASK;

	/* 4. check for supported tags. */
	switch (tag) {
		case RPMTAG_PROVIDEFLAGS:
		case RPMTAG_OBSOLETEFLAGS:
		case RPMTAG_CONFLICTFLAGS:
		case RPMTAG_REQUIREFLAGS:
			break;
		default:
			/* no way to optimize this case. */
			return DEP_UN;
	}

	/* 5. sanity checks */
	if (
	    ((Asense & RPMSENSE_LESS) && (Asense & RPMSENSE_GREATER)) ||
	    ((Bsense & RPMSENSE_LESS) && (Bsense & RPMSENSE_GREATER)) ||
	    ((Asense == 0) ^ (Aevr[0] == 0)) ||
	    ((Bsense == 0) ^ (Bevr[0] == 0))
	   )
		return DEP_UN;

	/* 5. filter out essentialy differ versions. */
	if (
	    ((Asense & RPMSENSE_LESS) && (Bsense & RPMSENSE_GREATER)) ||
	    ((Bsense & RPMSENSE_LESS) && (Asense & RPMSENSE_GREATER))
	   )
		return DEP_UN;

	/* 7. filter out essentialy differ flags. */
	if ((Aflags & ~RPMSENSE_SENSEMASK) != (Bflags & ~RPMSENSE_SENSEMASK))
	{
		rpmsenseFlags Areq, Breq;

		/* 7a. additional check for REQUIREFLAGS */
		if (tag != RPMTAG_REQUIREFLAGS)
			return DEP_UN;

		/* 7b. filter out essentialy differ requires. */
		if ((Aflags & ~RPMSENSE_SENSEMASK & ~_ALL_REQUIRES_MASK) !=
		    (Bflags & ~RPMSENSE_SENSEMASK & ~_ALL_REQUIRES_MASK))
			return DEP_UN;

		Areq = Aflags & _ALL_REQUIRES_MASK;
		Breq = Bflags & _ALL_REQUIRES_MASK;

		/* 7c. Aflags is legacy PreReq? */
		if (isLegacyPreReq (Areq))
		{
			if (Breq == 0)
				rc = DEP_ST;
			else if ((Breq & RPMSENSE_SCRIPT_PRE) == RPMSENSE_SCRIPT_PRE &&
			         (Breq & RPMSENSE_SCRIPT_POSTUN) == RPMSENSE_SCRIPT_POSTUN)
				rc = DEP_WK;
			else
				return DEP_UN;
		}

		/* 7d. Bflags is legacy PreReq? */
		else if (isLegacyPreReq (Breq))
		{
			if (Areq == 0)
				rc = DEP_WK;
			else if ((Areq & RPMSENSE_SCRIPT_PRE) == RPMSENSE_SCRIPT_PRE &&
			         (Areq & RPMSENSE_SCRIPT_POSTUN) == RPMSENSE_SCRIPT_POSTUN)
				rc = DEP_ST;
			else
				return DEP_UN;
		}

		/* 7e. Aflags contains Bflags? */
		else if (Breq && (Areq & Breq) == Breq)
			rc = DEP_ST;

		/* 7f. Bflags contains Aflags? */
		else if (Areq && (Areq & Breq) == Areq)
			rc = DEP_WK;

		else
			return DEP_UN;
	}

	/* 8. compare versions. */
	aEVR = xstrdup(Aevr);
	parseEVR(aEVR, &aE, &aV, &aR);
	bEVR = xstrdup(Bevr);
	parseEVR(bEVR, &bE, &bV, &bR);

	sense = rpmEVRcmp(aE, aV, aR, Aevr, bE, bV, bR, Bevr);
	aEVR = _free(aEVR);
	bEVR = _free(bEVR);

	/* 9. detect overlaps. */
	cmp_rc = compare_sense_flags (tag, sense, Asense, Bsense);

	/* 10. EVRs with serial are stronger. */
	if (cmp_rc == DEP_EQ)
	{
		if ((aE && *aE) && !(bE && *bE))
			cmp_rc = DEP_ST;
		else if ((bE && *bE) && !(aE && *aE))
			cmp_rc = DEP_WK;
	}

#if 0
		fprintf (stderr, "D: compare_sense_flags=%d: tag=%d, sense=%d, Asense=%#x, Bsense=%#x\n",
			cmp_rc, tag, sense, Asense, Bsense);
#endif

	if (cmp_rc == DEP_UN || rc == DEP_UN)
		return cmp_rc;

	/* 11. compare expected with received. */
	if (cmp_rc != rc && cmp_rc != DEP_EQ)
		return DEP_UN;

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

    depFlags = (depFlags & (RPMSENSE_SENSEMASK)) | extra;

    /*@-branchstate@*/
    if (depEVR == NULL)
	depEVR = "";
    /*@=branchstate@*/
    
    /* Check for duplicate dependencies. */
    if (hge(h, nametag, &dnt, (void **) &names, &len) && len > 0) {
	const char ** versions = NULL;
	rpmTagType dvt = RPM_STRING_ARRAY_TYPE;
	int *flags = NULL;
	int *indexes = NULL;
	int i, o_cnt = 0, duplicate = 0;
	char obsolete[len];

	memset (obsolete, 0, sizeof obsolete);

	if (flagtag) {
	    xx = hge(h, versiontag, &dvt, (void **) &versions, NULL);
	    xx = hge(h, flagtag, NULL, (void **) &flags, NULL);
	}
	if (indextag)
	    xx = hge(h, indextag, NULL, (void **) &indexes, NULL);

	for (i = len - 1; i >= 0; --i) {
	    if (indextag && indexes && indexes[i] != index)
		continue;

	    if (strcmp(names[i], depName))
		continue;

	    if (flagtag && flags && versions) {
	    	dep_compare_t rc = compare_deps (flagtag, versions[i], flags[i], depEVR, depFlags);

#if 0
		fprintf (stderr, "D: name=%s, compare_deps=%d: tag=%d, AEVR=%s, Aflags=%#x, BEVR=%s, Bflags=%#x\n",
			depName, rc, flagtag, versions[i], flags[i], depEVR, depFlags);
#endif
	    	switch (rc)
		{
			case DEP_EQ:
				rpmMessage (RPMMESS_DEBUG,
					"new dep \"%s\" already exists, optimized out\n",
					depName);
				break;
			case DEP_ST:
				rpmMessage (RPMMESS_DEBUG,
					"new dep \"%s\" is weaker, optimized out\n",
					depName);
				break;
			case DEP_WK:
				++o_cnt;
				obsolete[i] = 1;
			default:
				continue;
		}
	    }

	    /* This is a duplicate dependency. */
	    duplicate = 1;

	    if (o_cnt) {
		rpmMessage(RPMMESS_WARNING, "%d obsolete deps left", o_cnt);
		o_cnt = 0;
	    }

	    break;
	} /* end of main loop */

	if (o_cnt)
	{
		int     j, new_len = len - o_cnt;
		const char *new_names[new_len];
		const char *new_versions[new_len];
		int     new_flags[new_len];

		rpmMessage (RPMMESS_DEBUG, "%d old deps to be optimized out\n", o_cnt);
		for (i = 0, j = 0; i < len; ++i)
		{
			char   *p;

			if (obsolete[i])
			{
				rpmMessage (RPMMESS_DEBUG, "old dep \"%s\" optimized out\n", names[i]);
				continue;
			}

			p = alloca (1 + strlen (names[i]));
			strcpy (p, names[i]);
			new_names[j] = p;

			p = alloca (1 + strlen (versions[i]));
			strcpy (p, versions[i]);
			new_versions[j] = p;

			new_flags[j] = flags[i];
			++j;
		}

		if (   !headerModifyEntry (h, nametag, RPM_STRING_ARRAY_TYPE, new_names, new_len)
		    || !headerModifyEntry (h, versiontag, RPM_STRING_ARRAY_TYPE, new_versions, new_len)
		    || !headerModifyEntry (h, flagtag, RPM_INT32_TYPE, new_flags, new_len))
			rpmError (RPMERR_BADHEADER, "addReqProv: error modifying entry for dep %s\n", depName);
		rpmMessage (RPMMESS_DEBUG, "%d old deps optimized out, %d left\n", o_cnt, new_len);
	}

	versions = hfd(versions, dvt);
	names = hfd(names, dnt);
	if (duplicate)
	    return 0;
    }

	/* Do not add NEW provided requires. */
	if (   deps_opt_enabled ()
	    && (nametag == RPMTAG_REQUIRENAME)
	    && !isLegacyPreReq (depFlags)
	    && !(depFlags & _notpre (RPMSENSE_RPMLIB | RPMSENSE_KEYRING |
				     RPMSENSE_SCRIPT_PRE | RPMSENSE_SCRIPT_POSTUN)))
	{

		int     skip = 0;
		int    *flags = 0;
		const char **versions = 0;
		rpmTagType dvt = RPM_STRING_ARRAY_TYPE;

		names = NULL;
		hge (h, RPMTAG_PROVIDENAME, &dnt, (void **) &names, &len);
		hge (h, RPMTAG_PROVIDEVERSION, &dvt, (void **) &versions, NULL);
		hge (h, RPMTAG_PROVIDEFLAGS, NULL, (void **) &flags, NULL);

		while (names && flags && versions && (len > 0))
		{
			--len;

			if (strcmp (depName, names[len]))
				continue;
			if (!(depFlags & RPMSENSE_SENSEMASK))
			{
				skip = 1;
				break;
			}
			if (!(flags[len] & RPMSENSE_SENSEMASK))
				continue;
			if (rpmRangesOverlap ("", versions[len], flags[len],
					      "", depEVR, depFlags))
			{
				rpmMessage (RPMMESS_DEBUG,
					    "new dep \"%s\" already provided, optimized out\n",
					    depName);
				skip = 1;
				break;
			}
		}

		versions = hfd (versions, dvt);
		names = hfd (names, dnt);
		if (skip)
			return 0;

		if (*depName == '/' && !(depFlags & RPMSENSE_SENSEMASK))
		{
			const char **bn = NULL, **dn = NULL;
			const int_32 *di = NULL;
			rpmTagType bnt = 0, dnt = 0, dit = 0;
			int_32 bnc = 0;
			int i;
			(void) (hge(h, RPMTAG_DIRNAMES, &dnt, (void**)&dn, NULL) &&
				hge(h, RPMTAG_DIRINDEXES, &dit, (void**)&di, NULL) &&
				hge(h, RPMTAG_BASENAMES, &bnt, (void**)&bn, &bnc));
			for (i = 0; i < bnc; i++) {
				const char *d = dn[di[i]], *b = bn[i];
				size_t dl = strlen(d);
				if (strncmp(depName, d, dl) ||
				    strcmp(depName + dl, b))
					continue;
				rpmMessage (RPMMESS_DEBUG,
					    "new dep \"%s\" is packaged file, optimized out\n",
					    depName);
				skip = 1;
				break;
			}
			hfd(dn, dnt);
			hfd(di, dit);
			hfd(bn, bnt);
		}
		else {
			const char *N = NULL, *V = NULL, *R = NULL;
			headerNVR(h, &N, &V, &R);
			if (N && strcmp(depName, N) == 0) {
				if (!(depFlags & RPMSENSE_SENSEMASK))
					skip = 1;
				else if (V && R) {
					int_32 *E = NULL;
					char EVR[BUFSIZ];
					hge(h, RPMTAG_EPOCH, NULL, (void**) &E, NULL);
					if (E)
						snprintf(EVR, sizeof(EVR), "%d:%s-%s", *E, V, R);
					else
						snprintf(EVR, sizeof(EVR), "%s-%s", V, R);
					if (rpmRangesOverlap("", EVR, RPMSENSE_EQUAL,
							     "", depEVR, depFlags))
						skip = 1;
				}
				if (skip)
				rpmMessage (RPMMESS_DEBUG,
					    "new dep \"%s\" is the package name, optimized out\n",
					    depName);
			}
		}

		if (skip)
			return 0;
	}

	/* Remove OLD provided requires. */
	if (   deps_opt_enabled ()
	    && (nametag == RPMTAG_PROVIDENAME)
	    && hge (h, RPMTAG_REQUIRENAME, &dnt, (void **) &names, &len))
	{

		int    *flags = 0;
		const char **versions = 0;
		rpmTagType dvt = RPM_STRING_ARRAY_TYPE;
		int i, o_cnt = 0;
		char obsolete[len];

		memset (obsolete, 0, sizeof obsolete);

		hge (h, RPMTAG_REQUIREVERSION, &dvt, (void **) &versions, NULL);
		hge (h, RPMTAG_REQUIREFLAGS, NULL, (void **) &flags, NULL);

		for (i = len - 1; flags && versions && (i >= 0); --i)
		{
			rpmsenseFlags f = flags[i];

			if ((f & _notpre (RPMSENSE_RPMLIB | RPMSENSE_KEYRING |
					  RPMSENSE_SCRIPT_PRE | RPMSENSE_SCRIPT_POSTUN))
			    || isLegacyPreReq (f))
				continue;
			if (strcmp (depName, names[i]))
				continue;
			if (!(f & RPMSENSE_SENSEMASK))
			{
				++o_cnt;
				obsolete[i] = 1;
				continue;
			}
			if (!(depFlags & RPMSENSE_SENSEMASK))
				continue;
			if (rpmRangesOverlap ("", depEVR, depFlags,
					      "", versions[i], f))
			{
				++o_cnt;
				obsolete[i] = 1;
				continue;
			}
		}

		if (o_cnt)
		{
			int j, new_len = len - o_cnt;
			const char *new_names[new_len];
			const char *new_versions[new_len];
			int new_flags[new_len];

			rpmMessage (RPMMESS_DEBUG, "%d old deps to be optimized out\n", o_cnt);
			for (i = 0, j = 0; i < len; ++i)
			{
				char *p;

				if (obsolete[i])
				{
					rpmMessage (RPMMESS_DEBUG, "old dep \"%s\" optimized out\n", names[i]);
					continue;
				}

				p = alloca (1 + strlen (names[i]));
				strcpy (p, names[i]);
				new_names[j] = p;

				p = alloca (1 + strlen (versions[i]));
				strcpy (p, versions[i]);
				new_versions[j] = p;

				new_flags[j] = flags[i];
				++j;
			}

			if (!headerModifyEntry (h, RPMTAG_REQUIRENAME, RPM_STRING_ARRAY_TYPE, new_names, new_len) ||
			    !headerModifyEntry (h, RPMTAG_REQUIREVERSION, RPM_STRING_ARRAY_TYPE, new_versions, new_len) ||
			    !headerModifyEntry (h, RPMTAG_REQUIREFLAGS, RPM_INT32_TYPE, new_flags, new_len))
			    rpmError (RPMERR_BADHEADER, "addReqProv: error modifying entry for dep %s\n", depName);
			rpmMessage (RPMMESS_DEBUG, "%d old deps optimized out, %d left\n", o_cnt, new_len);
		}

		versions = hfd (versions, dvt);
		names = hfd (names, dnt);
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
