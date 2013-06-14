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
#define MAX_NUM_SHOOTS 5
/* For each shoot, track the list of its x,y coords as GPoints */
#define MAX_POINTS 60
/* Max allowed width of a shoot */
#define MAX_WIDTH 16
/* Number of growth units per tick (second) */
#define NUM_GROW_PER_TICK 6

typedef struct {
  GPoint pt[MAX_POINTS]; /* spine of the shoot */
  int    npts;
} Shoot;
Shoot shoots[MAX_NUM_SHOOTS];

Window     window;
TextLayer  time_layer;
Layer      garden;
static int num_shoots = 0;

/* We only need one of these at a time */
static GPoint shoot_path_points[MAX_POINTS*2];

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
 * (Re-)start all shoots at ground level (y = MAX_Y-1) 
 * at random x locations.
 */
void init_shoots()
{
  int s = 0;

  /*
  for (s=0; s<num_shoots; s++) {
    shoots[s].pt[0].x = random(MAX_X);
    shoots[s].pt[0].y = MAX_Y-1;
    shoots[s].npts    = 1;
  }
  */
  /* Only set up the first shoot */
  /* Ensure that the x coordinate isn't too close to an edge */
  shoots[s].pt[0].x = random(MAX_X - MAX_WIDTH*2) + MAX_WIDTH;
  shoots[s].pt[0].y = MAX_Y-1;
  shoots[s].npts    = 1;

  num_shoots = 1;

  /* Mark the garden layer dirty so it will update */
  layer_mark_dirty(&garden);

}


/* 
 * Draw the current garden state.
 */
void display_garden_update_callback(Layer *me, GContext* ctx) {
  (void)me;
  int s, i, npts;
  GPath     shoot;
  GPathInfo shoot_info;
  const int half_width = MAX_WIDTH/2;

  /* Set color to black and draw all shoots */
  graphics_context_set_stroke_color(ctx, GColorBlack);

  for (s=0; s<num_shoots; s++) {
    /* Create the path, using the spine and expanding it to left and right */
    npts = shoots[s].npts;
    for (i=0; i<npts; i++) { /* going up */
      /* Scale the width by the height of the shoot */
      int shoot_width = half_width - (i*half_width*1.0)/npts + 2;
      shoot_path_points[i] = shoots[s].pt[i];
      shoot_path_points[i].x -= shoot_width;
    }
    for (i=0; i<npts; i++) { /* going down */
      /* Scale the width by the height of the shoot */
      int shoot_width = half_width - ((npts-1-i)*half_width*1.0)/npts + 2;
      shoot_path_points[npts + i] = shoots[s].pt[npts-1-i];
      shoot_path_points[npts + i].x += shoot_width;
    }
    shoot_info.num_points = npts * 2; /* up and back down */
    shoot_info.points = shoot_path_points;

    /* Set it up */
    gpath_init(&shoot, &shoot_info);

    /* Draw the shoot */
    graphics_context_set_fill_color(ctx, GColorBlack);
    gpath_draw_filled(ctx, &shoot);

  }

}


/*
 * Update the shoots in the garden on each tick
 * by adding some random growth to each one.
 */
void handle_tick(AppContextRef ctx, PebbleTickEvent *event) {
  (void)event;
  (void)ctx;
  int i;

  /*********** Update the garden *************/

  /* If the minute is over, reset all shoots to the ground */
  if (event->units_changed & MINUTE_UNIT) {
    init_shoots();
  }

  /* Grow multiple units per tick */
  for (i=0; i<NUM_GROW_PER_TICK; i++) {
    /* Select a shoot at random */
    int s = random(MAX_NUM_SHOOTS);
    /* If this shoot doesn't exist yet, create it as a branch from shoot 0 */
    if (s >= num_shoots) {
      s = num_shoots; /* Make it the next shoot */

      int branch_pt = random(shoots[0].npts);
      shoots[s].pt[0].x = shoots[0].pt[branch_pt].x;
      shoots[s].pt[0].y = shoots[0].pt[branch_pt].y;
      shoots[s].npts    = 1;

      num_shoots++;
    }

    /* Grow it by a random amount (1 to 5 pixels),
     * in a random direction (-45 to 45 degrees) */
    int curpt = shoots[s].npts;
    /* This shouldn't be needed if the above MINUTE_UNIT checked worked;
     * but add it in here for safety. */
    if (curpt >= MAX_POINTS) {
      curpt = MAX_POINTS-1;
      shoots[s].npts--;  /* compensate for later ++ */
    }

    /* Assumes curpt is at least 1 */
    /* make this random */
    int grow_by = random(11) - 5; /* -5 to +5 */
    int new_x = shoots[s].pt[curpt-1].x + grow_by;
    if      (new_x < 0)     new_x = 0;
    else if (new_x > MAX_X) new_x = MAX_X;
    shoots[s].pt[curpt].x = new_x;

    grow_by = random(3) + 1; /* 1 to 3 */
    int new_y = shoots[s].pt[curpt-1].y - grow_by;
    if (new_y < 0) new_y = 0;
    shoots[s].pt[curpt].y = new_y;
    shoots[s].npts++; 
  }

  /* Mark the garden layer dirty so it will update */
  layer_mark_dirty(&garden);


  /*********** Update the time *************/
  static char timeText[] = "00:00:00"; 

  PblTm currentTime;

  get_time(&currentTime);
  string_format_time(timeText, sizeof(timeText), "%T", &currentTime);
  text_layer_set_text(&time_layer, timeText);

  layer_mark_dirty(&time_layer.layer);

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
  text_layer_init(&time_layer, window.layer.frame);
  text_layer_set_text_color(      &time_layer, GColorBlack);
  text_layer_set_background_color(&time_layer, GColorClear);
  text_layer_set_text_alignment(&time_layer, GTextAlignmentCenter);
  text_layer_set_font(&time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  /* By adding it second, it will be drawn after the base garden layer */
  layer_add_child(&window.layer, &time_layer.layer);

  /* Initialize the first shoot's starting location to the bottom of the screen,
     in a randomly chosen x location */
  init_shoots();

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
