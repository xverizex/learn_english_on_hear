#include <gtk/gtk.h>
#include <gst/gst.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

GtkApplication *app;

struct widgets {
	GtkWidget *window_main;
	GtkWidget *box_main;
	GtkWidget *text_view_main;
	GtkWidget *button_next;
	GtkWidget *button_prev;
	GtkWidget *button_play;
	GtkWidget *box_buttons;
	GtkWidget *header;
} w;

struct gst {
	GstElement *pipeline;
	GstElement *source;
	GstElement *demuxer;
	GstElement *decoder;
	GstElement *volume;
	GstElement *conv;
	GstElement *sink;
	GstBus *bus;
} gst;

int current_index = 1;

static void button_current_new_message_clicked_cb ( GtkButton *button, gpointer data ) {
}

char *path;
char buffer_text[255];
char buffer_sound[255];

static void load_words ( ) {
start:
	snprintf ( buffer_text, 255, "%s/%d.txt", path, current_index );
	snprintf ( buffer_sound, 255, "%s/%d.mp3", path, current_index );

	struct stat st;
	stat ( buffer_text, &st );
	FILE *fp = fopen ( buffer_text, "r" );
	if ( !fp ) {
		current_index = 1;
		goto start;
	}
	char *buf = (char *) calloc ( st.st_size + 1, 1 );
	fread ( buf, 1, st.st_size, fp );
	fclose ( fp );

	GtkTextBuffer *buffer = gtk_text_view_get_buffer ( ( GtkTextView * ) w.text_view_main );
	gtk_text_buffer_set_text ( buffer, buf, -1 );
}

static void action_activate_select_dir ( GSimpleAction *simple, GVariant *parameter, gpointer data ) {
	GtkWidget *dialog;
	GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
	int res;

	dialog = gtk_file_chooser_dialog_new ( "Open sound file",
			( GtkWindow * ) w.window_main,
			action,
			"Cancel",
			GTK_RESPONSE_CANCEL,
			"Open",
			GTK_RESPONSE_ACCEPT,
			NULL );

	res = gtk_dialog_run ( ( GtkDialog * ) dialog );
	if ( res == GTK_RESPONSE_ACCEPT ) {
		char *filename;
		GtkFileChooser *chooser = GTK_FILE_CHOOSER ( dialog );
		filename = gtk_file_chooser_get_filename ( chooser );
		current_index = 0;
		if ( path ) g_free ( path );
		path = g_strdup ( filename );
		load_words ( );
		g_free ( filename );
	}

	gtk_widget_destroy ( dialog );
}
static void action_activate_select_quit ( GSimpleAction *simple, GVariant *parameter, gpointer data ) {
	exit ( EXIT_SUCCESS );
}

static void create_actions ( ) {
	const GActionEntry entries[] = {
		{ "select_dir", action_activate_select_dir },
		{ "select_quit", action_activate_select_quit }
	};

	g_action_map_add_action_entries ( G_ACTION_MAP ( app ), entries, G_N_ELEMENTS ( entries ), NULL );
}

static void gst_demuxer_pad_added_cb ( GstElement *element, GstPad *pad, gpointer data ) {
	GstPad *sinkpad;
	sinkpad = gst_element_get_static_pad ( gst.decoder, "sink" );
	gst_pad_link ( pad, sinkpad );
	gst_object_unref ( sinkpad );
}


static void cb_message ( GstBus *bus, GstMessage *msg, gpointer data ) {
	switch ( GST_MESSAGE_TYPE ( msg ) ) {
		case GST_MESSAGE_TAG: {
				      }
				      break;
		case GST_MESSAGE_ERROR: {
					}
					break;
	}
}

static void init_sound ( ) {
	gst_init ( NULL, NULL );
	gst.pipeline = gst_pipeline_new ( "radio" );
	gst.source = gst_element_factory_make ( "filesrc", NULL );
	gst.demuxer = gst_element_factory_make ( "decodebin", NULL );
	gst.decoder = gst_element_factory_make ( "audioconvert", NULL );
	gst.volume = gst_element_factory_make ( "volume", NULL );
	gst.conv = gst_element_factory_make ( "audioconvert", NULL );
	gst.sink = gst_element_factory_make ( "autoaudiosink", NULL );

	gst_bin_add_many ( GST_BIN ( gst.pipeline ),
			gst.source,
			gst.demuxer,
			gst.decoder,
			gst.volume,
			gst.conv,
			gst.sink,
			NULL
			);

	gst_element_link_many ( gst.decoder, gst.volume, gst.conv, gst.sink, NULL );
	gst_element_link ( gst.source, gst.demuxer );
	g_signal_connect ( gst.demuxer, "pad-added", G_CALLBACK ( gst_demuxer_pad_added_cb ), NULL );

	g_object_set ( gst.volume, "volume", 1.0, NULL );

	gst.bus = gst_pipeline_get_bus ( GST_PIPELINE ( gst.pipeline ) );
	gst_bus_add_signal_watch ( gst.bus );

	g_signal_connect ( gst.bus, "message", G_CALLBACK ( cb_message ), NULL );
}

static void button_next_clicked_cb ( GtkButton *button, gpointer data ) {
	current_index++;
	load_words();
}
static void button_prev_clicked_cb ( GtkButton *button, gpointer data ) {
	current_index--;
	load_words();
}

static void button_play_clicked_cb ( GtkButton *button, gpointer data ) {
	gst_element_set_state ( gst.pipeline, GST_STATE_READY );
	gst_element_seek_simple ( gst.pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, 0L );
	g_object_set ( gst.source, "location", buffer_sound, NULL );
	gst_element_set_state ( gst.pipeline, GST_STATE_PLAYING );
}

static void app_activate_cb ( GtkApplication *app, gpointer data ) {
	w.window_main = gtk_application_window_new ( app );
	w.box_main = gtk_box_new ( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_container_add ( ( GtkContainer * ) w.window_main, w.box_main );
	gtk_window_set_default_size ( ( GtkWindow * ) w.window_main, 500, 600 );
	
	create_actions ( );

	GMenu *menu_app = g_menu_new ( );
	g_menu_append ( menu_app, "Выбрать каталог", "app.select_dir" );
	g_menu_append ( menu_app, "Выход", "app.select_quit" );

	gtk_application_set_app_menu ( app, ( GMenuModel * ) menu_app );

	w.header = gtk_header_bar_new ( );
	gtk_header_bar_set_decoration_layout ( ( GtkHeaderBar * ) w.header, ":menu,minimize,maximize,close" );
	gtk_header_bar_set_show_close_button ( ( GtkHeaderBar * ) w.header, TRUE );
	gtk_window_set_titlebar ( ( GtkWindow * ) w.window_main, w.header );

	w.text_view_main = gtk_text_view_new ( );
	w.button_play = gtk_button_new_from_icon_name ( "gtk-media-play-ltr", GTK_ICON_SIZE_DIALOG );
	g_signal_connect ( w.button_play, "clicked", G_CALLBACK ( button_play_clicked_cb ), NULL );
	gtk_widget_set_margin_top ( w.text_view_main, 32 );
	gtk_widget_set_margin_start ( w.text_view_main, 32 );
	gtk_widget_set_margin_end ( w.text_view_main, 32 );
	gtk_widget_set_margin_bottom ( w.text_view_main, 32 );

	gtk_widget_set_margin_start ( w.button_play, 32 );
	gtk_widget_set_margin_end ( w.button_play, 32 );
	gtk_widget_set_margin_bottom ( w.button_play, 32 );

	w.button_next = gtk_button_new_with_label ( "Next" );
	g_signal_connect ( w.button_next, "clicked", G_CALLBACK ( button_next_clicked_cb ), NULL );
	w.button_prev = gtk_button_new_with_label ( "Prev" );
	g_signal_connect ( w.button_prev, "clicked", G_CALLBACK ( button_prev_clicked_cb ), NULL );

	w.box_buttons = gtk_box_new ( GTK_ORIENTATION_HORIZONTAL, 0 );
	gtk_box_pack_end ( ( GtkBox * ) w.box_buttons, w.button_next, FALSE, FALSE, 0 );
	gtk_box_pack_end ( ( GtkBox * ) w.box_buttons, w.button_prev, FALSE, FALSE, 0 );
	gtk_widget_set_margin_start ( w.box_buttons, 32 );
	gtk_widget_set_margin_end ( w.box_buttons, 32 );
	gtk_widget_set_margin_bottom ( w.box_buttons, 16 );

	gtk_box_pack_start ( ( GtkBox * ) w.box_main, w.text_view_main, TRUE, TRUE, 0 );
	gtk_box_pack_start ( ( GtkBox * ) w.box_main, w.button_play, FALSE, FALSE, 0 );
	gtk_box_pack_end ( ( GtkBox * ) w.box_main, w.box_buttons, FALSE, FALSE, 0 );

	init_sound ( );

	gtk_widget_show_all ( w.window_main );
}

int main ( int argc, char **argv ) {
	app = gtk_application_new ( "org.xverizex.english_tutor", G_APPLICATION_FLAGS_NONE );
	g_application_register ( ( GApplication * ) app, NULL, NULL );
	g_signal_connect ( app, "activate", G_CALLBACK ( app_activate_cb ), NULL );
	return g_application_run ( ( GApplication * ) app, argc, argv );
}
