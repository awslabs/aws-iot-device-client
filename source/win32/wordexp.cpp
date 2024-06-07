#include "unistd.h"
#include <sys/types.h>
#include <Windows.h>
#include <vector>
#include <string>
#include <iostream>
#include "wordexp.h"

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
        return WRDE_BADCHAR;
    }

    argc = 0;
    wordsW = (wchar_t *)malloc((strlen(words) + 1) * sizeof(wchar_t));
    if (wordsW == NULL) {
        return WRDE_NOSPACE;
    }

    mbstowcs(wordsW, words, strlen(words) + 1);
    argvW = CommandLineToArgvW(wordsW, &argc);
    free(wordsW);

    if (argvW == NULL) {
        return WRDE_SYNTAX;
    }

    we->we_wordc = argc;
    we->we_wordv = (char **)malloc(argc * sizeof(char *));
    if (we->we_wordv == NULL) {
        LocalFree(argvW);
        return WRDE_NOSPACE;
    }

    for (i = 0; i < argc; i++) {
        size_t len = wcslen(argvW[i]);
        we->we_wordv[i] = (char *)malloc((len + 1) * sizeof(char));
        if (we->we_wordv[i] == NULL) {
            wordfree(we);
            LocalFree(argvW);
            return WRDE_NOSPACE;
        }
        wcstombs(we->we_wordv[i], argvW[i], len + 1);
    }

    LocalFree(argvW);
    return 0;
}
