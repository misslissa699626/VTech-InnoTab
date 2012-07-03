/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file luasdl.c
 * @brief Lua SDL interface
 * @author Anson Chuang
 */

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

#include <mach/typedef.h>
#include "SDL.h"
#include "SDL_gfxPrimitives.h"
#include "SDL_gfxBlitFunc.h"
#include "SDL_ttf.h"
#include "SDL_image.h"

#include "fntMgr.h"

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define SDL_EVENT_WAIT_FOREVER 0xffffffff
#define SDL_EVENT_NO_WAIT 0

enum {
	SDL_RGB565 = 0,
	SDL_RGBA8888,
};

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static int
getField(
	lua_State *L,
	int index,
	const char *key
)
{
	int result;
	lua_getfield(L, index, key);
	result = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	return result;
}

static gp_point_t
luaSdl_toPoint(
	lua_State *L,
	int index
)
{
	gp_point_t pnt;
	pnt.x = getField(L, index, "x");
	pnt.y = getField(L, index, "y");
	return pnt;
}

static gp_rect_t
luaSdl_toRect(
	lua_State *L,
	int index
)
{
	gp_rect_t rect;
	rect.x = getField(L, index, "x");
	rect.y = getField(L, index, "y");
	rect.width = getField(L, index, "width");
	rect.height = getField(L, index, "height");
	return rect;
}

static gp_color_t
luaSdl_toColor(
	lua_State *L,
	int index
)
{
	gp_color_t color = {0};

	/* Check for alpha channel */
	color.alpha = 255;
	lua_getfield(L, index, "a");
	if (!lua_isnoneornil(L, -1)) {
		color.alpha = (int32_t)lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	color.red = getField(L, index, "r");
	color.green = getField(L, index, "g");
	color.blue = getField(L, index, "b");

	return color;
}

/*!
 * @breif SDL init
 */
static int32_t
luaSDL_init(
	lua_State *L
)
{
	uint32_t flag;

	fntMgrInit();

	/* Init SDL */
	flag = luaL_checkint(L, 1);
	if (SDL_Init(flag) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
		goto __error_sdl;
	}

	/* Init SDL TTF */
	if ( TTF_Init() < 0 ) {
		fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
		goto __error_sdl_ttf;
	}

	/* Set video mode if it init SDL_INIT_VIDEO previously */
	if ((flag & SDL_INIT_VIDEO) == SDL_INIT_VIDEO) {
		uint32_t bpp;
		SDL_Surface *surface;

		if (lua_isnone(L, 2)) { /* RGB565 */
			bpp = 16;
		}
		else {
			switch (luaL_checkint(L, 2)) {
				case SDL_RGB565:
					bpp = 16;
					break;

				case SDL_RGBA8888:
					bpp = 32;
					break;

				default:
					bpp = 16;
					break;
			}
		}

		surface = SDL_SetVideoMode(0, 0, bpp, (SDL_DOUBLEBUF | SDL_HWSURFACE));
		if (surface == NULL) {
			printf("[%s:%d], set video mode error\n", __FUNCTION__, __LINE__);
			goto __error_sdl_set_video;
		}
		else {
			if (!lua_isnone(L, 2)) {
				switch (luaL_checkint(L, 2)) {
					case SDL_RGBA8888:
						surface->format->Rshift = 24;
						surface->format->Gshift = 16;
						surface->format->Bshift = 8;
						surface->format->Ashift = 0;
						surface->format->Rmask = 0xff000000;
						surface->format->Gmask = 0x00ff0000;
						surface->format->Bmask = 0x0000ff00;
						surface->format->Amask = 0x000000ff;
						break;
					default:
						break;
				}
			}
		}
	}

	lua_pushinteger(L, SP_OK);
	return 1;

__error_sdl_set_video:
	TTF_Quit();
__error_sdl_ttf:
	SDL_Quit();
__error_sdl:
	lua_pushinteger(L, SP_FAIL);
	return 1;
}

/*!
 * @breif SDL quit
 */
static int32_t
luaSDL_quit(
	lua_State *L
)
{
	fntMgrRelease();
	TTF_Quit();
	SDL_Quit();
	return 0;
}

/*!
 * @breif Get screen size
 * @return width, height
 */
static int32_t
luaSDL_getScreenSize(
	lua_State *L
)
{
	const SDL_VideoInfo *info = SDL_GetVideoInfo();
	lua_pushinteger(L, info->current_w);
	lua_pushinteger(L, info->current_h);
	return 2;
}

/*!
 * @breif Get sdl surface
 * @return SDL_Surface
 */
static int32_t
luaSDL_getSurface(
	lua_State *L
)
{
	SDL_Surface *surface;

	surface = SDL_GetVideoSurface();
	if (!surface) {
		lua_pushnil(L);
	}
	else {
		lua_pushlightuserdata(L, surface);
	}
	return 1;
}

/*!
 * @breif Flip surface
 * ret = flip(surface)
 */
static int32_t
luaSDL_flip(
	lua_State *L
)
{
	SDL_Surface *surface;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	if (SDL_Flip(surface) < 0) {
		printf("[%s:%d] Error\n", __FUNCTION__, __LINE__);
		lua_pushinteger(L, SP_FAIL);
	}
	else {
		lua_pushinteger(L, SP_OK);
	}
	return 1;
}

/*!
 * @breif Delay ms
 */
static int32_t
luaSDL_delay(
	lua_State *L
)
{
	uint32_t ms;

	ms = luaL_checkinteger(L, 1);
	SDL_Delay(ms);
	return 0;
}

/*!
 * @breif Get event
 */
static int32_t
luaSDL_getEvent(
	lua_State *L
)
{
	int32_t flag;
	int32_t ret;
	SDL_Event event;

	flag = luaL_checkinteger(L, 1);
	if (flag == SDL_EVENT_WAIT_FOREVER) {
		ret = SDL_WaitEvent(&event);
	}
	else {
		ret = SDL_PollEvent(&event);
	}

	if (ret == 1) {
		lua_newtable(L);
		lua_pushinteger(L, event.type);
		lua_setfield(L, -2, "type");
		lua_pushinteger(L, event.button.x);
		lua_setfield(L, -2, "x");
		lua_pushinteger(L, event.button.y);
		lua_setfield(L, -2, "y");
	}
	else {
		lua_pushnil(L);
	}
	return 1;
}

/*!
 * @breif Draw a line
 * ret = drawLine(surface, p1, p2, color)
 */
static int32_t
luaSDL_drawLine(
	lua_State *L
)
{
	SDL_Surface *surface;
	gp_point_t p1, p2;
	gp_color_t color;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	p1 = luaSdl_toPoint(L, 2);
	p2 = luaSdl_toPoint(L, 3);
	color = luaSdl_toColor(L, 4);

	lineRGBA(surface, p1.x, p1.y, p2.x, p2.y, color.red, color.green, color.blue, color.alpha);

	return 0;
}

/*!
 * @breif Draw rect
 *  drawRect({x, y, width, height}, color)
 *  drawRect(x, y, width, height, color)
 */
static int32_t
luaSDL_drawRect(
	lua_State *L
)
{
	SDL_Surface *surface;
	gp_rect_t rect;
	gp_color_t color;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	if (!lua_istable(L, 2)) { /* number mode */
		rect.x = (int16_t)lua_tointeger(L, 2);
		rect.y = (int16_t)lua_tointeger(L, 3);
		rect.width = (int16_t)lua_tointeger(L, 4);
		rect.height = (int16_t)lua_tointeger(L, 5);
		color = luaSdl_toColor(L, 6);
	}
	else {
		rect = luaSdl_toRect(L, 2);
		color = luaSdl_toColor(L, 3);
	}
	rectangleRGBA(surface, rect.x + rect.width, rect.y, rect.x, rect.y + rect.height, color.red, color.green, color.blue, color.alpha);
	return 0;
}

/*!
 * @breif Fill rect
 *  fillRect({x, y, width, height}, color)
 *  fillRect(x, y, width, height, color)
 */
static int32_t
luaSDL_fillRect(
	lua_State *L
)
{
	SDL_Surface *surface;
	gp_rect_t rect;
	gp_color_t color;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	if (!lua_istable(L, 2)) { /* number mode */
		rect.x = (int16_t)lua_tointeger(L, 2);
		rect.y = (int16_t)lua_tointeger(L, 3);
		rect.width = (int16_t)lua_tointeger(L, 4);
		rect.height = (int16_t)lua_tointeger(L, 5);
		color = luaSdl_toColor(L, 6);
	}
	else {
		rect = luaSdl_toRect(L, 2);
		color = luaSdl_toColor(L, 3);
	}
	boxRGBA(surface, rect.x + rect.width, rect.y, rect.x, rect.y + rect.height, color.red, color.green, color.blue, color.alpha);
	return 0;
}

/*!
 * @breif Draw ellipse
 *  drawEllipse({x, y, width, height}, color)
 *  drawEllipse(x, y, width, height, color)
 */
static int32_t
luaSDL_drawEllipse(
	lua_State *L
)
{
	SDL_Surface *surface;
	gp_rect_t rect;
	gp_color_t color;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	if (!lua_istable(L, 2)) { /* number mode */
		rect.x = (int16_t)lua_tointeger(L, 2);
		rect.y = (int16_t)lua_tointeger(L, 3);
		rect.width = (int16_t)lua_tointeger(L, 4);
		rect.height = (int16_t)lua_tointeger(L, 5);
		color = luaSdl_toColor(L, 6);
	}
	else {
		rect = luaSdl_toRect(L, 2);
		color = luaSdl_toColor(L, 3);
	}

	ellipseRGBA(surface, rect.x + rect.width/2, rect.y + rect.height/2, rect.width/2, rect.height/2, color.red, color.green, color.blue, color.alpha);

	return 0;
}

/*!
 * @breif Fill ellipse
 *  fillEllipse({x, y, width, height}, color)
 *  fillEllipse(x, y, width, height, color)
 */
static int32_t
luaSDL_fillEllipse(
	lua_State *L
)
{
	SDL_Surface *surface;
	gp_rect_t rect;
	gp_color_t color;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	if (!lua_istable(L, 2)) { /* number mode */
		rect.x = (int16_t)lua_tointeger(L, 2);
		rect.y = (int16_t)lua_tointeger(L, 3);
		rect.width = (int16_t)lua_tointeger(L, 4);
		rect.height = (int16_t)lua_tointeger(L, 5);
		color = luaSdl_toColor(L, 6);
	}
	else {
		rect = luaSdl_toRect(L, 2);
		color = luaSdl_toColor(L, 3);
	}

	filledEllipseRGBA(surface, rect.x + rect.width/2, rect.y + rect.height/2, rect.width/2, rect.height/2, color.red, color.green, color.blue, color.alpha);
	return 0;
}

/*!
 * @breif
 */
static int32_t
luaSDL_loadFont(
	lua_State *L
)
{
	return 0;
}

/*!
 * @breif
 */
static int32_t
luaSDL_freeFont(
	lua_State *L
)
{
  return 0;
}

/*!
 * @breif Draw text
 * drawText(surface, position, text, colorText, typeface, textSize)
 */
static int32_t
luaSDL_drawText(
	lua_State *L
)
{
	SDL_Surface *surface, *textSurface;
	gp_point_t pos;
	char *text;
	gp_color_t color;
	char *typeface = NULL;
	int32_t size;
	TTF_Font *font;
	SDL_Rect dstrect;
	SDL_Color forecol;

	surface = (SDL_Surface *) lua_touserdata(L, 1);
	pos = luaSdl_toPoint(L, 2);
	text = (char*)luaL_checkstring(L, 3);
	color = luaSdl_toColor(L, 4);
	typeface = (char *)luaL_checkstring(L, 5);
	size = luaL_checkinteger(L, 6);

	font = fntMgrOpenFont(typeface, size);
	if ( font == NULL ) {
		fprintf(stderr, "Couldn't load %d pt font from %s: %s\n",
				size, typeface, SDL_GetError());
		return 0;
	}

	/* draw font */
	forecol.r = color.red;
	forecol.g = color.green;
	forecol.b = color.blue;
	textSurface = TTF_RenderUTF8_Solid(font, text, forecol);
	if ( textSurface != NULL ) {
		dstrect.x = pos.x;
		dstrect.y = pos.y;
		SDL_BlitSurface(textSurface, NULL, surface, &dstrect);
		SDL_FreeSurface(textSurface);
	}

	/* close font */
	fntMgrCloseFont(typeface, size);
	return 0;
}

/*!
 * @breif Get text bound
 * w, h = getTextBound(text, typeface, textsize)
 */
static int32_t
luaSDL_getTextBound(
	lua_State *L
)
{
	char *typeface = NULL;
	TTF_Font *font;
	char *text;
	int32_t size;
	int32_t w, h;
	int32_t ret;

	text = (char*)luaL_checkstring(L, 1);
	typeface = (char *)luaL_checkstring(L, 2);
	size = luaL_checkinteger(L, 3);

	font = fntMgrOpenFont(typeface, size);
	if ( font == NULL ) {
		fprintf(stderr, "Couldn't load %d pt font from %s: %s\n",
				size, typeface, SDL_GetError());
		return 0;
	}

	ret = TTF_SizeText(font, text, &w, &h);
	/* close font */
	fntMgrCloseFont(typeface, size);

	if (ret < 0) {
		return 0;
	}
	else {
		lua_newtable(L);
		lua_pushinteger(L, w);
		lua_setfield(L, -2, "width");
		lua_pushinteger(L, h);
		lua_setfield(L, -2, "height");
		return 1;
	}

	return 0;
}

/*!
 * @breif
 */
static int32_t
luaSDL_loadImage(
	lua_State *L
)
{
	SDL_Surface *image;
	char *filename = NULL;

	filename = (char *)luaL_checkstring(L, 1);
	if (filename) {
		image = IMG_Load(filename);
		if (image) {
			lua_pushlightuserdata(L, image);
			return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

/*!
 * @breif
 */
static int32_t
luaSDL_freeImage(
	lua_State *L
)
{
	SDL_Surface *image = (SDL_Surface *)lua_touserdata(L, 1);
	SDL_FreeSurface(image);
	return 0;
}

static int32_t
drawImageZoom(
	SDL_Surface *src,
	SDL_Rect *srcrect,
	SDL_Surface *dst,
	SDL_Rect * dstrect
)
{
	printf("Not implement\n");

	return 1;
}

/*!
 * @breif Draw image
 * drawImage(surface, image)
 * drawImage(surface, {x, y}, image)
 * drawImage(surface, {x, y}, image, {x, y, width, height})
 * drawImage(surface, {x, y, width, height}, image)
 * drawImage(surface, {x, y, width, height}, image, {x, y, width, height})
 * drawImage(surface, x, y, image)
 * drawImage(surface, x, y, image, x, y, width, height)
 * drawImage(surface, x, y, width, height, image)
 * drawImage(surface, x, y, width, height, image, x, y, width, height)
 */
static int32_t
luaSDL_drawImage(
	lua_State *L
)
{
	SDL_Surface *surface = (SDL_Surface *) lua_touserdata(L, 1);
	int32_t result;
	SDL_Surface *image;

	if (lua_isnone(L, 3)) { /* image */
		image = (SDL_Surface *)lua_touserdata(L, 2);
		if (image == NULL) {
			lua_pushnil(L);
			return 1;
		}
		result = SDL_BlitSurface(image, NULL, surface, NULL);
	}
	else if (!lua_istable(L, 2)) { /* number mode */
		if (lua_islightuserdata(L, 4)) {
			SDL_Rect dstrect;
			dstrect.x = (int16_t)lua_tointeger(L, 2);
			dstrect.y = (int16_t)lua_tointeger(L, 3);
			image = (SDL_Surface *)lua_touserdata(L, 4);
			if (lua_isnoneornil(L, 5)) { /* x, y, image */
				result = SDL_BlitSurface(image, NULL, surface, &dstrect);
			}
			else { /* x, y, image, x, y, width, height */
				SDL_Rect srcrect;
				srcrect.x = (int16_t)lua_tointeger(L, 5);
				srcrect.y = (int16_t)lua_tointeger(L, 6);
				srcrect.w = (uint16_t)lua_tointeger(L, 7);
				srcrect.h = (uint16_t)lua_tointeger(L, 8);
				result = SDL_BlitSurface(image, &srcrect, surface, &dstrect);
			}
		}
		else {
			SDL_Rect dstrect;
			dstrect.x = (int16_t)lua_tointeger(L, 2);
			dstrect.y = (int16_t)lua_tointeger(L, 3);
			dstrect.w = (uint16_t)lua_tointeger(L, 4);
			dstrect.h = (uint16_t)lua_tointeger(L, 5);
			image = (SDL_Surface *)lua_touserdata(L, 6);
			if (lua_isnoneornil(L, 7)) { /* x, y, width, height, image */
				result = drawImageZoom(image, NULL, surface, &dstrect);
			}
			else { /* x, y, width, height, image, x, y, width, height */
				SDL_Rect srcrect;
				srcrect.x = (int16_t)lua_tointeger(L, 5);
				srcrect.y = (int16_t)lua_tointeger(L, 6);
				srcrect.w = (uint16_t)lua_tointeger(L, 7);
				srcrect.h = (uint16_t)lua_tointeger(L, 8);
				result = drawImageZoom(image, &srcrect, surface, &dstrect);
			}
		}
	}
	else if (lua_isnone(L, 4)) {

		lua_getfield(L, 2, "x");
		lua_getfield(L, 2, "y");
		lua_getfield(L, 2, "width");
		lua_getfield(L, 2, "height");
		if (lua_isnil(L, 6) || lua_isnil(L, 7)) { /* {x, y}, image */
			SDL_Rect dstrect;
			dstrect.x = (int16_t)lua_tointeger(L, 4);
			dstrect.y = (int16_t)lua_tointeger(L, 5);
			image = (SDL_Surface *)lua_touserdata(L, 3);
			if (image == NULL) {
				lua_pushnil(L);
				return 1;
			}
			result = SDL_BlitSurface(image, NULL, surface, &dstrect);
		}
		else { /* {x, ,y ,width, height}, image */
			SDL_Rect dstrect;

			dstrect.x = (int16_t)lua_tointeger(L, 4);
			dstrect.y = (int16_t)lua_tointeger(L, 5);
			dstrect.w = (uint16_t)lua_tointeger(L, 6);
			dstrect.h = (uint16_t)lua_tointeger(L, 7);
			image = (SDL_Surface *)lua_touserdata(L, 3);
			if (image == NULL) {
				lua_pushnil(L);
				return 1;
			}
			result = drawImageZoom(image, NULL, surface, &dstrect);
		}
	}
	else {
		SDL_Rect srcrect;
		lua_getfield(L, 2, "x");
		lua_getfield(L, 2, "y");
		lua_getfield(L, 2, "width");
		lua_getfield(L, 2, "height");
		lua_getfield(L, 4, "x");
		lua_getfield(L, 4, "y");
		lua_getfield(L, 4, "width");
		lua_getfield(L, 4, "height");
		image = (SDL_Surface *)lua_touserdata(L, 3);
		if (image == NULL) {
			lua_pushnil(L);
			return 1;
		}

		srcrect.x = (int16_t)lua_tointeger(L, 9);
		srcrect.y = (int16_t)lua_tointeger(L, 10);
		srcrect.w = (uint16_t)lua_tointeger(L, 11);
		srcrect.h = (uint16_t)lua_tointeger(L, 12);
		if (lua_isnil(L, 7) || lua_isnil(L, 8)) { /* {x, y}, image, {x, y, width, height} */
			SDL_Rect dstrect;
			dstrect.x = (int16_t)lua_tointeger(L, 5);
			dstrect.y = (int16_t)lua_tointeger(L, 6);
			result = SDL_BlitSurface(image, &srcrect, surface, &dstrect);
		}
		else { /* {x, y, width, height}, image, {x, y, width, height} */
			SDL_Rect dstrect;
			dstrect.x = (int16_t)lua_tointeger(L, 5);
			dstrect.y = (int16_t)lua_tointeger(L, 6);
			dstrect.w = (uint16_t)lua_tointeger(L, 7);
			dstrect.h = (uint16_t)lua_tointeger(L, 8);
			result = drawImageZoom(image, &srcrect, surface, &dstrect);
		}
	}

	if (result == 0) {
		lua_pushinteger(L, SP_OK);
	}
	else {
		fprintf(stderr, "[%s] Error : %s\n", __FUNCTION__, SDL_GetError());
		lua_pushinteger(L, SP_FAIL);
	}
	return 1;
}

static int32_t
luaSDL_imageGetSize(
	lua_State *L
)
{
	SDL_Surface *surface = (SDL_Surface *) lua_touserdata(L, 1);
	lua_pushinteger(L, surface->w);
	lua_pushinteger(L, surface->h);
	return 2;
}

static const struct luaL_reg sdl_funcs[] = {
	{"init", luaSDL_init},
	{"quit", luaSDL_quit},
	{"getScreenSize", luaSDL_getScreenSize},
	{"getSurface", luaSDL_getSurface},
	{"flip", luaSDL_flip},
	{"delay",  luaSDL_delay},

	/* event */
	{"getEvent", luaSDL_getEvent},

	/* draw */
	{"drawLine", luaSDL_drawLine},
	{"drawRect", luaSDL_drawRect},
	{"fillRect", luaSDL_fillRect},
	{"drawEllipse", luaSDL_drawEllipse},
	{"fillEllipse", luaSDL_fillEllipse},

	/* font */
	{"loadFont", luaSDL_loadFont},
	{"freeFont", luaSDL_freeFont},
	{"drawText", luaSDL_drawText},
	{"getTextBound", luaSDL_getTextBound},

	/* image */
	{"loadImage", luaSDL_loadImage},
	{"freeImage", luaSDL_freeImage},
	{"drawImage", luaSDL_drawImage},
	{"imageGetSize", luaSDL_imageGetSize},
	{NULL, NULL}
};

static struct {
    char    *name;
    int32_t   value;
} sdl_consts[] = {
	{"SDL_OK", SP_OK},
	{"SDL_FAIL", SP_FAIL},
	{"SDL_INIT_TIMER", SDL_INIT_TIMER},
	{"SDL_INIT_AUDIO", SDL_INIT_AUDIO},
	{"SDL_INIT_VIDEO", SDL_INIT_VIDEO},
	{"SDL_INIT_JOYSTICK", SDL_INIT_JOYSTICK},
	{"SDL_EVENT_WAIT_FOREVER", SDL_EVENT_WAIT_FOREVER},
	{"SDL_EVENT_NO_WAIT", SDL_EVENT_NO_WAIT},
	/* Video Surface Format */
	{"SDL_RGB565", SDL_RGB565},
	{"SDL_RGBA8888", SDL_RGBA8888},
	{NULL, 0}
};

LUALIB_API int
luaopen_sdl(
	lua_State *L
)
{
	int32_t i;

	luaL_register(L, "sdl", sdl_funcs);
	for (i = 0; sdl_consts[i].name != NULL; i++) {
		lua_pushstring(L, sdl_consts[i].name);
		lua_pushinteger(L, sdl_consts[i].value);
		lua_settable(L, -3);
	}

	return 1;
}
