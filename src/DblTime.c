#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define MY_UUID {0x9a, 0x25, 0x32, 0xa8, 0xc0, 0x01, 0x4f, 0x5c, 0x80, 0x3f, 0x8b, 0x18, 0x19, 0x5b, 0x3a, 0xbb}

PBL_APP_INFO(MY_UUID,
	     "DblTime", "Pebble Technology & KD5RXT",
	     1, 3, /* App major/minor version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
//	     APP_INFO_WATCH_FACE);
	     APP_INFO_STANDARD_APP);

Window window;


#define TOTAL_DATE_DIGITS 10
#define TOTAL_TIME_DIGITS 6
#define TOTAL_COLON_BLOCKS 8
#define TOTAL_TZ_IMAGES 4
#define SNOOZE_SECONDS 10
#define SETMODE_SECONDS 20

typedef enum {APP_IDLE_STATE = 0, APP_SNOOZE_STATE, APP_CHIME_STATE, APP_MD_STATE, APP_OFFSET_STATE, STATE_COUNT} APP_STATE;

PblTm previous_time;
PblTm previous_time2;
PblTm current_time2;
int previous_mode = 0;
int chime_enabled = false;
int snooze_enabled = true;
int month_before_day = true;
int toggle_flag = false;
int snooze_timer = SNOOZE_SECONDS;
int setmode_timer = SETMODE_SECONDS;
int time_offset = 0;

int app_state = APP_IDLE_STATE;

BmpContainer colon_block_images[TOTAL_COLON_BLOCKS];
BmpContainer time_am_pm_image;
BmpContainer time2_am_pm_image;
BmpContainer day_name_image;
BmpContainer day2_name_image;
BmpContainer date_digits_images[TOTAL_DATE_DIGITS];
BmpContainer date2_digits_images[TOTAL_DATE_DIGITS];
BmpContainer time_digits_images[TOTAL_TIME_DIGITS];
BmpContainer time2_digits_images[TOTAL_TIME_DIGITS];
BmpContainer tz_images[TOTAL_TZ_IMAGES];
BmpContainer chime_image;
BmpContainer snooze_image;
BmpContainer md_image;

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


void click_config_provider(ClickConfig **config, Window *window);
void display_chime(void);
void display_colons(void);
void display_md(void);
void display_offset(void);
void display_snooze(void);
void down_single_click_handler(ClickRecognizerRef recognizer, Window *window);
void handle_deinit(AppContextRef ctx);
void handle_init(AppContextRef ctx);
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t);
void select_long_click_handler(ClickRecognizerRef recognizer, Window *window);
void select_long_release_handler(ClickRecognizerRef recognizer, Window *window);
void select_single_click_handler(ClickRecognizerRef recognizer, Window *window);
void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin);
void toggle_chime(void);
void toggle_md(void);
void toggle_snooze(void);
void up_single_click_handler(ClickRecognizerRef recognizer, Window *window);
void update_display(PblTm *current_time);
void wakeup_display(void);


void click_config_provider(ClickConfig **config, Window *window)
{
   (void)window;

   config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_single_click_handler;

   config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_click_handler;
   config[BUTTON_ID_SELECT]->long_click.release_handler = (ClickHandler) select_long_release_handler;

   config[BUTTON_ID_UP]->click.handler = (ClickHandler) up_single_click_handler;
   config[BUTTON_ID_UP]->click.repeat_interval_ms = 100;

   config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) down_single_click_handler;
   config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 100;
}  // click_config_provider()


void display_chime(void)
{
   if (app_state == APP_CHIME_STATE)
   {
      if (toggle_flag)
      {
         if (chime_enabled != false)
         {
            set_container_image(&chime_image, RESOURCE_ID_IMAGE_CHIME, GPoint(27, 5));
         }
         else
         {
            set_container_image(&chime_image, RESOURCE_ID_IMAGE_NOCHIME, GPoint(27, 5));
         }
      }
      else
      {
         if (chime_enabled != false)
         {
            set_container_image(&chime_image, RESOURCE_ID_IMAGE_INV_CHIME, GPoint(27, 5));
         }
         else
         {
            set_container_image(&chime_image, RESOURCE_ID_IMAGE_INV_NOCHIME, GPoint(27, 5));
         }
      }
   }
   else
   {
      if (chime_enabled != false)
      {
         set_container_image(&chime_image, RESOURCE_ID_IMAGE_CHIME, GPoint(27, 5));
      }
      else
      {
         set_container_image(&chime_image, RESOURCE_ID_IMAGE_NOCHIME, GPoint(27, 5));
      }
   }
}  // display_chime()


void display_colons(void)
{
   if (time_offset != 0)
   {
      set_container_image(&colon_block_images[0], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(43, 56));
      set_container_image(&colon_block_images[1], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(43, 72));
      set_container_image(&colon_block_images[2], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(88, 56));
      set_container_image(&colon_block_images[3], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(88, 72));

      set_container_image(&colon_block_images[4], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(43, 131));
      set_container_image(&colon_block_images[5], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(43, 147));
      set_container_image(&colon_block_images[6], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(88, 131));
      set_container_image(&colon_block_images[7], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(88, 147));
   }
   else
   {
      set_container_image(&colon_block_images[0], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(43, 56 + 37));
      set_container_image(&colon_block_images[1], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(43, 72 + 37));
      set_container_image(&colon_block_images[2], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(88, 56 + 37));
      set_container_image(&colon_block_images[3], RESOURCE_ID_IMAGE_COLON_BLOCK, GPoint(88, 72 + 37));

      for (int i = 4; i < TOTAL_COLON_BLOCKS; i++)
      {
         layer_remove_from_parent(&colon_block_images[i].layer.layer);
         bmp_deinit_container(&colon_block_images[i]);
      }
   }
}  // display_colons()


void display_md(void)
{
   if (app_state == APP_MD_STATE)
   {
      if (toggle_flag)
      {
         if (month_before_day != false)
         {
            set_container_image(&md_image, RESOURCE_ID_IMAGE_MD, GPoint(47, 5));
         }
         else
         {
            set_container_image(&md_image, RESOURCE_ID_IMAGE_DM, GPoint(47, 5));
         }
      }
      else
      {
         if (month_before_day != false)
         {
            set_container_image(&md_image, RESOURCE_ID_IMAGE_INV_MD, GPoint(47, 5));
         }
         else
         {
            set_container_image(&md_image, RESOURCE_ID_IMAGE_INV_DM, GPoint(47, 5));
         }
      }
   }
   else
   {
      if (month_before_day != false)
      {
         set_container_image(&md_image, RESOURCE_ID_IMAGE_MD, GPoint(47, 5));
      }
      else
      {
         set_container_image(&md_image, RESOURCE_ID_IMAGE_DM, GPoint(47, 5));
      }
   }
}  // display_md()


void display_offset(void)
{
   if (app_state == APP_OFFSET_STATE)
   {
      if (toggle_flag)
      {
         set_container_image(&tz_images[0], RESOURCE_ID_IMAGE_DATENUM_TZ, GPoint(83, 5));
      }
      else
      {
         set_container_image(&tz_images[0], RESOURCE_ID_IMAGE_DATENUM_INV_TZ, GPoint(83, 5));
      }
   }
   else
   {
      set_container_image(&tz_images[0], RESOURCE_ID_IMAGE_DATENUM_TZ, GPoint(83, 5));
   }

   if (time_offset >= 0)
   {
      set_container_image(&tz_images[1], RESOURCE_ID_IMAGE_DATENUM_PLUS, GPoint(105, 5));
   }
   else
   {
      set_container_image(&tz_images[1], RESOURCE_ID_IMAGE_DATENUM_MINUS, GPoint(105, 5));
   }

   set_container_image(&tz_images[2], DATENUM_IMAGE_RESOURCE_IDS[abs(time_offset) / 10], GPoint(117, 5));
   set_container_image(&tz_images[3], DATENUM_IMAGE_RESOURCE_IDS[abs(time_offset) % 10], GPoint(129, 5));
}  // display_offset()


void display_snooze(void)
{
   if (app_state == APP_SNOOZE_STATE)
   {
      if (toggle_flag)
      {
         if (snooze_enabled != false)
         {
            set_container_image(&snooze_image, RESOURCE_ID_IMAGE_SNOOZE, GPoint(7, 5));
         }
         else
         {
            set_container_image(&snooze_image, RESOURCE_ID_IMAGE_NOSNOOZE, GPoint(7, 5));
         }
      }
      else
      {
         if (snooze_enabled != false)
         {
            set_container_image(&snooze_image, RESOURCE_ID_IMAGE_INV_SNOOZE, GPoint(7, 5));
         }
         else
         {
            set_container_image(&snooze_image, RESOURCE_ID_IMAGE_INV_NOSNOOZE, GPoint(7, 5));
         }
      }
   }
   else
   {
      if (snooze_enabled != false)
      {
         set_container_image(&snooze_image, RESOURCE_ID_IMAGE_SNOOZE, GPoint(7, 5));
      }
      else
      {
         set_container_image(&snooze_image, RESOURCE_ID_IMAGE_NOSNOOZE, GPoint(7, 5));
      }
   }
}  // display_snooze()


void down_single_click_handler(ClickRecognizerRef recognizer, Window *window)
{
   (void)recognizer;
   (void)window;

   switch (app_state)
   {
      case APP_IDLE_STATE:
         if (snooze_timer == 0)
         {
            // (re)mark previous time completely invalid so everything paints after snooze wake-up
            previous_time.tm_wday = -1;
            previous_time.tm_mon  = -1;
            previous_time.tm_mday = -1;
            previous_time.tm_year = -1;
            previous_time.tm_hour = -1;
            previous_time.tm_min  = -1;
            previous_time.tm_sec  = -1;

            previous_time2.tm_wday = -1;
            previous_time2.tm_mon  = -1;
            previous_time2.tm_mday = -1;
            previous_time2.tm_year = -1;
            previous_time2.tm_hour = -1;
            previous_time2.tm_min  = -1;
            previous_time2.tm_sec  = -1;

            // (re)display chime
            display_chime();

            // (re)display md
            display_md();

            // (re)display colons
            display_colons();

            // (re)display offset
            display_offset();

            // (re)display time(s)
            wakeup_display();
         }

         snooze_timer = SNOOZE_SECONDS;

         break;

      case APP_CHIME_STATE:
         toggle_chime();
         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_SNOOZE_STATE:
         toggle_snooze();
         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_MD_STATE:
         toggle_md();
         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_OFFSET_STATE:
         if (time_offset > -23)
         {
            time_offset--;

            // mark previous times completely invalid so everything paints
            previous_time.tm_wday = -1;
            previous_time.tm_mon  = -1;
            previous_time.tm_mday = -1;
            previous_time.tm_year = -1;
            previous_time.tm_hour = -1;
            previous_time.tm_min  = -1;
            previous_time.tm_sec  = -1;

            previous_time2.tm_wday = -1;
            previous_time2.tm_mon  = -1;
            previous_time2.tm_mday = -1;
            previous_time2.tm_year = -1;
            previous_time2.tm_hour = -1;
            previous_time2.tm_min  = -1;
            previous_time2.tm_sec  = -1;

            display_colons();
            display_offset();
            wakeup_display();
         }
         setmode_timer = SETMODE_SECONDS;
         break;

      default:
         break;
   }
}  // down_single_click_handler()


void handle_deinit(AppContextRef ctx)
{
   (void)ctx;

   bmp_deinit_container(&time_am_pm_image);
   bmp_deinit_container(&time2_am_pm_image);
   bmp_deinit_container(&day_name_image);
   bmp_deinit_container(&day2_name_image);
   bmp_deinit_container(&chime_image);
   bmp_deinit_container(&snooze_image);
   bmp_deinit_container(&md_image);

   for (int i = 0; i < TOTAL_COLON_BLOCKS; i++)
   {
      bmp_deinit_container(&colon_block_images[i]);
   }

   for (int i = 0; i < TOTAL_DATE_DIGITS; i++)
   {
      bmp_deinit_container(&date_digits_images[i]);
      bmp_deinit_container(&date2_digits_images[i]);
   }

   for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
   {
      bmp_deinit_container(&time_digits_images[i]);
      bmp_deinit_container(&time2_digits_images[i]);
   }

   for (int i = 0; i < TOTAL_TZ_IMAGES; i++)
   {
      bmp_deinit_container(&tz_images[i]);
   }
}  // handle_deinit()


void handle_init(AppContextRef ctx)
{
   (void)ctx;

   window_init(&window, "DblTime");
   window_set_fullscreen(&window, true);
   window_stack_push(&window, true /* Animated */);

   resource_init_current_app(&APP_RESOURCES);

   // mark previous time completely invalid so everything paints first time
   previous_time.tm_wday = -1;
   previous_time.tm_mon  = -1;
   previous_time.tm_mday = -1;
   previous_time.tm_year = -1;
   previous_time.tm_hour = -1;
   previous_time.tm_min  = -1;
   previous_time.tm_sec  = -1;

   previous_time2.tm_wday = -1;
   previous_time2.tm_mon  = -1;
   previous_time2.tm_mday = -1;
   previous_time2.tm_year = -1;
   previous_time2.tm_hour = -1;
   previous_time2.tm_min  = -1;
   previous_time2.tm_sec  = -1;

   // Attach custom button functionality
   window_set_click_config_provider(&window, (ClickConfigProvider) click_config_provider);

   display_chime();
   display_snooze();
   display_md();
   display_colons();
   display_offset();
   wakeup_display();
}  // handle_init()


void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
   (void)ctx;

   if (app_state == APP_IDLE_STATE)
   {
      if (snooze_enabled == true)
      {
         if (snooze_timer > 0)
         {
            snooze_timer--;

            if (snooze_timer == 0)
            {
               // release all active display elements except snooze
               layer_remove_from_parent(&time_am_pm_image.layer.layer);
               bmp_deinit_container(&time_am_pm_image);

               layer_remove_from_parent(&time2_am_pm_image.layer.layer);
               bmp_deinit_container(&time2_am_pm_image);

               layer_remove_from_parent(&day_name_image.layer.layer);
               bmp_deinit_container(&day_name_image);

               layer_remove_from_parent(&day2_name_image.layer.layer);
               bmp_deinit_container(&day2_name_image);

               layer_remove_from_parent(&chime_image.layer.layer);
               bmp_deinit_container(&chime_image);

               layer_remove_from_parent(&md_image.layer.layer);
               bmp_deinit_container(&md_image);

               for (int i = 0; i < TOTAL_DATE_DIGITS; i++)
               {
                  layer_remove_from_parent(&date_digits_images[i].layer.layer);
                  bmp_deinit_container(&date_digits_images[i]);

                  layer_remove_from_parent(&date2_digits_images[i].layer.layer);
                  bmp_deinit_container(&date2_digits_images[i]);
               }

               for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
               {
                  layer_remove_from_parent(&time_digits_images[i].layer.layer);
                  bmp_deinit_container(&time_digits_images[i]);

                  layer_remove_from_parent(&time2_digits_images[i].layer.layer);
                  bmp_deinit_container(&time2_digits_images[i]);
               }

               for (int i = 0; i < TOTAL_COLON_BLOCKS; i++)
               {
                  layer_remove_from_parent(&colon_block_images[i].layer.layer);
                  bmp_deinit_container(&colon_block_images[i]);
               }

               for (int i = 0; i < TOTAL_TZ_IMAGES; i++)
               {
                  layer_remove_from_parent(&tz_images[i].layer.layer);
                  bmp_deinit_container(&tz_images[i]);
               }
            }
            else
            {
               update_display(t->tick_time);
            }
         }
      }
      else
      {
         update_display(t->tick_time);
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
               display_chime();
               display_snooze();
               display_md();
               display_offset();
               wakeup_display();
            }
         }
      }

      snooze_timer = SNOOZE_SECONDS;
      update_display(t->tick_time);
   }
}  // handle_second_tick()


void select_long_click_handler(ClickRecognizerRef recognizer, Window *window)
{
   (void)recognizer;
   (void)window;

   app_state++;

   if (app_state != STATE_COUNT)
   {
      // (re)mark previous time completely invalid so everything paints after snooze
      previous_time.tm_wday = -1;
      previous_time.tm_mon  = -1;
      previous_time.tm_mday = -1;
      previous_time.tm_year = -1;
      previous_time.tm_hour = -1;
      previous_time.tm_min  = -1;
      previous_time.tm_sec  = -1;

      previous_time2.tm_wday = -1;
      previous_time2.tm_mon  = -1;
      previous_time2.tm_mday = -1;
      previous_time2.tm_year = -1;
      previous_time2.tm_hour = -1;
      previous_time2.tm_min  = -1;
      previous_time2.tm_sec  = -1;

      // (re)display colons;
      display_colons();

      // (re)display offset
      display_offset();

      // (re)display time(s)
      wakeup_display();

      setmode_timer = SETMODE_SECONDS;
   }
   else
   {
      app_state = APP_IDLE_STATE;

      snooze_timer = SNOOZE_SECONDS;

      setmode_timer = 0;
   }

   display_chime();
   display_snooze();
   display_md();
   display_offset();
   wakeup_display();
}  // select_long_click_handler()


void select_long_release_handler(ClickRecognizerRef recognizer, Window *window)
{
   (void)recognizer;
   (void)window;
}  // select_long_release_handler()


void select_single_click_handler(ClickRecognizerRef recognizer, Window *window)
{
   (void)recognizer;
   (void)window;

   switch (app_state)
   {
      case APP_IDLE_STATE:
         if (snooze_timer == 0)
         {
            // (re)mark previous time completely invalid so everything paints after snooze
            previous_time.tm_wday = -1;
            previous_time.tm_mon  = -1;
            previous_time.tm_mday = -1;
            previous_time.tm_year = -1;
            previous_time.tm_hour = -1;
            previous_time.tm_min  = -1;
            previous_time.tm_sec  = -1;

            previous_time2.tm_wday = -1;
            previous_time2.tm_mon  = -1;
            previous_time2.tm_mday = -1;
            previous_time2.tm_year = -1;
            previous_time2.tm_hour = -1;
            previous_time2.tm_min  = -1;
            previous_time2.tm_sec  = -1;

            // (re)display chime
            display_chime();

            // (re)display md
            display_md();

            // (re)display colons
            display_colons();

            // (re)display offset
            display_offset();

            // (re)display time(s)
            wakeup_display();
         }

         snooze_timer = SNOOZE_SECONDS;
         break;

      case APP_OFFSET_STATE:
         time_offset = 0;

         // mark previous times completely invalid so everything paints
         previous_time.tm_wday = -1;
         previous_time.tm_mon  = -1;
         previous_time.tm_mday = -1;
         previous_time.tm_year = -1;
         previous_time.tm_hour = -1;
         previous_time.tm_min  = -1;
         previous_time.tm_sec  = -1;

         previous_time2.tm_wday = -1;
         previous_time2.tm_mon  = -1;
         previous_time2.tm_mday = -1;
         previous_time2.tm_year = -1;
         previous_time2.tm_hour = -1;
         previous_time2.tm_min  = -1;
         previous_time2.tm_sec  = -1;

         setmode_timer = SETMODE_SECONDS;

         display_colons();
         display_offset();
         wakeup_display();
         break;

      default:
         setmode_timer = SETMODE_SECONDS;
         break;
   }

}  // select_single_click_handler()


void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin)
{
   layer_remove_from_parent(&bmp_container->layer.layer);
   bmp_deinit_container(bmp_container);

   bmp_init_container(resource_id, bmp_container);

   GRect frame = layer_get_frame(&bmp_container->layer.layer);
   frame.origin.x = origin.x;
   frame.origin.y = origin.y;
   layer_set_frame(&bmp_container->layer.layer, frame);

   layer_add_child(&window.layer, &bmp_container->layer.layer);
}  // set_container_image()


void toggle_chime(void)
{
   if (chime_enabled == false)
   {
      chime_enabled = true;
   }
   else
   {
      chime_enabled = false;
   }

   display_chime();
}  // toggle_chime()


void toggle_md(void)
{
   if (month_before_day == false)
   {
      month_before_day = true;
   }
   else
   {
      month_before_day = false;
   }

   // invalidate month & day to display new format
   previous_time.tm_mon = -1;
   previous_time.tm_mday = -1;
   previous_time2.tm_mon = -1;
   previous_time2.tm_mday = -1;

   display_md();
}  // toggle_md()


void toggle_snooze(void)
{
   if (snooze_enabled == false)
   {
      snooze_enabled = true;
   }
   else
   {
      snooze_enabled = false;
   }

   display_snooze();
}  // toggle_snooze()


void up_single_click_handler(ClickRecognizerRef recognizer, Window *window)
{
   (void)recognizer;
   (void)window;

   switch (app_state)
   {
      case APP_IDLE_STATE:
         if (snooze_timer == 0)
         {
            // (re)mark previous time completely invalid so everything paints after snooze
            previous_time.tm_wday = -1;
            previous_time.tm_mon  = -1;
            previous_time.tm_mday = -1;
            previous_time.tm_year = -1;
            previous_time.tm_hour = -1;
            previous_time.tm_min  = -1;
            previous_time.tm_sec  = -1;

            previous_time2.tm_wday = -1;
            previous_time2.tm_mon  = -1;
            previous_time2.tm_mday = -1;
            previous_time2.tm_year = -1;
            previous_time2.tm_hour = -1;
            previous_time2.tm_min  = -1;
            previous_time2.tm_sec  = -1;

            // (re)display chime
            display_chime();

            // (re)display md
            display_md();

            // (re)display colons
            display_colons();

            // (re)display offset
            display_offset();

            // (re)display time(s)
            wakeup_display();
         }

         snooze_timer = SNOOZE_SECONDS;

         break;

      case APP_CHIME_STATE:
         toggle_chime();
         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_SNOOZE_STATE:
         toggle_snooze();
         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_MD_STATE:
         toggle_md();
         setmode_timer = SETMODE_SECONDS;
         break;

      case APP_OFFSET_STATE:
         if (time_offset < 23)
         {
            time_offset++;

            // mark previous times completely invalid so everything paints
            previous_time.tm_wday = -1;
            previous_time.tm_mon  = -1;
            previous_time.tm_mday = -1;
            previous_time.tm_year = -1;
            previous_time.tm_hour = -1;
            previous_time.tm_min  = -1;
            previous_time.tm_sec  = -1;

            previous_time2.tm_wday = -1;
            previous_time2.tm_mon  = -1;
            previous_time2.tm_mday = -1;
            previous_time2.tm_year = -1;
            previous_time2.tm_hour = -1;
            previous_time2.tm_min  = -1;
            previous_time2.tm_sec  = -1;

            display_colons();
            display_offset();
            wakeup_display();
         }
         setmode_timer = SETMODE_SECONDS;
         break;

      default:
         break;
   }
}  // up_single_click_handler()


void update_display(PblTm *current_time)
{
   int y_offset;

   toggle_flag = !toggle_flag;
   current_time2 = *current_time;

   // TIME 2

   if (time_offset != 0)
   {
      // when displaying both times, no shift of the display required
      y_offset = 0;

      // calculate new date/time, including time_offset
      if (time_offset > 0)
      {
         // calculate positive offset
         current_time2.tm_hour += time_offset;

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
         current_time2.tm_hour += time_offset;

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

      // display day2 of the week
      if ((current_time2.tm_wday != previous_time2.tm_wday) || (previous_mode != clock_is_24h_style()))
      {
         set_container_image(&day2_name_image, DAY_NAME_IMAGE_RESOURCE_IDS[current_time2.tm_wday], GPoint(3, 99));
      }

      // display month2 number
      if ((current_time2.tm_mon != previous_time2.tm_mon) || (previous_mode != clock_is_24h_style()))
      {
         if (month_before_day == true)
         {
            set_container_image(&date2_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) / 10], GPoint(45, 99));
            set_container_image(&date2_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) % 10], GPoint(57, 99));
         }
         else
         {
            set_container_image(&date2_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) / 10], GPoint(81, 99));
            set_container_image(&date2_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_mon + 1) % 10], GPoint(93, 99));
         }
      }

      // display "/" between month2 & day2
      set_container_image(&date2_digits_images[2], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(69, 99));

      // display day2 of the month number
      if ((current_time2.tm_mday != previous_time2.tm_mday) || (previous_mode != clock_is_24h_style()))
      {
         if (month_before_day == true)
         {
            set_container_image(&date2_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday / 10], GPoint(81, 99));
            set_container_image(&date2_digits_images[4], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday % 10], GPoint(93, 99));
         }
         else
         {
            set_container_image(&date2_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday / 10], GPoint(45, 99));
            set_container_image(&date2_digits_images[4], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_mday % 10], GPoint(57, 99));
         }
      }

      // display "/" between day2 & year2
      set_container_image(&date2_digits_images[5], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(105, 99));

      // display 2-digit year
      if ((current_time2.tm_year != previous_time2.tm_year) || (previous_mode != clock_is_24h_style()))
      {
         set_container_image(&date2_digits_images[8], DATENUM_IMAGE_RESOURCE_IDS[(current_time2.tm_year / 10) % 10], GPoint(117, 99));
         set_container_image(&date2_digits_images[9], DATENUM_IMAGE_RESOURCE_IDS[current_time2.tm_year % 10], GPoint(129, 99));
      }

      // display time2 hour
      if ((current_time2.tm_hour != previous_time2.tm_hour) || (previous_mode != clock_is_24h_style()))
      {
         if (clock_is_24h_style())
         {
          set_container_image(&time2_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_hour / 10], GPoint(3, 117));
          set_container_image(&time2_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_hour % 10], GPoint(24, 117));
         }
         else
         {
            // display AM/PM
            if (current_time2.tm_hour >= 12)
            {
               set_container_image(&time2_am_pm_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(133, 135));
            }
            else
            {
               set_container_image(&time2_am_pm_image, RESOURCE_ID_IMAGE_AM_MODE, GPoint(133, 135));
            }

            if ((current_time2.tm_hour % 12) == 0)
            {
               set_container_image(&time2_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[1], GPoint(3, 117));
               set_container_image(&time2_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[2], GPoint(24, 117));
            }
            else
            {
               set_container_image(&time2_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time2.tm_hour % 12) / 10], GPoint(3, 117));
               set_container_image(&time2_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time2.tm_hour % 12) % 10], GPoint(24, 117));

               if ((current_time2.tm_hour % 12) < 10)
               {
                  layer_remove_from_parent(&time2_digits_images[0].layer.layer);
                  bmp_deinit_container(&time2_digits_images[0]);
               }
            }
         }
      }

      // display time2 minute
      if ((current_time2.tm_min != previous_time2.tm_min) || (previous_mode != clock_is_24h_style()))
      {
         set_container_image(&time2_digits_images[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_min / 10], GPoint(48, 117));
         set_container_image(&time2_digits_images[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_min % 10], GPoint(69, 117));
      }

      // display time2 second
      if ((current_time2.tm_sec != previous_time2.tm_sec) || (previous_mode != clock_is_24h_style()))
      {
         set_container_image(&time2_digits_images[4], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_sec / 10], GPoint(93, 117));
         set_container_image(&time2_digits_images[5], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time2.tm_sec % 10], GPoint(114, 117));
      }
   }
   else
   {
      // when only displaying one time, shift its location down to the middle of the display
      y_offset = 37;

      // release all time2 display elements
      layer_remove_from_parent(&time2_am_pm_image.layer.layer);
      bmp_deinit_container(&time2_am_pm_image);

      layer_remove_from_parent(&day2_name_image.layer.layer);
      bmp_deinit_container(&day2_name_image);

      for (int i = 0; i < TOTAL_DATE_DIGITS; i++)
      {
         layer_remove_from_parent(&date2_digits_images[i].layer.layer);
         bmp_deinit_container(&date2_digits_images[i]);
      }

      for (int i = 0; i < TOTAL_TIME_DIGITS; i++)
      {
         layer_remove_from_parent(&time2_digits_images[i].layer.layer);
         bmp_deinit_container(&time2_digits_images[i]);
      }

      for (int i = 4; i < TOTAL_COLON_BLOCKS; i++)
      {
         layer_remove_from_parent(&colon_block_images[i].layer.layer);
         bmp_deinit_container(&colon_block_images[i]);
      }
   }

   // TIME 1

   // display day of the week
   if ((current_time->tm_wday != previous_time.tm_wday) || (previous_mode != clock_is_24h_style()))
   {
      set_container_image(&day_name_image, DAY_NAME_IMAGE_RESOURCE_IDS[current_time->tm_wday], GPoint(3, 24 + y_offset));
   }

   // display month number
   if ((current_time->tm_mon != previous_time.tm_mon) || (previous_mode != clock_is_24h_style()))
   {
      if (month_before_day == true)
      {
         set_container_image(&date_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) / 10], GPoint(45, 24 + y_offset));
         set_container_image(&date_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) % 10], GPoint(57, 24 + y_offset));
      }
      else
      {
         set_container_image(&date_digits_images[0], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) / 10], GPoint(81, 24 + y_offset));
         set_container_image(&date_digits_images[1], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_mon + 1) % 10], GPoint(93, 24 + y_offset));
      }
   }

   // display "/" between month & day
   set_container_image(&date_digits_images[2], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(69, 24 + y_offset));

   // display day of the month number
   if ((current_time->tm_mday != previous_time.tm_mday) || (previous_mode != clock_is_24h_style()))
   {
      if (month_before_day == true)
      {
         set_container_image(&date_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday / 10], GPoint(81, 24 + y_offset));
         set_container_image(&date_digits_images[4], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday % 10], GPoint(93, 24 + y_offset));
      }
      else
      {
         set_container_image(&date_digits_images[3], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday / 10], GPoint(45, 24 + y_offset));
         set_container_image(&date_digits_images[4], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_mday % 10], GPoint(57, 24 + y_offset));
      }
   }

   // display "/" between day & year
   set_container_image(&date_digits_images[5], RESOURCE_ID_IMAGE_DATENUM_SLASH, GPoint(105, 24 + y_offset));

   // display 2-digit year
   if ((current_time->tm_year != previous_time.tm_year) || (previous_mode != clock_is_24h_style()))
   {
      set_container_image(&date_digits_images[8], DATENUM_IMAGE_RESOURCE_IDS[(current_time->tm_year / 10) % 10], GPoint(117, 24 + y_offset));
      set_container_image(&date_digits_images[9], DATENUM_IMAGE_RESOURCE_IDS[current_time->tm_year % 10], GPoint(129, 24 + y_offset));
   }

   // display time hour
   if ((current_time->tm_hour != previous_time.tm_hour) || (previous_mode != clock_is_24h_style()))
   {
      if (clock_is_24h_style())
      {
       set_container_image(&time_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_hour / 10], GPoint(3, 42 + y_offset));
       set_container_image(&time_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_hour % 10], GPoint(24, 42 + y_offset));
      }
      else
      {
         // display AM/PM
         if (current_time->tm_hour >= 12)
         {
            set_container_image(&time_am_pm_image, RESOURCE_ID_IMAGE_PM_MODE, GPoint(133, 60 + y_offset));
         }
         else
         {
            set_container_image(&time_am_pm_image, RESOURCE_ID_IMAGE_AM_MODE, GPoint(133, 60 + y_offset));
         }

         if ((current_time->tm_hour % 12) == 0)
         {
            set_container_image(&time_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[1], GPoint(3, 42 + y_offset));
            set_container_image(&time_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[2], GPoint(24, 42 + y_offset));
         }
         else
         {
            set_container_image(&time_digits_images[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time->tm_hour % 12) / 10], GPoint(3, 42 + y_offset));
            set_container_image(&time_digits_images[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[(current_time->tm_hour % 12) % 10], GPoint(24, 42 + y_offset));

            if ((current_time->tm_hour % 12) < 10)
            {
               layer_remove_from_parent(&time_digits_images[0].layer.layer);
               bmp_deinit_container(&time_digits_images[0]);
            }
         }
      }
   }

   // display time minute
   if ((current_time->tm_min != previous_time.tm_min) || (previous_mode != clock_is_24h_style()))
   {
      set_container_image(&time_digits_images[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min / 10], GPoint(48, 42 + y_offset));
      set_container_image(&time_digits_images[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min % 10], GPoint(69, 42 + y_offset));
   }

   // display time second
   if ((current_time->tm_sec != previous_time.tm_sec) || (previous_mode != clock_is_24h_style()))
   {
      set_container_image(&time_digits_images[4], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_sec / 10], GPoint(93, 42 + y_offset));
      set_container_image(&time_digits_images[5], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_sec % 10], GPoint(114, 42 + y_offset));
   }

   switch (app_state)
   {
      case APP_CHIME_STATE:
         display_chime();
         break;

      case APP_SNOOZE_STATE:
         display_snooze();
         break;

      case APP_MD_STATE:
         display_md();
         break;

      case APP_OFFSET_STATE:
         display_offset();
         break;
   }

   previous_time = *current_time;
   previous_time2 = current_time2;

   previous_mode = clock_is_24h_style();
}  // update_time()


void wakeup_display(void)
{
   PblTm time_now;

   get_time(&time_now);
   update_display(&time_now);
}  // wakeup_display()


void pbl_main(void *params)
{
   PebbleAppHandlers handlers =
   {
      .init_handler = &handle_init,
      .deinit_handler = &handle_deinit,

      .tick_info =
      {
         .tick_handler = &handle_second_tick,
         .tick_units = SECOND_UNIT
      }
   };

   app_event_loop(params, &handlers);
}  // pbl_main()

