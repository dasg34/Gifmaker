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

typedef struct _v_data {
   int file_num;
   unsigned char *data;
   int width;
   int height;
   int rotate;
} v_data;

static player_h player;
static Evas_Object *_main_slider, *_slider_layout, *_popup, *_bottom_layout, *_setting_layout;
static Ecore_Timer *_player_timer;
static double _start_value;
static Eina_Bool drag_start;

static int _total_frame;
static char **argv;
static PixelWand *background;
static Eina_Lock mutex;
static int frame = 1; //FIXME: i think be more better
static Ecore_Thread *_gif_maker_thread;
static Eina_Bool thread_cancel;

extern Elm_Object_Item *it_camera;

static void _slider_cb_changed(void *data, Evas_Object *obj, void *event_info);

static void
_popup_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout, *btn = data;

   thread_cancel = EINA_TRUE;
   layout = evas_object_data_get(_popup, "layout");
   elm_object_part_text_set(layout, "elm.text.description", "Wait...");
   elm_object_disabled_set(btn, EINA_TRUE);

   ecore_thread_cancel(_gif_maker_thread);
}

static void
_popup_btn_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout;

   thread_cancel = EINA_TRUE;
   layout = evas_object_data_get(_popup, "layout");
   elm_object_part_text_set(layout, "elm.text.description", "Wait...");
   elm_object_disabled_set(obj, EINA_TRUE);

   ecore_thread_cancel(_gif_maker_thread);
}

static void
_popup_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
popup_block_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_del(obj);
}

static void
popup_timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_del(obj);
}

static void
_popup_toast_open(char *text)
{
   Evas_Object *popup = elm_popup_add(_main_naviframe);
   elm_object_style_set(popup, "toast");
   elm_popup_timeout_set(popup, 2.0);
   elm_object_text_set(popup, text);
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
   evas_object_smart_callback_add(popup, "block,clicked", popup_block_clicked_cb, NULL);
   evas_object_smart_callback_add(popup, "timeout", popup_timeout_cb, NULL);

   evas_object_show(popup);
}

static void
_popup_fail_open(char *text)
{
   Evas_Object *popup;
   Evas_Object *btn1;

   /* popup */
   popup = elm_popup_add(_main_naviframe);
   elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
   eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
   elm_object_part_text_set(popup, "title,text", "Error");
   elm_object_text_set(popup, text);

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
   Evas_Object *layout, *progressbar;

   dlog_print(DLOG_ERROR, LOG_TAG, "frame : %d", frame);
   layout = evas_object_data_get(_popup, "layout");
   progressbar = evas_object_data_get(_popup, "progressbar");

   snprintf(text, sizeof(text), "%d/%d frame", frame, _total_frame);
   if (frame >= _total_frame)
      snprintf(text, sizeof(text), "Finalization...");
   if (thread_cancel)
      snprintf(text, sizeof(text), "Wait...");

   elm_object_part_text_set(layout, "elm.text.description", text);
   elm_progressbar_value_set(progressbar, frame / (double)_total_frame);

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
   evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   /* ok button */
   btn = elm_button_add(popup);
   elm_object_style_set(btn, "popup");
   elm_object_text_set(btn, "Cancel");
   elm_object_part_content_set(popup, "button1", btn);
   evas_object_smart_callback_add(btn, "clicked", _popup_btn_cancel_cb, popup);
   eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _popup_cancel_cb, btn);

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
_box_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_naviframe_item_pop(_main_naviframe);
   player_unprepare(player);
   player_destroy(player);
   player = NULL;
}

static void
_player_seek_completed_cb(void *user_data)
{
   evas_object_smart_callback_add(_main_slider, "changed", _slider_cb_changed, NULL);
}

static void
_slider_cb_changed(void *data, Evas_Object *obj, void *event_info)
{
   double value;
   char text[128];
   int ret;

   value = elm_slider_value_get(obj);
   _start_value = value;

   snprintf(text, sizeof(text), "Start Time : %.3lfs", value / 1000.0);
   elm_object_part_text_set(_bottom_layout, "start_text", text);

   ret = player_set_play_position(player, value, true, _player_seek_completed_cb, NULL);
   if (ret == PLAYER_ERROR_NONE)
      evas_object_smart_callback_del(_main_slider, "changed", _slider_cb_changed);
}

static Evas_Object *
_gif_maker_slider_add(Evas_Object *parent)
{
   int duration;

   Evas_Object *layout = my_layout_add(parent, "edje/maker_slider.edj", "main");
   _slider_layout = layout;
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   Evas_Object *slider = elm_slider_add(layout);
   _main_slider = slider;
   evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_smart_callback_add(slider, "changed", _slider_cb_changed, NULL);
   player_get_duration(player, &duration);
   elm_slider_min_max_set(slider, 0, duration);
   evas_object_show(slider);
   elm_object_part_content_set(layout, "slider", slider);

   return layout;
}

static void
_thread_start(void *_data, Ecore_Thread *thread)
{
   v_data *vdata = _data;
   MagickWand *sw;
   char gif_path[PATH_MAX];
   unsigned int jpeg_size;
   unsigned char *jpeg_data;
   int width, height;

   preference_get_int("width", &width);
   preference_get_int("height", &height);

   snprintf(gif_path, sizeof(gif_path), "%s%s_%d.gif", app_get_data_path(), "test", vdata->file_num);
   argv[vdata->file_num] = strdup(gif_path);
   dlog_print(DLOG_ERROR, LOG_TAG, "gif_path : %s", gif_path);
   dlog_print(DLOG_ERROR, LOG_TAG, "width : %d", width);
   dlog_print(DLOG_ERROR, LOG_TAG, "height : %d", height);
   image_util_encode_jpeg_to_memory(vdata->data, vdata->width, vdata->height, IMAGE_UTIL_COLORSPACE_RGB888, 100, &jpeg_data, &jpeg_size);
   free(vdata->data);

   sw = NewMagickWand();
   MagickReadImageBlob(sw, jpeg_data, jpeg_size);
   free(jpeg_data);
   MagickResizeImage(sw, width, height, Lanczos2SharpFilter);

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
   char *temp_value, out_path[PATH_MAX], out_file[PATH_MAX];
   int total_frame, data_size, width, height, rotate = 0, file_num = 0, fps, duration, video_duration, end;
   MagickWand *mw, *sw;
   metadata_extractor_h metadata_h;

   MagickWandGenesis();

   background = NewPixelWand();
   PixelSetColor(background, "#000000");

   metadata_extractor_create(&metadata_h);
   metadata_extractor_set_path(metadata_h, path);
   metadata_extractor_get_metadata(metadata_h, METADATA_ROTATE, &temp_value);
   if (temp_value)
      {
         rotate = atoi(temp_value);
         free(temp_value);
      }

   metadata_extractor_get_metadata(metadata_h, METADATA_VIDEO_WIDTH, &temp_value);
   width = atoi(temp_value);
   free(temp_value);
   metadata_extractor_get_metadata(metadata_h, METADATA_VIDEO_HEIGHT, &temp_value);
   height = atoi(temp_value);
   free(temp_value);
   metadata_extractor_get_metadata(metadata_h, METADATA_DURATION, &temp_value);
   video_duration = atoi(temp_value);
   free(temp_value);

   dlog_print(DLOG_ERROR, LOG_TAG, "video_duration : %d", video_duration);
   dlog_print(DLOG_ERROR, LOG_TAG, "width : %d", width);
   dlog_print(DLOG_ERROR, LOG_TAG, "height : %d", height);
   dlog_print(DLOG_ERROR, LOG_TAG, "rotate : %d", rotate);

   preference_get_int("fps_maker", &fps);
   preference_get_int("duration", &duration);

   end = _start_value + duration;
   if (video_duration < end)
      end = video_duration;

   total_frame = ((end - _start_value) * fps) / 1000;
   _total_frame = total_frame;
   argv = (char**)malloc(sizeof(char*) * (total_frame + 2));

   for (int i = _start_value; i <= end; i += 1000. / fps)
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
         _popup_fail_open("Please try again");
         ecore_thread_main_loop_end();
         return;
      }
   dlog_print(DLOG_INFO, LOG_TAG, "before write : %u", time(NULL));


   while(ecore_thread_active_get() > 1)
      {
         sleep(1);
         dlog_print(DLOG_ERROR, LOG_TAG, "ecore_thread_active_get : %d", ecore_thread_active_get());
      }

   mw = NewMagickWand();
   for(int i = 0 ; i < file_num; i++)
      {
         sw = NewMagickWand();
         MagickReadImage(sw, argv[i]);
         dlog_print(DLOG_INFO, LOG_TAG, "MagickReadImage %s", argv[i]);
         MagickSetImageDelay(sw, 100 / fps);
         MagickAddImage(mw,sw);
         DestroyMagickWand(sw);
      }

   for(int i = 0 ; i < file_num; i++)
      {
         free(argv[i]);
      }
   free(argv);

   time_t raw_time;
   struct tm* time_info;

   time(&raw_time);
   time_info = localtime(&raw_time);
   snprintf(out_file, sizeof(out_file), "gifmaker_%d%s%d%s%d_%s%d%s%d%s%d.gif", time_info->tm_year - 100,
            time_info->tm_mon < 9 ? "0" : "", time_info->tm_mon + 1,
            time_info->tm_mday < 10 ? "0" : "",  time_info->tm_mday,
            time_info->tm_hour < 10 ? "0" : "",  time_info->tm_hour,
            time_info->tm_min < 10 ? "0" : "",  time_info->tm_min,
            time_info->tm_sec < 10 ? "0" : "",  time_info->tm_sec);

   snprintf(out_path, sizeof(out_path), "/opt/usr/media/gifmaker/%s", out_file);
   MagickWriteImages(mw, out_path, true);
   DestroyMagickWand(mw);
   MagickWandTerminus();

   media_content_connect();
   media_content_scan_file(out_path);
   media_content_disconnect();

   app_control_h app_control;
   app_control_create(&app_control);
   app_control_set_uri(app_control, out_path);
   app_control_set_operation(app_control, APP_CONTROL_OPERATION_VIEW);
   app_control_send_launch_request(app_control, NULL, NULL);
   app_control_destroy(app_control);

   ecore_thread_main_loop_begin();
   _popup_toast_open("Success!");
   ecore_thread_main_loop_end();

}

static void
_thread_cb_end(void *data, Ecore_Thread *thread)
{
   char *path = data;
   device_power_release_lock(POWER_LOCK_DISPLAY);

   dlog_print(DLOG_INFO, LOG_TAG, "end time : %u", time(NULL));
   evas_object_del(_popup);
   eina_lock_free(&mutex);
   free(path);

   ecore_timer_thaw(_player_timer);
   player_start(player);

   if (it_camera)
     gif_recorder_prepare();
}

static void
_thread_cb_cancel(void *data, Ecore_Thread *thread)
{
   char *path = data;
   while(ecore_thread_active_get() > 1)
      sleep(1);
   device_power_release_lock(POWER_LOCK_DISPLAY);
   evas_object_del(_popup);
   eina_lock_free(&mutex);
   free(path);

   ecore_timer_thaw(_player_timer);
   player_start(player);

   if (it_camera)
      gif_recorder_prepare();
}

static void
_btn_cb_make(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   char *path = data;
   player_pause(player);
   ecore_timer_freeze(_player_timer);
   make_gif(path);
}


static void
_settings_cb_hide_done(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   evas_object_hide(_setting_layout);
   elm_layout_signal_callback_del(_setting_layout, "settings,hide,done", "", _settings_cb_hide_done);
}

static void
_setting_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_layout_signal_callback_add(_setting_layout, "settings,hide,done", "", _settings_cb_hide_done, NULL);
   elm_layout_signal_emit(_setting_layout, "settings,hide", "");
   elm_layout_signal_emit(_bottom_layout, "settings,hide", "");
}

static void
_settings_toggle()
{
   if (evas_object_visible_get(_setting_layout))
      {
         elm_layout_signal_callback_add(_setting_layout, "settings,hide,done", "", _settings_cb_hide_done, NULL);
         elm_layout_signal_emit(_setting_layout, "settings,hide", "");
         elm_layout_signal_emit(_bottom_layout, "settings,hide", "");
      }
   else
      {
         evas_object_show(_setting_layout);
         elm_layout_signal_emit(_setting_layout, "settings,show", "");
         elm_layout_signal_emit(_bottom_layout, "settings,show", "");
      }
}

static void
_settings_cb_hide_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _settings_toggle();
}

static void
_btn_cb_settings(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _settings_toggle();
}

static void
_setting_more_cb(void *data, Evas_Object *obj, void *event_info)
{
   _settings_toggle();
}

static void
_player_completed_cb(void *user_data)
{
   player_set_play_position(player, _start_value, true, NULL, NULL);
}

static void
_slider_cb_fps(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout = data;
   double value;
   char text[128];
   int duration, fps;

   value = elm_slider_value_get(obj);
   preference_set_int("fps_maker", (int)value);
   preference_get_int("fps_maker", &fps);
   snprintf(text, sizeof(text), "%d fps", fps);
   elm_object_part_text_set(layout, "fps_text", text);

   preference_get_int("duration", &duration);

   snprintf(text, sizeof(text), "%.3lfs", duration / 1000.0);
   elm_object_part_text_set(layout, "duration_text", text);

   snprintf(text, sizeof(text), "%.3lfs, %dfps", duration / 1000.0, fps);
   elm_object_part_text_set(_bottom_layout, "settings_text", text);
}

static void
_slider_cb_duration(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout = data;
   int fps;
   double value;
   char text[128];

   value = elm_slider_value_get(obj);

   preference_set_int("duration", value);

   preference_get_int("fps_maker", &fps);

   snprintf(text, sizeof(text), "%.3lfs", value / 1000);
   elm_object_part_text_set(layout, "duration_text", text);

   snprintf(text, sizeof(text), "%.3lfs, %dfps", value / 1000, fps);
   elm_object_part_text_set(_bottom_layout, "settings_text", text);
}

static Eina_Bool
_player_timer_cb(void *data)
{
   int position, duration;

   player_get_play_position(player, &position);

   preference_get_int("duration", &duration);
   if (position > _start_value + duration)
      player_set_play_position(player, _start_value, true, NULL, NULL);

   return EINA_TRUE;
}

void
gif_maker_open(char *path)
{
   char text[128];
   _start_value = 0;
   drag_start = EINA_FALSE;

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
   player_set_mute(player, EINA_TRUE);
   player_set_completed_cb(player, _player_completed_cb, NULL);
   int ret = player_prepare(player);
   if (ret == PLAYER_ERROR_NOT_SUPPORTED_FILE)
      {
         _popup_fail_open("Not supported file!");
         return;
      }
   player_start(player);
   _player_timer = ecore_timer_add(0.1, _player_timer_cb, NULL);

   Evas_Object *box = elm_box_add(table);
   eext_object_event_callback_add(box, EEXT_CALLBACK_BACK, _box_back_cb, NULL);
   eext_object_event_callback_add(box, EEXT_CALLBACK_MORE, _setting_more_cb, NULL);
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
   elm_layout_signal_callback_add(bottom_layout, "button,make", "", _btn_cb_make, path);
   elm_layout_signal_callback_add(bottom_layout, "button,settings", "", _btn_cb_settings, NULL);
   snprintf(text, sizeof(text), "Start Time : %.3lfs", _start_value / 1000);
   elm_object_part_text_set(_bottom_layout, "start_text", text);
   evas_object_show(bottom_layout);
   elm_box_pack_end(box, bottom_layout);

   Evas_Object *setting_layout = my_layout_add(table, "edje/setting_layout.edj", "main");
   _setting_layout = setting_layout;
   evas_object_size_hint_weight_set(setting_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(setting_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   eext_object_event_callback_add(setting_layout, EEXT_CALLBACK_MORE, _setting_more_cb, NULL);
   eext_object_event_callback_add(setting_layout, EEXT_CALLBACK_BACK, _setting_back_cb, NULL);
   elm_layout_signal_callback_add(setting_layout, "settings,hide,start", "", _settings_cb_hide_start, NULL);
   elm_table_pack(table, setting_layout, 0, 0, 1, 1);
   evas_object_hide(setting_layout);

   Evas_Object *setting_obj = my_layout_add(setting_layout, "edje/settings.edj", "main");
   evas_object_size_hint_weight_set(setting_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(setting_obj, EVAS_HINT_FILL, 1.0);
   elm_object_part_content_set(setting_layout, "layout", setting_obj);

   //fps setting
   int fps = -1;

   preference_get_int("fps_maker", &fps);
   if (fps < 0)
      {
         fps = 15;
         preference_set_int("fps_maker", fps);
      }

   Evas_Object *fps_slider = elm_slider_add(setting_obj);
   elm_slider_min_max_set(fps_slider, 5, 15);
   elm_slider_step_set(fps_slider, 1.0);
   elm_slider_value_set(fps_slider, fps);
   elm_object_part_content_set(setting_obj, "fps_slider", fps_slider);
   evas_object_smart_callback_add(fps_slider, "changed", _slider_cb_fps, setting_obj);
   snprintf(text, sizeof(text), "%d fps", fps);
   elm_object_part_text_set(setting_obj, "fps_text", text);

   //calculate duration
   int duration_val = -1;

   preference_get_int("duration", &duration_val);
   if (duration_val < 0)
      {
         duration_val = 3000;
         preference_set_int("duration", duration_val);
      }

   snprintf(text, sizeof(text), "%.3lfs", duration_val / 1000.0);
   elm_object_part_text_set(setting_obj, "duration_text", text);

   snprintf(text, sizeof(text), "%.3lfs, %dfps", duration_val / 1000.0, fps);
   elm_object_part_text_set(_bottom_layout, "settings_text", text);

   int width, height;
   player_get_video_size(player, &width, &height);
   if (width > 480)
      {
         height = 480 * height / width;
         width = 480;
      }

   preference_set_int("width", width);
   preference_set_int("height", height);

   Evas_Object *duration_slider = elm_slider_add(setting_obj);
   elm_slider_min_max_set(duration_slider, 0, 5000);
   elm_slider_step_set(duration_slider, 1.0);
   elm_slider_value_set(duration_slider, duration_val);
   evas_object_smart_callback_add(duration_slider, "changed", _slider_cb_duration, setting_obj);
   elm_object_part_content_set(setting_obj, "duration_slider", duration_slider);

   Elm_Object_Item *it;
   it = elm_naviframe_item_insert_after(_main_naviframe, elm_naviframe_top_item_get(_main_naviframe), NULL, NULL, NULL, table, NULL);
   elm_naviframe_item_title_enabled_set(it, EINA_FALSE, EINA_FALSE);
}

void
make_gif(char *path)
{
   char cmd[1024];

   eina_lock_new(&mutex);
   thread_cancel = EINA_FALSE;
   device_power_request_lock(POWER_LOCK_DISPLAY, 0);
   snprintf(cmd, sizeof(cmd), "exec rm -r %s/*.gif", app_get_data_path());
   system(cmd);

   _gif_maker_thread = ecore_thread_run(_thread_cb_start, _thread_cb_end, _thread_cb_cancel, strdup(path));
   _popup_progressbar_show();
}
