/**
 * Windows implemenatation of wordexp (limited functioanlity)
*/
#ifndef _WIN32

#pragma message("this wordexp.h implementation is for Windows only!")

#else
#ifndef _WORDEXP_H_
#define _WORDEXP_H_

struct _wordexp_t
{
    size_t we_wordc;	/* Count of words matched by words. */
    char **we_wordv;	/* Pointer to list of expanded words. */
    size_t we_offs;	/* Slots to reserve at the beginning of we_wordv. */
};

typedef struct _wordexp_t wordexp_t;

/* Bits set in the FLAGS argument to `wordexp'.  */
enum {
    WRDE_DOOFFS = (1 << 0),	/* Insert PWORDEXP->we_offs NULLs.  */
    WRDE_APPEND = (1 << 1),	/* Append to results of a previous call.  */
    WRDE_NOCMD = (1 << 2),	/* Don't do command substitution.  */
    WRDE_REUSE = (1 << 3),	/* Reuse storage in PWORDEXP.  */
    WRDE_SHOWERR = (1 << 4),	/* Don't redirect stderr to /dev/null.  */
    WRDE_UNDEF = (1 << 5),	/* Error for expanding undefined variables.  */
    __WRDE_FLAGS = (WRDE_DOOFFS | WRDE_APPEND | WRDE_NOCMD
            | WRDE_REUSE | WRDE_SHOWERR | WRDE_UNDEF)
};

/* Possible nonzero return values from `wordexp'.  */
enum {
#ifdef __USE_XOPEN
    WRDE_NOSYS = -1,		/* Never used since we support `wordexp'.  */
#endif
    WRDE_NOSPACE = 1,		/* Ran out of memory.  */
    WRDE_BADCHAR,		/* A metachar appears in the wrong place.  */
    WRDE_BADVAL,		/* Undefined var reference with WRDE_UNDEF.  */
    WRDE_CMDSUB,		/* Command substitution with WRDE_NOCMD.  */
    WRDE_SYNTAX			/* Shell syntax error.  */
};

/* Free the storage allocated by a `wordexp' call.  */
void wordfree(wordexp_t *we);

/* Do word expansion of WORDS into PWORDEXP.  */
int wordexp(const char *words, wordexp_t *we, int /*flags*/);

#endif /* _WORDEXP_H_  */
#endif