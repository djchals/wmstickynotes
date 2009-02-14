/*
 * $Id: $
 *
 * Heath Caldwell <hncaldwell@gmail.com>
 *
 */

/* http://mail.gnome.org/archives/gtk-list/2000-January/msg00072.html */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

#include "wmstickynotes.h"
#include "delete_button.xpm"
#include "resize_button.xpm"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/extensions/shape.h>

/* The place under HOME to store notes */
char *wmstickynotes_dir = ".wmstickynotes";

GdkColormap *colormap;

/* The highest note id used so far (this is used when making a new note so
 * that no ids are clobbered */
long int highest_note_id = 0;



void delete_note(GtkWidget *widget, Note *note)
{
	char *filename;
	asprintf(&filename, "%d", note->id);
	unlink(filename);
	free(note);
}

void save_note(GtkWidget *widget, Note *note)
{
	FILE *file;
	char *filename;
	GtkTextIter start;
	GtkTextIter end;

	gchar *text;

	gtk_text_buffer_get_start_iter(note->text_buffer, &start);
	gtk_text_buffer_get_end_iter(note->text_buffer, &end);

	text = gtk_text_buffer_get_text(note->text_buffer, &start, &end, FALSE);

	asprintf(&filename, "%d", note->id);
	file = fopen(filename, "w");
	free(filename);

	fprintf(file, "%d,%d,%d,%d,%d\n%s", note->x, note->y, note->width, note->height, note->color, text);
	fclose(file);

	g_free(text);
}

gboolean note_configure_event(GtkWidget *window, GdkEventConfigure *event, Note *note)
{
	note->x = event->x;
	note->y = event->y;
	note->width = event->width;
	note->height = event->height;
	save_note(window, note);
	return FALSE;
}

void bar_pressed(GtkWidget *widget, GdkEventButton *event, Note *note)
{
	gtk_window_begin_move_drag(GTK_WINDOW(note->window), event->button, event->x_root, event->y_root, event->time);
}

void resize_button_pressed(GtkWidget *widget, GdkEventButton *event, Note *note)
{
	gtk_window_begin_resize_drag(GTK_WINDOW(note->window), GDK_WINDOW_EDGE_SOUTH_EAST, event->button, event->x_root, event->y_root, event->time);
}

void delete_button_pressed(GtkWidget *widget, GdkEventButton *event, GtkWidget *window)
{
	if(event->button != 1) return;

	gtk_widget_destroy(window);
}

void create_note(Note *old_note, int color)
{
	GtkWidget *window;
	GtkWidget *text_widget;
	GtkWidget *vbox;
	GtkWidget *top_hbox;
	GtkWidget *mid_hbox;
	GtkWidget *bottom_bar;
	GtkWidget *bottom_hbox;
	GtkWidget *top_bar;
	GtkWidget *top_bar_box;
	GtkWidget *delete_button;
	GtkWidget *delete_button_box;
	GdkPixmap *delete_button_pixmap;
	GdkBitmap *delete_button_mask;
	GtkWidget *resize_button;
	GtkWidget *resize_button_box;
	GdkPixmap *resize_button_pixmap;
	GdkBitmap *resize_button_mask;
	GdkGC *gc;
	GdkColor gcolor;

	Note *note;
	
	note = old_note ? old_note : malloc(sizeof(Note));

	if(!old_note) {
		highest_note_id++;
		note->id = highest_note_id;
		note->color = color;
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_default_size(GTK_WINDOW(window), 150, 150);

	text_widget = gtk_text_view_new_with_buffer(old_note ? old_note->text_buffer : NULL);
	note->text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_widget));

	note->window = window;

	vbox = gtk_vbox_new(FALSE, 0);
	top_hbox = gtk_hbox_new(FALSE, 0);
	mid_hbox = gtk_hbox_new(FALSE, 0);
	bottom_hbox = gtk_hbox_new(FALSE, 0);
	top_bar = gtk_label_new("");
	top_bar_box = gtk_event_box_new();
	gtk_widget_set_size_request(top_bar, -1, 10);
	bottom_bar = gtk_label_new("");
	gtk_widget_set_size_request(bottom_bar, -1, 8);

	delete_button_pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &delete_button_mask, NULL, delete_button_xpm);
	delete_button = gtk_image_new_from_pixmap(delete_button_pixmap, delete_button_mask);
	delete_button_box = gtk_event_box_new();
	gtk_widget_set_size_request(delete_button_box, 10, 10);

	resize_button_pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &resize_button_mask, NULL, resize_button_xpm);
	resize_button = gtk_image_new_from_pixmap(resize_button_pixmap, resize_button_mask);
	resize_button_box = gtk_event_box_new();

	gdk_color_parse(color_schemes[note->color].top, &gcolor);
	gtk_widget_modify_bg(top_bar_box, GTK_STATE_NORMAL, &gcolor);
	gtk_widget_modify_bg(delete_button_box, GTK_STATE_NORMAL, &gcolor);
	gdk_color_parse(color_schemes[note->color].background, &gcolor);
	gtk_widget_modify_base(text_widget, GTK_STATE_NORMAL, &gcolor);
	gtk_widget_modify_bg(window, GTK_STATE_NORMAL, &gcolor);
	gtk_widget_modify_bg(resize_button_box, GTK_STATE_NORMAL, &gcolor);

	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_container_add(GTK_CONTAINER(top_bar_box), top_bar);
	gtk_container_add(GTK_CONTAINER(delete_button_box), delete_button);
	gtk_container_add(GTK_CONTAINER(resize_button_box), resize_button);
	gtk_box_pack_start(GTK_BOX(top_hbox), top_bar_box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(top_hbox), delete_button_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mid_hbox), text_widget, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(bottom_hbox), bottom_bar, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(bottom_hbox), resize_button_box, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), top_hbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), mid_hbox, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), bottom_hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(window);

	if(old_note) {
		gtk_window_resize(GTK_WINDOW(window), old_note->width, old_note->height);
		gtk_window_move(GTK_WINDOW(window), old_note->x, old_note->y);
	} else {
		gtk_window_get_position(GTK_WINDOW(window), &(note->x), &(note->y));
		gtk_window_get_size(GTK_WINDOW(window), &(note->width), &(note->height));
	}

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(delete_note), note);
	g_signal_connect(G_OBJECT(window), "configure-event", G_CALLBACK(note_configure_event), note);
	g_signal_connect(G_OBJECT(delete_button_box), "button-press-event", G_CALLBACK(delete_button_pressed), window);
	g_signal_connect(G_OBJECT(resize_button_box), "button-press-event", G_CALLBACK(resize_button_pressed), note);
	g_signal_connect(G_OBJECT(note->text_buffer), "changed", G_CALLBACK(save_note), note);
	g_signal_connect(G_OBJECT(top_bar_box), "button-press-event", G_CALLBACK(bar_pressed), note);
}

void new_note_button_clicked(GtkButton *button, char *color)
{
	int c;

	for(c=0; c<=5; c++) {
		if(!strcmp(color_schemes[c].name, color)) break;
	}
	if(strcmp(color_schemes[c].name, color)) c = 0;

	create_note(NULL, c);
}

void read_old_notes()
{
	Note *note;
	GtkTextIter iter;
	DIR *dir = opendir(".");
	FILE *file;
	struct dirent *entry;
	int i;
	char buffer[256];

	rewinddir(dir);
	while((entry = readdir(dir)) != NULL) {
		/* Check if it is a valid note name */
		for(i=0; entry->d_name[i]; i++) {
			if(entry->d_name[i] < '0' || entry->d_name[i] > '9') break;
		}
		if(i < strlen(entry->d_name)) continue;

		file = fopen(entry->d_name, "r");
		note = malloc(sizeof(Note));

		note->id = atoi(entry->d_name);
		if(note->id > highest_note_id) highest_note_id = note->id;

		fscanf(file, "%d,%d,%d,%d,%d\n", &(note->x), &(note->y), &(note->width), &(note->height), &(note->color));
		if(note->color > 5) note->color = 0;

		note->text_buffer = gtk_text_buffer_new(NULL);
		while(fgets(buffer, 256, file)) {
			gtk_text_buffer_get_end_iter(note->text_buffer, &iter);
			gtk_text_buffer_insert(note->text_buffer, &iter, buffer, -1);
		}

		create_note(note, 0);

		fclose(file);
	}

	closedir(dir);
}

int main(int argc, char *argv[])
{
	GtkWidget *window;
	GtkWidget *box;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *yellow_button;
	GtkWidget *green_button;
	GtkWidget *orange_button;
	GtkWidget *red_button;
	GtkWidget *blue_button;
	GtkWidget *white_button;
	GdkColor color;
	XWMHints mywmhints;
	char *dir;

	gtk_init(&argc, &argv);

	colormap = gdk_colormap_new(gdk_visual_get_system(), TRUE);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 64, 64);

	box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER (window), box);

	vbox = gtk_vbox_new(FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 0);

	table = gtk_table_new(2, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 1);
	gtk_table_set_col_spacings(GTK_TABLE(table), 1);

	yellow_button = gtk_button_new();
	green_button = gtk_button_new();
	orange_button = gtk_button_new();
	red_button = gtk_button_new();
	blue_button = gtk_button_new();
	white_button = gtk_button_new();

	gdk_color_parse (color_schemes[0].top, &color);
	gtk_widget_modify_bg(yellow_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg(yellow_button, GTK_STATE_PRELIGHT, &color);
	gdk_color_parse (color_schemes[1].top, &color);
	gtk_widget_modify_bg(green_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg(green_button, GTK_STATE_PRELIGHT, &color);
	gdk_color_parse (color_schemes[2].top, &color);
	gtk_widget_modify_bg(orange_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg(orange_button, GTK_STATE_PRELIGHT, &color);
	gdk_color_parse (color_schemes[3].top, &color);
	gtk_widget_modify_bg(red_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg(red_button, GTK_STATE_PRELIGHT, &color);
	gdk_color_parse (color_schemes[4].top, &color);
	gtk_widget_modify_bg(blue_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg(blue_button, GTK_STATE_PRELIGHT, &color);
	gdk_color_parse (color_schemes[5].background, &color);
	gtk_widget_modify_bg(white_button, GTK_STATE_NORMAL, &color);
	gtk_widget_modify_bg(white_button, GTK_STATE_PRELIGHT, &color);

	gtk_table_attach_defaults(GTK_TABLE(table), yellow_button, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), green_button, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), orange_button, 2, 3, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), red_button, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), blue_button, 1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), white_button, 2, 3, 1, 2);

	gdk_color_parse ("#fafafa", &color);
	gtk_widget_modify_bg(box, GTK_STATE_NORMAL, &color);

	gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 4);
	gtk_container_add(GTK_CONTAINER(box), vbox);

	gtk_widget_show_all(window);

	mywmhints.initial_state = WithdrawnState;
	mywmhints.icon_window = GDK_WINDOW_XWINDOW(box->window);
	mywmhints.icon_x = 0; 
	mywmhints.icon_y = 0; 
	mywmhints.window_group = GDK_WINDOW_XWINDOW(window->window);
	mywmhints.flags = StateHint | IconWindowHint | IconPositionHint | WindowGroupHint;

	XSetWMHints(GDK_DISPLAY(), GDK_WINDOW_XWINDOW(window->window), &mywmhints);

	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(G_OBJECT(yellow_button), "clicked", G_CALLBACK(new_note_button_clicked), "yellow");
	g_signal_connect(G_OBJECT(green_button), "clicked", G_CALLBACK(new_note_button_clicked), "green");
	g_signal_connect(G_OBJECT(orange_button), "clicked", G_CALLBACK(new_note_button_clicked), "orange");
	g_signal_connect(G_OBJECT(red_button), "clicked", G_CALLBACK(new_note_button_clicked), "red");
	g_signal_connect(G_OBJECT(blue_button), "clicked", G_CALLBACK(new_note_button_clicked), "blue");
	g_signal_connect(G_OBJECT(white_button), "clicked", G_CALLBACK(new_note_button_clicked), "white");

	umask(077);

	dir = calloc(strlen(wmstickynotes_dir) + strlen(getenv("HOME")) + 2, sizeof(char));
	strcpy(dir, getenv("HOME"));
	strcat(dir, "/");
	strcat(dir, wmstickynotes_dir);

	if(chdir(dir)) {
		if(errno == ENOENT) {
			if(mkdir(dir, 0777)) {
				fprintf(stderr, "Couldn't make directory: %s\n", dir);
				exit(1);
			}
			if(chdir(dir)) {
				fprintf(stderr, "Couldn't change to directory: %s\n", dir);
				exit(1);
			}
		} else {
			fprintf(stderr, "Couldn't change to directory: %s\n", dir);
			exit(1);
		}
	}

	free(dir);

	read_old_notes();
	gtk_main();

	return 0;
}

