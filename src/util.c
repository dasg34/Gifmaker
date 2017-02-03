#include <private.h>

#include <mime_type.h>

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
_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   char *path = data;
   app_control_h app_control;
   Elm_Object_Item *it = event_info;

   elm_gengrid_item_selected_set(it, EINA_FALSE);

   app_control_create(&app_control);
   app_control_set_uri(app_control, path);
   app_control_set_operation(app_control, APP_CONTROL_OPERATION_VIEW);
   app_control_send_launch_request(app_control, NULL, NULL);
}

void
gengrid_item_set(Evas_Object *grid, char *path, const char *type, Elm_Gengrid_Item_Class *gic)
{
   char sub_path[PATH_MAX];
   struct dirent *pDirent = NULL;
   DIR *dir = opendir(path);
   char *mime_type, *str_temp;
   if (!dir)
     return;

   while ((pDirent = readdir(dir)) != NULL)
      {
         if (pDirent->d_name[0] == '.')
            continue;

         snprintf(sub_path, PATH_MAX, "%s/%s", path, pDirent->d_name);

         char *extension = strrchr(pDirent->d_name, '.');
         if (extension)
            {
               extension++;
               mime_type_get_mime_type(extension, &mime_type);

               if (strstr(mime_type, type))
                  {
                     str_temp = strdup(sub_path);
                     elm_gengrid_item_append(grid, gic, str_temp, _select_cb, str_temp);
                  }
               free(mime_type);
            }
         gengrid_item_set(grid, sub_path, type, gic);

      }
   closedir(dir);
}
