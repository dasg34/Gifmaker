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

#define DEBUG 1

#if DEBUG == 1
#define dlog_print(...) ;
#endif

extern Evas_Object *_main_naviframe, *_main_layout, *_win;

#endif /* __gifmaker_H__ */
