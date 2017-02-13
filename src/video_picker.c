#include <private.h>
#include <view.h>
#include <util.h>

#include <player.h>
#include <system_info.h>
#include <media_content.h>

static player_h player;

static void
_popup_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
_popup_open()
{
   Evas_Object *popup;
   Evas_Object *btn1;

   /* popup */
   popup = elm_popup_add(_main_naviframe);
   elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
   eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, eext_popup_back_cb, NULL);
   elm_object_text_set(popup,"Please select a video");

   /* ok button */
   btn1 = elm_button_add(popup);
   elm_object_style_set(btn1, "popup");
   elm_object_text_set(btn1, "OK");
   elm_object_part_content_set(popup, "button1", btn1);
   evas_object_smart_callback_add(btn1, "clicked", _popup_btn_clicked_cb, popup);
   evas_object_show(popup);
}

static void
_simple_video_player_close()
{
   player_stop(player);
   player_unprepare(player);
   player_destroy(player);
   elm_naviframe_item_pop(_main_naviframe);
}

static void
_rect_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   _simple_video_player_close();
}

static void
_player_completed_cb(void *user_data)
{
   _simple_video_player_close();
}

static void
_rect_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   _simple_video_player_close();
}

static void
_simple_video_player_open(char *path)
{
   Evas_Object *rect = evas_object_image_filled_add(_main_naviframe);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   eext_object_event_callback_add(rect, EEXT_CALLBACK_BACK, _rect_back_cb, NULL);
   evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_UP, _rect_mouse_up_cb, NULL);
   evas_object_show(rect);
   elm_naviframe_item_simple_push(_main_naviframe, rect);

   player_create(&player);
   player_set_completed_cb(player, _player_completed_cb, NULL);
   player_set_uri(player, path);
   player_set_display(player, PLAYER_DISPLAY_TYPE_EVAS, GET_DISPLAY(rect));
   player_prepare(player);
   player_start(player);
}

static void
thumbnail_completed_cb(media_content_error_e error, const char *path, void *data)
{
   Evas_Object *img = data;

   elm_image_file_set(img, path, NULL);
}

static void
_gengrid_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_naviframe_item_pop(_main_naviframe);
}

static void
_gengrid_longpress_cb(void *data, Evas_Object *obj, void *event_info)
{
   char *path;
   Elm_Object_Item *it;
   media_info_h media;

   it = event_info;
   media = elm_object_item_data_get(it);

   media_info_get_file_path(media, &path);

   _simple_video_player_open(path);

   free(path);

}

static void
_btn_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_naviframe_item_pop(_main_naviframe);
}

static void
_btn_done_cb(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Object *gengrid = data;
   Elm_Object_Item *it;
   media_info_h media;
   char *path;

   it = elm_gengrid_selected_item_get(gengrid);

   if (!it)
      {
         _popup_open();
         return;
      }
   media = elm_object_item_data_get(it);

   media_info_get_file_path(media, &path);

   gif_maker_open(path);
}

static char *
_grid_text_get(void *data, Evas_Object *obj, const char *part)
{
   media_info_h media = data;
   char *dispname;

   media_info_get_display_name(media, &dispname);

   return dispname;
}

static Evas_Object *
_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
   media_info_h media = data;

   if (strcmp(part, "elm.swallow.icon"))
      return NULL;

   Evas_Object *img = elm_image_add(obj);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(img);

   media_info_create_thumbnail(media, thumbnail_completed_cb, img);

   return img;
}

static void
_grid_del(void *data, Evas_Object *obj)
{
   media_info_h media = data;

   media_info_cancel_thumbnail(media);

   media_info_destroy(media);
}

void
video_picker_orient_set(Evas_Object *gengrid)
{
   int height, width;
   int rotation = elm_win_rotation_get(_win);

   system_info_get_platform_int("http://tizen.org/feature/screen.width", &width);
   system_info_get_platform_int("http://tizen.org/feature/screen.height", &height);

   if (rotation == 90 || rotation == 270)
      elm_gengrid_item_size_set(gengrid, height / 3, width / 2);
   else
      elm_gengrid_item_size_set(gengrid, width / 2, height / 4);
}

static void
_win_rotation_cb(void *data, Evas_Object *obj, void *event_info)
{
   video_picker_orient_set((Evas_Object *)data);
}

void
video_picker_open()
{
   int height, width;
   Elm_Gengrid_Item_Class *gic;
   Evas_Object *btn;

   Evas_Object *layout = my_layout_add(_main_naviframe, "edje/gengrid_layout.edj", "main");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   Evas_Object *gengrid = elm_gengrid_add(layout);
   eext_object_event_callback_add(gengrid, EEXT_CALLBACK_BACK, _gengrid_back_cb, NULL);
   evas_object_smart_callback_add(gengrid, "longpressed", _gengrid_longpress_cb, NULL);
   elm_gengrid_align_set(gengrid, 0.0, 0.0);
   video_picker_orient_set(gengrid);

   gic = elm_gengrid_item_class_new();
   gic->item_style = "type2";
   gic->func.text_get = _grid_text_get;
   gic->func.content_get = _grid_content_get;
   gic->func.del = _grid_del;

   evas_object_data_set(gengrid, "item_class", gic);
   gengrid_item_set(gengrid, "video");
   evas_object_data_del(gengrid, "item_class");
   elm_gengrid_item_class_free(gic);

   if (elm_gengrid_items_count(gengrid) < 1)
      {
         Evas_Object *nocontents = elm_layout_add(layout);
         elm_layout_theme_set(nocontents, "layout", "nocontents", "default");
         evas_object_size_hint_weight_set(nocontents, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
         evas_object_size_hint_align_set(nocontents, EVAS_HINT_FILL, EVAS_HINT_FILL);
         elm_object_part_text_set(nocontents, "elm.text", "No Videos");
         elm_layout_signal_emit(nocontents, "align.center", "elm");
         elm_object_part_content_set(layout, "gengrid", nocontents);
      }
   else
      elm_object_part_content_set(layout, "gengrid", gengrid);

   Elm_Object_Item *nf_it = elm_naviframe_item_push(_main_naviframe, "Video Picker", NULL, NULL, layout, NULL);

   btn = elm_button_add(_main_naviframe);
   elm_object_style_set(btn, "naviframe/title_left");
   evas_object_smart_callback_add(btn, "clicked", _btn_cancel_cb, NULL);
   elm_object_item_part_content_set(nf_it, "title_left_btn", btn);
   elm_object_text_set(btn, "CANCEL");

   btn = elm_button_add(_main_naviframe);
   elm_object_style_set(btn, "naviframe/title_right");
   evas_object_smart_callback_add(btn, "clicked", _btn_done_cb, gengrid);
   elm_object_item_part_content_set(nf_it, "title_right_btn", btn);
   elm_object_text_set(btn, "DONE");

   evas_object_smart_callback_add(_win, "wm,rotation,changed", _win_rotation_cb, gengrid);
}
