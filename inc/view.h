#ifndef VIEW_H_
#define VIEW_H_


void maker_open();

void viewer_open();

void gengrid_item_set(Evas_Object *grid, const char *type);

void viewer_orient_set();

void video_picker_orient_set();

void video_picker_open();

void gif_maker_open(char *path);

#endif /* VIEW_H_ */
