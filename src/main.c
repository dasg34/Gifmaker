#include <private.h>
#include <view.h>
#include <util.h>

Evas_Object *_main_naviframe, *_main_layout;
app_device_orientation_e orientation;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

void
_maker_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   maker_open();
}

void
_viewer_select_cb(void *data, Evas_Object *obj, void *event_info)
{
   viewer_open();
}

static void
create_base_gui()
{
   char *img_path;

	Evas_Object *win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_conformant_set(win, EINA_TRUE);
	elm_win_autodel_set(win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(win, "delete,request", win_delete_request_cb, NULL);

	/* Conformant */
	Evas_Object *conform = elm_conformant_add(win);
	evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, conform);
	evas_object_show(conform);

	/* Indicator */
   elm_win_indicator_mode_set(win, ELM_WIN_INDICATOR_SHOW);
   elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_TRANSPARENT);

   /* Naviframe */
   Evas_Object *naviframe = elm_naviframe_add(conform);
   _main_naviframe = naviframe;
   evas_object_size_hint_weight_set(naviframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_content_set(conform, naviframe);
   evas_object_show(naviframe);

	/* toolbar */
	Evas_Object *toolbar = elm_toolbar_add(naviframe);
   elm_object_style_set(toolbar, "tabbar");
   elm_toolbar_select_mode_set(toolbar, ELM_OBJECT_SELECT_MODE_ALWAYS);
   elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_EXPAND);
   elm_toolbar_transverse_expanded_set(toolbar, EINA_TRUE);
   evas_object_show(toolbar);

#define TOOLBAR_ITEM_ADD(name, path, cb, data) \
      img_path = app_res_path_get(path); \
      elm_toolbar_item_append(toolbar, img_path, name, cb, data); \
      free(img_path);

   TOOLBAR_ITEM_ADD("Maker", "images/ic_maker.png", _maker_select_cb, NULL);
   TOOLBAR_ITEM_ADD("Viewer", "images/ic_gif.png", _viewer_select_cb, NULL);
   //TOOLBAR_ITEM_ADD("Settings", "images/ic_setting.png", NULL, NULL);
   //TOOLBAR_ITEM_ADD("Video", "images/ic_video.png", _video_select_cb, NULL);
   //TOOLBAR_ITEM_ADD("Camera", "images/ic_camera.png", NULL, NULL);


   Elm_Object_Item *nf_it = elm_naviframe_item_push(naviframe, NULL, NULL, NULL, NULL, "tabbar/icon/notitle");
   elm_naviframe_item_style_set(nf_it, "tabbar/icon/notitle");
   elm_object_item_part_content_set(nf_it, "tabbar", toolbar);
   maker_open();

	/* Show window after base gui is set up */
	evas_object_show(win);
}

static bool
app_create(void *data)
{
   elm_config_accel_preference_set("gl");

	create_base_gui();

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/

	int ret;
	char *language;

	ret = app_event_get_language(event_info, &language);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_event_get_language() failed. Err = %d.", ret);
		return;
	}

	if (language != NULL) {
		elm_language_set(language);
		free(language);
	}
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
   app_event_get_device_orientation(event_info, &orientation);
   viewer_orient_set();
   video_picker_orient_set();

	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, NULL);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, NULL);

	ret = ui_app_main(argc, argv, &event_callback, NULL);
	if (ret != APP_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "ui_app_main() is failed. err = %d", ret);

	return ret;
}
