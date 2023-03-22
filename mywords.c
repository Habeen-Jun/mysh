#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "words.h"

#ifndef BUFSIZE
#define BUFSIZE 128
#endif

#define IS_EXCEPTIONS(x) (x == '|' | x == '<' | x == '>')

static char buf[BUFSIZE];
static unsigned pos;
static unsigned bytes;
static int closed;
static int input;

void words_init(int fd)
{
	input = fd;
	pos = 0;
	bytes = 0;
	closed = 0;
}


char *words_next(void)
{
	if (closed) return NULL;
    char *word = NULL;

    if (buf[pos] == '\n') {
        word = realloc(word, 2);
        word = memcpy(word, buf + pos, 1);
        word[1] = '\0';
        ++pos;
        return word;
    }
	// skip whitespace
	while (1) {
		// ensure we have a char to read
		if (pos == bytes) {
			bytes = read(input, buf, BUFSIZE);
			if (bytes < 1) {
				closed = 1;
				return NULL;
			}
			pos = 0;
		}

		if (!isspace(buf[pos])) break;
		
		++pos;
	}
	
    // consider exceptions as a token
    if (IS_EXCEPTIONS(buf[pos])) {
            word = realloc(word, 2);
            word = memcpy(word, buf + pos, 1);
            word[1] = '\0';
            ++pos;
            return word;
    }

	// start reading a word
	int start = pos;
	
	int wordlen = 0;
	do {
		++pos;

		if (pos == bytes) {
			// save word so far
			int fraglen = pos - start;
			word = realloc(word, wordlen + fraglen + 1);
			memcpy(word + wordlen, buf + start, fraglen);
			wordlen += fraglen;
		
			// refresh the buffer
			bytes = read(input, buf, BUFSIZE);
			if (bytes < 1) {
				closed = 1;
				break;
			}
			pos = 0;
			start = 0;
		}
		
	} while (!isspace(buf[pos]) && !IS_EXCEPTIONS(buf[pos])); // Stop right before exceptions
	
	// grab the word from the current buffer
	// (Note: start == pos if we refreshed the buffer and got a space first.)
	if (start < pos) {
		int fraglen = pos - start;
		word = realloc(word, wordlen + fraglen + 1);
		memcpy(word + wordlen, buf + start, fraglen);
		wordlen += fraglen;
	}
	
	if (word) {
		word[wordlen] = '\0';
	}
	
	return word;
}
