#include <stdio.h>  /* stderr, fprintf */
#include <stdlib.h> /* exit, EXIT_FAILURE */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "guible.h"
#include "guible.c"

#define arraySize(X) (sizeof(X) / sizeof(*(X)))

#define WIN_W     785
#define WIN_MAX_H 400

#define FONT_NORM "./Arial.ttf"
#define FONT_BOLD "./Arial-Bold.ttf"

enum {
	FieldTitle = 0,
	FieldName,
	FieldSurname,
	FieldBirthNumber,
	FieldBirthYear,
	FieldSex,
};

static GbFieldConf header[] = {
	[FieldTitle]       = {"Titul",         0.6, GbUnlocked},
	[FieldName]        = {"Meno",          1,   GbUnlocked},
	[FieldSurname]     = {"Priezvisko",    1,   GbUnlocked},
	[FieldBirthNumber] = {"Rodné číslo",   1.2, GbUnlocked},
	[FieldBirthYear]   = {"Rok narodenia", 1.2, GbLocked},
	[FieldSex]         = {"Pohlavie",      0.8, GbLocked},
};

static GbTable *table;

static void handleRow(GbTable *table, GbCell *row, int y, void *userData) {
	(void)table; (void)userData;

	if (row[FieldBirthNumber].len > 5) {
		GbTable_invalidValueMessageBox(table, y, FieldBirthNumber, "Length > 5");
		return;
	}

	GbCell_fmt(row + FieldTitle, "%i", y);
	GbCell_set(row + FieldSex, row[FieldName].content);
}

static void setup(void) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Failed to initialize SDL: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	if (TTF_Init() < 0) {
		fprintf(stderr, "Failed to initialize SDL_ttf: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	table = GbTable_newWindow("My table", header, arraySize(header), WIN_W, WIN_MAX_H,
	                          handleRow, NULL, GbStyle_default(FONT_NORM, FONT_BOLD, 17));
	if (table == NULL) {
		fprintf(stderr, "Failed to create table window: %s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
}

static void cleanup(void) {
	GbTable_destroyWindow(table);

	TTF_Quit();
	SDL_Quit();
}

int main(void) {
	setup();

	do
		GbTable_render(table);
	while (GbTable_input(table));

	cleanup();
	return 0;
}
