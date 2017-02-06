#include <private.h>
#include <view.h>
#include <util.h>

#include <player.h>
#include <media_content.h>

static player_h player;
static Evas_Object *_main_slider, *_slider_layout, *_play_btn;
static Ecore_Timer *_slider_timer;
static double _start_value, _end_value = 9999999;
static Eina_Bool grag_start;

static void
_player_pause()
{
   player_pause(player);
   ecore_timer_freeze(_slider_timer);

   Evas_Object *img = elm_image_add(_play_btn);
   char *img_path = app_res_path_get("images/ic_play.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   evas_object_del(elm_object_part_content_get(_play_btn, "icon"));
   elm_object_part_content_set(_play_btn, "icon", img);

}

static void
_player_start()
{
   player_start(player);
   ecore_timer_thaw(_slider_timer);

   Evas_Object *img = elm_image_add(_play_btn);
   char *img_path = app_res_path_get("images/ic_pause.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   evas_object_del(elm_object_part_content_get(_play_btn, "icon"));
   elm_object_part_content_set(_play_btn, "icon", img);

}

static void
_box_back_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_naviframe_item_pop(_main_naviframe);
   player_unprepare(player);
   player_destroy(player);
   player = NULL;
}

static void
_end_pick()
{
   Evas_Coord w, h;
   double value;
   int duration;

   evas_object_geometry_get(_main_slider, NULL, NULL, &w, &h);
   player_get_duration(player, &duration);
   value = elm_slider_value_get(_main_slider);
   _end_value = value;

   w *= 1 - (value / (double)duration);

   Evas_Object *pad = evas_object_rectangle_add(evas_object_evas_get(_slider_layout));
   evas_object_size_hint_min_set(pad, w - ELM_SCALE_SIZE(9) * (1 - (value / (double)duration)), 0);
   evas_object_color_set(pad, 249, 249, 249, 255);
   elm_object_part_content_set(_slider_layout, "end_clipper", pad);

   Evas_Object *img = elm_image_add(_slider_layout);
   char *img_path = app_res_path_get("images/ic_point.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   evas_object_size_hint_min_set(img, h / 4, h / 4);
   evas_object_color_set(img, 228, 142, 0, 255);
   elm_object_part_content_set(_slider_layout, "end_point", img);
}

static void
_start_pick()
{
   Evas_Coord w, h;
   double value;
   int duration;

   evas_object_geometry_get(_main_slider, NULL, NULL, &w, &h);
   player_get_duration(player, &duration);
   value = elm_slider_value_get(_main_slider);
   _start_value = value;

   w *= (value / (double)duration);

   Evas_Object *pad = evas_object_rectangle_add(evas_object_evas_get(_slider_layout));
   evas_object_size_hint_min_set(pad, w - ELM_SCALE_SIZE(9) * (value / (double)duration), 0);
   evas_object_color_set(pad, 249, 249, 249, 255);
   elm_object_part_content_set(_slider_layout, "start_clipper", pad);

   Evas_Object *img = elm_image_add(_slider_layout);
   char *img_path = app_res_path_get("images/ic_point.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   evas_object_size_hint_min_set(img, h / 4, h / 4);
   evas_object_color_set(img, 228, 142, 0, 255);
   elm_object_part_content_set(_slider_layout, "start_point", img);
}

static void
_slider_cb_drag_start(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   grag_start = EINA_TRUE;
   _player_pause();
}

static void
_slider_cb_drag_stop(void *data, Evas_Object *obj, void *event_info)
{
   double value, new_value;

   grag_start = EINA_FALSE;

   value = elm_slider_value_get(obj);
   new_value = value;

   if (value < _start_value)
      new_value = _start_value;
   else if (value > _end_value)
      new_value = _end_value;

   elm_slider_value_set(obj, new_value);
   player_set_play_position(player, new_value, true, NULL, NULL);
}

static void
_slider_cb_changed(void *data, Evas_Object *obj, void *event_info)
{
   double value;

   if (!grag_start)
      return;

   value = elm_slider_value_get(obj);

   player_set_play_position(player, value, false, NULL, NULL);
}

static void
_box_pad_add(Evas_Object *box, Evas_Coord size)
{
   Evas_Object *pad = evas_object_rectangle_add(evas_object_evas_get(box));
   evas_object_size_hint_min_set(pad, ELM_SCALE_SIZE(size), ELM_SCALE_SIZE(size));
   evas_object_color_set(pad, 0, 0, 0, 0);
   evas_object_show(pad);
   elm_box_pack_end(box, pad);
}

static Evas_Object *
_gif_maker_slider_add(Evas_Object *parent)
{
   int duration;
   Evas_Object *pad;

   Evas_Object *layout = my_layout_add(parent, "edje/maker_slider.edj", "main");
   _slider_layout = layout;
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);

   pad = evas_object_rectangle_add(evas_object_evas_get(layout));
   evas_object_size_hint_min_set(pad, ELM_SCALE_SIZE(35), ELM_SCALE_SIZE(35));
   evas_object_color_set(pad, 0, 0, 0, 0);
   elm_object_part_content_set(layout, "padding.left", pad);

   pad = evas_object_rectangle_add(evas_object_evas_get(layout));
   evas_object_size_hint_min_set(pad, ELM_SCALE_SIZE(35), ELM_SCALE_SIZE(35));
   evas_object_color_set(pad, 0, 0, 0, 0);
   elm_object_part_content_set(layout, "padding.right", pad);

   Evas_Object *slider = elm_slider_add(layout);
   evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, EVAS_HINT_FILL);
   //evas_object_smart_callback_add(slider, "slider,drag,start", _slider_cb_drag_start, NULL);
   evas_object_smart_callback_add(slider, "slider,drag,stop", _slider_cb_drag_stop, NULL);
   evas_object_event_callback_add(slider, EVAS_CALLBACK_MOUSE_DOWN, _slider_cb_drag_start, NULL);
   //evas_object_event_callback_add(slider, EVAS_CALLBACK_MOUSE_UP, _slider_cb_drag_stop, NULL);
   evas_object_smart_callback_add(slider, "changed", _slider_cb_changed, NULL);
   player_get_duration(player, &duration);
   elm_slider_min_max_set(slider, 0, duration);
   _main_slider = slider;
   evas_object_show(slider);
   elm_object_part_content_set(layout, "slider", slider);

   return layout;
}

static Eina_Bool
_timer_cb_play(void *data)
{
   int ms;
   double value;

   value = elm_slider_value_get(_main_slider);
   player_get_play_position(player, &ms);

   if (value >= _end_value)
      {
         _player_pause();
         ms = _end_value;
      }

   elm_slider_value_set(_main_slider, ms);

   return EINA_TRUE;
}

static void
_btn_cb_play(void *data, Evas_Object *obj, void *event_info)
{
   player_state_e state;

   player_get_state(player, &state);
   if (state == PLAYER_STATE_PLAYING)
       _player_pause();
   else
      _player_start();
}

static void
_btn_cb_pick(void *data, Evas_Object *obj, void *event_info)
{
   const char *text;

   text = elm_object_text_get(obj);

   _player_pause();

   if (!strcmp(text, "Start"))
      {
         elm_object_text_set(obj, "End");
         _start_pick();
      }
   else if (!strcmp(text, "End"))
      {
         elm_object_text_set(obj, "Reset");
         _end_pick();
      }
   else if (!strcmp(text, "Reset"))
      {
         elm_object_text_set(obj, "Start");
         evas_object_del(elm_object_part_content_get(_slider_layout, "end_clipper"));
         evas_object_del(elm_object_part_content_get(_slider_layout, "start_clipper"));
         evas_object_del(elm_object_part_content_get(_slider_layout, "start_point"));
         evas_object_del(elm_object_part_content_get(_slider_layout, "end_point"));
         elm_slider_value_set(_main_slider, 0.0);

         player_set_play_position(player, 0, true, NULL, NULL);
         _end_value = 9999999;
         _start_value = 0;
      }
}

void
gif_maker_open(char *path)
{
   Evas_Object *table = elm_table_add(_main_naviframe);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(table);

   Evas_Object *rect = evas_object_image_filled_add(_main_naviframe);
   evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(rect);
   elm_table_pack(table, rect, 0, 0, 1, 1);

   player_create(&player);
   player_set_uri(player, path);
   player_set_display(player, PLAYER_DISPLAY_TYPE_EVAS, GET_DISPLAY(rect));
   player_prepare(player);
   player_start(player);
   player_pause(player);

   _slider_timer = ecore_timer_add(0.1, _timer_cb_play, NULL);
   ecore_timer_freeze(_slider_timer);

   Evas_Object *box = elm_box_add(table);
   elm_box_padding_set(box, ELM_SCALE_SIZE(10), ELM_SCALE_SIZE(10));
   eext_object_event_callback_add(box, EEXT_CALLBACK_BACK, _box_back_cb, NULL);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, box, 0, 0, 1, 1);
   evas_object_show(box);

   Evas_Object *pad = evas_object_rectangle_add(evas_object_evas_get(box));
   evas_object_size_hint_weight_set(pad, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(pad, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_color_set(pad, 0, 0, 0, 0);
   evas_object_show(pad);
   elm_box_pack_end(box, pad);

   Evas_Object *slider = _gif_maker_slider_add(box);
   elm_box_pack_end(box, slider);

   Evas_Object *btn_box = elm_box_add(_main_naviframe);
   elm_box_padding_set(btn_box, ELM_SCALE_SIZE(10), ELM_SCALE_SIZE(10));
   elm_box_horizontal_set(btn_box, EINA_TRUE);
   evas_object_size_hint_weight_set(btn_box, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(btn_box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(btn_box);
   elm_box_pack_end(box, btn_box);

   _box_pad_add(btn_box, 5);

   Evas_Object *btn_left = elm_button_add(btn_box);
   elm_object_style_set(btn_left, "bottom");
   elm_object_text_set(btn_left, "Start");
   evas_object_size_hint_weight_set(btn_left, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn_left, EVAS_HINT_FILL, 0.5);
   evas_object_show(btn_left);
   elm_box_pack_end(btn_box, btn_left);

   Evas_Object *btn = elm_button_add(btn_box);
   _play_btn = btn;
   elm_object_style_set(btn, "bottom");
   evas_object_smart_callback_add(btn, "clicked", _btn_cb_play, NULL);
   evas_object_smart_callback_add(btn_left, "clicked", _btn_cb_pick, btn);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, 0.5, 0.5);
   evas_object_show(btn);
   elm_box_pack_end(btn_box, btn);

   Evas_Object *img = elm_image_add(btn);
   char *img_path = app_res_path_get("images/ic_play.png");
   elm_image_file_set(img, img_path, NULL);
   free(img_path);
   elm_object_part_content_set(btn, "icon", img);

   btn = elm_button_add(btn_box);
   elm_object_style_set(btn, "bottom");
   elm_object_text_set(btn, "Make!");
   //evas_object_smart_callback_add(btn, "clicked", btn_clicked_cb, NULL);
   evas_object_size_hint_weight_set(btn, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(btn, EVAS_HINT_FILL, 0.5);
   evas_object_show(btn);
   elm_box_pack_end(btn_box, btn);

   _box_pad_add(btn_box, 5);

   _box_pad_add(box, 10);

   elm_naviframe_item_simple_push(_main_naviframe, table);
}
