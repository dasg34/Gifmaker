#include <private.h>

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
