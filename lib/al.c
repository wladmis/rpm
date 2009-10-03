#include "system.h"
#include "rpmlib.h"
#include "debug.h"

#include "depends.h"
#include "al.h"

struct alEntry {
    const char *name;
    int len;
    /* entry-specific members */
};

struct alIndex {
    int sorted;
    int size;
    /* flexible array of entries */
};

/**
 * Compare two available index entries by name (qsort/bsearch).
 * @param one		1st available prov entry
 * @param two		2nd available prov entry
 * @return		result of comparison
 */
static inline
int nameCmp(const void * one, const void * two)		/*@*/
{
    const struct alEntry *a = one, *b = two;
    int lencmp = a->len - b->len;
    if (lencmp)
	return lencmp;
    return strcmp(a->name, b->name);
}

static
void *axSearch(void *index, int esize, const char *name, int *nfound)
{
    if (nfound)
	*nfound = 0;

    struct alIndex *ax = index;
    if (ax == NULL)
	return NULL;
    assert(ax->size > 0);

    char *entries = (char *)(ax + 1);
    struct alEntry needle = { name, strlen(name) };
    if (ax->size == 1) {
	if (nameCmp(entries, &needle))
	    return NULL;
	if (nfound)
	    *nfound = 1;
	return entries;
    }
    if (!ax->sorted) {
	qsort(entries, ax->size, esize, nameCmp);
	ax->sorted = 1;
    }

    char *first, *last;
    first = last = bsearch(&needle, entries, ax->size, esize, nameCmp);
    if (first == NULL)
	return NULL;

    if (nfound) {
	*nfound = 1;

	/* rewind to the first match */
	while (first > entries) {
	    if (nameCmp(first - esize, &needle))
		break;
	    first -= esize;
	    (*nfound)++;
	}

	/* rewind to the last match */
	while (last + esize < entries + esize * ax->size) {
	    if (nameCmp(last + esize, &needle))
		break;
	    last += esize;
	    (*nfound)++;
	}
    }

    return first;
}

static
void *axGrow(void *index, int esize, int more)
{
    struct alIndex *ax = index;
    if (ax) {
	assert(ax->size > 0);
	ax = xrealloc(ax, sizeof(*ax) + esize * (ax->size + more));
    }
    else {
	ax = xmalloc(sizeof(*ax) + esize * more);
	ax->size = 0;
    }
    return ax;
}

/** \ingroup rpmdep
 * A single available item (e.g. a Provides: dependency).
 */
struct availableIndexEntry {
/*@dependent@*/ struct availablePackage * package; /*!< Containing package. */
/*@dependent@*/ const char * entry;	/*!< Available item name. */
    int entryLen;			/*!< No. of bytes in name. */
    int entryIx;			/*!< Item index. */
    enum indexEntryType {
	IET_PROVIDES=1		/*!< A Provides: dependency. */
    } type;				/*!< Type of available item. */
} ;

/** \ingroup rpmdep
 * Index of all available items.
 */
struct availableIndex {
    int size;				/*!< No. of available items. */
    struct availableIndexEntry index[1]; /*!< Array of available items. */
} ;

/**
 * Compare two available index entries by name (qsort/bsearch).
 * @param one		1st available index entry
 * @param two		2nd available index entry
 * @return		result of comparison
 */
static int indexcmp(const void * one, const void * two)		/*@*/
{
    const struct availableIndexEntry * a = one;
    const struct availableIndexEntry * b = two;
    int lenchk = a->entryLen - b->entryLen;

    if (lenchk)
	return lenchk;

    return strcmp(a->entry, b->entry);
}

void alMakeIndex(availableList al)
{
    if (al->index) // got an index
	return;

    int i, j, k;
    int ai_size = 0;
    for (i = 0; i < al->size; i++) 
	ai_size += al->list[i].providesCount;
    if (ai_size == 0)
	return;

    struct availableIndex *ai = al->index =
	    xmalloc(sizeof(*ai) + sizeof(*ai->index) * (ai_size - 1));
    ai->size = ai_size;

    k = 0;
    for (i = 0; i < al->size; i++) {
	for (j = 0; j < al->list[i].providesCount; j++) {
	    ai->index[k].package = al->list + i;
	    ai->index[k].entry = al->list[i].provides[j];
	    ai->index[k].entryLen = strlen(al->list[i].provides[j]);
	    ai->index[k].entryIx = j;
	    ai->index[k].type = IET_PROVIDES;
	    k++;
	}
    }

    qsort(ai->index, ai->size, sizeof(*ai->index), indexcmp);
}

/**
 * Destroy available item index.
 * @param al		available list
 */
static void alFreeIndex(availableList al)
	/*@modifies al @*/
{
    al->index = _free(al->index);
}

/**
 * Compare two directory info entries by name (qsort/bsearch).
 * @param one		1st directory info
 * @param two		2nd directory info
 * @return		result of comparison
 */
static int dirInfoCompare(const void * one, const void * two)	/*@*/
{
    const dirInfo a = (const dirInfo) one;
    const dirInfo b = (const dirInfo) two;
    int lenchk = a->dirNameLen - b->dirNameLen;

    if (lenchk)
	return lenchk;

    /* XXX FIXME: this might do "backward" strcmp for speed */
    return strcmp(a->dirName, b->dirName);
}

/**
 * Check added package file lists for package(s) that provide a file.
 * @param al		available list
 * @param keyType	type of dependency
 * @param fileName	file name to search for
 * @return		available package pointer
 */
static /*@only@*/ /*@null@*/ struct availablePackage **
alAllFileSatisfiesDepend(const availableList al,
		const char * keyType, const char * fileName)
	/*@*/
{
    int i, found;
    const char * dirName;
    const char * baseName;
    struct dirInfo_s dirNeedle;
    dirInfo dirMatch;
    struct availablePackage ** ret;

    /* Solaris 2.6 bsearch sucks down on this. */
    if (al->numDirs == 0 || al->dirs == NULL || al->list == NULL)
	return NULL;

    {	char * t;
	dirName = t = xstrdup(fileName);
	if ((t = strrchr(t, '/')) != NULL) {
	    t++;		/* leave the trailing '/' */
	    *t = '\0';
	}
    }

    dirNeedle.dirName = (char *) dirName;
    dirNeedle.dirNameLen = strlen(dirName);
    dirMatch = bsearch(&dirNeedle, al->dirs, al->numDirs,
		       sizeof(dirNeedle), dirInfoCompare);
    if (dirMatch == NULL) {
	dirName = _free(dirName);
	return NULL;
    }

    /* rewind to the first match */
    while (dirMatch > al->dirs && dirInfoCompare(dirMatch-1, &dirNeedle) == 0)
	dirMatch--;

    /*@-nullptrarith@*/		/* FIX: fileName NULL ??? */
    baseName = strrchr(fileName, '/') + 1;
    /*@=nullptrarith@*/

    for (found = 0, ret = NULL;
	 dirMatch <= al->dirs + al->numDirs &&
		dirInfoCompare(dirMatch, &dirNeedle) == 0;
	 dirMatch++)
    {
	/* XXX FIXME: these file lists should be sorted and bsearched */
	for (i = 0; i < dirMatch->numFiles; i++) {
	    if (dirMatch->files[i].baseName == NULL ||
			strcmp(dirMatch->files[i].baseName, baseName))
		continue;

	    if (keyType)
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (added files)\n"),
			keyType, fileName);

	    ret = xrealloc(ret, (found+2) * sizeof(*ret));
	    if (ret)	/* can't happen */
		ret[found++] = al->list + dirMatch->files[i].pkgNum;
	    /*@innerbreak@*/ break;
	}
    }

    dirName = _free(dirName);
    /*@-mods@*/		/* FIX: al->list might be modified. */
    if (ret)
	ret[found] = NULL;
    /*@=mods@*/
    return ret;
}

struct availablePackage **
alAllSatisfiesDepend(const availableList al,
		const char * keyType, const char * keyDepend,
		const char * keyName, const char * keyEVR, int keyFlags)
{
    struct availablePackage * p, ** ret = NULL;
    int i, rc, found;

    if (*keyName == '/') {
	ret = alAllFileSatisfiesDepend(al, keyType, keyName);
	/* XXX Provides: /path was broken with added packages (#52183). */
	if (ret != NULL && *ret != NULL)
	    return ret;
    }

    const struct availableIndex *ai = al->index;
    if (ai == NULL)
	return NULL;

    struct availableIndexEntry needle, * match;
    needle.entry = keyName;
    needle.entryLen = strlen(keyName);
    match = bsearch(&needle, ai->index, ai->size, sizeof(*ai->index), indexcmp);
    if (match == NULL)
	return NULL;

    /* rewind to the first match */
    while (match > ai->index && indexcmp(match-1, &needle) == 0)
	match--;

    for (ret = NULL, found = 0;
	 match < ai->index + ai->size &&
		indexcmp(match, &needle) == 0;
	 match++)
    {

	p = match->package;
	rc = 0;
	switch (match->type) {
	case IET_PROVIDES:
	    i = match->entryIx;
	    {	const char * proEVR;
		int proFlags;

		proEVR = (p->providesEVR ? p->providesEVR[i] : NULL);
		proFlags = (p->provideFlags ? p->provideFlags[i] : 0);
		if ((keyFlags & RPMSENSE_SENSEMASK) && !(proFlags & RPMSENSE_SENSEMASK))
		    proFlags |= RPMSENSE_EQUAL;
		rc = rpmRangesOverlap(p->provides[i], proEVR, proFlags,
				keyName, keyEVR, keyFlags);
		if (rc)
		    /*@switchbreak@*/ break;
	    }
	    if (keyType && keyDepend && rc)
		rpmMessage(RPMMESS_DEBUG, _("%s: %-45s YES (added provide)\n"),
				keyType, keyDepend+2);
	    break;
	}

	if (rc) {
	    ret = xrealloc(ret, (found + 2) * sizeof(*ret));
	    if (ret)	/* can't happen */
		ret[found++] = p;
	}
    }

    if (ret)
	ret[found] = NULL;

    return ret;
}

struct availablePackage *
alAddPackage(availableList al,
		Header h, /*@null@*/ /*@dependent@*/ const void * key,
		/*@null@*/ FD_t fd, /*@null@*/ rpmRelocation * relocs)
{
    HGE_t hge = (HGE_t)headerGetEntryMinMemory;
    HFD_t hfd = headerFreeData;
    rpmTagType dnt, bnt;
    struct availablePackage * p;
    rpmRelocation * r;
    int i;
    int_32 * dirIndexes;
    const char ** dirNames;
    int numDirs, dirNum;
    int * dirMapping;
    struct dirInfo_s dirNeedle;
    dirInfo dirMatch;
    int first, last, fileNum;
    int origNumDirs;
    int pkgNum;

    AUTO_REALLOC(al->list, al->size);
    pkgNum = al->size++;
    p = al->list + pkgNum;
    p->h = headerLink(h);	/* XXX reference held by transaction set */
    p->depth = p->npreds = 0;
    memset(&p->tsi, 0, sizeof(p->tsi));

    (void) headerNVR(p->h, &p->name, &p->version, &p->release);

    if (!hge(h, RPMTAG_EPOCH, NULL, (void **) &p->epoch, NULL))
	p->epoch = NULL;

    if (!hge(h, RPMTAG_BUILDTIME, NULL, (void **) &p->buildtime, NULL))
	p->buildtime = NULL;

    if (!hge(h, RPMTAG_PROVIDENAME, NULL, (void **) &p->provides,
	&p->providesCount)) {
	p->providesCount = 0;
	p->provides = NULL;
	p->providesEVR = NULL;
	p->provideFlags = NULL;
    } else {
	if (!hge(h, RPMTAG_PROVIDEVERSION,
			NULL, (void **) &p->providesEVR, NULL))
	    p->providesEVR = NULL;
	if (!hge(h, RPMTAG_PROVIDEFLAGS,
			NULL, (void **) &p->provideFlags, NULL))
	    p->provideFlags = NULL;
    }

    if (!hge(h, RPMTAG_REQUIRENAME, NULL, (void **) &p->requires,
	&p->requiresCount)) {
	p->requiresCount = 0;
	p->requires = NULL;
	p->requiresEVR = NULL;
	p->requireFlags = NULL;
    } else {
	if (!hge(h, RPMTAG_REQUIREVERSION,
			NULL, (void **) &p->requiresEVR, NULL))
	    p->requiresEVR = NULL;
	if (!hge(h, RPMTAG_REQUIREFLAGS,
			NULL, (void **) &p->requireFlags, NULL))
	    p->requireFlags = NULL;
    }

    if (!hge(h, RPMTAG_BASENAMES, &bnt, (void **)&p->baseNames, &p->filesCount))
    {
	p->filesCount = 0;
	p->baseNames = NULL;
    } else {
	(void) hge(h, RPMTAG_DIRNAMES, &dnt, (void **) &dirNames, &numDirs);
	(void) hge(h, RPMTAG_DIRINDEXES, NULL, (void **) &dirIndexes, NULL);

	/* XXX FIXME: We ought to relocate the directory list here */

	dirMapping = alloca(sizeof(*dirMapping) * numDirs);

	/* allocated enough space for all the directories we could possible
	   need to add */
	al->dirs = xrealloc(al->dirs, 
			    sizeof(*al->dirs) * (al->numDirs + numDirs));
	origNumDirs = al->numDirs;

	for (dirNum = 0; dirNum < numDirs; dirNum++) {
	    dirNeedle.dirName = (char *) dirNames[dirNum];
	    dirNeedle.dirNameLen = strlen(dirNames[dirNum]);
	    dirMatch = bsearch(&dirNeedle, al->dirs, origNumDirs,
			       sizeof(dirNeedle), dirInfoCompare);
	    if (dirMatch) {
		dirMapping[dirNum] = dirMatch - al->dirs;
	    } else {
		dirMapping[dirNum] = al->numDirs;
		al->dirs[al->numDirs].dirName = xstrdup(dirNames[dirNum]);
		al->dirs[al->numDirs].dirNameLen = strlen(dirNames[dirNum]);
		al->dirs[al->numDirs].files = NULL;
		al->dirs[al->numDirs].numFiles = 0;
		al->numDirs++;
	    }
	}

	dirNames = hfd(dirNames, dnt);

	first = 0;
	while (first < p->filesCount) {
	    last = first;
	    while ((last + 1) < p->filesCount) {
		if (dirIndexes[first] != dirIndexes[last + 1])
		    /*@innerbreak@*/ break;
		last++;
	    }

	    dirMatch = al->dirs + dirMapping[dirIndexes[first]];
	    dirMatch->files = xrealloc(dirMatch->files,
		sizeof(*dirMatch->files) * 
		    (dirMatch->numFiles + last - first + 1));
	    if (p->baseNames != NULL)	/* XXX can't happen */
	    for (fileNum = first; fileNum <= last; fileNum++) {
		dirMatch->files[dirMatch->numFiles].baseName =
		    p->baseNames[fileNum];
		dirMatch->files[dirMatch->numFiles].pkgNum = pkgNum;
		dirMatch->numFiles++;
	    }

	    first = last + 1;
	}

	if (origNumDirs + al->numDirs)
	    qsort(al->dirs, al->numDirs, sizeof(dirNeedle), dirInfoCompare);

    }

    p->key = key;
    p->fd = (fd != NULL ? fdLink(fd, "alAddPackage") : NULL);

    if (relocs) {
	for (i = 0, r = relocs; r->oldPath || r->newPath; i++, r++)
	    {};
	p->relocs = xmalloc((i + 1) * sizeof(*p->relocs));

	for (i = 0, r = relocs; r->oldPath || r->newPath; i++, r++) {
	    p->relocs[i].oldPath = r->oldPath ? xstrdup(r->oldPath) : NULL;
	    p->relocs[i].newPath = r->newPath ? xstrdup(r->newPath) : NULL;
	}
	p->relocs[i].oldPath = NULL;
	p->relocs[i].newPath = NULL;
    } else {
	p->relocs = NULL;
    }

    alFreeIndex(al);

    return p;
}

void alFree(availableList al)
{
    HFD_t hfd = headerFreeData;
    struct availablePackage * p;
    rpmRelocation * r;
    int i;

    if ((p = al->list) != NULL)
    for (i = 0; i < al->size; i++, p++) {

	{   tsortInfo tsi;
	    while ((tsi = p->tsi.tsi_next) != NULL) {
		p->tsi.tsi_next = tsi->tsi_next;
		tsi->tsi_next = NULL;
		tsi = _free(tsi);
	    }
	}

	p->provides = hfd(p->provides, -1);
	p->providesEVR = hfd(p->providesEVR, -1);
	p->requires = hfd(p->requires, -1);
	p->requiresEVR = hfd(p->requiresEVR, -1);
	p->baseNames = hfd(p->baseNames, -1);
	p->h = headerFree(p->h);

	if (p->relocs) {
	    for (r = p->relocs; (r->oldPath || r->newPath); r++) {
		r->oldPath = _free(r->oldPath);
		r->newPath = _free(r->newPath);
	    }
	    p->relocs = _free(p->relocs);
	}
	if (p->fd != NULL)
	    p->fd = fdFree(p->fd, "alAddPackage (alFree)");
    }

    if (al->dirs != NULL)
    for (i = 0; i < al->numDirs; i++) {
	al->dirs[i].dirName = _free(al->dirs[i].dirName);
	al->dirs[i].files = _free(al->dirs[i].files);
    }

    al->dirs = _free(al->dirs);
    al->numDirs = 0;
    al->list = _free(al->list);
    alFreeIndex(al);
}
