/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* *                                                                 * */
/* *                           DblTime2                              * */
/* *                                                                 * */
/* *             A watchface that dislays two timezones              * */
/* *                                                                 * */
/* *                 [ SDK 2.0 compatible version ]                  * */
/* *                                                                 * */
/* *                    by Mark J Culross, KD5RXT                    * */
/* *                                                                 * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include <pebble.h>

// This is a custom defined key for saving the snooze_enabled flag
#define PKEY_SNOOZE_ENABLED 21453
#define SNOOZE_ENABLED_DEFAULT true

// This is a custom defined key for saving the chime_enabled flag
#define PKEY_CHIME_ENABLED 32145
#define CHIME_ENABLED_DEFAULT false

// This is a custom defined key for saving the show_seconds flag
#define PKEY_SHOW_SECONDS 53214
#define SHOW_SECONDS_DEFAULT true

// This is a custom defined key for saving the month_before_day flag
#define PKEY_MONTH_BEFORE_DAY 45321
#define MONTH_BEFORE_DAY_DEFAULT true

// This is a custom defined key for saving the time_offset value
#define PKEY_TIME_OFFSET 14532
#define TIME_OFFSET_DEFAULT 0

Window *window;
Layer *window_layer;


#define TOTAL_DATE_DIGITS 8
#define TOTAL_TIME_DIGITS 8
#define TOTAL_TZ_IMAGES 5
#define SNOOZE_SECONDS 15
#define SETMODE_SECONDS 20

typedef enum {APP_IDLE_STATE = 0, APP_SNOOZE_STATE, APP_CHIME_STATE, APP_MD_STATE, APP_SECS_STATE, APP_OFFSET_STATE, STATE_COUNT} APP_STATE;

static struct tm previous_time;
static struct tm previous_time2;
static struct tm current_time2;

static bool chime_enabled = CHIME_ENABLED_DEFAULT;
static bool snooze_enabled = SNOOZE_ENABLED_DEFAULT;
static bool show_seconds = SHOW_SECONDS_DEFAULT;
static bool month_before_day = MONTH_BEFORE_DAY_DEFAULT;
static bool toggle_flag = false;
static bool refresh_display = true;
static bool light_on = false;

static int snooze_timer = SNOOZE_SECONDS;
static int setmode_timer = SETMODE_SECONDS;
static int time_offset = TIME_OFFSET_DEFAULT;

static int app_state = APP_IDLE_STATE;

static int splash_timer = 5;
static int light_timer = 0;

static GBitmap *splash_image;
static GBitmap *time_image[TOTAL_TIME_DIGITS];
static GBitmap *time2_image[TOTAL_TIME_DIGITS];
static GBitmap *date_image[TOTAL_DATE_DIGITS];
static GBitmap *date2_image[TOTAL_DATE_DIGITS];
static GBitmap *tz_image[TOTAL_TZ_IMAGES];
static GBitmap *am_pm_image;
static GBitmap *am_pm2_image;
static GBitmap *day_image;
static GBitmap *day2_image;
static GBitmap *chime_image;
static GBitmap *snooze_image;
static GBitmap *md_image;
static GBitmap *secs_image;

static BitmapLayer *time_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] =
{
   RESOURCE_ID_IMAGE_DAY_NAME_SUN,
   RESOURCE_ID_IMAGE_DAY_NAME_MON,
   RESOURCE_ID_IMAGE_DAY_NAME_TUE,
   RESOURCE_ID_IMAGE_DAY_NAME_WED,
   RESOURCE_ID_IMAGE_DAY_NAME_THU,
   RESOURCE_ID_IMAGE_DAY_NAME_FRI,
   RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

const int DATENUM_IMAGE_RESOURCE_IDS[] =
{
   RESOURCE_ID_IMAGE_DATENUM_0,
   RESOURCE_ID_IMAGE_DATENUM_1,
   RESOURCE_ID_IMAGE_DATENUM_2,
   RESOURCE_ID_IMAGE_DATENUM_3,
   RESOURCE_ID_IMAGE_DATENUM_4,
   RESOURCE_ID_IMAGE_DATENUM_5,
   RESOURCE_ID_IMAGE_DATENUM_6,
   RESOURCE_ID_IMAGE_DATENUM_7,
   RESOURCE_ID_IMAGE_DATENUM_8,
   RESOURCE_ID_IMAGE_DATENUM_9,
};

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] =
{
   RESOURCE_ID_IMAGE_NUM_0,
   RESOURCE_ID_IMAGE_NUM_1,
   RESOURCE_ID_IMAGE_NUM_2,
   RESOURCE_ID_IMAGE_NUM_3,
   RESOURCE_ID_IMAGE_NUM_4,
   RESOURCE_ID_IMAGE_NUM_5,
   RESOURCE_ID_IMAGE_NUM_6,
   RESOURCE_ID_IMAGE_NUM_7,
   RESOURCE_ID_IMAGE_NUM_8,
   RESOURCE_ID_IMAGE_NUM_9,
};



static void click_config_provider(void *context);
static void deinit(void);
static void display_chime(GContext *ctx, bool is_night);
static void display_md(GContext *ctx, bool is_night);
static void display_offset(GContext *ctx, bool is_night);
static void display_secs(GContext *ctx, bool is_night);
static void display_snooze(GContext *ctx, bool is_night);
static void down_single_click_handler(ClickRecognizerRef recognizer, void *context);
static void handle_accel_tap(AccelAxisType axis, int32_t direction);
static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);
static void init(void);
static void select_long_click_handler(ClickRecognizerRef recognizer, void *context);
static void select_long_release_handler(ClickRecognizerRef recognizer, void *context);
static void select_single_click_handler(ClickRecognizerRef recognizer, void *context);
static void set_bitmap_image(GContext *ctx, GBitmap **bmp_image, const int resource_id, GPoint this_origin, bool invert);
static void toggle_chime(void);
static void toggle_md(void);
static void toggle_snooze(void);
static void toggle_secs(void);
static void up_single_click_handler(ClickRecognizerRef recognizer, void *context);
static void update_display(Layer *layer, GContext *ctx);


static void click_config_provider(void *context)
{
   window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
   window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
   window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
   window_long_click_subscribe(BUTTON_ID_SELECT, 250, select_long_click_handler, select_long_release_handler);
}  // click_config_provider()


static void deinit(void)
{
   // Save all settings into persistent storage on app exit
   persist_write_int(PKEY_CHIME_ENABLED, chime_enabled);
   persist_write_int(PKEY_SNOOZE_ENABLED, snooze_enabled);
   persist_write_int(PKEY_SHOW_SECONDS, show_seconds);
   persist_write_int(PKEY_MONTH_BEFORE_DAY, month_before_day);
   persist_write_int(PKEY_TIME_OFFSET, time_offset);

   layer_remove_from_parent(bitmap_layer_get_layer(time_layer));
   bitmap_layer_destroy(time_layer);

   gbitmap_destroy(secs_image);
   gbitmap_destroy(md_image);
   gbitmap_destroy(snooze_image);
   gbitmap_destroy(chime_image);
   gbitmap_destroy(day2_image);
   gbitmap_destroy(day_image);
   gbitmap_destroy(am_pm2_image);
   gbitmap_destroy(am_pm_image);

   for (int i = 0; i < TOTAL_TZ_IMAGES; i++)
   {
      gbitmap_destroy(tz_image[i]);
   }

   for (int i = 0; i < TOTAL_DATE_DIGITS; i++)
   {
      gbitmap_destroy(date2_image[i]);
      gbitmap_destroy(date_image[i]);
   }

   for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
   {
      gbitmap_destroy(time2_image[i]);
      gbitmap_destroy(time_image[i]);
   }

   gbitmap_destroy(splash_image);
   bitmap_layer_destroy(time_layer);

   tick_timer_service_unsubscribe();
   accel_tap_service_unsubscribe();
   window_destroy(window);
}  // deinit()


static void display_chime(GContext *ctx, bool is_night)
{
   if (app_state == APP_CHIME_STATE)
   {
      if (toggle_flag)
      {
         if (chime_enabled != false)
         {
            set_bitmap_image(ctx, &chime_image, RESOURCE_ID_IMAGE_CHIME, GPoint(22, 5), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &chime_image, RESOURCE_ID_IMAGE_NOCHIME, GPoint(22, 5), is_night);
         }
      }
      else
      {
         if (chime_enabled != false)
         {
            set_bitmap_image(ctx, &chime_image, RESOURCE_ID_IMAGE_CHIME, GPoint(22, 5), !is_night);
         }
         else
         {
            set_bitmap_image(ctx, &chime_image, RESOURCE_ID_IMAGE_NOCHIME, GPoint(22, 5), !is_night);
         }
      }
   }
   else
   {
      if (chime_enabled != false)
      {
         set_bitmap_image(ctx, &chime_image, RESOURCE_ID_IMAGE_CHIME, GPoint(22, 5), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &chime_image, RESOURCE_ID_IMAGE_NOCHIME, GPoint(22, 5), is_night);
      }
   }
}  // display_chime()


static void display_md(GContext *ctx, bool is_night)
{
   if (app_state == APP_MD_STATE)
   {
      if (toggle_flag)
      {
         if (month_before_day != false)
         {
            set_bitmap_image(ctx, &md_image, RESOURCE_ID_IMAGE_MD, GPoint(42, 5), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &md_image, RESOURCE_ID_IMAGE_DM, GPoint(42, 5), is_night);
         }
      }
      else
      {
         if (month_before_day != false)
         {
            set_bitmap_image(ctx, &md_image, RESOURCE_ID_IMAGE_MD, GPoint(42, 5), !is_night);
         }
         else
         {
            set_bitmap_image(ctx, &md_image, RESOURCE_ID_IMAGE_DM, GPoint(42, 5), !is_night);
         }
      }
   }
   else
   {
      if (month_before_day != false)
      {
         set_bitmap_image(ctx, &md_image, RESOURCE_ID_IMAGE_MD, GPoint(42, 5), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &md_image, RESOURCE_ID_IMAGE_DM, GPoint(42, 5), is_night);
      }
   }
}  // display_md()


static void display_offset(GContext *ctx, bool is_night)
{
   if (app_state == APP_OFFSET_STATE)
   {
      if (toggle_flag)
      {
         set_bitmap_image(ctx, &tz_image[0], RESOURCE_ID_IMAGE_DATENUM_TZ, GPoint(73, 5), is_night);
         set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_PLUS, GPoint(95, 5), is_night);
         set_bitmap_image(ctx, &tz_image[2], DATENUM_IMAGE_RESOURCE_IDS[abs(time_offset) / 20], GPoint(107, 3), is_night);
         set_bitmap_image(ctx, &tz_image[3], DATENUM_IMAGE_RESOURCE_IDS[(abs(time_offset) / 2) % 10], GPoint(119, 3), is_night);
         if (time_offset >= 0)
         {
            set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_PLUS, GPoint(95, 5), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_MINUS, GPoint(95, 5), is_night);
         }
         if ((abs(time_offset) % 2) != 0)
         {
            set_bitmap_image(ctx, &tz_image[4], RESOURCE_ID_IMAGE_DATENUM_30, GPoint(132, 12), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &tz_image[4], RESOURCE_ID_IMAGE_DATENUM_00, GPoint(132, 5), is_night);
         }
      }
      else
      {
         set_bitmap_image(ctx, &tz_image[0], RESOURCE_ID_IMAGE_DATENUM_TZ, GPoint(73, 5), !is_night);
         set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_PLUS, GPoint(95, 5), !is_night);
         set_bitmap_image(ctx, &tz_image[2], DATENUM_IMAGE_RESOURCE_IDS[abs(time_offset) / 20], GPoint(107, 3), !is_night);
         set_bitmap_image(ctx, &tz_image[3], DATENUM_IMAGE_RESOURCE_IDS[(abs(time_offset) / 2) % 10], GPoint(119, 3), !is_night);
         if (time_offset >= 0)
         {
            set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_PLUS, GPoint(95, 5), !is_night);
         }
         else
         {
            set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_MINUS, GPoint(95, 5), !is_night);
         }
         if ((abs(time_offset) % 2) != 0)
         {
            set_bitmap_image(ctx, &tz_image[4], RESOURCE_ID_IMAGE_DATENUM_30, GPoint(132, 12), !is_night);
         }
         else
         {
            set_bitmap_image(ctx, &tz_image[4], RESOURCE_ID_IMAGE_DATENUM_00, GPoint(132, 5), !is_night);
         }
      }
   }
   else
   {
      set_bitmap_image(ctx, &tz_image[0], RESOURCE_ID_IMAGE_DATENUM_TZ, GPoint(73, 5), is_night);
      set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_MINUS, GPoint(95, 5), is_night);
      set_bitmap_image(ctx, &tz_image[2], DATENUM_IMAGE_RESOURCE_IDS[abs(time_offset) / 20], GPoint(107, 3), is_night);
      set_bitmap_image(ctx, &tz_image[3], DATENUM_IMAGE_RESOURCE_IDS[(abs(time_offset) / 2) % 10], GPoint(119, 3), is_night);
      if (time_offset >= 0)
      {
         set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_PLUS, GPoint(95, 5), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &tz_image[1], RESOURCE_ID_IMAGE_DATENUM_MINUS, GPoint(95, 5), is_night);
      }
      if ((abs(time_offset) % 2) != 0)
      {
         set_bitmap_image(ctx, &tz_image[4], RESOURCE_ID_IMAGE_DATENUM_30, GPoint(132, 12), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &tz_image[4], RESOURCE_ID_IMAGE_DATENUM_00, GPoint(132, 5), is_night);
      }
   }
}  // display_offset()


static void display_secs(GContext *ctx, bool is_night)
{
   if (app_state == APP_SECS_STATE)
   {
      if (toggle_flag)
      {
         if (show_seconds != false)
         {
            set_bitmap_image(ctx, &secs_image, RESOURCE_ID_IMAGE_SECS, GPoint(56, 5), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &secs_image, RESOURCE_ID_IMAGE_NOSECS, GPoint(56, 5), is_night);
         }
      }
      else
      {
         if (show_seconds != false)
         {
            set_bitmap_image(ctx, &secs_image, RESOURCE_ID_IMAGE_SECS, GPoint(56, 5), !is_night);
         }
         else
         {
            set_bitmap_image(ctx, &secs_image, RESOURCE_ID_IMAGE_NOSECS, GPoint(56, 5), !is_night);
         }
      }
   }
   else
   {
      if (show_seconds != false)
      {
         set_bitmap_image(ctx, &secs_image, RESOURCE_ID_IMAGE_SECS, GPoint(56, 5), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &secs_image, RESOURCE_ID_IMAGE_NOSECS, GPoint(56, 5), is_night);
      }
   }
}  // display_secs()


static void display_snooze(GContext *ctx, bool is_night)
{
   if (app_state == APP_SNOOZE_STATE)
   {
      if (toggle_flag)
      {
         if (snooze_enabled != false)
         {
            set_bitmap_image(ctx, &snooze_image, RESOURCE_ID_IMAGE_SNOOZE, GPoint(2, 5), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &snooze_image, RESOURCE_ID_IMAGE_NOSNOOZE, GPoint(2, 5), is_night);
         }
      }
      else
      {
         if (snooze_enabled != false)
         {
            set_bitmap_image(ctx, &snooze_image, RESOURCE_ID_IMAGE_SNOOZE, GPoint(2, 5), !is_night);
         }
         else
         {
            set_bitmap_image(ctx, &snooze_image, RESOURCE_ID_IMAGE_NOSNOOZE, GPoint(2, 5), !is_night);
         }
      }
   }
   else
   {
      if (snooze_enabled != false)
      {
         set_bitmap_image(ctx, &snooze_image, RESOURCE_ID_IMAGE_SNOOZE, GPoint(2, 5), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &snooze_image, RESOURCE_ID_IMAGE_NOSNOOZE, GPoint(2, 5), is_night);
      }
   }
}  // display_snooze()


static void down_single_click_handler(ClickRecognizerRef recognizer, void *context)
{
   switch (app_state)
   {
      case APP_IDLE_STATE:
         if (snooze_timer == 0)
         {
            // activate refresh so everything paints after snooze wake-up
            refresh_display = true;
            layer_mark_dirty(window_layer);
         }

         snooze_timer = SNOOZE_SECONDS;

         break;

      case APP_CHIME_STATE:
         toggle_chime();

         // Save chime_enabled setting into persistent storage
         persist_write_int(PKEY_CHIME_ENABLED, chime_enabled);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_SNOOZE_STATE:
         toggle_snooze();

         // Save snooze_enabled setting into persistent storage
         persist_write_int(PKEY_SNOOZE_ENABLED, snooze_enabled);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_MD_STATE:
         toggle_md();

         // Save month_before_day setting into persistent storage
         persist_write_int(PKEY_MONTH_BEFORE_DAY, month_before_day);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_SECS_STATE:
         toggle_secs();

         // Save show_seconds setting into persistent storage
         persist_write_int(PKEY_SHOW_SECONDS, show_seconds);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_OFFSET_STATE:
         if (time_offset > -47)
         {
            time_offset--;

            // Save time_offset setting into persistent storage
            persist_write_int(PKEY_TIME_OFFSET, time_offset);

            // activate refresh so everything paints after snooze wake-up
            refresh_display = true;
            layer_mark_dirty(window_layer);
         }


         setmode_timer = SETMODE_SECONDS;
         break;

      default:
         break;
   }
}  // down_single_click_handler()


static void handle_accel_tap(AccelAxisType axis, int32_t direction)
{
   light_timer = 4;
   splash_timer = 0;

   light_on = !light_on;

   if (light_on)
   {
      light_enable(true);
   }
   else
   {
      light_enable(false);
   }

   snooze_timer =  SNOOZE_SECONDS;

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // handle_accel_tap()


static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed)
{
   if (splash_timer > 0)
   {
      splash_timer--;

      if (splash_timer == 0)
      {
         refresh_display = true;
         layer_mark_dirty(window_layer);
      }
      else
      {
         return;
      }
   }
   else
   {
      if (light_timer > 0)
      {
         light_timer--;

         if (light_timer == 0)
         {
            light_enable(false);
            light_on = false;
         }
      }
   }

   // hourly chime
   if ((chime_enabled) && (tick_time->tm_hour != previous_time.tm_hour))
   {
      vibes_double_pulse();
      snooze_timer = SNOOZE_SECONDS;
      refresh_display = true;
      layer_mark_dirty(window_layer);
   }

   if (app_state == APP_IDLE_STATE)
   {
      if (snooze_enabled == true)
      {
         if (snooze_timer > 0)
         {
            snooze_timer--;
         }
      }
   }
   else
   {
      if (app_state != APP_IDLE_STATE)
      {
         if (setmode_timer > 0)
         {
            setmode_timer--;

            if (setmode_timer == 0)
            {
               app_state = APP_IDLE_STATE;
            }
         }
      }

      snooze_timer = SNOOZE_SECONDS;
   }

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // handle_second_tick()


static void init(void)
{
   GRect dummy_frame = { {0, 0}, {0, 0} };

   window = window_create();
   if (window == NULL)
   {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "...couldn't allocate window memory...");
      return;
   }

   // Get all settings from persistent storage for use if they exist, otherwise use the default
   chime_enabled = persist_exists(PKEY_CHIME_ENABLED) ? persist_read_int(PKEY_CHIME_ENABLED) : CHIME_ENABLED_DEFAULT;
   snooze_enabled = persist_exists(PKEY_SNOOZE_ENABLED) ? persist_read_int(PKEY_SNOOZE_ENABLED) : SNOOZE_ENABLED_DEFAULT;
   show_seconds = persist_exists(PKEY_SHOW_SECONDS) ? persist_read_int(PKEY_SHOW_SECONDS) : SHOW_SECONDS_DEFAULT;
   month_before_day = persist_exists(PKEY_MONTH_BEFORE_DAY) ? persist_read_int(PKEY_MONTH_BEFORE_DAY) : MONTH_BEFORE_DAY_DEFAULT;
   time_offset = persist_exists(PKEY_TIME_OFFSET) ? persist_read_int(PKEY_TIME_OFFSET) : TIME_OFFSET_DEFAULT;

   window_set_fullscreen(window, true);
   window_stack_push(window, true /* Animated */);
   window_layer = window_get_root_layer(window);

   window_set_click_config_provider(window, click_config_provider);
   layer_set_update_proc(window_layer, update_display);

   splash_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH); 
   time_layer = bitmap_layer_create(dummy_frame);
   bitmap_layer_set_background_color(time_layer, GColorClear);
   bitmap_layer_set_bitmap(time_layer, splash_image);
   layer_add_child(window_layer, bitmap_layer_get_layer(time_layer));

   for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
   {
     time_image[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUM_0);
     time2_image[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_NUM_0);
   }

   for (int i = 0; i < TOTAL_DATE_DIGITS; i++)
   {
     date_image[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DATENUM_0);
     date2_image[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DATENUM_0);
   }

   for (int i = 0; i < TOTAL_TZ_IMAGES; i++)
   {
     tz_image[i] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DATENUM_0);
   }

   am_pm_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_AM_MODE);
   am_pm2_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_AM_MODE);

   day_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DAY_NAME_SUN);
   day2_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DAY_NAME_SUN);

   chime_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHIME);

   snooze_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SNOOZE);

   md_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MD);

   secs_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SECS);

   accel_tap_service_subscribe(&handle_accel_tap);
   tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
}  // init()


static void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
   app_state++;

   if (app_state != STATE_COUNT)
   {
      setmode_timer = SETMODE_SECONDS;
   }
   else
   {
      app_state = APP_IDLE_STATE;

      snooze_timer = SNOOZE_SECONDS;

      setmode_timer = 0;
   }

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // select_long_click_handler()


static void select_long_release_handler(ClickRecognizerRef recognizer, void *context)
{
   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // select_long_release_handler()


static void select_single_click_handler(ClickRecognizerRef recognizer, void *context)
{
   switch (app_state)
   {
      case APP_IDLE_STATE:
         if (snooze_timer == 0)
         {
            // activate refresh so everything paints after snooze wake-up
            refresh_display = true;
            layer_mark_dirty(window_layer);
         }

         light_timer = 4;

         if (splash_timer > 0)
         {
            splash_timer = 0;

            // activate refresh so everything paints
            refresh_display = true;
            layer_mark_dirty(window_layer);
         }


         light_on = !light_on;

         if (light_on)
         {
            light_enable(true);
         }
         else
         {
            light_enable(false);
         }

         snooze_timer = SNOOZE_SECONDS;
         break;

      case APP_OFFSET_STATE:
         time_offset = 0;

         // Save time_offset setting into persistent storage
         persist_write_int(PKEY_TIME_OFFSET, time_offset);

         // activate refresh so everything paints
         refresh_display = true;
         layer_mark_dirty(window_layer);

         setmode_timer = SETMODE_SECONDS;
         break;

      default:
         setmode_timer = SETMODE_SECONDS;
         break;
   }

}  // select_single_click_handler()


static void set_bitmap_image(GContext *ctx, GBitmap **bmp_image, const int resource_id, GPoint this_origin, bool invert)
{
   gbitmap_destroy(*bmp_image);

   *bmp_image = gbitmap_create_with_resource(resource_id);

   GRect frame = (GRect)
   {
      .origin = this_origin,
      .size = (*bmp_image)->bounds.size
   };

   if (invert)
   {
      graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
   }
   else
   {
      graphics_context_set_compositing_mode(ctx, GCompOpAssign);
   }

   graphics_draw_bitmap_in_rect(ctx, *bmp_image, frame);

   layer_mark_dirty(window_layer);
}  // set_bitmap_image()


static void toggle_chime(void)
{
   if (chime_enabled == false)
   {
      chime_enabled = true;
   }
   else
   {
      chime_enabled = false;
   }

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // toggle_chime()


static void toggle_md(void)
{
   if (month_before_day == false)
   {
      month_before_day = true;
   }
   else
   {
      month_before_day = false;
   }

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // toggle_md()


static void toggle_snooze(void)
{
   if (snooze_enabled == false)
   {
      snooze_enabled = true;
   }
   else
   {
      snooze_enabled = false;
   }

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // toggle_snooze()


static void toggle_secs(void)
{
   if (show_seconds == false)
   {
      show_seconds = true;
   }
   else
   {
      show_seconds = false;
   }

   refresh_display = true;
   layer_mark_dirty(window_layer);
}  // toggle_secs()


static void up_single_click_handler(ClickRecognizerRef recognizer, void *context)
{
   switch (app_state)
   {
      case APP_IDLE_STATE:
         if (snooze_timer == 0)
         {
            // activate refresh so everything paints after snooze wake-up
            refresh_display = true;
            layer_mark_dirty(window_layer);
         }

         snooze_timer = SNOOZE_SECONDS;

         break;

      case APP_CHIME_STATE:
         toggle_chime();

         // Save chime_enabled setting into persistent storage
         persist_write_int(PKEY_CHIME_ENABLED, chime_enabled);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_SNOOZE_STATE:
         toggle_snooze();

         // Save snooze_enabled setting into persistent storage
         persist_write_int(PKEY_SNOOZE_ENABLED, snooze_enabled);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_MD_STATE:
         toggle_md();

         // Save month_before_day setting into persistent storage
         persist_write_int(PKEY_MONTH_BEFORE_DAY, month_before_day);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_SECS_STATE:
         toggle_secs();

         // Save show_seconds setting into persistent storage
         persist_write_int(PKEY_SHOW_SECONDS, show_seconds);

         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_OFFSET_STATE:
         if (time_offset < 47)
         {
            time_offset++;

            // Save time_offset setting into persistent storage
            persist_write_int(PKEY_TIME_OFFSET, time_offset);

            // activate refresh so everything paints after snooze wake-up
            refresh_display = true;
            layer_mark_dirty(window_layer);
         }

         setmode_timer = SETMODE_SECONDS;
         break;

      default:
         break;
   }
}  // up_single_click_handler()


static void update_display(Layer *layer, GContext *ctx)
{
   time_t t = time(NULL);
   struct tm *current_time = localtime(&t);

   int y_offset;
   bool is_night = false;

   if (splash_timer > 0)
   {
      if ((current_time->tm_hour < 6) || (current_time->tm_hour > 18))
      {
         set_bitmap_image(ctx, &splash_image, RESOURCE_ID_IMAGE_SPLASH, GPoint(0, 0), true);
      }
      else
      {
         set_bitmap_image(ctx, &splash_image, RESOURCE_ID_IMAGE_SPLASH, GPoint(0, 0), false);
      }
      return;
   }

   if (refresh_display)
   {
      if (((current_time->tm_hour < 6) || (current_time->tm_hour > 18)) && (time_offset == 0))
      {
         set_bitmap_image(ctx, &splash_image, RESOURCE_ID_IMAGE_WHITE, GPoint(0, 0), true);
      }
      else
      {
         set_bitmap_image(ctx, &splash_image, RESOURCE_ID_IMAGE_WHITE, GPoint(0, 0), false);
      }
   }

   if ((snooze_timer == 0) && (snooze_enabled))
   {
      if (((current_time->tm_hour < 6) || (current_time->tm_hour > 18)) && (time_offset == 0))
      {
         display_snooze(ctx, true);
      }
      else
      {
         display_snooze(ctx, false);
      }
      return;
   }

   toggle_flag = !toggle_flag;
   current_time2 = *current_time;

   // calculate new date/time for TIME 2, including time_offset
   if (time_offset > 0)
   {
      // calculate positive offset
      if ((abs(time_offset) %2) != 0)
      {
         current_time2.tm_min += 30;

         if (current_time2.tm_min >= 60)
         {
            current_time2.tm_min %= 60;

            current_time2.tm_hour += 1;
         }
      }

      current_time2.tm_hour += (time_offset / 2);

      if (current_time2.tm_hour >= 24)
      {
         current_time2.tm_hour %= 24;
         current_time2.tm_wday += 1;
         current_time2.tm_wday = current_time2.tm_wday % 7;

        current_time2.tm_mday += 1;

         switch (current_time2.tm_mon)
         {
            case 0:  // Jan
            case 2:  // Mar
            case 4:  // May
            case 6:  // Jul
            case 7:  // Aug
            case 9:  // Oct
            case 11: // Dec
               if (current_time2.tm_mday > 31)
               {
                  current_time2.tm_mday = 1;
                  current_time2.tm_mon += 1;
               }
               break;

            case 1:  // Feb
               // leap year defined as even multiple of 4, but not even multiple of 100, unless also even multiple of 400
               if (((current_time2.tm_year % 4) == 0) && (!((current_time2.tm_year % 100) == 0)))
               {
                  if (current_time2.tm_mday > 29)
                  {
                     current_time2.tm_mday = 1;
                     current_time2.tm_mon += 1;
                  }
               }
               else
               {
                  if ((current_time2.tm_year % 400) == 0)
                  {
                     if (current_time2.tm_mday > 29)
                     {
                        current_time2.tm_mday = 1;
                        current_time2.tm_mon += 1;
                     }
                  }
                  else
                  {
                     if (current_time2.tm_mday > 28)
                     {
                        current_time2.tm_mday = 1;
                        current_time2.tm_mon += 1;
                     }
                  }
               }
               break;

            case 3:  // Apr
            case 5:  // Jun
            case 8:  // Sep
            case 10: // Nov
               if (current_time2.tm_mday > 30)
               {
                  current_time2.tm_mday = 1;
                  current_time2.tm_mon += 1;
               }
               break;
         }

         if (current_time2.tm_mon > 11)
         {
            current_time2.tm_mon = 0;

            current_time2.tm_year += 1;
         }
      }
   }
   else
   {
      // calculate negative offset
      if ((abs(time_offset) % 2) != 0)
      {
         current_time2.tm_min -= 30;

         if (current_time2.tm_min < 0)
         {
            current_time2.tm_min += 60;
            current_time2.tm_min %= 60;

            current_time2.tm_hour -= 1;
         }
      }

      current_time2.tm_hour += (time_offset / 2);

      if (current_time2.tm_hour < 0)
      {
         current_time2.tm_hour += 24;
         current_time2.tm_hour %= 24;
         current_time2.tm_wday += 6;  // when combined with next statement, same as subtracting 1-day
         current_time2.tm_wday = current_time2.tm_wday % 7;

         current_time2.tm_mday -= 1;

         switch (current_time2.tm_mon)
         {
            // NOTE: at this point, the month in the switch statement refers to the month that we are about to change to
            case 0:  // Jan
            case 1:  // Feb
            case 3:  // Apr
            case 5:  // Jun
            case 7:  // Aug
            case 8:  // Sep
            case 10: // Nov
               if (current_time2.tm_mday < 1)
               {
                  current_time2.tm_mday = 31;
                  current_time2.tm_mon -= 1;
               }
               break;

            case 2:  // Mar
               // leap year defined as even multiple of 4, but not even multiple of 100, unless also even multiple of 400
               if (((current_time2.tm_year % 4) == 0) && (!((current_time2.tm_year % 100) == 0)))
               {
                  if (current_time2.tm_mday < 1)
                  {
                     current_time2.tm_mday = 29;
                     current_time2.tm_mon -= 1;
                  }
               }
               else
               {
                  if ((current_time2.tm_year % 400) == 0)
                  {
                     if (current_time2.tm_mday < 1)
                     {
                        current_time2.tm_mday = 29;
                        current_time2.tm_mon -= 1;
                     }
                  }
                  else
                  {
                     if (current_time2.tm_mday < 1)
                     {
                        current_time2.tm_mday = 28;
                        current_time2.tm_mon -= 1;
                     }
                  }
               }
               break;

            case 4:  // May
            case 6:  // Jul
            case 9:  // Oct
            case 11: // Dec
               if (current_time2.tm_mday < 1)
               {
                  current_time2.tm_mday = 30;
                  current_time2.tm_mon -= 1;
               }
               break;
         }

         if (current_time2.tm_mon < 0)
         {
            current_time2.tm_mon += 12;

            current_time2.tm_year -= 1;
         }
      }
   }

   // TIME 2

   if (time_offset != 0)
   {
      // when displaying both times, display TIME 2 below TIME 1
      y_offset = 97;
   }
   else
   {
      // when only displaying TIME 1, then pu them both in same place & let TIME 2 overwrite TIME 1 
      y_offset = 59;
   }

   if ((current_time2.tm_hour < 6) || (current_time2.tm_hour > 18))
   {
      is_night = true;
   }
   else
   {
      is_night = false;
   }

   // refresh the display each hour in case we just transitioned night-to-day or day-to-night
   if (current_time2.tm_min == 0)
   {
      refresh_display = true;
   }

   // display day2 of the week
   if ((current_time2.tm_wday != previous_time2.tm_wday) || (refresh_display))
   {
      set_bitmap_image(ctx, &day2_image, DAY_NAME_IMAGE_RESOURCE_IDS[current_time2.tm_wday], GPoint(1, y_offset), is_night);
   }

   // display month2 number
   if ((current_time2.tm_mon != previous_time2.tm_mon) || (refresh_display))
   {
      if (month_before_day == true)
      {
         set_bitmap_image(ctx, &date2_image[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) / 10], GPoint(47, y_offset), is_night);
         set_bitmap_image(ctx, &date2_image[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) % 10], GPoint(59, y_offset), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &date2_image[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) / 10], GPoint(83, y_offset), is_night);
         set_bitmap_image(ctx, &date2_image[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) % 10], GPoint(95, y_offset), is_night);
      }
   }

   // display "/" between month2 & day2
   set_bitmap_image(ctx, &date2_image[2], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(71, y_offset), is_night);

   // display day2 of the month number
   if ((current_time2.tm_mday != previous_time2.tm_mday) || (refresh_display))
   {
      if (month_before_day == true)
      {
         set_bitmap_image(ctx, &date2_image[3], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday / 10], GPoint(83, y_offset), is_night);
         set_bitmap_image(ctx, &date2_image[4], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday % 10], GPoint(95, y_offset), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &date2_image[3], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday / 10], GPoint(47, y_offset), is_night);
         set_bitmap_image(ctx, &date2_image[4], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday % 10], GPoint(59, y_offset), is_night);
      }
   }

   // display "/" between day2 & year2
   set_bitmap_image(ctx, &date2_image[5], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(107, y_offset), is_night);

   // display 2-digit year
   if ((current_time2.tm_year != previous_time2.tm_year) || (refresh_display))
   {
      set_bitmap_image(ctx, &date2_image[6], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_year / 10) % 10], GPoint(119, y_offset), is_night);
      set_bitmap_image(ctx, &date2_image[7], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_year % 10], GPoint(131, y_offset), is_night);
   }

   // display time2 hour
   if ((current_time2.tm_hour != previous_time2.tm_hour) || (refresh_display))
   {
      if (clock_is_24h_style())
      {
         set_bitmap_image(ctx, &time2_image[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_hour / 10], GPoint(1, 18 + y_offset), is_night);
         set_bitmap_image(ctx, &time2_image[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_hour % 10], GPoint(22, 18 + y_offset), is_night);

         if (show_seconds)
         {
            set_bitmap_image(ctx, &am_pm2_image, RESOURCE_ID_IMAGE_NONE_MODE, GPoint(133, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time2_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time2_image[5], RESOURCE_ID_IMAGE_COLON, GPoint(88, 18 + y_offset), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &am_pm2_image, RESOURCE_ID_IMAGE_NONE_MODE, GPoint(88, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time2_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
         }
      }
      else
      {
         // display AM/PM
         if (current_time2.tm_hour >= 12)
         {
            if (show_seconds)
            {
               set_bitmap_image(ctx, &am_pm2_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(133, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time2_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time2_image[5], RESOURCE_ID_IMAGE_COLON, GPoint(88, 18 + y_offset), is_night);
            }
            else
            {
               set_bitmap_image(ctx, &am_pm2_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(88, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time2_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
            }
         }
         else
         {
            if (show_seconds)
            {
               set_bitmap_image(ctx, &am_pm2_image, RESOURCE_ID_IMAGE_AM_MODE, GPoint(133, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time2_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time2_image[5], RESOURCE_ID_IMAGE_COLON, GPoint(88, 18 + y_offset), is_night);
            }
            else
            {
               set_bitmap_image(ctx, &am_pm2_image, RESOURCE_ID_IMAGE_AM_MODE, GPoint(88, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time2_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
            }
         }

         if ((current_time2.tm_hour % 12) == 0)
         {
            set_bitmap_image(ctx, &time2_image[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[1], GPoint(1, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time2_image[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[2], GPoint(22, 18 + y_offset), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &time2_image[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time2.tm_hour % 12) / 10], GPoint(1, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time2_image[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time2.tm_hour % 12) % 10], GPoint(22, 18 + y_offset), is_night);

            if ((current_time2.tm_hour % 12) < 10)
            {
               set_bitmap_image(ctx, &time2_image[0], RESOURCE_ID_IMAGE_NUM_BLANK, GPoint(1, 18 + y_offset), is_night);
            }
         }
      }
   }

   // display time2 minute
   if ((current_time2.tm_min != previous_time2.tm_min) || (refresh_display))
   {
      set_bitmap_image(ctx, &time2_image[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_min / 10], GPoint(46, 18 + y_offset), is_night);
      set_bitmap_image(ctx, &time2_image[4], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_min % 10], GPoint(67, 18 + y_offset), is_night);
   }

   // display time2 second
   if (show_seconds)
   {
      if ((current_time2.tm_sec != previous_time2.tm_sec) || (refresh_display))
      {
         set_bitmap_image(ctx, &time2_image[6], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_sec / 10], GPoint(91, 18 + y_offset), is_night);
         set_bitmap_image(ctx, &time2_image[7], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_sec % 10], GPoint(112, 18 + y_offset), is_night);
      }
   }
   else
   {
      set_bitmap_image(ctx, &time2_image[5], RESOURCE_ID_IMAGE_NO_COLON, GPoint(140, 18 + y_offset), is_night);
      set_bitmap_image(ctx, &time2_image[6], RESOURCE_ID_IMAGE_NUM_BLANK, GPoint(98, 18 + y_offset), is_night);
      set_bitmap_image(ctx, &time2_image[7], RESOURCE_ID_IMAGE_NUM_BLANK, GPoint(119, 18 + y_offset), is_night);
   }

   // TIME 1

   if (time_offset != 0)
   {
      // when displaying both times, display TIME 1 above TIME 2
      y_offset = 22;
   }
   else
   {
      // when only displaying TIME 1, then pu them both in same place & let TIME 2 overwrite TIME 1 
      y_offset = 59;
   }

   if ((current_time->tm_hour < 6) || (current_time->tm_hour > 18))
   {
      is_night = true;
   }
   else
   {
      is_night = false;
   }

   // refresh the display each hour in case we just transitioned night-to-day or day-to-night
   if (current_time->tm_min == 0)
   {
      refresh_display = true;
   }

   // display day of the week
   if ((current_time->tm_wday != previous_time.tm_wday) || (refresh_display))
   {
      set_bitmap_image(ctx, &day_image, DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday], GPoint(1, y_offset), is_night);
   }

   // display month number
   if ((current_time->tm_mon != previous_time.tm_mon) || (refresh_display))
   {
      if (month_before_day == true)
      {
         set_bitmap_image(ctx, &date_image[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) / 10], GPoint(47, y_offset), is_night);
         set_bitmap_image(ctx, &date_image[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) % 10], GPoint(59, y_offset), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &date_image[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) / 10], GPoint(83, y_offset), is_night);
         set_bitmap_image(ctx, &date_image[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) % 10], GPoint(95, y_offset), is_night);
      }
   }

   // display "/" between month & day
   set_bitmap_image(ctx, &date_image[2], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(71, y_offset), is_night);

   // display day of the month number
   if ((current_time->tm_mday != previous_time.tm_mday) || (refresh_display))
   {
      if (month_before_day == true)
      {
         set_bitmap_image(ctx, &date_image[3], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday / 10], GPoint(83, y_offset), is_night);
         set_bitmap_image(ctx, &date_image[4], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday % 10], GPoint(95, y_offset), is_night);
      }
      else
      {
         set_bitmap_image(ctx, &date_image[3], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday / 10], GPoint(47, y_offset), is_night);
         set_bitmap_image(ctx, &date_image[4], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday % 10], GPoint(59, y_offset), is_night);
      }
   }

   // display "/" between day & year
   set_bitmap_image(ctx, &date_image[5], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(107, y_offset), is_night);

   // display 2-digit year
   if ((current_time->tm_year != previous_time.tm_year) || (refresh_display))
   {
      set_bitmap_image(ctx, &date_image[6], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_year / 10) % 10], GPoint(119, y_offset), is_night);
      set_bitmap_image(ctx, &date_image[7], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_year % 10], GPoint(131, y_offset), is_night);
   }

   // display time hour
   if ((current_time->tm_hour != previous_time.tm_hour) || (refresh_display))
   {
      if (clock_is_24h_style())
      {
         set_bitmap_image(ctx, &time_image[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_hour / 10], GPoint(1, 18 + y_offset), is_night);
         set_bitmap_image(ctx, &time_image[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_hour % 10], GPoint(22, 18 + y_offset), is_night);

         if (show_seconds)
         {
            set_bitmap_image(ctx, &am_pm_image, RESOURCE_ID_IMAGE_NONE_MODE, GPoint(133, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time_image[5], RESOURCE_ID_IMAGE_COLON, GPoint(88, 18 + y_offset), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &am_pm_image, RESOURCE_ID_IMAGE_NONE_MODE, GPoint(88, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
         }
      }
      else
      {
         // display AM/PM
         if (current_time->tm_hour >= 12)
         {
            if (show_seconds)
            {
               set_bitmap_image(ctx, &am_pm_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(133, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time_image[5], RESOURCE_ID_IMAGE_COLON, GPoint(88, 18 + y_offset), is_night);
            }
            else
            {
               set_bitmap_image(ctx, &am_pm_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(88, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
            }
         }
         else
         {
            if (show_seconds)
            {
               set_bitmap_image(ctx, &am_pm_image, RESOURCE_ID_IMAGE_AM_MODE, GPoint(133, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time_image[5], RESOURCE_ID_IMAGE_COLON, GPoint(88, 18 + y_offset), is_night);
            }
            else
            {
               set_bitmap_image(ctx, &am_pm_image, RESOURCE_ID_IMAGE_AM_MODE, GPoint(88, 18 + y_offset), is_night);
               set_bitmap_image(ctx, &time_image[2], RESOURCE_ID_IMAGE_COLON, GPoint(43, 18 + y_offset), is_night);
            }
         }

         if ((current_time->tm_hour % 12) == 0)
         {
            set_bitmap_image(ctx, &time_image[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[1], GPoint(1, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time_image[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[2], GPoint(22, 18 + y_offset), is_night);
         }
         else
         {
            set_bitmap_image(ctx, &time_image[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time->tm_hour % 12) / 10], GPoint(1, 18 + y_offset), is_night);
            set_bitmap_image(ctx, &time_image[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time->tm_hour % 12) % 10], GPoint(22, 18 + y_offset), is_night);

            if ((current_time->tm_hour % 12) < 10)
            {
               set_bitmap_image(ctx, &time_image[0], RESOURCE_ID_IMAGE_NUM_BLANK, GPoint(1, 18 + y_offset), is_night);
            }
         }
      }
   }

   // display time minute
   if ((current_time->tm_min != previous_time.tm_min) || (refresh_display))
   {
      set_bitmap_image(ctx, &time_image[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min / 10], GPoint(46, 18 + y_offset), is_night);
      set_bitmap_image(ctx, &time_image[4], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min % 10], GPoint(67, 18 + y_offset), is_night);
   }

   // display time second
   if (show_seconds)
   {
      if ((current_time->tm_sec != previous_time.tm_sec) || (refresh_display))
      {
         set_bitmap_image(ctx, &time_image[6], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_sec / 10], GPoint(91, 18 + y_offset), is_night);
         set_bitmap_image(ctx, &time_image[7], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_sec % 10], GPoint(112, 18 + y_offset), is_night);
      }
   }
   else
   {
      set_bitmap_image(ctx, &time_image[5], RESOURCE_ID_IMAGE_NO_COLON, GPoint(140, 18 + y_offset), is_night);
      set_bitmap_image(ctx, &time_image[6], RESOURCE_ID_IMAGE_NUM_BLANK, GPoint(98, 18 + y_offset), is_night);
      set_bitmap_image(ctx, &time_image[7], RESOURCE_ID_IMAGE_NUM_BLANK, GPoint(119, 18 + y_offset), is_night);
   }

   if (((current_time->tm_hour < 6) || (current_time->tm_hour > 18)) && (time_offset == 0))
   {
      is_night = true;
   }
   else
   {
      is_night = false;
   }

   display_snooze(ctx, is_night);
   display_chime(ctx, is_night);
   display_md(ctx, is_night);
   display_secs(ctx, is_night);
   display_offset(ctx, is_night);

   previous_time = *current_time;
   previous_time2 = current_time2;

   refresh_display = false;
}  // update_display()


int main(void)
{
   init();
   app_event_loop();
   deinit();
}  // main()









