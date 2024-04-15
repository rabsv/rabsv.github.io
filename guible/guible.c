#include "guible.h"

#define gbOutOfMem()                                                   \
	do {                                                               \
		fprintf(stderr, "%s:%i: Out of memory\n", __FILE__, __LINE__); \
		abort();                                                       \
	} while (0)

typedef struct {
	GbFieldConf conf;

	int x, w;
} GbField;

enum {
	GbButtonAdd = 0,
	GbButtonClose,
	GbButtonMinimize,

	GbButtonsCount,
};

struct GbTable {
	GbField *fields;
	GbStyle  style;
	GbCell **cells;
	int      cap, cols, rows;

	GbRowHandler rowHandler;
	void        *userData;

	int barSize, buttonSize;
	struct {
		SDL_Rect    rect;
		const char *text;
	} buttons[GbButtonsCount];

	int width, height, maxHeight, rowHeight;

	SDL_Rect table;
	struct {
		int width, height, bufferHeight, scroll;
		SDL_Point mouse;
	} view;

	struct {
		bool     visible, focused;
		SDL_Rect track, thumb;
		int      thumbMouseY;
	} scrollbar;

	struct {
		int       scroll;
		size_t    cursor, selectA, selectB;
		SDL_Point focusedPos, hoveredPos;
		GbCell   *focused;
		bool      selecting;
	} cell;

	SDL_Point mouse;
	bool      mouseDown, mouseWasDown;
	int       hoveredId, focusedId;

	bool deleting, running;

	SDL_Window   *win;
	SDL_Renderer *ren;
	uint32_t      winId;
	bool          winFocus;

	struct {
		TTF_Font *norm, *bold;
	} font;

	struct {
		SDL_Cursor *next, *norm, *beam, *hand, *no;
	} cursor;

	struct {
		SDL_Texture *tex;
		SDL_Color    color;
		char        *text;
	} textCache[GB_TEXT_CACHE_CAP];
	size_t textCachePos;
};

static uint8_t gbCodepointSizeMap[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0,
};

GB_DEF size_t gbGetCodepointSize(char ch) {
	return gbCodepointSizeMap[(uint8_t)ch];
}

GB_DEF size_t gbU8Length(const char *str) {
	size_t len = 0;
	while (*str != '\0') {
		++ len;
		str += gbGetCodepointSize(*str);
	}
	return len;
}

GB_DEF const char *gbU8At(const char *str, size_t pos) {
	size_t len = 0;
	while (*str != '\0') {
		if (len == pos)
			return str;

		++ len;
		str += gbGetCodepointSize(*str);
	}
	return str;
}

static const char *gbU8Prev(const char *str, size_t pos) {
	const char *it = gbU8At(str, pos);
	do {
		-- it;
		if (it < str)
			return NULL;
	} while ((*it & 0x80) != 0 && (*it & 0xC0) != 0xC0);
	return it;
}

static char *gbStrdup(const char *str) {
	char *duped = gbAlloc(strlen(str) + 1);
	if (duped == NULL)
		gbOutOfMem();

	strcpy(duped, str);
	return duped;
}

GB_DEF void GbCell_set(GbCell *this, const char *text) {
	size_t newSize = strlen(text);

	if (this->size != newSize) {
		this->content = gbRealloc(this->content, newSize + 1);
		if (this->content == NULL)
			gbOutOfMem();
	}

	strcpy(this->content, text);
	this->size = newSize;
	this->len  = gbU8Length(this->content);
}

GB_DEF void GbCell_fmt(GbCell *this, const char *fmt, ...) {
	char    content[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(content, sizeof(content), fmt, args);
	va_end(args);

	GbCell_set(this, content);
}

GB_DEF void GbCell_append(GbCell *this, const char *text) {
	size_t newSize = this->size + strlen(text);

	this->content = gbRealloc(this->content, newSize + 1);
	if (this->content == NULL)
		gbOutOfMem();

	strcat(this->content, text);
	this->size = newSize;
	this->len  = gbU8Length(this->content);
}

static void GbCell_insert(GbCell *this, const char *text, size_t pos) {
	size_t idx      = gbU8At(this->content, pos) - this->content;
	size_t textSize = strlen(text);
	size_t newSize  = this->size + textSize;

	if (textSize == 0)
		return;

	this->content = gbRealloc(this->content, newSize + 1);
	if (this->content == NULL)
		gbOutOfMem();

	for (size_t i = this->size; i --> idx;)
		this->content[i + textSize] = this->content[i];

	memcpy(this->content + idx, text, textSize);
	this->content[newSize] = 0;
	this->size = newSize;
	this->len  = gbU8Length(this->content);
}

GB_DEF GbStyle GbStyle_default(const char *fontNormPath, const char *fontBoldPath, int fontSize) {
	return (GbStyle){
		.mainColor = {
			{0x00, 0x00, 0x00, 0xFF}, /* Fg */
			{0xFF, 0xFF, 0xFF, 0xFF}, /* Bg */
			{0xEE, 0xEE, 0xEE, 0xFF}, /* Alt bg */
		},

		.headerColor = {
			{0x00, 0x00, 0x00, 0xFF}, /* Fg */
			{0x88, 0xCC, 0xEE, 0xFF}, /* Bg */
		},

		.borderColor = {
			{0x55, 0x55, 0x55, 0x44}, /* Normal */
			{0x55, 0x55, 0x55, 0x44}, /* Header */
		},

		.hoverColor = {0x00, 0x00, 0x00, 0x0B},
		.focusColor = {0x66, 0xAA, 0xCC, 0xAA},

		.selectionColor = {0x88, 0xCC, 0xEE, 0xFF},

		.scrollbarColor = {
			{0xDD, 0xDD, 0xDD, 0xFF}, /* Track */
			{0xC1, 0xC1, 0xC1, 0xFF}, /* Thumb */
			{0xAF, 0xAF, 0xAF, 0xFF}, /* Thumb hovered */
			{0x99, 0x99, 0x99, 0xFF}, /* Thumb focused */
		},

		.deleteColor = {
			{0xEE, 0x88, 0x88, 0xFF}, /* Bg */
			{0xEE, 0x88, 0x88, 0xDD}, /* Bg hovered */
			{0xFF, 0xFF, 0xFF, 0xFF}, /* Fg */
			{0xFF, 0xFF, 0xFF, 0xFF}, /* Fg hovered */
			{0x00, 0x00, 0x00, 0x22}, /* Border */
			{0x00, 0x00, 0x00, 0x22}, /* Border hovered */
		},

		.buttonColor = {
			{0x00, 0x00, 0x00, 0x11}, /* Bg */
			{0x00, 0x00, 0x00, 0x1F}, /* Bg hovered */
			{0x00, 0x00, 0x00, 0x88}, /* Fg */
			{0x00, 0x00, 0x00, 0xFF}, /* Fg hovered */
			{0x00, 0x00, 0x00, 0x22}, /* Border */
			{0x00, 0x00, 0x00, 0x22}, /* Border hovered */
		},

		.padding     = 10,
		.margin      = 5,
		.borderSize  = 2,
		.focusSize   = 3,
		.cursorWidth = 2,

		.scrollbarWidth  = 15,
		.scrollbarMargin = 2,

		.fontPath = {fontNormPath, fontBoldPath},
		.fontSize = fontSize,

		.borderless = true,
	};
}

static void GbTable_calculateButtons(GbTable *this) {
	int posRight = this->width;

	for (int id = 0; id < GbButtonsCount; ++ id) {
		TTF_SizeUTF8(this->font.norm, this->buttons[id].text,
		             &this->buttons[id].rect.w, NULL);
		this->buttons[id].rect.w += this->style.padding * 2;
		this->buttons[id].rect.h  = this->buttonSize;
		this->buttons[id].rect.y  = this->style.padding + this->style.borderSize;

		switch (id) {
		case GbButtonAdd:
			this->buttons[id].rect.x = this->style.padding;
			break;

		case GbButtonClose: case GbButtonMinimize:
			this->buttons[id].rect.x = (posRight -= this->buttons[id].rect.w + this->style.padding);
			break;
		}
	}
}

static void GbTable_calculateScrollbarThumb(GbTable *this) {
	float thumbHeight = (float)this->view.height / this->view.bufferHeight;
	this->scrollbar.thumb = (SDL_Rect){
		.x = this->width - this->style.scrollbarWidth + this->style.scrollbarMargin,
		.y = round(((float)this->view.scroll / this->view.bufferHeight) * this->view.height),
		.w = this->style.scrollbarWidth - this->style.scrollbarMargin * 2,
		.h = thumbHeight * this->view.height,
	};
}

static void GbTable_calculateHeight(GbTable *this) {
	bool scrollAtEnd = this->view.scroll >= this->view.bufferHeight - this->view.height;

	this->table.h = this->style.borderSize +
	                (this->style.borderSize + this->rowHeight) * (this->rows + 1);

	this->view.bufferHeight = this->table.h + this->style.margin * 2;
	this->height      = this->view.bufferHeight + this->barSize;
	this->view.height = this->height - this->barSize;

	int prevScroll = this->view.scroll;

	if (this->height > this->maxHeight) {
		this->height      = this->maxHeight;
		this->view.height = this->maxHeight - this->barSize;

		if (scrollAtEnd)
			this->view.scroll = this->view.bufferHeight - this->view.height;
	} else if (this->view.scroll != 0)
		this->view.scroll = 0;

	this->view.mouse.y += this->view.scroll - prevScroll;

	this->scrollbar.track = (SDL_Rect){
		.x = this->width - this->style.scrollbarWidth,
		.y = 0,
		.w = this->style.scrollbarWidth,
		.h = this->view.height,
	};

	GbTable_calculateScrollbarThumb(this);
	GbTable_calculateButtons(this);
}

static void GbTable_calculateTable(GbTable *this) {
	float fieldsSize = 0;
	for (int x = 0; x < this->cols; ++ x)
		fieldsSize += this->fields[x].conf.size;

	int w   = this->table.w - this->style.borderSize * (this->cols + 1);
	int sum = 0;
	for (int x = 0; x < this->cols; ++ x) {
		this->fields[x].x = sum + this->style.borderSize * (x + 1);
		this->fields[x].w = x == this->cols - 1? w - sum :
		                    round(this->fields[x].conf.size / fieldsSize * w);

		sum += this->fields[x].w;
	}

	GbTable_calculateButtons(this);
}

static void GbTable_updateRow(GbTable *this, int y) {
	if (this->rowHandler != NULL)
		this->rowHandler(this, this->cells[y], y, this->userData);
}

static void GbTable_unfocusCell(GbTable *this) {
	if (this->cell.focused != NULL)
		GbTable_updateRow(this, this->cell.focusedPos.y);

	this->cell.focused = NULL;
	SDL_StopTextInput();
}

static void GbTable_focusCell(GbTable *this, SDL_Point pos) {
	if (this->cell.focused != NULL)
		GbTable_updateRow(this, this->cell.focusedPos.y);

	this->cell.focusedPos = pos;
	this->cell.focused    = &this->cells[pos.y][pos.x];
	SDL_StartTextInput();
}

GB_DEF GbTable *GbTable_newWindow(const char *title, GbFieldConf *fieldsConf, int cols, int width,
                                  int maxHeight, GbRowHandler rowHandler, void* userData,
                                  GbStyle style) {
	GbTable *this = gbAlloc(sizeof(*this));
	if (this == NULL)
		gbOutOfMem();

	memset(this, 0, sizeof(*this));
	this->running         = true;
	this->style           = style;
	this->view.width      = width;
	this->width           = width;
	this->maxHeight       = maxHeight;
	this->cols            = cols;
	this->rowHandler      = rowHandler;
	this->userData        = userData;
	this->style           = style;
	this->rowHeight       = style.padding * 2 + style.fontSize;
	this->table.w         = width - style.margin * 2;
	this->table.x         = style.margin;
	this->table.y         = style.margin;
	this->winFocus        = true;
	this->cell.hoveredPos = (SDL_Point){-1, -1};
	this->hoveredId       = -1;
	this->focusedId       = -1;
	this->buttonSize      = style.fontSize + style.padding;
	this->barSize         = this->buttonSize + this->style.padding * 2 + this->style.borderSize;

	this->buttons[GbButtonAdd].text      = "+";
	this->buttons[GbButtonClose].text    = "Close";
	this->buttons[GbButtonMinimize].text = "Minimize";

	this->font.norm = TTF_OpenFont(style.fontPath[0], style.fontSize);
	if (this->font.norm == NULL)
		goto fail;

	this->font.bold = TTF_OpenFont(style.fontPath[1], style.fontSize);
	if (this->font.bold == NULL)
		goto fail;

	this->cap   = GB_CHUNK_SIZE;
	this->cells = gbAlloc(this->cap * sizeof(*this->cells));
	if (this->cells == NULL)
		gbOutOfMem();

	this->fields = gbAlloc(sizeof(*this->fields) * cols);
	if (this->fields == NULL)
		gbOutOfMem();

	for (int i = 0; i < cols; ++ i)
		this->fields[i].conf = fieldsConf[i];

	GbTable_calculateTable(this);
	GbTable_calculateHeight(this);

	this->win = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                             width, this->height, SDL_WINDOW_BORDERLESS * style.borderless);
	if (this->win == NULL)
		goto fail;

	this->winId = SDL_GetWindowID(this->win);

	this->ren = SDL_CreateRenderer(this->win, -1,
	                               SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (this->ren == NULL)
		goto fail;

	if (SDL_SetRenderDrawBlendMode(this->ren, SDL_BLENDMODE_BLEND) < 0)
		goto fail;

	this->cursor.norm = SDL_GetDefaultCursor();
	this->cursor.beam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	this->cursor.hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	this->cursor.no   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

	return this;

fail:
	gbFree(this);
	return NULL;
}

GB_DEF void GbTable_destroyWindow(GbTable *this) {
	for (int y = 0; y < this->rows; ++ y) {
		for (int x = 0; x < this->cols; ++ x)
			gbFree(this->cells[y][x].content);

		gbFree(this->cells[y]);
	}

	gbFree(this->cells);
	gbFree(this->fields);

	for (size_t i = 0; i < GB_TEXT_CACHE_CAP; ++ i) {
		if (this->textCache[i].tex == NULL)
			break;

		gbFree(this->textCache[i].text);
		SDL_DestroyTexture(this->textCache[i].tex);
	}

	SDL_DestroyRenderer(this->ren);
	SDL_DestroyWindow(this->win);

	gbFree(this);
}

static void GbTable_addRow(GbTable *this) {
	if (this->rows >= this->cap) {
		this->cap  *= 2;
		this->cells = gbRealloc(this->cells, this->cap * sizeof(*this->cells));
		if (this->cells == NULL)
			gbOutOfMem();
	}

	this->cells[this->rows] = gbAlloc(this->cols * sizeof(**this->cells));
	if (this->cells[this->rows] == NULL)
		gbOutOfMem();

	for (int x = 0; x < this->cols; ++ x) {
		this->cells[this->rows][x].content = gbStrdup("");
		this->cells[this->rows][x].size    = 0;
		this->cells[this->rows][x].len     = 0;
	}

	GbTable_updateRow(this, this->rows);

	++ this->rows;
	GbTable_calculateHeight(this);
	SDL_SetWindowSize(this->win, this->width, this->height);
}

static void GbTable_deleteRow(GbTable *this, int y) {
	for (int x = 0; x < this->cols; ++ x)
		gbFree(this->cells[y][x].content);

	gbFree(this->cells[y]);

	for (int i = y + 1; i < this->rows; ++ i) {
		this->cells[i - 1] = this->cells[i];
		GbTable_updateRow(this, i - 1);
	}

	-- this->rows;
	GbTable_calculateHeight(this);
	SDL_SetWindowSize(this->win, this->width, this->height);
}

static SDL_Rect GbTable_getCellRect(GbTable *this, SDL_Point pos) {
	gbAssert(pos.y >= -1);
	return (SDL_Rect){
		.x = this->table.x + this->fields[pos.x].x,
		.y = this->table.y + this->style.borderSize +
		     (this->rowHeight + this->style.borderSize) * (pos.y + 1),
		.w = this->fields[pos.x].w,
		.h = this->rowHeight,
	};
}

static SDL_Rect GbTable_getDeleteButtonRect(GbTable *this, int y) {
	gbAssert(y >= -1);
	return (SDL_Rect){
		.x = this->style.margin + this->style.padding / 2,
		.y = this->table.y + (this->rowHeight - this->buttonSize) / 2 +
		     (this->rowHeight + this->style.borderSize) * (y + 1),
		.w = this->buttonSize,
		.h = this->buttonSize,
	};
}

static bool GbTable_isCellFocused(GbTable *this, SDL_Point pos) {
	return this->cell.focused != NULL &&
	       this->cell.focusedPos.x == pos.x && this->cell.focusedPos.y == pos.y;
}

static void GbTable_renderRect(GbTable *this, SDL_Color color, SDL_Rect rect) {
	SDL_SetRenderDrawColor(this->ren, color.r, color.g, color.b, color.a);
	SDL_RenderFillRect(this->ren, &rect);
}

static void GbTable_renderOutline(GbTable *this, SDL_Color color, SDL_Rect rect, int size) {
	SDL_Rect border = rect;
	border.x -= size;
	border.w  = size;
	GbTable_renderRect(this, color, border);

	border.x += rect.w + size;
	GbTable_renderRect(this, color, border);

	border = rect;
	border.y -= size;
	border.x -= size;
	border.w += size * 2;
	border.h  = size;
	GbTable_renderRect(this, color, border);

	border.y += rect.h + size;
	GbTable_renderRect(this, color, border);
}

static void GbTable_renderText(GbTable *this, TTF_Font *font, const char *text,
                               SDL_Color color, int x, int y) {
	SDL_Texture *tex;
	SDL_Rect     rect = {.x = x, .y = y};

	/* Attempt to find already rendered text in cache */
	for (size_t i = 0; i < GB_TEXT_CACHE_CAP; ++ i) {
		if (this->textCache[i].tex == NULL)
			break;

		if (strcmp(this->textCache[i].text, text) == 0 &&
		    this->textCache[i].color.r == color.r &&
		    this->textCache[i].color.g == color.g &&
		    this->textCache[i].color.b == color.b &&
		    this->textCache[i].color.a == color.a) {
			tex = this->textCache[i].tex;
			goto render;
		}
	}

	/* Text not found in cache */
	if (this->textCachePos >= GB_TEXT_CACHE_CAP)
		this->textCachePos = 0;

	if (this->textCache[this->textCachePos].tex != NULL) {
		SDL_DestroyTexture(this->textCache[this->textCachePos].tex);
		gbFree(this->textCache[this->textCachePos].text);
	}

	SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text, color);
	tex = SDL_CreateTextureFromSurface(this->ren, surf);
	SDL_FreeSurface(surf);

	this->textCache[this->textCachePos].tex   = tex;
	this->textCache[this->textCachePos].color = color;
	this->textCache[this->textCachePos].text  = gbStrdup(text);
	++ this->textCachePos;

render:
	SDL_QueryTexture(tex, NULL, NULL, &rect.w, &rect.h);
	SDL_RenderCopy(this->ren, tex, NULL, &rect);
}

static int GbTable_focusedCellSubWidth(GbTable *this, int idx) {
	char *ptr = (char*)gbU8At(this->cell.focused->content, idx);
	char  tmp = *ptr;
	*ptr = 0;

	int x;
	TTF_SizeUTF8(this->font.norm, this->cell.focused->content, &x, NULL);

	*ptr = tmp;
	return x + this->style.padding;
}

static void GbTable_renderCell(GbTable *this, SDL_Point pos) {
	SDL_Rect rect = GbTable_getCellRect(this, pos);

	/* Render horizontal border */
	SDL_Rect border = rect;
	border.x -= this->style.borderSize;
	border.w  = this->style.borderSize;
	GbTable_renderRect(this, this->style.borderColor[pos.y < 0], border);
	if (pos.x == this->cols - 1) {
		border.x += rect.w + this->style.borderSize;
		GbTable_renderRect(this, this->style.borderColor[pos.y < 0], border);
	}

	/* Alternating background color */
	if (pos.y >= 0 && pos.y % 2)
		GbTable_renderRect(this, this->style.mainColor[2], rect);

	/* Hovered color */
	if (this->cell.hoveredPos.x == pos.x && this->cell.hoveredPos.y == pos.y)
		GbTable_renderRect(this, this->style.hoverColor, rect);

	/* Render cell content */
	SDL_Rect prevViewport, viewport = rect;
	SDL_RenderGetViewport(this->ren, &prevViewport);

	viewport.y += prevViewport.y;
	viewport.y += this->style.padding;
	viewport.h -= this->style.padding; /* Keep some space for letters that go below the line */

	if (viewport.y + viewport.h > prevViewport.y + prevViewport.h)
		viewport.h = prevViewport.y + prevViewport.h - viewport.y;

	SDL_RenderSetViewport(this->ren, &viewport);

	const char *text;
	TTF_Font   *font;
	if (pos.y >= 0) {
		text = this->cells[pos.y][pos.x].content;
		font = this->font.norm;
	} else {
		text = this->fields[pos.x].conf.title;
		font = this->font.bold;
	}

	SDL_Color color;
	if (pos.y < 0)
		color = this->style.headerColor[0];
	else
		color = this->style.mainColor[0];

	int scroll = 0;

	if (GbTable_isCellFocused(this, pos)) {
		/* Render selection */
		int start = GbTable_focusedCellSubWidth(this, this->cell.selectA);
		int end   = GbTable_focusedCellSubWidth(this, this->cell.selectB);

		if (start != end) {
			if (end < start) {
				int tmp = start;
				start   = end;
				end     = tmp;
			}
			GbTable_renderRect(this, this->style.selectionColor, (SDL_Rect){
				.x = start - this->cell.scroll,
				.y = 0,
				.w = end - start,
				.h = this->style.fontSize,
			});
		}

		/* Render cursor */
		int x = GbTable_focusedCellSubWidth(this, this->cell.cursor);

		if (x > rect.w + this->cell.scroll - this->style.cursorWidth - this->style.padding)
			this->cell.scroll = x - rect.w + this->style.cursorWidth + this->style.padding;

		if (x < this->cell.scroll + this->style.padding)
			this->cell.scroll = x - this->style.padding;

		GbTable_renderRect(this, color, (SDL_Rect){
			.x = x - this->cell.scroll,
			.y = 0,
			.w = this->style.cursorWidth,
			.h = this->style.fontSize,
		});

		scroll = this->cell.scroll;
	}

	GbTable_renderText(this, font, text, color, this->style.padding - scroll, 0);

	SDL_RenderSetViewport(this->ren, &prevViewport);
}

static void GbTable_renderButton(GbTable *this, bool hovered, TTF_Font *font,
                                 const char *text, SDL_Color color[6], SDL_Rect rect) {
	GbTable_renderRect(this, color[hovered], rect);

	int textWidth;
	TTF_SizeUTF8(font, text, &textWidth, NULL);
	gbAssert(rect.w >= textWidth);

	int x = rect.x + (rect.w - textWidth) / 2;
	int y = rect.y + (rect.h - this->style.fontSize) / 2;
	GbTable_renderText(this, font, text, color[2 + hovered], x, y);

	rect.x += this->style.borderSize;
	rect.y += this->style.borderSize;
	rect.w -= this->style.borderSize * 2;
	rect.h -= this->style.borderSize * 2;
	GbTable_renderOutline(this, color[4 + hovered], rect, this->style.borderSize);
}

float easeInQuad(float t) {
	return t * t;
}

static void GbTable_renderView(GbTable *this) {
	SDL_Rect viewport = {
		.x = 0,
		.y = -this->view.scroll,
		.w = this->view.width - (this->scrollbar.visible? this->style.scrollbarWidth : 0),
		.h = this->view.height + this->view.scroll,
	};
	SDL_RenderSetViewport(this->ren, &viewport);

	/* Header background */
	GbTable_renderRect(this, this->style.headerColor[1], (SDL_Rect){
		.x = this->table.x,
		.y = this->table.y,
		.w = this->table.w,
		.h = this->rowHeight + this->style.borderSize * 2,
	});

	/* Render horizontal lines, cells and delete buttons */
	for (int y = -1; y <= this->rows; ++ y) {
		GbTable_renderRect(this, this->style.borderColor[y <= 0], (SDL_Rect){
			.x = this->table.x,
			.y = this->table.y + (this->rowHeight + this->style.borderSize) * (y + 1),
			.w = this->table.w,
			.h = this->style.borderSize,
		});

		if (y >= this->rows)
			continue;

		if (this->deleting && y >= 0) {
			bool hovered = this->hoveredId == GbButtonsCount + y;
			GbTable_renderButton(this, hovered, this->font.bold, "X", this->style.deleteColor,
			                     GbTable_getDeleteButtonRect(this, y));
		}

		for (int x = 0; x < this->cols; ++ x)
			GbTable_renderCell(this, (SDL_Point){x, y});
	}

	if (this->cell.focused != NULL)
		GbTable_renderOutline(this, this->style.focusColor,
		                      GbTable_getCellRect(this, this->cell.focusedPos),
		                      this->style.focusSize);

	int size = 15;
	for (int i = 0; i < size; ++ i) {
		GbTable_renderRect(this, (SDL_Color){0, 0, 0, easeInQuad((float)i / size) * 30}, (SDL_Rect){
			.x = 0,
			.y = this->view.scroll + this->view.height - (size - i),
			.w = viewport.w,
			.h = 1,
		});
	}
}

static void GbTable_renderScrollbar(GbTable *this) {
	if (!this->scrollbar.visible)
		return;

	SDL_RenderSetViewport(this->ren, NULL);

	GbTable_renderRect(this, this->style.scrollbarColor[0], this->scrollbar.track);

	SDL_Color color;
	if (this->scrollbar.focused)
		color = this->style.scrollbarColor[3];
	else
		color = this->style.scrollbarColor[1 + SDL_PointInRect(&this->mouse,
		                                                       &this->scrollbar.thumb)];
	GbTable_renderRect(this, color, this->scrollbar.thumb);
}

static void GbTable_renderButtons(GbTable *this) {
	SDL_Rect viewport = {
		.x = 0,
		.y = this->view.height,
		.w = this->width,
		.h = this->barSize,
	};
	SDL_RenderSetViewport(this->ren, &viewport);

	GbTable_renderRect(this, this->style.borderColor[0], (SDL_Rect){
		.x = 0,
		.y = 0,
		.w = this->width,
		.h = this->style.borderSize,
	});

	for (int id = 0; id < GbButtonsCount; ++ id)
		GbTable_renderButton(this, this->hoveredId == id, this->font.norm, this->buttons[id].text,
		                     this->style.buttonColor, this->buttons[id].rect);
}

GB_DEF void GbTable_render(GbTable *this) {
	SDL_SetRenderDrawColor(this->ren, this->style.mainColor[1].r, this->style.mainColor[1].g,
	                       this->style.mainColor[1].b, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(this->ren);

	GbTable_renderView(this);
	GbTable_renderScrollbar(this);
	GbTable_renderButtons(this);

	SDL_RenderPresent(this->ren);
}

static void GbTable_inputCells(GbTable *this) {
	SDL_Rect view = {
		.x = 0,
		.y = 0,
		.w = this->view.width,
		.h = this->view.height,
	};

	if (this->cell.focused == NULL || !this->cell.selecting) {
		this->cell.hoveredPos = (SDL_Point){-1, -1};
		if (SDL_PointInRect(&this->mouse, &view)) {
			for (int y = 0; y < this->rows; ++ y) {
				for (int x = 0; x < this->cols; ++ x) {
					SDL_Rect rect = GbTable_getCellRect(this, (SDL_Point){x, y});
					if (SDL_PointInRect(&this->view.mouse, &rect)) {
						this->cell.hoveredPos = (SDL_Point){x, y};
						this->cursor.next     = this->fields[x].conf.locked?
						                        this->cursor.no : this->cursor.beam;
						goto end;
					}
				}
			}
		}
	} else
		this->cursor.next = this->cursor.beam;
end:

	if (!this->mouseDown) {
		if (this->cell.selecting)
			this->cell.selecting = false;
		return;
	}

	if (this->cell.hoveredPos.x != -1) {
		if (this->fields[this->cell.hoveredPos.x].conf.locked) {
			GbCell *cell = &this->cells[this->cell.hoveredPos.y][this->cell.hoveredPos.x];
			SDL_SetClipboardText(cell->content);
			GbTable_unfocusCell(this);
		} else {
			if (this->cell.focused == NULL || this->cell.focusedPos.x != this->cell.hoveredPos.x ||
			    this->cell.focusedPos.y != this->cell.hoveredPos.y) {
				/* Only switch to a cell if it was clicked */
				if (this->mouseWasDown || !SDL_PointInRect(&this->mouse, &view))
					return;

				GbTable_focusCell(this, this->cell.hoveredPos);
				this->cell.scroll = 0;
			}

			SDL_Rect rect = GbTable_getCellRect(this, this->cell.hoveredPos);
			this->cell.cursor = 0;

			int mouseX = this->view.mouse.x - rect.x + this->cell.scroll;
			if (mouseX > this->style.padding) {
				for (this->cell.cursor = 0; this->cell.cursor < this->cell.focused->len;
				     ++ this->cell.cursor) {
					int x = GbTable_focusedCellSubWidth(this, this->cell.cursor);
					if (this->cell.cursor + 1 <= this->cell.focused->len)
						x += (GbTable_focusedCellSubWidth(this, this->cell.cursor + 1) - x) / 2;

					if (mouseX < x)
						break;
				}
			}

			if (!this->mouseWasDown) {
				this->cell.selecting = true;
				this->cell.selectB = this->cell.cursor;
				this->cell.selectA = this->cell.cursor;
			} else
				this->cell.selectB = this->cell.cursor;
		}
	} else if (!this->mouseWasDown) /* If clicked outside a cell */
		GbTable_unfocusCell(this);
}

static void GbTable_inputScrollbar(GbTable *this) {
	if (!this->scrollbar.visible) {
		if (this->view.bufferHeight > this->view.height) {
			this->scrollbar.visible = true;
			this->table.w -= this->style.scrollbarWidth;
			GbTable_calculateTable(this);

			this->cell.hoveredPos = (SDL_Point){-1, -1};
			this->hoveredId       = -1;
			this->focusedId       = -1;
		}
		return;
	}

	if (this->view.bufferHeight <= this->view.height) {
		this->scrollbar.visible = false;
		this->table.w += this->style.scrollbarWidth;
		GbTable_calculateTable(this);
		return;
	}

	if (!this->scrollbar.focused && this->mouseDown &&
	    SDL_PointInRect(&this->mouse, &this->scrollbar.thumb)) {
		this->scrollbar.focused     = true;
		this->scrollbar.thumbMouseY = this->mouse.y - this->scrollbar.thumb.y;
	} else if (this->scrollbar.focused && !this->mouseDown)
		this->scrollbar.focused = false;
}

static bool GbTable_inputButton(GbTable *this, int id, SDL_Rect rect, SDL_Point mouse) {
	if (this->focusedId == id && !this->mouseDown) {
		this->focusedId = -1;
		return SDL_PointInRect(&mouse, &rect);
	} else if (this->focusedId == -1 && SDL_PointInRect(&mouse, &rect)) {
		if (this->mouseDown && !this->mouseWasDown)
			this->focusedId = id;
		else
			this->hoveredId = id;
	}

	return false;
}

static void GbTable_inputButtons(GbTable *this) {
	this->hoveredId = -1;

	if (this->cell.selecting)
		return;

	for (int id = 0; id < GbButtonsCount; ++ id) {
		SDL_Point mouse = {
			.x = this->mouse.x,
			.y = this->mouse.y - this->view.height,
		};
		if (GbTable_inputButton(this, id, this->buttons[id].rect, mouse)) {
			switch (id) {
			case GbButtonAdd:      GbTable_addRow(this);          break;
			case GbButtonClose:    this->running = false;         break;
			case GbButtonMinimize: SDL_MinimizeWindow(this->win); break;
			}
		}
	}

	if (this->deleting) {
		for (int y = 0; y < this->rows; ++ y) {
			if (GbTable_inputButton(this, GbButtonsCount + y,
			                        GbTable_getDeleteButtonRect(this, y), this->view.mouse))
				GbTable_deleteRow(this, y);
		}
	}

	if (this->hoveredId != -1 || this->focusedId != -1)
		this->cursor.next = this->cursor.hand;
}

static void GbTable_scrollTo(GbTable *this, int to) {
	int prevScroll    = this->view.scroll;
	this->view.scroll = to;

	if (this->view.scroll < 0)
		this->view.scroll = 0;
	if (this->view.scroll > this->view.bufferHeight - this->view.height)
		this->view.scroll = this->view.bufferHeight - this->view.height;

	this->view.mouse.y += this->view.scroll - prevScroll;

	GbTable_calculateScrollbarThumb(this);
}

static void GbTable_cellSelection(GbTable *this, char **start, char **end) {
	*start = (char*)gbU8At(this->cell.focused->content, this->cell.selectA);
	*end   = (char*)gbU8At(this->cell.focused->content, this->cell.selectB);
	if (*end < *start) {
		char *tmp = *start;
		*start    = *end;
		*end      = tmp;
	}
}

static void GbTable_cellCancelSelection(GbTable *this) {
	this->cell.selectA = 0;
	this->cell.selectB = 0;
}

static bool GbTable_cellEraseSelected(GbTable *this) {
	if (this->cell.selectA == this->cell.selectB)
		return false;

	char *start, *end;
	GbTable_cellSelection(this, &start, &end);

	size_t size     = end - start;
	size_t moveSize = this->cell.focused->size - (size_t)(end - this->cell.focused->content);
	for (size_t i = 0; i < moveSize; ++ i)
		start[i] = end[i];

	this->cell.focused->size -= size;
	this->cell.focused->content[this->cell.focused->size] = 0;

	this->cell.cursor = this->cell.selectA > this->cell.selectB?
	                    this->cell.selectB : this->cell.selectA;
	GbTable_cellCancelSelection(this);

	this->cell.focused->len = gbU8Length(this->cell.focused->content);
	return true;
}

static void GbTable_cellBackspace(GbTable *this) {
	if (GbTable_cellEraseSelected(this))
		return;

	if (this->cell.cursor == 0)
		return;

	char *prev = (char*)gbU8Prev(this->cell.focused->content, this->cell.cursor);

	if (this->cell.cursor == this->cell.focused->len) {
		*prev = 0;
		-- this->cell.cursor;
		-- this->cell.focused->len;
		this->cell.focused->size -= gbGetCodepointSize(*prev);
	} else {
		size_t idx  = prev - this->cell.focused->content;
		size_t size = gbGetCodepointSize(this->cell.focused->content[idx]);
		size_t len  = strlen(this->cell.focused->content);

		for (size_t i = idx + size; i < len; ++ i)
			this->cell.focused->content[i - size] = this->cell.focused->content[i];

		this->cell.focused->content[len - size] = 0;
		-- this->cell.cursor;
		-- this->cell.focused->len;
		this->cell.focused->size -= size;
	}
}

static void GbTable_cellInsert(GbTable *this, const char *text) {
	GbTable_cellEraseSelected(this);
	GbCell_insert(this->cell.focused, text, this->cell.cursor);
	this->cell.cursor += gbU8Length(text);
}

static void GbTable_cellPaste(GbTable *this) {
	GbTable_cellInsert(this, SDL_GetClipboardText());
}

static void GbTable_cellCopy(GbTable *this) {
	if (this->cell.selectA == this->cell.selectB)
		return;

	char *start, *end;
	GbTable_cellSelection(this, &start, &end);

	char tmp = *end;
	*end = 0;
	SDL_SetClipboardText(start);
	*end = tmp;
}

static void GbTable_cellCut(GbTable *this) {
	GbTable_cellCopy(this);
	GbTable_cellEraseSelected(this);
}

GB_DEF bool GbTable_input(GbTable *this) {
	SDL_Event evt;
	while (SDL_PollEvent(&evt)) {

		if (evt.type == SDL_WINDOWEVENT && evt.window.windowID == this->winId) {
			switch (evt.window.event) {
			case SDL_WINDOWEVENT_FOCUS_GAINED: this->winFocus = true;  break;
			case SDL_WINDOWEVENT_FOCUS_LOST:   this->winFocus = false; break;

			case SDL_WINDOWEVENT_CLOSE: this->running = false; break;
			}
		}

		if (!this->winFocus)
			continue;

		this->cursor.next = this->cursor.norm;

		switch (evt.type) {
		case SDL_MOUSEMOTION:
			this->mouse.x = evt.motion.x;
			this->mouse.y = evt.motion.y;

			this->view.mouse    = this->mouse;
			this->view.mouse.y += this->view.scroll;

			if (this->scrollbar.focused) {
				int y = this->mouse.y - this->scrollbar.thumbMouseY;
				GbTable_scrollTo(this, (float)y / this->view.height * this->view.bufferHeight);
			}
			break;

		case SDL_MOUSEWHEEL:
			GbTable_scrollTo(this, this->view.scroll - evt.wheel.y * this->rowHeight);
			break;

		case SDL_MOUSEBUTTONDOWN: this->mouseDown = true;  break;
		case SDL_MOUSEBUTTONUP:   this->mouseDown = false; break;

		case SDL_KEYDOWN:
			switch (evt.key.keysym.sym) {
			case SDLK_TAB:
				if ((this->deleting = !this->deleting)) {
					this->table.w -= this->rowHeight;
					this->table.x += this->rowHeight;
				} else {
					this->table.w += this->rowHeight;
					this->table.x -= this->rowHeight;
				}

				GbTable_calculateTable(this);
				break;
			}

			if (this->cell.focused == NULL)
				break;

			switch (evt.key.keysym.sym) {
			case SDLK_LEFT:
				if (this->cell.selectA != this->cell.selectB)
					GbTable_cellCancelSelection(this);
				else {
					if (this->cell.cursor > 0)
						-- this->cell.cursor;
				}
				break;

			case SDLK_RIGHT:
				if (this->cell.selectA != this->cell.selectB)
					GbTable_cellCancelSelection(this);
				else {
					if (this->cell.cursor < this->cell.focused->len)
						++ this->cell.cursor;
				}
				break;

			case SDLK_v:
				if (SDL_GetModState() & KMOD_CTRL)
					GbTable_cellPaste(this);
				break;

			case SDLK_c:
				if (SDL_GetModState() & KMOD_CTRL)
					GbTable_cellCopy(this);
				break;

			case SDLK_x:
				if (SDL_GetModState() & KMOD_CTRL)
					GbTable_cellCut(this);
				break;

			case SDLK_a:
				if (SDL_GetModState() & KMOD_CTRL) {
					this->cell.selectA = 0;
					this->cell.selectB = this->cell.focused->len;
				}
				break;

			case SDLK_BACKSPACE: GbTable_cellBackspace(this); break;

			case SDLK_ESCAPE:
			case SDLK_RETURN: GbTable_unfocusCell(this); break;
			}
			break;

		case SDL_TEXTINPUT: GbTable_cellInsert(this, evt.text.text); break;
		}

		GbTable_inputScrollbar(this);

		if (!this->scrollbar.focused) {
			GbTable_inputCells(this);
			GbTable_inputButtons(this);
		}

		this->mouseWasDown = this->mouseDown;

		if (SDL_GetCursor() != this->cursor.next)
			SDL_SetCursor(this->cursor.next);
	}

	return this->running;
}

GB_DEF void GbTable_invalidValueMessageBox(GbTable *this, int y, int field, const char *fmt, ...) {
	gbAssert(y < this->rows && field < this->cols);

	char    msg[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	char title[1024];
	snprintf(title, sizeof(title), "Row %i: Invalid value in field \"%s\"",
	         y, this->fields[field].conf.title);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, msg, this->win);

	for (int x = 0; x < this->cols; ++ x)
		GbCell_set(&this->cells[y][x], "");

	GbTable_updateRow(this, y);
}

#undef gbOutOfMem
