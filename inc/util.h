#ifndef UTIL_H_
#define UTIL_H_


char *app_res_path_get(const char *res_name);

Evas_Object *my_layout_add(Evas_Object *parent, const char *edj_name, const char *group);

void make_gif(char *path);

void gif_recorder_unprepare();

void gif_recorder_prepare();

#endif /* UTIL_H_ */
