/*
 * Garden watch app for the Pebble watch.
 * Each second tick causes new pseudo-random growth in the garden.
 *
 * Kiri Wagstaff, 5/17/2013
 */

#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID { 0xAB, 0x9D, 0xC7, 0x3B, 0xF5, 0x6F, 0x49, 0xCA, 0x85, 0x8B, 0x45, 0x47, 0xCC, 0xB1, 0xF2, 0x0B }
PBL_APP_INFO(MY_UUID,
             "Garden", "Kiri Wagstaff",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

/* The display is 144 x 168 pixels max */
#define MAX_X 143
#define MAX_Y 167

/* Allow up to this many shoots */
#define MAX_NUM_SHOOTS 1
/* For each shoot, track the list of its x,y coords as GPoints */
#define MAX_POINTS 60
typedef struct {
  GPoint pt[MAX_POINTS];
  int    npts;
} Shoot;
Shoot shoots[MAX_NUM_SHOOTS];

Window     window;
TextLayer  time_layer;
Layer      garden;
static int num_shoots = 0;

/* 
 * Generate a random number from 0 to max-1, 
 * using the method suggested by user FlashBIOS here:
 * http://forums.getpebble.com/discussion/4456/random-number-generation
 */
int random(int max)
{
  /* Seed it with the same number for repeatability */
  static long seed = 100;
  /* Or, seed it with some piece of the startup time,
   * so we get different gardens each time. */

  /* Generate a new number, which will become the next seed */
  seed = (((seed * 214013L + 2531011L) >> 16) & 32767);

  return (seed % max);
}


/* 
 * Draw the current garden state.
 */
void display_garden_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  int s, i;

  /* Set color to black and draw all shoots */
  graphics_context_set_stroke_color(ctx, GColorBlack);

  for (s=0; s<num_shoots; s++) {
    for (i=1; i<shoots[s].npts; i++)
      graphics_draw_line(ctx, shoots[s].pt[i-1], shoots[s].pt[i]);
  }

}


/*
 * Update the shoots in the garden on each tick
 * by adding some random growth to each one.
 */
void handle_tick(AppContextRef ctx, PebbleTickEvent *event) {
  (void)event;
  (void)ctx;

  /* Select a shoot at random */
  const int s = 0;

  /* Grow it by a random amount (1 to 5 pixels),
   * in a random direction (-45 to 45 degrees) */
  int curpt = shoots[s].npts;

  /* If the minute is over, reset all shoots to the ground */
  if (curpt >= MAX_POINTS) {
    shoots[s].pt[0].x = 100;  /* make this random */
    shoots[s].pt[0].y = MAX_Y-1;
    shoots[s].npts    = 1;
    curpt = 1;
  }
  /* Assumes curpt is at least 1 */
  /* make this random */
  int grow_by = random(11) - 5;
  shoots[s].pt[curpt].x = shoots[s].pt[curpt-1].x + grow_by;
  grow_by = random(3) + 1;
  shoots[s].pt[curpt].y = shoots[s].pt[curpt-1].y - grow_by;
  shoots[s].npts++; 

  /* Mark the garden layer dirty so it will update */
  layer_mark_dirty(&garden);
}


/* 
 * Initialize the app and the garden data structures.
 */
void handle_init(AppContextRef ctx) {
  (void)ctx;

  window_init(&window, "Garden");
  window_stack_push(&window, true /* Animated */);

  /* Create the garden layer */
  layer_init(&garden, window.layer.frame);
  garden.update_proc = &display_garden_update_callback;
  layer_add_child(&window.layer, &garden);

  /* Create a text layer to report the time */

  /* By adding it second, it will be drawn after the base garden layer */

  num_shoots = 1;
  /* Initialize the shoot starting locations to the bottom of the screen,
     in randomly chosen x locations */
  shoots[0].pt[0].x = 75;
  shoots[0].pt[0].y = MAX_Y-1;

  shoots[0].npts    = 1;

}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .tick_info = {
      .tick_handler = &handle_tick,
      .tick_units   = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
