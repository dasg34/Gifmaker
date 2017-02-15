#include <private.h>
#include <view.h>
#include <util.h>

#include <camera.h>
#include <recorder.h>
#include <app_preference.h>
#include <media_content.h>
#include <metadata_extractor.h>

#define FOCUS_CIRCLE_SIZE 50

static camera_h g_camera;
static recorder_h recorder;
static Evas_Object *_bottom_layout;
static Eina_List *resolution_list;
static Ecore_Timer *record_timer;
static double record_time;
static app_event_handler_h handlers;

extern double _end_value;

Elm_Object_Item *it_camera;

typedef struct _video_resolution {
   int width;
   int height;
} video_resolution;

static void
_resolution_list_free()
{
   video_resolution *data;

   if (!resolution_list)
      return;

   EINA_LIST_FREE(resolution_list, data)
      free(data);
   resolution_list = NULL;
}

static Eina_Bool
_record_timer_cb(void *data)
{
   char text[128];

   record_time += 0.1;
   snprintf(text, sizeof(text), "00:0%d / 00:05", (int)(record_time + 0.001));

   elm_object_part_text_set(_bottom_layout, "record_text", text);

   return EINA_TRUE;
}

void
_camera_back()
{
   int rots[4] = { 0, 90, 180, 270 };

   ui_app_remove_event_handler(handlers);
   camera_stop_preview(g_camera);
   camera_destroy(g_camera);
   recorder_unprepare(recorder);
   recorder_destroy(recorder);
   _resolution_list_free();
   elm_naviframe_item_pop(_main_naviframe);
   elm_win_wm_rotation_available_rotations_set(_win, (const int *)(&rots), 4);
   it_camera = NULL;
}

static void
_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   int rots[4] = { 0, 90, 180, 270 };

   ui_app_remove_event_handler(handlers);
   camera_stop_preview(g_camera);
   camera_destroy(g_camera);
   recorder_unprepare(recorder);
   recorder_destroy(recorder);
   _resolution_list_free();
   elm_naviframe_item_pop(_main_naviframe);
   elm_win_wm_rotation_available_rotations_set(_win, (const int *)(&rots), 4);
   it_camera = NULL;
}

static void
_recorder_recording_limit_reached_cb(recorder_recording_limit_type_e type, void *user_data)
{
   recorder_pause(recorder);

   ecore_timer_del(record_timer);
   record_timer = NULL;
}

static bool
_recorder_supported_video_resolution_cb(int width, int height, void *user_data)
{
   if (width > 650)
      return true;

   video_resolution *v_resol = malloc(sizeof(video_resolution));

   v_resol->width = width;
   v_resol->height = height;

   resolution_list = eina_list_append(resolution_list, v_resol);

   return true;
}

static void
_recoder_init(Evas_Object *rect)
{
   char filename[PATH_MAX];
   char *data_path;

   camera_create(CAMERA_DEVICE_CAMERA0, &g_camera);
   camera_set_display(g_camera, CAMERA_DISPLAY_TYPE_EVAS, GET_DISPLAY(rect));
   camera_set_display_mode(g_camera, CAMERA_DISPLAY_MODE_LETTER_BOX);
   camera_attr_set_af_mode(g_camera, CAMERA_ATTR_AF_FULL);
   camera_start_focusing(g_camera, true);

   recorder_create_videorecorder(g_camera, &recorder);
   recorder_set_video_resolution(recorder, 1280, 720);
   recorder_set_file_format(recorder, RECORDER_FILE_FORMAT_MP4);
   recorder_set_video_encoder(recorder, RECORDER_VIDEO_CODEC_MPEG4);
   recorder_set_audio_encoder(recorder, RECORDER_AUDIO_CODEC_DISABLE);

   data_path = app_get_data_path();
   snprintf(filename, sizeof(filename), "%s/gifmaker.mp4", data_path);
   free(data_path);
   recorder_set_filename(recorder, filename);

   recorder_attr_set_mute(recorder, true);
   recorder_attr_set_time_limit(recorder, 5);
   recorder_set_recording_limit_reached_cb(recorder, _recorder_recording_limit_reached_cb, NULL);

   camera_start_preview(g_camera);
   recorder_prepare(recorder);
   recorder_attr_set_video_encoder_bitrate(recorder, 2000000);
}

static void
_settings_cb_hide_done(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   evas_object_hide(obj);
   elm_layout_signal_callback_del(obj, "settings,hide,done", "", _settings_cb_hide_done);
}

static void
_setting_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bottom_layout = data;

   elm_layout_signal_emit(bottom_layout, "mouse,clicked,*", "settings_event");
}

static void
_settings_show(Evas_Object *setting_layout)
{
   elm_layout_signal_emit(setting_layout, "settings,show", "");
   eext_object_event_callback_add(setting_layout, EEXT_CALLBACK_BACK, _setting_back_cb, _bottom_layout);
}

static void
_settings_hide(Evas_Object *setting_layout)
{
   int width, height;

   elm_layout_signal_callback_add(setting_layout, "settings,hide,done", "", _settings_cb_hide_done, NULL);
   elm_layout_signal_emit(setting_layout, "settings,hide", "");

   preference_get_int("width", &width);
   preference_get_int("height", &height);

   recorder_set_video_resolution(recorder, width, height);
   camera_stop_preview(g_camera);
   camera_set_preview_resolution(g_camera, width, height);
   camera_start_preview(g_camera);
   camera_start_focusing(g_camera, true);
}

static void
_btn_cb_settings_show(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Evas_Object *setting_layout = data;

   evas_object_show(setting_layout);
   _settings_show(setting_layout);
}

static void
_btn_cb_settings_hide(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Evas_Object *setting_layout = data;

   _settings_hide(setting_layout);
   eext_object_event_callback_del(setting_layout, EEXT_CALLBACK_BACK, _setting_back_cb);
}

static void
_setting_more_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *bottom_layout = data;

   elm_layout_signal_emit(bottom_layout, "mouse,clicked,*", "settings_event");
}

static void
_settings_cb_hide_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Evas_Object *bottom_layout = data;

   elm_layout_signal_emit(bottom_layout, "mouse,clicked,*", "settings_event");
}

static void
_slider_cb_fps(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout = data;
   double value;
   char text[128];
   int fps;

   value = elm_slider_value_get(obj);
   preference_set_int("fps_camera", (int)value);
   preference_get_int("fps_camera", &fps);
   snprintf(text, sizeof(text), "%d fps", fps);
   elm_object_part_text_set(layout, "fps_text", text);

   elm_object_part_text_set(_bottom_layout, "settings_text", text);
}

static void
_slider_cb_resolution(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout = data;
   video_resolution *v_resol;
   char text[128];
   int index, fps;

   index = (int)elm_slider_value_get(obj);

   v_resol = eina_list_nth(resolution_list, index);

   preference_set_int("camera_index", index);
   preference_set_int("width", v_resol->width);
   preference_set_int("height", v_resol->height);
   preference_get_int("fps_camera", &fps);

   snprintf(text, sizeof(text), "%d x %dpx", v_resol->width, v_resol->height);
   elm_object_part_text_set(layout, "resolution_text", text);

   snprintf(text, sizeof(text), "%d x %d, %dfps", v_resol->width, v_resol->height, fps);
   elm_object_part_text_set(_bottom_layout, "settings_text", text);
}

static void
_btn_cb_record(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   recorder_start(recorder);

   if (record_timer)
      ecore_timer_thaw(record_timer);
   else
      {
         record_time = 0;
         record_timer = ecore_timer_add(0.1, _record_timer_cb, NULL);
      }
}

static void
_btn_cb_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   char filename[PATH_MAX];
   char *data_path;
   int fps;

   recorder_commit(recorder);
   ecore_timer_del(record_timer);
   record_timer = NULL;
   elm_object_part_text_set(_bottom_layout, "record_text", "00:00 / 00:05");

   data_path = app_get_data_path();
   snprintf(filename, sizeof(filename), "%s/gifmaker.mp4", data_path);
   free(data_path);

   preference_get_int("fps_camera", &fps);
   preference_set_int("fps_maker", fps);

   _end_value = record_time * 1000;

   _back_cb(NULL, NULL, NULL);

   make_gif(filename);
}

static void
_btn_cb_pause(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   recorder_pause(recorder);
   ecore_timer_freeze(record_timer);
}

static void
_btn_cb_flash_on(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   camera_attr_set_flash_mode(g_camera, CAMERA_ATTR_FLASH_MODE_PERMANENT);
   preference_set_boolean("camera_flash", true);
}

static void
_btn_cb_flash_off(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   camera_attr_set_flash_mode(g_camera, CAMERA_ATTR_FLASH_MODE_OFF);
   preference_set_boolean("camera_flash", false);
}

static void
_btn_cb_viewer(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   viewer_open(EINA_TRUE);
}

static void
_recorder_rotation_set()
{
   int orientation = app_get_device_orientation();
   recorder_rotation_e camera_rotation;

   switch (orientation)
   {
   case 0:
      camera_rotation = RECORDER_ROTATION_90;
      break;
   case 90:
      camera_rotation = RECORDER_ROTATION_180;
      break;
   case 180:
      camera_rotation = RECORDER_ROTATION_270;
      break;
   case 270:
      camera_rotation = RECORDER_ROTATION_NONE;
      break;
   default:
      camera_rotation = RECORDER_ROTATION_90;
      break;
   }
   recorder_attr_set_orientation_tag(recorder, camera_rotation);
}

static void
_on_rotate_cb(app_event_info_h event_info, void *user_data)
{
   _recorder_rotation_set();
}

static void
_camera_focus_changed_cb(camera_focus_state_e state, void *data)
{
   Evas_Object *img = data;

   if(state != CAMERA_FOCUS_STATE_ONGOING)
      {
         evas_object_hide(img);
         elm_layout_signal_emit(img, "focus,hide", "");
      }
}

static void
_camera_cb_focus_set(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Object *img = data;
   Evas_Event_Mouse_Up *event = event_info;
   int width, height, w, h, cw;
   double new_height, pad;
   camera_attr_clear_af_area(g_camera);
   preference_get_int("height", &height);
   preference_get_int("width", &width);
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);

   new_height = width * (w / (double)height);
   pad = (h - new_height) / (double)2;

   if (event->canvas.y < pad || event->canvas.y > new_height + pad)
      return;

   evas_object_geometry_get(img, NULL, NULL, &cw, NULL);
   evas_object_move(img, event->canvas.x - cw / 2, event->canvas.y - cw / 2);
   evas_object_show(img);
   elm_layout_signal_emit(img, "focus,show", "");

   camera_attr_set_af_area(g_camera, width * (event->canvas.y - pad) / new_height, (double)height * (w - event->canvas.x) / (double)w);
   camera_start_focusing(g_camera, false);
}

void
gif_camera_open()
{
   //focus circle
   Evas_Object *img = my_layout_add(_main_naviframe, "edje/focus_circle.edj", "main");

   Evas_Object *table = elm_table_add(_main_naviframe);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(table);

   Evas_Object *bg = evas_object_rectangle_add(evas_object_evas_get(table));
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_color_set(bg, 0, 0, 0, 255);
   evas_object_show(bg);
   elm_table_pack(table, bg, 0, 0, 1, 1);

   Evas_Object *rect = evas_object_image_filled_add(evas_object_evas_get(table));
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_UP, _camera_cb_focus_set, img);
   evas_object_show(rect);
   elm_table_pack(table, rect, 0, 0, 1, 1);

   _recoder_init(rect);
   _recorder_rotation_set();
   camera_set_focus_changed_cb(g_camera, _camera_focus_changed_cb, img);

   Evas_Object *box = elm_box_add(table);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_align_set(box, 0.5, 1.0);
   elm_table_pack(table, box, 0, 0, 1, 1);
   evas_object_show(box);

   Evas_Object *bottom_layout = my_layout_add(box, "edje/bottom_camera.edj", "main");
   _bottom_layout = bottom_layout;
   evas_object_size_hint_weight_set(bottom_layout, EVAS_HINT_EXPAND, 0);
   evas_object_size_hint_align_set(bottom_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   eext_object_event_callback_add(bottom_layout, EEXT_CALLBACK_BACK, _back_cb, handlers);
   eext_object_event_callback_add(bottom_layout, EEXT_CALLBACK_MORE, _setting_more_cb, bottom_layout);
   elm_layout_signal_callback_add(bottom_layout, "button,record", "", _btn_cb_record, NULL);
   elm_layout_signal_callback_add(bottom_layout, "button,stop", "", _btn_cb_stop, NULL);
   elm_layout_signal_callback_add(bottom_layout, "button,pause", "", _btn_cb_pause, NULL);
   elm_layout_signal_callback_add(bottom_layout, "button,flash,on", "", _btn_cb_flash_on, NULL);
   elm_layout_signal_callback_add(bottom_layout, "button,flash,off", "", _btn_cb_flash_off, NULL);
   elm_layout_signal_callback_add(bottom_layout, "button,viewer", "", _btn_cb_viewer, NULL);


   elm_box_pack_end(box, bottom_layout);
   evas_object_show(bottom_layout);

   Evas_Object *setting_layout = my_layout_add(table, "edje/setting_layout.edj", "main");
   evas_object_size_hint_weight_set(setting_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(setting_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   eext_object_event_callback_add(setting_layout, EEXT_CALLBACK_MORE, _setting_more_cb, bottom_layout);
   eext_object_event_callback_add(setting_layout, EEXT_CALLBACK_BACK, _setting_back_cb, bottom_layout);
   elm_layout_signal_callback_add(setting_layout, "settings,hide,start", "", _settings_cb_hide_start, bottom_layout);
   elm_layout_signal_callback_add(bottom_layout, "button,settings,show", "", _btn_cb_settings_show, setting_layout);
   elm_layout_signal_callback_add(bottom_layout, "button,settings,hide", "", _btn_cb_settings_hide, setting_layout);
   elm_table_pack(table, setting_layout, 0, 0, 1, 1);
   evas_object_hide(setting_layout);

   Evas_Object *setting_obj = my_layout_add(setting_layout, "edje/settings.edj", "main");
   evas_object_size_hint_weight_set(setting_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(setting_obj, EVAS_HINT_FILL, 1.0);
   elm_object_part_content_set(setting_layout, "layout", setting_obj);

   //fps setting
   int fps = -1;
   char text[128];

   preference_get_int("fps_camera", &fps);
   if (fps < 0)
      {
         fps = 7;
         preference_set_int("fps_camera", fps);
      }

   Evas_Object *fps_slider = elm_slider_add(setting_obj);
   elm_slider_min_max_set(fps_slider, 1, 7);
   elm_slider_step_set(fps_slider, 1.0);
   elm_slider_value_set(fps_slider, fps);
   elm_object_part_content_set(setting_obj, "fps_slider", fps_slider);
   evas_object_smart_callback_add(fps_slider, "changed", _slider_cb_fps, setting_obj);
   snprintf(text, sizeof(text), "%d fps", fps);
   elm_object_part_text_set(setting_obj, "fps_text", text);

   //resolution
   video_resolution *v_resol;
   int index = -1;
   _resolution_list_free();
   recorder_foreach_supported_video_resolution(recorder, _recorder_supported_video_resolution_cb, NULL);

   Evas_Object *resolution_slider = elm_slider_add(setting_obj);
   elm_slider_step_set(resolution_slider, 1.0);
   evas_object_smart_callback_add(resolution_slider, "changed", _slider_cb_resolution, setting_obj);
   elm_object_part_content_set(setting_obj, "resolution_slider", resolution_slider);

   preference_get_int("camera_index", &index);

   if (index < 0)
      index = eina_list_count(resolution_list) - 1;

   elm_slider_min_max_set(resolution_slider, 0, eina_list_count(resolution_list) - 1);
   elm_slider_value_set(resolution_slider, index);

   v_resol = eina_list_nth(resolution_list, index);

   snprintf(text, sizeof(text), "%d x %d, %dfps", v_resol->width, v_resol->height, fps);
   elm_object_part_text_set(bottom_layout, "settings_text", text);

   snprintf(text, sizeof(text), "%d x %dpx", v_resol->width, v_resol->height);
   elm_object_part_text_set(setting_obj, "resolution_text", text);

   preference_set_int("camera_index", index);
   preference_set_int("width", v_resol->width);
   preference_set_int("height", v_resol->height);
   recorder_set_video_resolution(recorder, v_resol->width, v_resol->height);
   camera_set_preview_resolution(g_camera, v_resol->width, v_resol->height);

   preference_set_boolean("camera_flash", false);

   it_camera = elm_naviframe_item_simple_push(_main_naviframe, table);

   ui_app_add_event_handler(&handlers, APP_EVENT_DEVICE_ORIENTATION_CHANGED, _on_rotate_cb, NULL);
   elm_win_wm_rotation_available_rotations_set(_win, NULL, 0);
   elm_win_rotation_with_resize_set(_win, 0);
}
