#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "main.h"
#include "util.h"

void draw_menu(void);
char *logo_str[] = {
  "   _  __ ________                       ",
  "  | |/ // ____/ /___ _____  ____  __  __",
  "  |   // /_  / / __ `/ __ \\/ __ \\/ / / /",
  " /   |/ __/ / / /_/ / /_/ / /_/ / /_/ / ",
  "/_/|_/_/   /_/\\____/ .___/ .___/\\__, /  ",
  "                  /_/   /_/    /____/   ",
  "                                        ",
  "                                        ",
  "Programmed by Alisa, 2024               ",
  "                                        ",
  "                                        ",
  "Press any key to start                  ",
  "Press Q to exit                         ",
};
int logo_num = 13;
int logo_width = 40;

int menu_keys(void) {
  XEvent report;
  KeySym key;
  while (1) {
    XNextEvent(disp, &report);
    switch (report.type) {
      case Expose:
        /* Only draw if last contiguous expose */
        if (report.xexpose.count != 0) break;
        draw_menu();
        break;
      case KeyPress:
        key = XLookupKeysym(&report.xkey, 0);
        if (key == NoSymbol || is_modifier(key)) break;
        if (key == XK_Escape || key == XK_q) return 1;
        return 0;
      case ButtonPress:
        if (report.xbutton.state) break;
        return 0;
      default:
        break;
    }
  }
}

int get_centered(const char *str, int len) {
  return (WWIDTH - XTextWidth(font_info, str, len)) / 2;
}

void draw_menu(void) {
  int font_height = font_info->ascent + font_info->descent;
  int logox, y, len, i;
  char *buf;

  y = (WHEIGHT - (logo_num+1)*font_height) / 2;

  /* Nice, stippled background */
  XFillRectangle(disp, win, stipplem_gc, 0, 0, WWIDTH, WHEIGHT);

  /* Logo and information */
  /* This x position calculation assumes monospace font */
  logox = get_centered(logo_str[0], logo_width);
  for (i = 0; i < logo_num; i++) {
    XDrawImageString(disp, win, gc, logox, y, logo_str[i], logo_width);
    y += font_height;
  }

  /* Current highscore */
  buf = malloc(logo_width + 1);
  sprintf(buf, "Highscore: %u", highscore);
  len = strlen(buf);
  memset(buf + len, ' ', logo_width - len);
  XDrawImageString(disp, win, gc, logox, y, buf, logo_width);
  free(buf);
}
