/**
 * 
*/
#ifndef _WIN32

#pragma message("this wordexp.h implementation is for Windows only!")

#else
    #ifndef _WORDEXP_H_
    #define _WORDEXP_H_

    #include "unistd.h"
    #include <sys/types.h>
    #include <Windows.h>
    #include <vector>
    #include <string>
    #include <iostream>

    #ifdef __cplusplus
    extern "C" {
    #endif

    struct _wordexp_t
    {
    size_t we_wordc;	/* Count of words matched by words. */
    char **we_wordv;	/* Pointer to list of expanded words. */
    size_t we_offs;	/* Slots to reserve at the beginning of we_wordv. */
    };

    typedef struct _wordexp_t wordexp_t;

    #define	WRDE_DOOFFS	0x0001	/* Use we_offs. */
    #define	WRDE_APPEND	0x0002	/* Append to output from previous call. */
    #define	WRDE_NOCMD	0x0004	/* Don't perform command substitution. */
    #define	WRDE_REUSE	0x0008	/* pwordexp points to a wordexp_t struct returned from
                                    a previous successful call to wordexp. */
    #define	WRDE_SHOWERR	0x0010	/* Print error messages to stderr. */
    #define	WRDE_UNDEF	0x0020	/* Report attempt to expand undefined shell variable. */

    enum {
    WRDE_SUCCESS,
    WRDE_NOSPACE,
    WRDE_BADCHAR,
    WRDE_BADVAL,
    WRDE_CMDSUB,
    WRDE_SYNTAX,
    WRDE_NOSYS
    };

    /* Note: This implementation of wordexp requires a version of bash
    that supports the --wordexp and --protected arguments to be present
    on the system.  It does not support the WRDE_UNDEF flag. */
    //inline
    //int wordexp(const char *words, wordexp_t *p, int /*flags*/)
    /*{
        DWORD length = ExpandEnvironmentStringsA(words, NULL, 0);
        if (length == 0) {
            std::cerr << "Error expanding string\n";
            return -1;
        }
        std::vector<char> expanded(length);
        if (ExpandEnvironmentStringsA(words, expanded.data(), length) == 0) {
            std::cerr << "Error expanding string\n";
            return -1;
        }
        char* token = strtok(expanded.data(), " ");
        p->we_wordc = 0;
        while (token != NULL) {
//            p->we_wordv[p->we_wordc++] = strdup(token);
            *p->we_wordv = strdup(token);
            token = strtok(NULL, " ");
        }
        return 0;        
    };

    inline
    void wordfree(wordexp_t *p)
    {
        if (p)
            free(p);
    };
*/

inline
void wordfree(wordexp_t *we) {
    int i;
    if (we != NULL && we->we_wordv != NULL) {
        for (i = 0; i < we->we_wordc; i++) {
            free(we->we_wordv[i]);
        }
        free(we->we_wordv);
        we->we_wordv = NULL;
        we->we_wordc = 0;
    }
}

inline
int wordexp(const char *words, wordexp_t *we, int /*flags*/) {
    int argc;
    wchar_t **argvW;
    wchar_t *wordsW;
    int i;

    if (words == NULL || we == NULL) {
        return -1;
    }

    // Validate the input string for unsupported characters
    char forbiddenChars[] = "*?[]{}()|&;<>'\"";
    if (strpbrk(words, forbiddenChars) != NULL) {
        // Unsupported character found
        return -1;
    }
    
    argc = 0;
    wordsW = (wchar_t *)malloc((strlen(words) + 1) * sizeof(wchar_t));
    if (wordsW == NULL) {
        return -1;
    }

    mbstowcs(wordsW, words, strlen(words) + 1);
    argvW = CommandLineToArgvW(wordsW, &argc);
    free(wordsW);

    if (argvW == NULL) {
        return -1;
    }

    we->we_wordc = argc;
    we->we_wordv = (char **)malloc(argc * sizeof(char *));
    if (we->we_wordv == NULL) {
        LocalFree(argvW);
        return -1;
    }

    for (i = 0; i < argc; i++) {
        size_t len = wcslen(argvW[i]);
        we->we_wordv[i] = (char *)malloc((len + 1) * sizeof(char));
        if (we->we_wordv[i] == NULL) {
            wordfree(we);
            LocalFree(argvW);
            return -1;
        }
        wcstombs(we->we_wordv[i], argvW[i], len + 1);
    }

    LocalFree(argvW);
    return 0;
}

    #ifdef __cplusplus
    }
    #endif

    #endif /* _WORDEXP_H_  */
#endif