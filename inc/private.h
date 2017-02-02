#ifndef __gifmaker_H__
#define __gifmaker_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "gifmaker"

#if !defined(PACKAGE)
#define PACKAGE "org.yohoho.gifmaker"
#endif


char *app_res_path_get(const char *res_name);

Evas_Object *my_layout_add(Evas_Object *parent, const char *edj_name, const char *group);

#endif /* __gifmaker_H__ */
