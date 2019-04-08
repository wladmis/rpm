/** \ingroup rpmts
 * \file lib/rpmvercmp.c
 */

#include "system.h"

#include <ctype.h>

#define ALT_RPM_API /* for isChangeNameMoreFresh, parseEVR, rpmEVRDTCompare */

#include <rpm/rpmvercmp.h>	/* rpmvercmp proto */
#include <rpm/rpmlib.h>		/* other our protos */
#include <rpm/rpmstring.h>

#include <rpm/rpmlog.h>
#include <rpm/rpmmacro.h>

#include "debug.h"

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
int rpmvercmp(const char * a, const char * b)
{
    /* easy comparison to see if versions are identical */
    if (rstreq(a, b)) return 0;

    char oldch1, oldch2;
    char abuf[strlen(a)+1], bbuf[strlen(b)+1];
    char *str1 = abuf, *str2 = bbuf;
    char * one, * two;
    int rc;
    int isnum;

    strcpy(str1, a);
    strcpy(str2, b);

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    while (*one || *two) {
	while (*one && !risalnum(*one) && *one != '~') one++;
	while (*two && !risalnum(*two) && *two != '~') two++;

	/* handle the tilde separator, it sorts before everything else */
	if (*one == '~' || *two == '~') {
	    if (*one != '~') return 1;
	    if (*two != '~') return -1;
	    one++;
	    two++;
	    continue;
	}

	/* If we ran to the end of either, we are finished with the loop */
	if (!(*one && *two)) break;

	str1 = one;
	str2 = two;

	/* grab first completely alpha or completely numeric segment */
	/* leave one and two pointing to the start of the alpha or numeric */
	/* segment and walk str1 and str2 to end of segment */
	if (risdigit(*str1)) {
	    while (*str1 && risdigit(*str1)) str1++;
	    while (*str2 && risdigit(*str2)) str2++;
	    isnum = 1;
	} else {
	    while (*str1 && risalpha(*str1)) str1++;
	    while (*str2 && risalpha(*str2)) str2++;
	    isnum = 0;
	}

	/* save character at the end of the alpha or numeric segment */
	/* so that they can be restored after the comparison */
	oldch1 = *str1;
	*str1 = '\0';
	oldch2 = *str2;
	*str2 = '\0';

	/* this cannot happen, as we previously tested to make sure that */
	/* the first string has a non-null segment */
	if (one == str1) return -1;	/* arbitrary */

	/* take care of the case where the two version segments are */
	/* different types: one numeric, the other alpha (i.e. empty) */
	/* alpha segments are always newer than numeric segments */
	/* XXX See patch #60884 (and details) from bugzilla #50977. */
	if (two == str2) return (isnum ? -1 : 1);

	if (isnum) {
	    size_t onelen, twolen;
	    /* this used to be done by converting the digit segments */
	    /* to ints using atoi() - it's changed because long  */
	    /* digit segments can overflow an int - this should fix that. */

	    /* throw away any leading zeros - it's a number, right? */
	    while (*one == '0') one++;
	    while (*two == '0') two++;

	    /* whichever number has more digits wins */
	    onelen = strlen(one);
	    twolen = strlen(two);
	    if (onelen > twolen) return 1;
	    if (twolen > onelen) return -1;
	}

	/* strcmp will return which one is greater - even if the two */
	/* segments are alpha or if they are numeric.  don't return  */
	/* if they are equal because there might be more segments to */
	/* compare */
	rc = strcmp(one, two);
	if (rc) return (rc < 1 ? -1 : 1);

	/* restore character that was replaced by null above */
	*str1 = oldch1;
	one = str1;
	*str2 = oldch2;
	two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
}

static int upgrade_honor_buildtime(void)
{
    static int honor_buildtime = -1;

    if (honor_buildtime < 0)
	honor_buildtime = rpmExpandNumeric("%{?_upgrade_honor_buildtime}%{?!_upgrade_honor_buildtime:1}") ? 1 : 0;

    return honor_buildtime;
}

static int rpm_cmp_uint(const unsigned long long one,
                        const unsigned long long two)
{
    if (one < two)
	return -1;
    else if (one > two)
	return 1;
    else
	return 0;
}

static int rpm_cmp_disttag(const char * const fdt,
                           const char * const sdt)
{
    /* the branch prefixes (the only substrings important for the decision) */
    const char * one, * two;

    {
        /* The lengths of the branch substrings in the disttags. */
        const size_t flen = (strchr(fdt, '+') ? : fdt) - fdt;
        const size_t slen = (strchr(sdt, '+') ? : sdt) - sdt;

        /* The space allocated with alloca(3) remains until
           the end of the function or the closest enclosing scope
           which defines any variable length array. */
        one = strndupa(fdt, flen);
        two = strndupa(sdt, slen);
    }

    /* A hack to make packages upgrade between branches possible:
     * simply order by rpmvercmp, also honor %_priority_distbranch.
     */

    int rc = rpmvercmp(one, two);

    if (rc != 0) {
        static const char * pri_branch = NULL;
        if (!pri_branch)
            pri_branch = rpmExpand("%{?_priority_distbranch}", NULL) ? : "";

        /* Prefer packages built for pri_branch. */
        if (*pri_branch) {
            /* Only one branch substring can match, because they are knwown
             * to be different. */
            if (strcmp(one, pri_branch) == 0)
                rc = 1;
            else if (strcmp(two, pri_branch) == 0)
                rc = -1;
        }
    }

    return rc;
}

/* Decide which package is "newer" (for upgrade).
 */
int rpmEVRDTCompare(const struct rpmEVRDT * const fst,
                    const struct rpmEVRDT * const snd)
{
    int rc = 0; /* same "new" (an upgrade in neither direction is possible) */

    rc = rpm_cmp_uint(fst->has_epoch ? fst->epoch : 0,
                      snd->has_epoch ? snd->epoch : 0);

    if (rc) return rc;

    if (!fst->version && snd->version)
        rc = -1;
    if (fst->version && !snd->version)
        rc = 1;
    if (fst->version && snd->version)
        rc = rpmvercmp(fst->version,
                       snd->version);

    if (rc) return rc;

    if (!fst->release && snd->release)
        rc = -1;
    if (fst->release && !snd->release)
        rc = 1;
    if (fst->release && snd->release)
        rc = rpmvercmp(fst->release,
                       snd->release);

    if (rc) return rc;

    /* NB: if one of disttags is absent, we don't decide based on the disttags;
       rather we fallback to the decision based on the buildtimes.
    */
    if (fst->disttag && snd->disttag)
        rc = rpm_cmp_disttag(fst->disttag,
                             snd->disttag);

    if (rc) return rc;

    if (upgrade_honor_buildtime()) {
        /* Currently an absent buildtime is treated as the least one.
           Another possibility could be to skip buildtime comparison then.
           However, the current treatment is good for the work of hsh --with-stuff
           in case when buildtime is absent in the non-local repo indexes.
        */
        if (!fst->has_buildtime && snd->has_buildtime)
            rc = -1;
        if (fst->has_buildtime && !snd->has_buildtime)
            rc = 1;
        if (fst->has_buildtime && snd->has_buildtime)
            rc = rpm_cmp_uint(fst->buildtime,
                              snd->buildtime);
    }

    return rc;
}

/* The difference of this helper from headerGetNumber() (apart from
   the integer type) is that it signals an absent entry by returning 0
   (otherwise puts the value into the memory location given by the caller).
*/
static int headerGetNumberULL(Header h,
                              rpmTagVal tag,
                              unsigned long long * const result)
{
    int rc = 0;
    struct rpmtd_s td;
    headerGet(h, tag, &td, HEADERGET_DEFAULT); /* headerGetNumber() uses HEADERGET_EXT,
                                                  whose meaning is unknown to me. */
    if (rpmtdCount(&td) == 1) {
        *result = rpmtdGetNumber(&td);
        rc = 1;
    }
    rpmtdFreeData(&td);
    return rc;
}

/* A convenient local helper.

   We don't add another function akin to headerNEVRA() to the visible API,
   because it's deprecated and because in headerNEVRA()'s arguments
   the type of the integer pointer (Epoch) is inconvenient for us.
*/
static void headerGetEVRDT(Header const h,
                           struct rpmEVRDT * const res)
{
    res->has_epoch = headerGetNumberULL(h, RPMTAG_EPOCH, &res->epoch);
    res->version = headerGetString(h, RPMTAG_VERSION);
    res->release = headerGetString(h, RPMTAG_RELEASE);
    res->disttag = headerGetString(h, RPMTAG_DISTTAG);
    res->has_buildtime = headerGetNumberULL(h, RPMTAG_BUILDTIME, &res->buildtime);
}

int rpmVersionCompare(Header first, Header second)
{
    struct rpmEVRDT firstVerInfo, secondVerInfo;

    headerGetEVRDT(first, &firstVerInfo);
    headerGetEVRDT(second, &secondVerInfo);

    return rpmEVRDTCompare(&firstVerInfo, &secondVerInfo);
}

static
Header newHeaderEVR(const char *e, const char *v, const char *r)
{
    Header  h;

    if (!(h = headerNew()))
	return h;

    if (e) {
	uint32_t i = strtoul(e, NULL, 10);

	headerPutUint32(h, RPMTAG_EPOCH, &i, 1);
    }
    if (v)
	headerPutString(h, RPMTAG_VERSION, v);
    if (r)
	headerPutString(h, RPMTAG_RELEASE, r);
    return h;
}

int isChangeNameMoreFresh(const char * const head,
			  const char * const tail[3])
{
    int result;
    const char * evr[3];
    const char * wordAfterEmail;
    char * copy;

    Header h1, h2;

    rpmlog(RPMLOG_DEBUG, "test: is '%s' more fresh than e=%s, v=%s, r=%s?\n",
	   head, tail[0], tail[1], tail[2]);

    /* find the next to <email> word begin */
    if ((wordAfterEmail = strrchr(head, '>')))
	++wordAfterEmail;
    else
	wordAfterEmail = head;
    while (*wordAfterEmail && isspace(*wordAfterEmail))
	++wordAfterEmail;
    /* found. */
    copy = xstrdup(wordAfterEmail);
    parseEVR(copy, &evr[0], &evr[1], &evr[2]);
    /* The order of two argument groups is important:
       if evr[] (passed as B on the second place) has no epoch,
       rpmEVRcmp() assumes the same as in tail[];
       This fits our needs: the epoch may be omitted in a changelog entry (evr[])
       but there are no problems in specifying it in the format (tail[]). */

    h1 = newHeaderEVR(tail[0], tail[1], tail[2]);
    h2 = newHeaderEVR(evr[0], evr[1], evr[2]);
    if (!h1 || !h2)
    {
	rpmlog(RPMLOG_WARNING, "isChangeNameMoreFresh: headerNew failed");
	return 0;
    }

    result = rpmVersionCompare(h1, h2) < 0;

    h2 = headerFree(h2);
    h1 = headerFree(h1);
    free(copy);
    return result;
}
