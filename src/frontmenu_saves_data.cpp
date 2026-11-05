/******************************************************************************/
// Free implementation of Bullfrog's Dungeon Keeper strategy game.
/******************************************************************************/
/** @file frontmenu_saves_data.cpp
 *     GUI menus for saved games support (save and load screens).
 * @par Purpose:
 *     Structures to show and maintain menus used for saving and loading.
 * @par Comment:
 *     None.
 * @author   KeeperFX Team
 * @date     07 Dec 2012 - 11 Feb 2013
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */
/******************************************************************************/
#include "pre_inc.h"
#include "frontmenu_options.h"
#include "globals.h"
#include "bflib_basics.h"

#include "bflib_guibtns.h"
#include "bflib_sprite.h"
#include "bflib_sprfnt.h"
#include "bflib_vidraw.h"

#include "gui_frontbtns.h"
#include "gui_draw.h"
#include "frontend.h"
#include "frontmenu_saves.h"
#include "gui_frontmenu.h"
#include "sprites.h"
#include "gui_vscroll.h"
#include "config_settings.h"
#include "frontmenu_options.h"
#include "game_legacy.h"
#include "post_inc.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************/
struct GuiButtonInit load_menu_buttons[] = {
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0, 999,  10, 999,  10,155, 32, gui_area_text,                     1, GUIStr_MnuLoad,0,       {0},               0, NULL },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               0, 999,  58, 999,  58,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[0]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               1, 999,  90, 999,  90,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[1]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               2, 999, 122, 999, 122,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[2]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               3, 999, 154, 999, 154,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[3]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               4, 999, 186, 999, 186,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[4]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               5, 999, 218, 999, 218,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[5]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               6, 999, 250, 999, 250,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[6]}, 0, gui_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_load_game,      NULL,        NULL,               7, 999, 282, 999, 282,300, 32, draw_load_button,                  1, GUIStr_Empty,  0,{.str = input_string[7]}, 0, gui_load_game_maintain },
  { LbBtnT_HoldableBtn,BID_DEFAULT, 0, 0, gui_vscroll_input,  NULL,        NULL,               0, 368,  58, 368,  58, 33,254, gui_vscroll_draw,                  0, GUIStr_Empty,  0,{0},                      0, gui_vscroll_maintain },
  // Delete buttons, one per on-screen row. They used to sit in the right margin
  // at x=378; upstream's scrollbar (#5106) now occupies 368..401, so they move to
  // the empty left margin instead of overlapping it. They must not carry the
  // clickable flag: that fades the menu out, and deleting a save should leave you
  // looking at the list.
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               0,  34,  59,  34,  59, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               1,  34,  91,  34,  91, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               2,  34, 123,  34, 123, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               3,  34, 155,  34, 155, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               4,  34, 187,  34, 187, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               5,  34, 219,  34, 219, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               6,  34, 251,  34, 251, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,        NULL,               7,  34, 283,  34, 283, 30, 30, draw_delete_save_button,           0, GUIStr_Empty,  0,       {0},               0, gui_delete_save_maintain },
  {-1,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0,   0,   0,   0,   0,  0,  0, NULL,                              0,   0,           0,       {0},               0, NULL },
};

struct GuiButtonInit save_menu_buttons[] = {
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,              NULL, NULL,  0, 999,  10, 999,  10,155, 32, gui_area_text,    1, GUIStr_MnuSave,0,       {0},               0, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  0, 999,  58, 999,  58,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[0]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  1, 999,  90, 999,  90,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[1]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  2, 999, 122, 999, 122,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[2]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  3, 999, 154, 999, 154,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[3]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  4, 999, 186, 999, 186,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[4]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  5, 999, 218, 999, 218,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[5]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  6, 999, 250, 999, 250,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[6]},SAVE_TEXTNAME_LEN, NULL },
  { 5,                 -2,         -1, 1, gui_save_game,     NULL, NULL,  7, 999, 282, 999, 282,300, 32, gui_area_text,    1, GUIStr_Empty,  0,{.str = input_string[7]},SAVE_TEXTNAME_LEN, NULL },
  { LbBtnT_HoldableBtn,BID_DEFAULT, 0, 0, gui_vscroll_input, NULL, NULL,  0, 368,  58, 368,  58, 33,254, gui_vscroll_draw, 0, GUIStr_Empty,  0,{0},                     0,                 gui_vscroll_maintain },
  // Delete buttons, one per on-screen row, in the left margin (the right one is
  // the scrollbar's now). Overwriting a slot has always been possible; this is
  // for freeing one you no longer recognise.
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  0,  34,  59,  34,  59, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  1,  34,  91,  34,  91, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  2,  34, 123,  34, 123, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  3,  34, 155,  34, 155, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  4,  34, 187,  34, 187, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  5,  34, 219,  34, 219, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  6,  34, 251,  34, 251, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, gui_ask_delete_save,NULL,NULL,  7,  34, 283,  34, 283, 30, 30, draw_delete_save_button, 0, GUIStr_Empty, 0,{0},           0,       gui_delete_save_maintain },
  {-1,                 0,           0, 0, NULL,              NULL, NULL,  0,   0,   0,   0,   0,  0,  0, NULL,             0, 0,             0,{0},                     0,                 NULL },
};

/** Confirmation for deleting a saved game. Modelled on the quit confirmation:
 *  same yes/no buttons, so it reads as part of the stock UI, but its own header
 *  wording (gui_delete_confirm_header_maintain, from the SAVE_TRASH_CONFIRM
 *  translation.toml alias) rather than the shared GUIStr_ConfirmYouSure --
 *  deletion here is recoverable, so the dialog no longer says "delete". The
 *  extra line naming the save is not decoration - the dialog covers the list,
 *  and the whole point of the feature is clearing out saves you can no longer
 *  tell apart. */
struct GuiButtonInit delete_save_menu_buttons[] = {
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0, 999,  10, 999,  10,240, 32, gui_area_text,                     1, GUIStr_ConfirmYouSure, 0, {0},            0, gui_delete_confirm_header_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0, 999,  48, 999,  48,300, 32, gui_area_text,                     0, GUIStr_Empty,          0, {.str = delete_save_name}, 0, NULL },
  // The yes/no sprites are drawn from scr_pos and are far taller than the click
  // area, so scr_pos sits above pos to centre the glyph on it - same offsets the
  // stock quit confirmation uses.
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_delete_save_cancelled,NULL,  NULL,               0, 103,  80, 105, 114, 46, 32, gui_area_normal_button, GBS_options_button_smd_no, GUIStr_ConfirmNo,      0, {0},            0, NULL },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 1, gui_delete_save_confirmed,NULL,  NULL,               0, 169,  80, 171, 114, 46, 32, gui_area_normal_button, GBS_options_button_smd_yes,GUIStr_ConfirmYes,     0, {0},            0, NULL },
  {-1,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0,   0,   0,   0,   0,  0,  0, NULL,                              0,   0,           0,       {0},               0, NULL },
};

struct GuiButtonInit frontend_load_menu_buttons[] = {
  { LbBtnT_NormalBtn,  BID_MENU_TITLE, 0, 0, NULL,               NULL,        NULL,               0, 999,  30, 999,  30,371, 46, frontend_draw_large_menu_button,   0, GUIStr_Empty,  0,       {7},            0, NULL },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0,  82, 124,  82, 124,220, 26, frontend_draw_scroll_box_tab,      0, GUIStr_Empty,  0,      {28},            0, NULL },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0,  82, 150,  82, 150,450,182, frontend_draw_scroll_box,          0, GUIStr_Empty,  0,      {26},            0, NULL },
  { LbBtnT_HoldableBtn,BID_DEFAULT, 0, 0, frontend_load_game_up,NULL,frontend_over_button,     0, 532, 149, 532, 149, 26, 14, frontend_draw_slider_button,       0, GUIStr_Empty,  0,      {17},            0, frontend_load_game_up_maintain },
  { LbBtnT_HoldableBtn,BID_DEFAULT, 0, 0, frontend_load_game_down,NULL,frontend_over_button,   0, 532, 317, 532, 317, 26, 14, frontend_draw_slider_button,       0, GUIStr_Empty,  0,      {18},            0, frontend_load_game_down_maintain },
  { LbBtnT_DraggableBtn,BID_DEFAULT, 0, 0, frontend_load_game_scroll,NULL,  NULL,               0, 536, 163, 534, 163, 20,154, frontend_draw_games_scroll_tab,    0, GUIStr_Empty,  0,      {40},            0, NULL },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, NULL,               NULL,        NULL,               0, 102, 125, 102, 125,220, 26, frontend_draw_text,                0, GUIStr_Empty,  0,      {30},            0, NULL },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_load_game,NULL,frontend_over_button,        0,  95, 157,  95, 157,424, 22, frontend_draw_load_game_button,    0, GUIStr_Empty,  0,      {45},            0, frontend_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_load_game,NULL,frontend_over_button,        0,  95, 185,  95, 185,424, 22, frontend_draw_load_game_button,    0, GUIStr_Empty,  0,      {46},            0, frontend_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_load_game,NULL,frontend_over_button,        0,  95, 213,  95, 213,424, 22, frontend_draw_load_game_button,    0, GUIStr_Empty,  0,      {47},            0, frontend_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_load_game,NULL,frontend_over_button,        0,  95, 241,  95, 241,424, 22, frontend_draw_load_game_button,    0, GUIStr_Empty,  0,      {48},            0, frontend_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_load_game,NULL,frontend_over_button,        0,  95, 269,  95, 269,424, 22, frontend_draw_load_game_button,    0, GUIStr_Empty,  0,      {49},            0, frontend_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_load_game,NULL,frontend_over_button,        0,  95, 297,  95, 297,424, 22, frontend_draw_load_game_button,    0, GUIStr_Empty,  0,      {50},            0, frontend_load_game_maintain },
  { LbBtnT_NormalBtn,  BID_DEFAULT, 0, 0, frontend_change_state,NULL,frontend_over_button,     1, 999, 404, 999, 404,371, 46, frontend_draw_large_menu_button,   0, GUIStr_Empty,  0,       {6},            0, NULL },
  {-1,  0, 0, 0, NULL,               NULL,        NULL,               0,   0,   0,   0,   0,  0,  0, NULL,                              0,   0,           0,       {0},            0, NULL },
};

struct GuiMenu load_menu =
 {   GMnu_LOAD, 0, 4, load_menu_buttons,          POS_GAMECTR,POS_GAMECTR, 436, 350, gui_pretty_background, 0, NULL,    init_load_menu,          0, 1, 0,};
struct GuiMenu save_menu =
 {   GMnu_SAVE, 0, 4, save_menu_buttons,          POS_GAMECTR,POS_GAMECTR, 436, 350, gui_pretty_background, 0, NULL,    init_save_menu,          0, 1, 0,};
struct GuiMenu delete_save_menu =
 {GMnu_DELETE_SAVE, 0, 1, delete_save_menu_buttons,  POS_GAMECTR,POS_GAMECTR, 330, 172, gui_pretty_background,       0, NULL,    NULL,                    0, 1, 0,};
struct GuiMenu frontend_load_menu =
 { GMnu_FELOAD, 0, 1, frontend_load_menu_buttons,  POS_SCRCTR, POS_SCRCTR, 640, 480, NULL,                  0, NULL,    NULL,                    0, 0, 0,};

/******************************************************************************/
#ifdef __cplusplus
}
#endif
/******************************************************************************/
