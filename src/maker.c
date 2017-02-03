#include <private.h>
#include <view.h>
#include <util.h>

void
img_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
}

static void
_layout_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   ui_app_exit();
}

void
maker_open()
{
   char *img_path;

   Evas_Object *table = elm_table_add(_main_naviframe);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);

   Evas_Object *bg = evas_object_rectangle_add(evas_object_evas_get(table));
   evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bg, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_color_set(bg, 60, 185, 203, 255);
   elm_table_pack(table, bg, 0, 0, 1, 1);
   evas_object_show(bg);

   Evas_Object *box = elm_box_add(table);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   eext_object_event_callback_add(box, EEXT_CALLBACK_BACK, _layout_back_cb, NULL);
   elm_table_pack(table, box, 0, 0, 1, 1);
   evas_object_show(box);

   Evas_Object *img = elm_image_add(box);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, 0.5, 0.8);
   evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(111), ELM_SCALE_SIZE(83));
   evas_object_event_callback_add(img, EVAS_CALLBACK_MOUSE_UP, img_mouse_down_cb, NULL);
   img_path = app_res_path_get("images/img_video_add.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   evas_object_show(img);
   elm_box_pack_end(box, img);

   Evas_Object *label = elm_label_add(box);
   elm_object_text_set(label, "<color=#ffffff>Video to GIF</color>");
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, 0.5, 0.01);
   evas_object_show(label);
   elm_box_pack_end(box, label);

   img = elm_image_add(box);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, 0.5, 0.8);
   evas_object_size_hint_min_set(img, ELM_SCALE_SIZE(111), ELM_SCALE_SIZE(83));
   evas_object_event_callback_add(img, EVAS_CALLBACK_MOUSE_UP, img_mouse_down_cb, NULL);
   img_path = app_res_path_get("images/img_camera.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   evas_object_show(img);
   elm_box_pack_end(box, img);

   label = elm_label_add(box);
   elm_object_text_set(label, "<color=#ffffff>GIF Camera</color>");
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, 0.5, 0.01);
   evas_object_show(label);
   elm_box_pack_end(box, label);

   evas_object_del(elm_object_content_unset(_main_naviframe));
   elm_object_content_set(_main_naviframe, table);
}
