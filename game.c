#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "main.h"
#include "menu.h"
#include "util.h"

int birdy;
unsigned score;
unsigned frame;

struct spike {
  struct spike *next;
  struct spike *prev;
  int x;
  int hole_top;
};
struct spike *spikes, *lastsp;

enum crash { NONE = 0, GROUND, SPIKE };
enum crash crash;

/* 18 bytes:
 *  - "Score: " (7 bytes)
 *  - number (10 bytes enough for any 32-bit int)
 *  - null byte
 */
char score_str[18];
int score_len;
int score_changed;

const int bird_size = 32;
const int bird_half = bird_size/2;
const int birdx = 64;
const int bird_up = 16;
const int spike_move = 4;
const int spike_width = 64;
const int spike_dist = 160;
const int spike_hole = 112;
const int spike_maxheight = WHEIGHT - spike_hole;
const int gravity = 1;
const int max_accel_up = 12;
const int max_accel_down = 16;
const int key_press_frames = 8;

Pixmap bird1, bird2, birdc;

void draw_game(void) {
  struct spike *s;
  if (score_changed) {
    sprintf(score_str, "Score: %u", score);
    score_len = strlen(score_str);
    score_changed = 0;
  }
  XFillRectangle(disp, pix, rgc, 0, 0, WWIDTH, WHEIGHT);
  XCopyPlane(disp, (frame / 15) % 2 ? bird1 : bird2,
            pix, gc, 0, 0, bird_size, bird_size, birdx, birdy, 1);
  for (s = spikes; s; s = s->next) {
    int bottom = s->hole_top + spike_hole;
    XFillRectangle(disp, pix, stipple_gc, s->x, 0, spike_width, s->hole_top);
    XDrawRectangle(disp, pix, gc, s->x, -1, spike_width, s->hole_top);
    XFillRectangle(disp, pix, stipple_gc, s->x, bottom, spike_width,
                   WHEIGHT - bottom);
    XDrawRectangle(disp, pix, gc, s->x, bottom, spike_width,
                   WHEIGHT - bottom);
  }
  XDrawImageString(disp, pix, gc, 16, 16, score_str, score_len);
  XCopyArea(disp, pix, win, gc, 0, 0, WWIDTH, WHEIGHT, 0, 0);
}

int crashed(void) {
  struct spike *s;
  if (birdy > WHEIGHT) return GROUND;

  /* Get left-most spike that isn't to the left of the bird */
  for (s = lastsp; s && s->x + spike_width < birdx; s = s->prev);
  if (s) {
    if (s->x > birdx + bird_size) {
      /* Bird hasn't reached this spike yet */
      return NONE;
    }
    if (birdy < s->hole_top || birdy + bird_size > s->hole_top + spike_hole)
      return SPIKE;
  }
  return NONE;
}

void new_spike(void) {
  struct spike *new = malloc(sizeof(struct spike));
  new->x = WWIDTH;
  new->hole_top = random() % spike_maxheight;
  new->next = spikes;
  if (spikes) spikes->prev = new;
  new->prev = NULL;
  spikes = new;
  if (!lastsp) lastsp = new;
}

void display_score(void) {
  int scwidth = 480;
  int scheight = WHEIGHT / 2;
  int scx = (WWIDTH - scwidth) / 2;
  int scy = (WHEIGHT - scheight) / 2;
  int fh = font_info->ascent + font_info->descent;
  int x = scx + 32, y = scy + (scheight / 2) - 2*fh;
  char buf[32];
  XFillRectangle(disp, win, stipple_gc, 0, 0, WWIDTH, WHEIGHT);
  XFillRectangle(disp, win, rgc, scx, scy, scwidth, scheight);
  XDrawRectangle(disp, win, gc, scx, scy, scwidth, scheight);

  XDrawString(disp, win, gc, x, y, "GAME OVER", 9);
  y += fh;
  sprintf(buf, "You crashed into %s.", (crash == GROUND) ? "the ground"
                                                         : "a spike"   );
  XDrawString(disp, win, gc, x, y, buf, strlen(buf));
  y += 2*fh;
  sprintf(buf, "Your score: %u %s", score,
          score > highscore ? "(NEW HIGHSCORE)" : "");
  XDrawString(disp, win, gc, x, y, buf, strlen(buf));
  y += fh;
  if (score > highscore) {
    highscore = score;
    write_highscore(highscore);
  }
  sprintf(buf, "Highscore:  %u", highscore);
  XDrawString(disp, win, gc, x, y, buf, strlen(buf));
  y += 2*fh;
  XDrawString(disp, win, gc, x, y, "Q to quit, any other key to restart", 35);

  XSync(disp, 1);
}

int game(void) {
  XEvent report;
  struct spike *s;
  frame = 0;
  int key_pressed = 0, accel = 0;
  birdy = WHEIGHT / 2;
  spikes = NULL;
  lastsp = NULL;
  score = 0;
  score_changed = 1;
  while (1) {
    frame++;
    /* Handle events */
    key_pressed = 0;
    while (QLength(disp) > 0) {
      XNextEvent(disp, &report);
      switch (report.type) {
        case KeyPress:
        case ButtonPress:
          key_pressed = key_press_frames;
          birdy -= bird_up;
          break;
      }
    }

    /* Update bird position */
    if (key_pressed) {
      accel -= bird_up;
      if (accel < -max_accel_up) accel = -max_accel_up;
    } else {
      accel += gravity;
      if (accel > max_accel_down) accel = max_accel_down;
    }
    birdy += accel;
    /* Update spikes position */
    for (s = spikes; s; s = s->next) s->x -= spike_move;
    if (lastsp && lastsp->x + spike_width < 0) {
      struct spike *old = lastsp;
      if (lastsp->prev) {
        lastsp->prev->next = NULL;
        lastsp = lastsp->prev;
      } else {
        /* lastsp was the only spike in the spikes list */
        lastsp = NULL;
        spikes = NULL;
      }
      free(old);
      /* Passed a spike, increment the score */
      score++;
      score_changed = 1;
    }
    /* Generate new spikes */
    if (!spikes || spikes->x + spike_width + spike_dist < WWIDTH) {
      new_spike();
    }

    if (crash = crashed()) break;

    draw_game();
    XFlush(disp);
    usleep(DELAY);
  }

  /* Crash animation */
  draw_game();
  /* TODO: transparent instead of opaque crash (XChangeGC?) */
  XCopyPlane(disp, birdc, win, gc, 0, 0, bird_size, bird_size, birdx, birdy, 1);
  XFlush(disp);

  while (spikes) {
    struct spike *old = spikes;
    spikes = spikes->next;
    free(old);
  }
  sleep(1);
  display_score();
  XSync(disp, 1);
  return menu_keys();
}

void init_game_resources(void) {
  char *files[] = { "pix/bird1.xpm", "pix/bird2.xpm", "pix/crash.xpm" };
  Pixmap *pixs[] = { &bird1, &bird2, &birdc };
  int ret, xhot, yhot, w, h;
  int i;

  /* Seed RNG */
  srand(time(NULL));

  /* Load bitmaps */
  for (i = 0; i < 3; i++) {
    ret = XReadBitmapFile(disp, win, files[i],
                          &w, &h, pixs[i], &xhot, &yhot);
    if (ret != BitmapSuccess) {
      fprintf(stderr, "%s: failed to read bitmap %s\n", progname, files[i]);
      exit(1);
    }
    if (w != bird_size || h != bird_size) {
      fprintf(stderr, "%s: expected bitmap of size %dx%d, but got %dx%d\n",
          progname, bird_size, bird_size, w, h);
      exit(1);
    }
  }
}

void free_game_resources(void) {
  XFreePixmap(disp, bird1);
  XFreePixmap(disp, bird2);
  XFreePixmap(disp, birdc);
}
