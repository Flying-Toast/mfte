#ifndef __HAVE_STYLE_H
#define __HAVE_STYLE_H

#include "render.h"

#define BG_COLOR 0x282c34
#define WHITE_COLOR 0xabb2bf
#define GREEN_COLOR 0x98c379
#define BLUE_COLOR 0x61afef
#define RED_COLOR 0xe06c75
#define YELLOW_COLOR 0xe5c07b
#define PURPLE_COLOR 0xc678dd

#define LIGHTBG_COLOR 0x2c323c
#define LIGHTERBG_COLOR 0x3e4452
#define GUTTER_COLOR 0x4b5263

#define GUTTER_STYLE ((struct style) { .fg = GUTTER_COLOR, .bg = BG_COLOR })
#define NORMAL_STYLE ((struct style) { .fg = WHITE_COLOR, .bg = BG_COLOR })
#define STATUSLINE_NORMAL_MODE_STYLE ((struct style) { .fg = BG_COLOR, .bg = GREEN_COLOR })
#define STATUSLINE_COMMAND_MODE_STYLE ((struct style) { .fg = BG_COLOR, .bg = GREEN_COLOR })
#define STATUSLINE_INSERT_MODE_STYLE ((struct style) { .fg = BG_COLOR, .bg = BLUE_COLOR })
#define STATUSLINE_SECONDARY_STYLE ((struct style) { .fg = WHITE_COLOR, .bg = LIGHTERBG_COLOR, })
#define ERRORMSG_STYLE ((struct style) { .fg = RED_COLOR, .bg = BG_COLOR })

#endif
