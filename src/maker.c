#include <private.h>
#include <view.h>
#include <util.h>

static void
_gif_camera_mouse_up_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
//TODO
}

static void
_video_picker_mouse_up_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   video_picker_open();
}

static void
_layout_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   ui_app_exit();
}

static void
_win_rotation_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *layout = data;
   int rotation = elm_win_rotation_get(_win);
   char *path;

   path = app_res_path_get("edje/maker_layout.edj");
   if (rotation == 90 || rotation == 270)
      elm_layout_file_set(layout, path, "rotation");
   else
      elm_layout_file_set(layout, path, "main");

   free(path);
}

void
maker_open()
{
   Evas_Object *layout;
   int rotation = elm_win_rotation_get(_win);

   if (rotation == 90 || rotation == 270)
      layout = my_layout_add(_main_naviframe, "edje/maker_layout.edj", "rotation");
   else
      layout = my_layout_add(_main_naviframe, "edje/maker_layout.edj", "main");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_layout_signal_callback_add(layout, "button,video2gif", "", _video_picker_mouse_up_cb, NULL);
   elm_layout_signal_callback_add(layout, "button,gifcamera", "", _gif_camera_mouse_up_cb, NULL);
   eext_object_event_callback_add(layout, EEXT_CALLBACK_BACK, _layout_back_cb, NULL);
   elm_object_part_content_set(_main_naviframe, "elm.swallow.content", layout);

   evas_object_smart_callback_add(_win, "wm,rotation,changed", _win_rotation_cb, layout);
}
