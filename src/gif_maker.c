#include <private.h>
#include <view.h>
#include <util.h>

#include <metadata_extractor.h>
#include <player.h>
#include <media_content.h>
#include <image_util.h>
#include <MagickWand/MagickWand.h>
#include <device/power.h>
#include <app_preference.h>

//FIXME: file name!

typedef struct _v_data {
   int file_num;
   unsigned char *data;
   int width;
   int height;
   int rotate;
} v_data;

static player_h player;
static Evas_Object *_main_slider, *_slider_layout, *_popup, *_bottom_layout;
static Ecore_Timer *_slider_timer;
static double _start_value, _end_value = 999999;
static Eina_Bool drag_start;

static int fps = 15;
static char **argv;
static PixelWand *background;
static Eina_Lock mutex;
static int frame = 1; //FIXME: i think be more better
static Ecore_Thread *_gif_maker_thread;
static Eina_Bool thread_cancel;

static void
_popup_btn_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout;

   thread_cancel = EINA_TRUE;
   layout = evas_object_data_get(_popup, "layout");
   elm_object_part_text_set(layout, "elm.text.description", "Wait...");

   ecore_thread_cancel(_gif_maker_thread);
}

static void
_popup_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
_popup_fail_open()
{
   Evas_Object *popup;
   Evas_Object *btn1;

   /* popup */
   popup = elm_popup_add(_main_naviframe);
   elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
   eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
   elm_object_part_text_set(popup, "title,text", "Fail");
   elm_object_text_set(popup,"Please try again");

   /* ok button */
   btn1 = elm_button_add(popup);
   elm_object_style_set(btn1, "popup");
   elm_object_text_set(btn1, "OK");
   elm_object_part_content_set(popup, "button1", btn1);
   evas_object_smart_callback_add(btn1, "clicked", _popup_btn_clicked_cb, popup);
   evas_object_show(popup);
}

static void
_popup_progress_increase()
{
   char text[128];
   int total_frame;
   Evas_Object *layout, *progressbar;

   dlog_print(DLOG_ERROR, LOG_TAG, "frame : %d", frame);
   layout = evas_object_data_get(_popup, "layout");
   progressbar = evas_object_data_get(_popup, "progressbar");

   preference_get_int("total_frame", &total_frame);

   snprintf(text, sizeof(text), "%d/%d", frame, total_frame);
   if (frame >= total_frame)
      snprintf(text, sizeof(text), "finalization");
   if (thread_cancel)
      snprintf(text, sizeof(text), "Wait...");

   elm_object_part_text_set(layout, "elm.text.description", text);
   elm_progressbar_value_set(progressbar, frame / (double)total_frame);

   frame++;
}

static void
_popup_progressbar_show()
{
   Evas_Object *popup;
   Evas_Object *layout;
   Evas_Object *btn;
   Evas_Object *progressbar;

   frame = 1;
   popup = elm_popup_add(_main_naviframe);
   _popup = popup;
   elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
   eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   /* ok button */
   btn = elm_button_add(popup);
   elm_object_style_set(btn, "popup");
   elm_object_text_set(btn, "Cancel");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_smart_callback_add(btn, "clicked", _popup_btn_cancel_cb, popup);

   /* layout */
   layout = my_layout_add(popup, "edje/popup.edj", "progressbar");
   evas_object_data_set(popup, "layout", layout);
   elm_object_part_text_set(layout, "elm.text.description", "Loading...");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   /* progressbar */
   progressbar = elm_progressbar_add(layout);
   evas_object_data_set(popup, "progressbar", progressbar);
   evas_object_size_hint_align_set(progressbar, EVAS_HINT_FILL, 0.5);
   evas_object_size_hint_weight_set(progressbar, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_part_content_set(layout, "progressbar", progressbar);

   elm_object_content_set(popup, layout);

   evas_object_show(popup);
}

static void
_player_pause()
{
   player_pause(player);
   ecore_timer_freeze(_slider_timer);
   elm_layout_signal_emit(_bottom_layout, "play", "play_button");
}

static void
_player_start()
{
   player_start(player);
   ecore_timer_thaw(_slider_timer);
   elm_layout_signal_emit(_bottom_layout, "pause", "play_button");
}

static void
_box_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_naviframe_item_pop(_main_naviframe);
   player_unprepare(player);
   player_destroy(player);
   player = NULL;
   eina_lock_free(&mutex);
}

static void
_slider_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _player_pause();
}

static void
_slider_cb_drag_start(void *data, Evas_Object *obj, void *event_info)
{
   drag_start = EINA_TRUE;
   _player_pause();
}

static Eina_Bool
_tiemr_cb(void *data)
{
   double *value = data;
   player_set_play_position(player, *value, true, NULL, NULL);
   free(value);
   return EINA_FALSE;
}

static void
_slider_cb_drag_stop(void *data, Evas_Object *obj, void *event_info)
{
   double value, *new_value;

   drag_start = EINA_FALSE;

   new_value = malloc(sizeof(double));
   value = elm_slider_value_get(obj);
   *new_value = value;

   if (value < _start_value)
      *new_value = _start_value;
   else if (value > _end_value)
      *new_value = _end_value;

   elm_slider_value_set(obj, *new_value);

   if (player_set_play_position(player, *new_value, true, NULL, NULL) != PLAYER_ERROR_NONE)
      ecore_timer_add(0.3, _tiemr_cb, new_value);

}

static void
_slider_cb_changed(void *data, Evas_Object *obj, void *event_info)
{
   double value;

   value = elm_slider_value_get(obj);

   if (drag_start)
      player_set_play_position(player, value, false, NULL, NULL);
   else
      player_set_play_position(player, value, true, NULL, NULL);
}

static void
_layout_cb_start_point_drag(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   double max_value, value;
   Evas_Coord w, geo;
   char text[20];

   evas_object_geometry_get(_main_slider, NULL, NULL, &w, NULL);
   edje_object_part_geometry_get(elm_layout_edje_get(obj), "start_clipper", NULL, NULL, &geo, NULL);
   elm_slider_min_max_get(_main_slider, NULL, &max_value);

   value = (geo * max_value) / (double)(w - ELM_SCALE_SIZE(11));
   elm_slider_value_set(_main_slider, value);
   player_set_play_position(player, value, false, NULL, NULL);

   snprintf(text, sizeof(text), "%.3lf", value / 1000);
   elm_object_part_text_set(_bottom_layout, "start_text", text);
}

static void
_layout_cb_start_point_drag_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   double max_value, *value;
   Evas_Coord w, geo;

   value = malloc(sizeof(double));
   evas_object_geometry_get(_main_slider, NULL, NULL, &w, NULL);
   edje_object_part_geometry_get(elm_layout_edje_get(obj), "start_clipper", NULL, NULL, &geo, NULL);
   elm_slider_min_max_get(_main_slider, NULL, &max_value);

   *value = (geo * max_value) / (double)(w - ELM_SCALE_SIZE(11));
   _start_value = *value;

   if (player_set_play_position(player, *value, true, NULL, NULL) != PLAYER_ERROR_NONE)
      ecore_timer_add(0.3, _tiemr_cb, value);
}

static void
_layout_cb_end_point_drag(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   double max_value, value;
   Evas_Coord w, geo;
   char text[20];

   evas_object_geometry_get(_main_slider, NULL, NULL, &w, NULL);
   edje_object_part_geometry_get(elm_layout_edje_get(obj), "start_clipper", NULL, NULL, &geo, NULL);
   elm_slider_min_max_get(_main_slider, NULL, &max_value);

   value = (geo * max_value) / (double)(w - ELM_SCALE_SIZE(11));
   elm_slider_value_set(_main_slider, value);
   player_set_play_position(player, value, true, NULL, NULL);

   edje_object_part_geometry_get(elm_layout_edje_get(obj), "end_clipper", NULL, NULL, &geo, NULL);
   value = (geo * max_value) / (double)(w - ELM_SCALE_SIZE(11));
   _end_value = max_value - value;

   snprintf(text, sizeof(text), "%.3lf", _end_value / 1000);
   elm_object_part_text_set(_bottom_layout, "end_text", text);
}

static Evas_Object *
_gif_maker_slider_add(Evas_Object *parent)
{
   int duration;

   Evas_Object *layout = my_layout_add(parent, "edje/maker_slider.edj", "main");
   _slider_layout = layout;
   elm_layout_signal_callback_add(layout, "start,point,drag", "", _layout_cb_start_point_drag, NULL);
   elm_layout_signal_callback_add(layout, "end,point,drag", "", _layout_cb_end_point_drag, NULL);
   elm_layout_signal_callback_add(layout, "start,point,drag,stop", "", _layout_cb_start_point_drag_stop, NULL);
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   Evas_Object *slider = elm_slider_add(layout);
   evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(slider, "slider,drag,stop", _slider_cb_drag_stop, NULL);
   evas_object_smart_callback_add(slider, "slider,drag,start", _slider_cb_drag_start, NULL);
   evas_object_event_callback_add(slider, EVAS_CALLBACK_MOUSE_DOWN, _slider_cb_mouse_down, NULL);
   evas_object_smart_callback_add(slider, "changed", _slider_cb_changed, NULL);
   player_get_duration(player, &duration);
   elm_slider_min_max_set(slider, 0, duration);
   _main_slider = slider;
   evas_object_show(slider);
   elm_object_part_content_set(layout, "slider", slider);

   return layout;
}

static Eina_Bool
_timer_cb_play(void *data)
{
   int ms;
   double value;

   value = elm_slider_value_get(_main_slider);
   player_get_play_position(player, &ms);

   if (value >= _end_value - 90)
      {
         _player_pause();
         ms = _end_value;
      }

   elm_slider_value_set(_main_slider, ms);

   return EINA_TRUE;
}

static void
_btn_cb_play(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   player_state_e state;

   player_get_state(player, &state);
   if (state == PLAYER_STATE_PLAYING)
      _player_pause();
   else
      _player_start();
}

static void
_thread_start(void *_data, Ecore_Thread *thread)
{
   v_data *vdata = _data;
   MagickWand *sw;
   char gif_path[PATH_MAX];
   unsigned int jpeg_size;
   unsigned char *jpeg_data;

   snprintf(gif_path, sizeof(gif_path), "%s%s_%d.gif", app_get_data_path(), "test", vdata->file_num);
   argv[vdata->file_num] = strdup(gif_path);
   dlog_print(DLOG_ERROR, LOG_TAG, "gif_path : %s", gif_path);
   image_util_encode_jpeg_to_memory(vdata->data, vdata->width, vdata->height, IMAGE_UTIL_COLORSPACE_RGB888, 100, &jpeg_data, &jpeg_size);
   free(vdata->data);

   sw = NewMagickWand();
   MagickReadImageBlob(sw, jpeg_data, jpeg_size);
   free(jpeg_data);
   MagickResizeImage(sw, vdata->width / 2, vdata->height / 2, Lanczos2SharpFilter);

   if (vdata->rotate)
         MagickRotateImage(sw, background, vdata->rotate);

   MagickWriteImage(sw, gif_path);
   DestroyMagickWand(sw);

   free(vdata);

   dlog_print(DLOG_ERROR, LOG_TAG, "savegif");
}

static void
_thread_end(void *_data, Ecore_Thread *thread)
{
   eina_lock_take(&mutex);
   _popup_progress_increase();
   eina_lock_release(&mutex);
}

static void
_thread_cb_start(void *_path, Ecore_Thread *thread)
{
   char *path = _path;

   void *data;
   char *temp_value;
   int total_frame, data_size, width, height, rotate = 0, file_num = 0;
   MagickWand *mw, *sw;
   metadata_extractor_h metadata_h;

   MagickWandGenesis();

   background = NewPixelWand();
   PixelSetColor(background, "#000000");

   metadata_extractor_create(&metadata_h);
   metadata_extractor_set_path(metadata_h, path);
   free(path);

   metadata_extractor_get_metadata(metadata_h, METADATA_ROTATE, &temp_value);
   if (temp_value)
      rotate = atoi(temp_value);
   metadata_extractor_get_metadata(metadata_h, METADATA_VIDEO_WIDTH, &temp_value);
   width = atoi(temp_value);
   metadata_extractor_get_metadata(metadata_h, METADATA_VIDEO_HEIGHT, &temp_value);
   height = atoi(temp_value);
   dlog_print(DLOG_ERROR, LOG_TAG, "_end_value : %lf", _end_value);
   dlog_print(DLOG_ERROR, LOG_TAG, "width : %d", width);
   dlog_print(DLOG_ERROR, LOG_TAG, "height : %d", height);
   dlog_print(DLOG_ERROR, LOG_TAG, "rotate : %d", rotate);

   total_frame = ((_end_value - _start_value) * fps) / 1000;
   preference_set_int("total_frame", total_frame);
   argv = (char**)malloc(sizeof(char*) * (total_frame + 2));

   for (int i = _start_value; i <= _end_value; i += 1000. / fps)
      {
         if (ecore_thread_check(thread))
            {
               metadata_extractor_destroy(metadata_h);
               return;
            }

         int ret = metadata_extractor_get_frame_at_time(metadata_h, i, true, &data, &data_size);
         if (ret != METADATA_EXTRACTOR_ERROR_NONE)
            continue;

         v_data *vdata = (v_data *)malloc(sizeof(v_data));
         vdata->file_num = file_num++;
         dlog_print(DLOG_ERROR, LOG_TAG, "file_num : %d", file_num);

         vdata->data = data;
         vdata->rotate = rotate;
         vdata->width = width;
         vdata->height = height;
         ecore_thread_run(_thread_start, _thread_end, NULL, vdata);

         while(ecore_thread_pending_get() > 1)
            usleep(100000);

      }

   metadata_extractor_destroy(metadata_h);

   if (file_num <= 0)
      {
         ecore_thread_main_loop_begin();
         _popup_fail_open();
         ecore_thread_main_loop_end();
         return;
      }
   dlog_print(DLOG_INFO, LOG_TAG, "before write : %u", time(NULL));

   while(ecore_thread_active_get() > 1)
      sleep(1);

   mw = NewMagickWand();
   for(int i = 0 ; i < file_num; i++)
      {
         sw = NewMagickWand();
         MagickReadImage(sw, argv[i]);
         dlog_print(DLOG_INFO, LOG_TAG, "MagickReadImage %s",  argv[i]);
         MagickSetImageDelay(sw, 100 / fps);
         MagickAddImage(mw,sw);
         DestroyMagickWand(sw);
      }

   for(int i = 0 ; i < file_num; i++)
      {
         free(argv[i]);
      }
   free(argv);

   MagickWriteImages(mw, "/opt/usr/media/Images/test.gif", true);
   DestroyMagickWand(mw);
   MagickWandTerminus();

}

static void
_thread_cb_end(void *data, Ecore_Thread *thread)
{
   device_power_release_lock(POWER_LOCK_DISPLAY);

   media_content_connect();
   media_content_scan_file("/opt/usr/media/Images/test.gif");
   media_content_disconnect();


   dlog_print(DLOG_INFO, LOG_TAG, "end time : %u", time(NULL));
   evas_object_del(_popup);
}

static void
_thread_cb_cancel(void *data, Ecore_Thread *thread)
{
   while(ecore_thread_active_get() > 1)
      sleep(1);
   device_power_release_lock(POWER_LOCK_DISPLAY);
   evas_object_del(_popup);
}

static void
_btn_cb_make(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   char *path = data;

   thread_cancel = EINA_FALSE;
   device_power_request_lock(POWER_LOCK_DISPLAY, 0);
   dlog_print(DLOG_INFO, LOG_TAG, "start time : %u", time(NULL));

   _gif_maker_thread = ecore_thread_run(_thread_cb_start, _thread_cb_end, _thread_cb_cancel, strdup(path));
   _popup_progressbar_show();
}

static void
_player_completed_cb(void *user_data)
{
   int duration;
   player_get_duration(player, &duration);
   elm_slider_value_set(_main_slider, duration);
   _player_pause();
}

void
gif_maker_open(char *path)
{
   char text[10];
   int duration;
   _start_value = 0;
   drag_start = EINA_FALSE;

   eina_lock_new(&mutex);

   Evas_Object *table = elm_table_add(_main_naviframe);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(table);

   Evas_Object *rect = evas_object_image_filled_add(_main_naviframe);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(rect);
   elm_table_pack(table, rect, 0, 0, 1, 1);

   player_create(&player);
   player_set_uri(player, path);
   player_set_display(player, PLAYER_DISPLAY_TYPE_EVAS, GET_DISPLAY(rect));
   player_prepare(player);
   player_set_completed_cb(player, _player_completed_cb, NULL);
   player_start(player);
   player_pause(player);

   player_get_duration(player, &duration);
   _end_value = duration;
   _slider_timer = ecore_timer_add(0.1, _timer_cb_play, NULL);
   ecore_timer_freeze(_slider_timer);

   Evas_Object *box = elm_box_add(table);
   eext_object_event_callback_add(box, EEXT_CALLBACK_BACK, _box_back_cb, NULL);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, box, 0, 0, 1, 1);
   evas_object_show(box);

   Evas_Object *pad = evas_object_rectangle_add(evas_object_evas_get(box));
   evas_object_size_hint_weight_set(pad, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_color_set(pad, 0, 0, 0, 0);
   evas_object_show(pad);
   elm_box_pack_end(box, pad);

   Evas_Object *slider = _gif_maker_slider_add(box);
   elm_box_pack_end(box, slider);

   Evas_Object *bottom_layout = my_layout_add(box, "edje/button_layout.edj", "main");
   _bottom_layout = bottom_layout;
   evas_object_size_hint_weight_set(bottom_layout, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bottom_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_signal_callback_add(bottom_layout, "button,play", "", _btn_cb_play, NULL);
   elm_layout_signal_callback_add(bottom_layout, "button,make", "", _btn_cb_make, path);
   snprintf(text, sizeof(text), "%.3lf", _start_value / 1000);
   elm_object_part_text_set(_bottom_layout, "start_text", text);
   snprintf(text, sizeof(text), "%.3lf", _end_value / 1000);
   elm_object_part_text_set(_bottom_layout, "end_text", text);
   evas_object_show(bottom_layout);
   elm_box_pack_end(box, bottom_layout);

   elm_naviframe_item_simple_push(_main_naviframe, table);
}
