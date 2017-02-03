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

extern Evas_Object *_main_naviframe, *_main_layout;

extern app_device_orientation_e orientation;


#endif /* __gifmaker_H__ */
