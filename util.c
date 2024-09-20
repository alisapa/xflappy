#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *progname;
char *fpath = NULL;

void die(char *msg) {
  fprintf(stderr, "%s: %s\n", progname, msg);
  exit(1);
}

/* When we say "press any key to continue, that does not include the
 * modifier keys. This is because in some tiling window managers,
 * changing the position of the window requires pressing modifier keys.
 * We don't want to start the game when the user was only intending to resize
 * the window. */
/* Also note that this can't be done with report.xkey.state, because when the
 * modifier key itself is pressed (e.g. Ctrl), the resulting KeyPress event
 * still has state == 0. Only subsequent keypresses while the modifier is being
 * held down will have state != 0. */
int is_modifier(KeySym k) {
  /* TODO: is there any better way to do this? */
  return   k == XK_Control_L || k == XK_Control_R
        || k == XK_Meta_L    || k == XK_Meta_R
        || k == XK_Alt_L     || k == XK_Alt_R
        || k == XK_Super_L   || k == XK_Super_R
        || k == XK_Hyper_L   || k == XK_Hyper_R
        || k == XK_Caps_Lock || k == XK_Shift_Lock;
}

char *filepath(void) {
  const char *fname = ".xflappy-highscore";
  char *home = getenv("HOME"), *str;
  if (!home) {
    fprintf(stderr, "%s: failed to read $HOME environment variable\n",
            progname);
    return NULL;
  }
  str = malloc(strlen(home) + strlen(fname) + 2);
  if (!str) return NULL;
  sprintf(str, "%s/%s", home, fname);
  return str;
}

unsigned read_highscore(void) {
  FILE *fp;
  char buf[11];
  if (!fpath) goto failed;
  fp = fopen(fpath, "r");
  if (!fp) goto failed;
  if (!fgets(buf, 11, fp)) goto failed;
  fclose(fp);
  return atoi(buf);
failed:
  fprintf(stderr, "%s: failed to read highscore file\n", progname);
  return 0;
}

void write_highscore(unsigned high) {
  FILE *fp;
  if (!fpath) goto failed;
  fp = fopen(fpath, "w");
  if (!fp) goto failed;
  fprintf(fp, "%u", high);
  fclose(fp);
  return;
failed:
  fprintf(stderr, "%s: failed to write highscore file\n", progname);
}
