#include <private.h>
#include <view.h>

#include <mime_type.h>
#include <media_content.h>

static void (*_select_cb)(void *data, Evas_Object *obj, void *event_info);

char *
app_res_path_get(const char *res_name)
{
   char *res_path, *path;
   int pathlen;

   res_path = app_get_resource_path();
   if (!res_path)
      {
         dlog_print(DLOG_ERROR, LOG_TAG, "res_path_get ERROR!");
         return NULL;
      }

   pathlen = strlen(res_name) + strlen(res_path) + 1;
   path = malloc(sizeof(char) * pathlen);
   snprintf(path, pathlen, "%s%s", res_path, res_name);
   free(res_path);

   return path;
}

Evas_Object *
my_layout_add(Evas_Object *parent, const char *edj_name, const char *group)
{
   Evas_Object *layout;
   char *path;

   path = app_res_path_get(edj_name);

   layout = elm_layout_add(parent);
   elm_layout_file_set(layout, path, group);
   free(path);

   return layout;
}

static void
_gif_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   char *path;
   media_info_h media = data;
   app_control_h app_control;
   Elm_Object_Item *it = event_info;

   elm_gengrid_item_selected_set(it, EINA_FALSE);

   media_info_get_file_path(media, &path);

   app_control_create(&app_control);
   app_control_set_uri(app_control, path);
   app_control_set_operation(app_control, APP_CONTROL_OPERATION_VIEW);
   app_control_send_launch_request(app_control, NULL, NULL);
   free(path);
}

static void
_video_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   char *path;
   media_info_h media;
   Elm_Object_Item *it = event_info;

   elm_gengrid_item_selected_set(it, EINA_FALSE);

   media = elm_object_item_data_get(it);
   media_info_get_file_path(media, &path);
   gif_maker_open(path);
   free(path);
}

static bool
_media_item_cb(media_info_h media, void *data)
{
   Evas_Object *grid = data;
   Elm_Gengrid_Item_Class *gic;
   media_info_h new_media;

   media_info_clone(&new_media, media);

   gic = evas_object_data_get(grid, "item_class");

   elm_gengrid_item_append(grid, gic, new_media, _select_cb, new_media);

   return true;
}

void
gengrid_item_set(Evas_Object *grid, const char *type)
{
   filter_h filter = NULL;

   media_content_connect();
   media_filter_create(&filter);

   if (!strcmp(type, "gif"))
      {
         media_filter_set_condition(filter, "MEDIA_TYPE = 0 and MEDIA_MIME_TYPE = 'image/gif'", MEDIA_CONTENT_COLLATE_NOCASE);
         _select_cb = _gif_select_cb;
      }
   else if (!strcmp(type, "video"))
      {
         media_filter_set_condition(filter, "MEDIA_TYPE = 1", MEDIA_CONTENT_COLLATE_NOCASE);
         _select_cb = _video_select_cb;
      }

   media_filter_set_order(filter, MEDIA_CONTENT_ORDER_DESC, MEDIA_ADDED_TIME, MEDIA_CONTENT_COLLATE_NOCASE);
   media_info_foreach_media_from_db(filter, _media_item_cb, grid);

   media_filter_destroy(filter);
   media_content_disconnect();
}
