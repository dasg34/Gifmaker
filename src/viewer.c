#include <private.h>
#include <view.h>
#include <util.h>

#include <storage.h>
#include <system_info.h>

static Eina_Bool scroll_start;
static Evas_Object *_gif_gengrid;

static void
_scroll_start_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eina_List *list, *l;
   Elm_Object_Item *it;
   Evas_Object *img;

   scroll_start = EINA_TRUE;
   list = elm_gengrid_realized_items_get(obj);

   if (!list)
      return;

   EINA_LIST_FOREACH(list, l, it)
      {
         img = elm_object_item_part_content_get(it, "elm.swallow.icon");

         if (elm_image_animated_play_get(img))
            elm_image_animated_play_set(img, EINA_FALSE);
      }
}


static void
_scroll_stop_cb(void *data, Evas_Object *obj, void *event_info)
{
   Eina_List *list, *l;
   Elm_Object_Item *it;
   Evas_Object *img;

   scroll_start = EINA_FALSE;
   list = elm_gengrid_realized_items_get(obj);

   if (!list)
      return;

   EINA_LIST_FOREACH(list, l, it)
      {
         img = elm_object_item_part_content_get(it, "elm.swallow.icon");

         if (!elm_image_animated_play_get(img))
            elm_image_animated_play_set(img, EINA_TRUE);
      }
}

static void
_layout_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   ui_app_exit();
}

static char *
_grid_text_get(void *data, Evas_Object *obj, const char *part)
{
   char *path = data;

   if (!path)
      return NULL;

   return strdup(basename(path));
}

static Evas_Object *
_grid_content_get(void *data, Evas_Object *obj, const char *part)
{
   char *path = data;

   if (strcmp(part, "elm.swallow.icon"))
      return NULL;

   Evas_Object *img = elm_image_add(obj);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_image_file_set(img, path, NULL);

   if (elm_image_animated_available_get(img))
      {
         elm_image_animated_set(img, EINA_TRUE);
         if (!scroll_start)
            elm_image_animated_play_set(img, EINA_TRUE);
      }

   evas_object_show(img);

   return img;
}

void
viewer_orient_set()
{
   int height, width;

   if (!evas_object_visible_get(_gif_gengrid))
     return;

   system_info_get_platform_int("http://tizen.org/feature/screen.width", &width);
   system_info_get_platform_int("http://tizen.org/feature/screen.height", &height);

   if (orientation == APP_DEVICE_ORIENTATION_90 ||
       orientation == APP_DEVICE_ORIENTATION_270)
      elm_gengrid_item_size_set(_gif_gengrid, height / 3, width / 2);
   else
      elm_gengrid_item_size_set(_gif_gengrid, width / 2, height / 4);
}

void
viewer_open()
{
   int height, width;
   Elm_Gengrid_Item_Class *gic;

   Evas_Object *layout = my_layout_add(_main_naviframe, "edje/gengrid_layout.edj", "main");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   Evas_Object *gengrid = elm_gengrid_add(layout);
   _gif_gengrid = gengrid;
   elm_gengrid_horizontal_set(gengrid, EINA_TRUE);
   evas_object_smart_callback_add(gengrid, "scroll", _scroll_start_cb, NULL);
   evas_object_smart_callback_add(gengrid, "scroll,drag,stop", _scroll_stop_cb, NULL);
   evas_object_smart_callback_add(gengrid, "scroll,anim,stop", _scroll_stop_cb, NULL);
   eext_object_event_callback_add(gengrid, EEXT_CALLBACK_BACK, _layout_back_cb, NULL);

   system_info_get_platform_int("http://tizen.org/feature/screen.width", &width);
   system_info_get_platform_int("http://tizen.org/feature/screen.height", &height);
   if (orientation == APP_DEVICE_ORIENTATION_90 ||
       orientation == APP_DEVICE_ORIENTATION_270)
      elm_gengrid_item_size_set(_gif_gengrid, height / 3, width / 2);
   else
      elm_gengrid_item_size_set(_gif_gengrid, width / 2, height / 4);
   elm_gengrid_align_set(gengrid, 0.0, 0.0);


   gic = elm_gengrid_item_class_new();
   gic->item_style = "type2";
   gic->func.text_get = _grid_text_get;
   gic->func.content_get = _grid_content_get;

   gengrid_item_set(gengrid, "/opt/usr/media/", "gif", gic);
   elm_gengrid_item_class_free(gic);

   if (elm_gengrid_items_count(gengrid) < 1)
      {
         Evas_Object *nocontents = elm_layout_add(layout);
         elm_layout_theme_set(nocontents, "layout", "nocontents", "default");
         evas_object_size_hint_weight_set(nocontents, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
         evas_object_size_hint_align_set(nocontents, EVAS_HINT_FILL, EVAS_HINT_FILL);
         elm_object_part_text_set(nocontents, "elm.text", "No GIFs");
         elm_layout_signal_emit(nocontents, "align.center", "elm");
         elm_object_part_content_set(layout, "gengrid", nocontents);
      }
   else
      elm_object_part_content_set(layout, "gengrid", gengrid);

   evas_object_del(elm_object_content_unset(_main_naviframe));
   elm_object_content_set(_main_naviframe, layout);
}
