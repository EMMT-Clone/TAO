/*
 * cmdline.c --
 *
 * Packing of command arguments and parsing of comman lines.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2018, Éric Thiébaut.
 */

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "tao-private.h"

#define FAILURE  -1
#define SUCCESS   0

#define IS_SPACE(c) ((c) == ' ' || (c) == '\t')

int
tao_split_command(tao_error_t** errs, const char*** list,
                  const char* cmd, long length)
{
    /* Get the length of the command line if not specified by the caller. */
    if (length == -1) {
        length = tao_strlen(cmd);
    } else if (length < 0) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return FAILURE;
    }

    /*
     * As we will allocate enough memory for the worst possible case, we want
     * to trim any leading and trailing characters that can be ignored.
     * Because we also want to accommodate for various end of line styles, we
     * first trim a single trailing line-feed (LF) character, or a trailing
     * carriage-return (CR), or a trailing CR-LF sequence of 2 characters.
     * Then we trim leading and trailing spaces.
     */
    if (length >= 2 && cmd[length-2] == '\r' && cmd[length-1] == '\n') {
        length -= 2;
    } else if (length >= 1 && (cmd[length-1] == '\n' ||
                               cmd[length-1] == '\r')) {
        length -= 1;
    }
    if (length > 0) {
        /* Trim leading and trailing spaces. */
        long first = 0, last = length - 1;
        while (first <= last) {
            int c = cmd[first];
            if (IS_SPACE(c)) {
                ++first;
            } else {
                break;
            }
        }
        while (first < last) {
            int c = cmd[last];
            if (IS_SPACE(c)) {
                --last;
            } else {
                break;
            }
        }
        length = (last + 1) - first;
        cmd += first;
    }

    /*
     * We want to store the list of words in a single block of memory (so that
     * the caller just has to free the list when no longer needed).  The total
     * size of parsed words (including terminating nulls) cannot be more than
     * the length of the trimmed command string plus one final null.  To that
     * amount, we must add the memory needed to store an array of pointers to
     * each words plus a final NULL pointer.
     */
    long maxwords = (length + 1)/2; /* the maximum number of words */
    long arrsize = sizeof(void*)*(maxwords + 1); /* the maximum size of the
                                                  * array of pointers */
    long fullsize = arrsize + length + 1; /* the size to allocate */

    /*
     * Free the list if non-NULL, then allocate enough memory.
     */
    void* ptr = *list;
    if (ptr != NULL) {
        /* In case of interrupts, first set the caller's pointer, then free. */
        *list = NULL;
        free(ptr);
    }
    ptr = tao_malloc(errs, fullsize);
    if (ptr == NULL) {
        return FAILURE;
    }
    memset(ptr, 0, arrsize); /* just in case of interrupts and errors, only
                              * valid pointers to words will be stored in the
                              * list at any moment */
    const char** argv = ptr;
    *list = argv;
    char* word = (char*)ptr + arrsize;

    /* Parse the command line into words. */
    int iarg = 0;
    bool sep = true; /* a separator is not needed for the first word */
    for (long i = 0; i < length; ++i) {
        /* Get next first non-space character. */
        int c = cmd[i];
        if (IS_SPACE(c)) {
            sep = true;
            while (++i < length) {
                c = cmd[i];
                if (! IS_SPACE(c)) {
                    break;
                }
            }
            if (i >= length) {
                /* This should not be necessary because we trimmed trailing
                 * spaces, but just in case of... */
                break;
            }
        }

        /* There must be a separator between words. */
        if (! sep) {
            tao_push_error(errs, __func__, TAO_MISSING_SEPARATOR);
            return FAILURE;
        }
        sep = false;

        /* Parse next word. */
        long j = 0; /* word index */
        if (c == '\'') {
            /* Singly quoted string. */
            while (true) {
                if (++i >= length) {
                    goto missing_close;
                }
                c = cmd[i];
                if (c == '\'') {
                    /* End of string. */
                    break;
                } else if (c == '\0' || c == '\n' || c == '\r') {
                    goto bad_character;
                }
                word[j++] = c;
            }
        } else if (c == '"') {
            /* Doubly quoted string. */
            while (true) {
                if (++i >= length) {
                    goto missing_close;
                }
                c = cmd[i];
                if (c == '"') {
                    /* End of string. */
                    break;
                } else if (c == '\0' || c == '\n' || c == '\r') {
                    goto bad_character;
                } else if (c == '\\') {
                    /* Escape sequence. */
                    if (++i >= length) {
                        goto bad_escape;
                    }
                    c = cmd[i];
                    if (c == 'n') {
                        c = '\n';
                    } else if (c == 'r') {
                        c = '\r';
                    } else if (c == 't') {
                        c = '\t';
                    } else if (c != '\\' && c != '"') {
                        goto bad_escape;
                    }
                }
                word[j++] = c;
            }
        } else {
            /* Ordinary word. */
            while (true) {
                /* At that point, we know that current character is not a space
                 * (and not a quote if it is the 1st character of the word). */
                switch (c) {
                case '\0':
                case '\n':
                case '\r':
                case '\\':
                case '"':
                case '\'':
                    goto bad_character;
                }
                word[j++] = c;
                if (++i >= length) {
                    break;
                }
                c = cmd[i];
                if (IS_SPACE(c)) {
                    /* End of word but remember that a space has been seen. */
                    sep = true;
                    break;
                }
            }
        }

        /* Register the new word. */
        word[j++] = '\0';
        argv[iarg] = word;
        ++iarg;
        word = &word[j];
    }
    return iarg;

 bad_escape:
    tao_push_error(errs, __func__, TAO_BAD_ESCAPE);
    return FAILURE;

 bad_character:
    tao_push_error(errs, __func__, TAO_BAD_CHARACTER);
    return FAILURE;

 missing_close:
    tao_push_error(errs, __func__, TAO_UNCLOSED_STRING);
    return FAILURE;
}

int
tao_pack_words(tao_error_t** errs, tao_buffer_t* dest,
               const char* argv[], int argc)
{
    /* Check arguments. */
    if (argc < 0 && argc != -1) {
        tao_push_error(errs, __func__, TAO_BAD_ARGUMENT);
        return FAILURE;
    }

    /*
     * In order to not change the contents that may exist in the destination
     * buffer on entry, adjusting of the buffer contents is deferred until the
     * successful termination of the function.  The following variable is used
     * to keep track of the size of the new contents.
     */
    long off = 0;

    /*
     * The following variables are used to keep track of the position and size
     * of the unused part of the buffer after its current contents and which
     * can be used to write the result.  This information must be updated every
     * time we need to resize the unused part.
     */
    char* buf;
    long avail = tao_get_buffer_unused_part(dest, (void**)&buf);

    /*
     * Encode all words of the list.
     */
    for (int iarg = 0; (iarg < argc) || (argc == -1 &&
                                         argv[iarg] != NULL); ++iarg) {
        /* Get next word. */
        const char* word = argv[iarg];
        if (word == NULL) {
            tao_push_error(errs, __func__, TAO_BAD_ADDRESS);
            return FAILURE;
        }

        /*
         * Make a first pass to measure the word length, figure out whether
         * quoting is necessary and count the number of escape sequences that
         * may be needed.  We try to use as few escape sequences as possible
         * even though we may accept to decode more.
         */
        int quote = 0;
        long len = 0, esc = 0;
        while (true) {
            int c = word[len];
            if (c == '\0') {
                break;
            }
            ++len;
            switch (c) {
            case '"':
                /* Require an escape sequence if in a double quoted string. */
                ++esc;
            case ' ':
            case '\t':
            case '\r':
                /* Require at least single quoting. */
                if (quote < 1) {
                    quote = 1;
                }
                break;

            case '\n':
            case '\\':
                ++esc;
            case '\'':
                /* Require double quoting. */
                quote = 2;
                break;
            }
        }

        /*
         * Make sure there is enough space to store the new word in the buffer
         * after its contents.
         */
        long minsize = off + len + 1;
        if (len == 0 || quote == 1) {
            /* Add the characters needed for the surrounding quotes. */
            minsize += 2;
        } else if (quote == 2) {
            /* Add the number of characters needed for the surrounding quotes
             * and the escape sequences. */
            minsize += esc + 2;
        }
        if (avail < minsize) {
            /*
             * Not enough space is available.  Resize the buffer and retrieve
             * the new position and size of its unused part.
             */
            if (tao_resize_buffer(errs, dest, minsize) == -1) {
                return FAILURE;
            }
            avail = tao_get_buffer_unused_part(dest, (void**)&buf);
            TAO_ASSERT(avail >= minsize, FAILURE);
        }

        /*
         * Encode the word.
         */
        if (iarg > 0) {
            /* Append word separator. */
            buf[off++] = ' ';
        }
        if (len == 0) {
            /* An empty word is surrounded by single quotes. */
            buf[off++] = '\'';
            buf[off++] = '\'';
        } else if (quote == 0) {
            /* No quotes needed. */
            for (long i = 0; i < len; ++i) {
                buf[off + i] = word[i];
            }
            off += len;
        } else if (quote == 1) {
            /* Singly quoted string. */
            buf[off++] = '\'';
            for (long i = 0; i < len; ++i) {
                buf[off + i] = word[i];
            }
            off += len;
            buf[off++] = '\'';
        } else {
            /* Doubly quoted string.  */
            buf[off++] = '"';
            for (long i = 0; i < len; ++i) {
                int c = word[i];
                if (c == '"' || c == '\\') {
                    buf[off++] = '\\';
                } else if (c == '\n') {
                    buf[off++] = '\\';
                    c = 'n';
                }
                buf[off++] = c;
            }
            buf[off++] = '"';
        }
        TAO_ASSERT(off == (iarg > 0 ? minsize : minsize - 1), FAILURE);
    }

    /*
     * Add the final end of command marker.  We have to make sure that at least
     * one more character can be written.  This should be always the case
     * unless an empty word list was given.
     */
    if (avail < off + 1) {
        if (tao_resize_buffer(errs, dest, off + 1) == -1) {
            return FAILURE;
        }
        avail = tao_get_buffer_unused_part(dest, (void**)&buf);
        TAO_ASSERT(avail >= off + 1, FAILURE);
    }
    buf[off++] = '\n';

    /*
     * Adjust the size of the buffer contents to account for the added
     * command.
     */
    return tao_adjust_buffer_contents_size(errs, dest, off);
}

int
tao_parse_int(const char* str, int* ptr)
{
    char* end;
    long lval = strtol(str, &end, 0);
    int ival = lval;
    if (*end != '\0' || *str == '\0' || ival != lval) {
        return -1;
    }
    *ptr = ival;
    return 0;
}

int
tao_parse_long(const char* str, long* ptr)
{
    char* end;
    long lval = strtol(str, &end, 0);
    if (*end != '\0' || *str == '\0') {
        return -1;
    }
    *ptr = lval;
    return 0;
}

int
tao_parse_double(const char* str, double* ptr)
{
    char* end;
    double dval = strtod(str, &end);
    if (*end != '\0' || *str == '\0') {
        return -1;
    }
    *ptr = dval;
    return 0;
}
