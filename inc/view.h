#ifndef VIEW_H_
#define VIEW_H_


void maker_open();

void viewer_open(Eina_Bool new_win);

void gengrid_item_set(Evas_Object *grid, const char *type);

void video_picker_open();

void gif_maker_open(char *path);

void gif_camera_open();

#endif /* VIEW_H_ */
