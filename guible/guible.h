#ifndef GUIBLE_H_HEADER_GUARD
#define GUIBLE_H_HEADER_GUARD

#include <string.h>  /* memset, memcpy, strcpy, strcat, strlen, strcmp, memmove */
#include <stdint.h>  /* uint32_t, uint8_t */
#include <assert.h>  /* assert */
#include <stdlib.h>  /* size_t, malloc, realloc, free, abort */
#include <stdio.h>   /* fprintf, stderr */
#include <stdbool.h> /* bool, true, false */
#include <math.h>    /* round */
#include <stdarg.h>  /* va_list, va_start, va_end, va_arg */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#ifndef gbAlloc
#	define gbAlloc(SIZE) malloc(SIZE)
#endif

#ifndef gbRealloc
#	define gbRealloc(PTR, SIZE) realloc(PTR, SIZE)
#endif

#ifndef gbFree
#	define gbFree(PTR) free(PTR)
#endif

#ifndef gbAssert
#	define gbAssert(COND) assert(COND)
#endif

#ifndef GB_CHUNK_SIZE
#	define GB_CHUNK_SIZE 32
#endif

#ifndef GB_DEF
#	define GB_DEF
#endif

#ifndef GB_TEXT_CACHE_CAP
#	define GB_TEXT_CACHE_CAP 256
#endif

GB_DEF size_t gbGetCodepointSize(char ch);
GB_DEF size_t gbU8Length(const char *str);
GB_DEF const char *gbU8At(const char *str, size_t pos);

enum {
	GbUnlocked = 0,
	GbLocked,
};

typedef struct {
	const char *title;
	float       size;
	bool        locked;
} GbFieldConf;

typedef struct {
	char  *content;
	size_t size, len;
} GbCell;

GB_DEF void GbCell_set   (GbCell *cell, const char *text);
GB_DEF void GbCell_fmt   (GbCell *cell, const char *fmt, ...);
GB_DEF void GbCell_append(GbCell *cell, const char *text);

typedef struct {
	SDL_Color mainColor[3], headerColor[2], borderColor[2], hoverColor, focusColor;
	SDL_Color deleteColor[6], buttonColor[6], scrollbarColor[4], selectionColor;

	int padding, margin, borderSize, focusSize, scrollbarWidth, scrollbarMargin, cursorWidth;

	const char *fontPath[2];
	int         fontSize;

	bool borderless;
} GbStyle;

GB_DEF GbStyle GbStyle_default(const char *fontNormPath, const char *fontBoldPath, int fontSize);

typedef struct GbTable GbTable;

typedef void (*GbRowHandler)(GbTable*, GbCell*, int, void*);

GB_DEF GbTable *GbTable_newWindow(const char *title, GbFieldConf *fieldsConf, int cols, int width,
                                  int maxHeight, GbRowHandler rowHandler, void* userData,
                                  GbStyle style);
GB_DEF void GbTable_destroyWindow(GbTable *this);

GB_DEF void GbTable_render(GbTable *this);
GB_DEF bool GbTable_input(GbTable *this);

GB_DEF void GbTable_invalidValueMessageBox(GbTable *this, int row, int field, const char *fmt, ...);

#endif
