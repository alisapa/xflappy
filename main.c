#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "game.h"
#include "util.h"
#include "pix/stipple.xpm"
#include "pix/stipplem.xpm"

char *progname;
Display *disp;
Screen  *screen_ptr;
int      screen_num;
Window  win;
Pixmap  pix;
GC      gc, rgc, stipple_gc, stipplem_gc;
XFontStruct *font_info;
Font    font;
XWindowAttributes wattrs;
int highscore = 0;

extern char *fpath;

Window open_window(char*,int,int,int,char**);
void init_font(char*);
void init_gc(void);
void init_stipple_gcs(void);
void init_pixmap(void);

char *merr = "memory allocation error";

int main(int argc, char **argv) {
  progname = argv[0];

  disp = XOpenDisplay(NULL);
  if (!disp) die("failed to open display");
  screen_num = DefaultScreen(disp);
  screen_ptr = DefaultScreenOfDisplay(disp);

  win = open_window("XFlappy", 640, 480, argc, argv);

  XSelectInput(disp, win, ExposureMask | KeyPressMask | ButtonPressMask
                          | StructureNotifyMask);

  XGetWindowAttributes(disp, win, &wattrs);
  init_font("9x15");
  init_gc();
  init_stipple_gcs();
  init_pixmap();
  init_game_resources();
  fpath = filepath();
  highscore = read_highscore();
  XMapWindow(disp, win);

  draw_menu();
  if (menu_keys()) goto quit;
  while (game() == 0);
quit:
  free(fpath);
  XDestroyWindow(disp, win);
  XUnloadFont(disp, font);
  XFreeGC(disp, gc);
  XFreeGC(disp, rgc);
  XFreeGC(disp, stipple_gc);
  XFreeGC(disp, stipplem_gc);
  XFreePixmap(disp, pix);
  free_game_resources();
  XCloseDisplay(disp);
  return 0;
}

Window open_window(char *name, int width, int height, int argc, char **argv) {
  int x, y;
  int border_width;
  Window w;

  XWMHints *wm_hints;
  XClassHint *class_hints;
  XSizeHints *size_hints;
  XTextProperty windowName, iconName;
  int ret;

  x = y = 0;
  border_width = 0;
  w = XCreateSimpleWindow(disp, RootWindow(disp, screen_num), x, y,
                          width, height, border_width,
                          BlackPixel(disp, screen_num),
                          WhitePixel(disp, screen_num));
  
  /* TODO: icon pixmap */

  size_hints = XAllocSizeHints();
  wm_hints = XAllocWMHints();
  class_hints = XAllocClassHint();
  if (!size_hints || !wm_hints || !class_hints) die(merr);

  size_hints->flags = PPosition | USSize;
  size_hints->width = width;
  size_hints->height = height;

  ret = XStringListToTextProperty(&name, 1, &windowName);
  if (ret == 0) die(merr);

  wm_hints->initial_state = NormalState;
  wm_hints->input = True;
  wm_hints->flags = StateHint | InputHint;
  class_hints->res_name = progname;
  class_hints->res_class = "XFlappy";

  XSetWMProperties(disp, w, &windowName, NULL, argv, argc, size_hints,
                   wm_hints, class_hints);

  /* TODO: Is it correct to free the allocated hints here?
   *       Valgrind does not complain, but what does the documentation say? */
  XFree(windowName.value);
  XFree(size_hints);
  XFree(wm_hints);
  XFree(class_hints);
  return w;
}

void init_font(char *name) {
  font_info = XLoadQueryFont(disp, name);
  if (!font_info) {
    fprintf(stderr, "%s: failed to load font %s\n", progname, name);
    exit(1);
  }
  font = font_info->fid;
}

void init_gc(void) {
  XGCValues values;
  unsigned long valuemask;
  values.foreground = BlackPixel(disp, screen_num);
  values.background = WhitePixel(disp, screen_num);
  values.line_width = 1;
  values.line_style = LineSolid;
  values.fill_style = FillSolid;
  values.font       = font;
  valuemask = GCForeground | GCBackground | GCLineWidth | GCLineStyle
              | GCFillStyle | GCFont;
  gc = XCreateGC(disp, win, valuemask, &values);
  if (!gc) die("failed to create GC");

  values.foreground = WhitePixel(disp, screen_num);
  values.background = BlackPixel(disp, screen_num);
  rgc = XCreateGC(disp, win, valuemask, &values);
}

void init_stipple_gcs(void) {
  Pixmap stipple, stipplem;
  XGCValues values;
  unsigned long valuemask;

  stipple = XCreateBitmapFromData(disp, RootWindow(disp, screen_num),
                                  stipple_bits, stipple_width, stipple_height);
  if (!stipple) die("Failed to create stipple");

  values.foreground = BlackPixel(disp, screen_num);
  values.background = WhitePixel(disp, screen_num);
  values.line_width = 1;
  values.line_style = LineSolid;
  values.fill_style = FillOpaqueStippled;
  values.font       = font;
  values.stipple    = stipple;
  valuemask = GCForeground | GCBackground | GCLineWidth | GCLineStyle
              | GCFillStyle | GCFont | GCStipple;
  stipple_gc = XCreateGC(disp, win, valuemask, &values);
  if (!stipple_gc) die("failed to create GC");

  stipplem = XCreateBitmapFromData(disp, RootWindow(disp, screen_num),
                                   stipplem_bits, stipplem_width,
                                   stipplem_height);
  values.stipple    = stipplem;
  stipplem_gc = XCreateGC(disp, win, valuemask, &values);
  if (!stipplem_gc) die("failed to create GC");
}

/* Game draws to an off-screen pixmap first, and then copies the result to
 * the window. */
void init_pixmap(void) {
  pix = XCreatePixmap(disp, win, WWIDTH, WHEIGHT, wattrs.depth);
}
