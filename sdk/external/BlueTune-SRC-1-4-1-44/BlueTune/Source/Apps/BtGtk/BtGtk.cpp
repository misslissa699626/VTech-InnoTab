/*****************************************************************
|
|      File: BltGtk.cpp
|
|      BlueTune - GTK GUI
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Atomix.h"
#include "Neptune.h"
#include "BlueTune.h"

/*----------------------------------------------------------------------
|    BtPlayer
+---------------------------------------------------------------------*/
class BtPlayer : public BLT_Player
{
public:
    // methods
    BtPlayer(NPT_SelectableMessageQueue* queue);
    
    // BLT_DecoderClient methods
    void OnDecoderStateNotification(BLT_DecoderServer::State state);
    void OnStreamTimeCodeNotification(BLT_TimeCode time_code);
    void OnStreamPositionNotification(BLT_StreamPosition& position);
    void OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info);

    // members
    NPT_SelectableMessageQueue* m_Queue;
    GtkWidget*                  m_MainWindow;
    GtkWidget*                  m_MainMenu;
    GtkWidget*                  m_PlayButton;
    GtkWidget*                  m_PauseButton;
    GtkWidget*                  m_StopButton;
    GtkWidget*                  m_InputNameEntry;
    GtkWidget*                  m_SetInputButton;
    GtkWidget*                  m_TrackSlider;
    GtkAdjustment*              m_TrackAdjustment;
    GtkWidget*                  m_TimeCodeLabel;
    GtkWidget*                  m_DecoderStateLabel;
    GtkWidget*                  m_PropertyList;
    GtkWidget*                  m_FileDialog;
    bool                        m_TrackSliderIsActive;
};

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
const int BT_PLAYER_TRACK_SLIDER_RANGE   = 100;
const int BT_PLAYER_TRACK_SLIDER_WIDTH   = 200;
const int BT_PLAYER_TRACK_SLIDER_HEIGHT  = 10;

/*----------------------------------------------------------------------
|       handle_delete_event
+---------------------------------------------------------------------*/
static gint 
handle_delete_event(GtkWidget* widget,
		    GdkEvent*  event,
		    gpointer   data)
{
    BLT_COMPILER_UNUSED(widget);
    BLT_COMPILER_UNUSED(event);
    BLT_COMPILER_UNUSED(data);

    return TRUE;
}

/*----------------------------------------------------------------------
|       handle_destroy_event
+---------------------------------------------------------------------*/
static gint
handle_destroy_event(GtkWidget* widget, 
                     gpointer   data)
{
    BLT_COMPILER_UNUSED(widget);
    BLT_COMPILER_UNUSED(data);

    gtk_main_quit();

    return TRUE;
}

/*----------------------------------------------------------------------
|       handle_file_ok_callback
+---------------------------------------------------------------------*/
static void 
handle_file_ok_callback(GtkWidget *widget,
			gpointer   data)
{
    BtPlayer* player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    const char* filename;    
    filename = gtk_file_selection_get_filename(
			GTK_FILE_SELECTION(player->m_FileDialog));
    player->SetInput(filename);
    player->Play();

    if (player->m_FileDialog) {
        gtk_widget_destroy(player->m_FileDialog);
        player->m_FileDialog = NULL;
    }
}

/*----------------------------------------------------------------------
|       handle_file_cancel_callback
+---------------------------------------------------------------------*/
static void 
handle_file_cancel_callback(GtkWidget* widget,
			    gpointer   data)
{
    BtPlayer *player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    if (player->m_FileDialog) {
        gtk_widget_destroy(player->m_FileDialog);
        player->m_FileDialog = NULL;
    }
}

/*----------------------------------------------------------------------
|       handle_track_slider_button_pressed
+---------------------------------------------------------------------*/
static gint 
handle_track_slider_button_pressed(GtkWidget*      widget,
                                   GdkEventButton* event,
                                   gpointer        data)
{
    BtPlayer *player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);
    BLT_COMPILER_UNUSED(event);

    // we have started dragging the slider
    player->m_TrackSliderIsActive = true;

    return FALSE;
}

/*----------------------------------------------------------------------
|       handle_track_slider_button_released
+---------------------------------------------------------------------*/
static gint
handle_track_slider_button_released(GtkWidget*      widget,
                                    GdkEventButton* event,
                                    gpointer        data)
{
    BtPlayer *player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);
    BLT_COMPILER_UNUSED(event);

    // we are not draggind the slider anymore
    player->m_TrackSliderIsActive = false;

    // seek to desired position
    int offset = (int)player->m_TrackAdjustment->value;
    player->SeekToPosition(offset, BT_PLAYER_TRACK_SLIDER_RANGE);

    return FALSE;
}

/*----------------------------------------------------------------------
|       menu_open
+---------------------------------------------------------------------*/
static void
menu_open(GtkWidget* widget,
	  gpointer   data)
{
    BtPlayer *player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    // exit if dialog is already up
    if (player->m_FileDialog) return;

    // create dialog
    player->m_FileDialog = gtk_file_selection_new("Select a File");
    
    // set handlers
    gtk_signal_connect(
	    GTK_OBJECT(GTK_FILE_SELECTION(player->m_FileDialog)->ok_button),
	    "clicked", GTK_SIGNAL_FUNC(handle_file_ok_callback),
	    player);
    gtk_signal_connect(
	    GTK_OBJECT(GTK_FILE_SELECTION(player->m_FileDialog)->cancel_button),
	    "clicked", GTK_SIGNAL_FUNC(handle_file_cancel_callback),
	    player);

    gtk_widget_show(player->m_FileDialog);
}

/*----------------------------------------------------------------------
|       menu_exit
+---------------------------------------------------------------------*/
static void
menu_exit(GtkWidget* widget,
	  gpointer   data)
{
    BLT_COMPILER_UNUSED(widget);
    BLT_COMPILER_UNUSED(data);

    gtk_main_quit();
}

/*----------------------------------------------------------------------
|       handle_play_callback
+---------------------------------------------------------------------*/
static void 
handle_play_callback(GtkWidget* widget,
		     gpointer   data )
{
    BtPlayer* player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    player->Play();
}

/*----------------------------------------------------------------------
|       handle_pause_callback
+---------------------------------------------------------------------*/
static void 
handle_pause_callback(GtkWidget* widget,
                      gpointer   data )
{
    BtPlayer* player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    player->Pause();
}

/*----------------------------------------------------------------------
|       handle_stop_callback
+---------------------------------------------------------------------*/
static void 
handle_stop_callback(GtkWidget* widget,
		     gpointer   data )
{
    BtPlayer* player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    player->Stop();
}

/*----------------------------------------------------------------------
|       handle_set_input_callback
+---------------------------------------------------------------------*/
static void 
handle_set_input_callback(GtkWidget* widget,
		          gpointer   data )
{
    BtPlayer* player = (BtPlayer*)data;

    BLT_COMPILER_UNUSED(widget);

    // clear the status display
    for (int i=0; i<=7; i++) {
        gtk_clist_set_text(GTK_CLIST(player->m_PropertyList), i, 1, "");
    }
    
    // set the input to the name entered in the edit field
    const gchar* input_name = gtk_entry_get_text((GtkEntry*)player->m_InputNameEntry);
    player->SetInput(input_name);
    player->Play();
}

/*----------------------------------------------------------------------
|       handle_input_callback
+---------------------------------------------------------------------*/
static void
handle_input_callback(gpointer          data, 
                      gint              source, 
                      GdkInputCondition condition)
{
    BtPlayer*  player = (BtPlayer*)data;
    BLT_COMPILER_UNUSED(source);
    BLT_COMPILER_UNUSED(condition);

    // process all messages in the queue
    BLT_Result result;
    do {
        result = player->PumpMessage(BLT_FALSE);
    } while (BLT_SUCCEEDED(result));

}

/*----------------------------------------------------------------------
|       main menu
+---------------------------------------------------------------------*/
static GtkItemFactoryEntry
MenuItems[] = {
    {const_cast<gchar*>("/_File"),        NULL,         NULL,      0, const_cast<gchar*>("<Branch>"), NULL },
    {const_cast<gchar*>("/File/_Open"),   const_cast<gchar*>("<control>O"), GtkItemFactoryCallback(menu_open), 
     0, NULL, NULL},
    {const_cast<gchar*>("/File/_Exit"),   const_cast<gchar*>("<control>E"), GtkItemFactoryCallback(menu_exit), 
     0, NULL, NULL}
};

/*----------------------------------------------------------------------
|       BtPlayer::BtPlayer
+---------------------------------------------------------------------*/
BtPlayer::BtPlayer(NPT_SelectableMessageQueue* queue) :
    BLT_Player(queue),
    m_Queue(queue),
    m_FileDialog(NULL),
    m_TrackSliderIsActive(false)    
{
    // create the main window
    m_MainWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
           
    // handle the window delete event
    gtk_signal_connect(GTK_OBJECT(m_MainWindow), "delete_event",
		       GTK_SIGNAL_FUNC(handle_delete_event), NULL);
           
    // handle the destroy event
    gtk_signal_connect(GTK_OBJECT(m_MainWindow), "destroy",
		       GTK_SIGNAL_FUNC(handle_destroy_event), NULL);
           
    // set the border width of the main window.
    gtk_container_set_border_width(GTK_CONTAINER(m_MainWindow), 5);
           
    // create main menu
    {
        GtkItemFactory *item_factory;
	GtkAccelGroup  *accel_group;

	MenuItems[1].callback_action = (guint)this;
	accel_group = gtk_accel_group_new();
	item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", 
					    accel_group);
	gtk_item_factory_create_items(item_factory, 
				      sizeof(MenuItems)/sizeof(MenuItems[0]),
				      MenuItems,
				      NULL);
#if GTK_CHECK_VERSION(1,3,15)
        gtk_window_add_accel_group(GTK_WINDOW(m_MainWindow), accel_group);
#else
	gtk_accel_group_attach(accel_group, GTK_OBJECT(m_MainWindow));
#endif
	m_MainMenu = gtk_item_factory_get_widget(item_factory, "<main>");
    }

    // create a box to hold the buttons
    GtkWidget* button_box1 = gtk_hbox_new(FALSE, 5);
    GtkWidget* button_box2 = gtk_hbox_new(FALSE, 2);

    // create the buttons
    m_PlayButton  = gtk_button_new_with_label("Play");
    m_PauseButton = gtk_button_new_with_label("Pause");
    m_StopButton  = gtk_button_new_with_label("Stop");

    // set event handlers for the buttons
    gtk_signal_connect(GTK_OBJECT(m_PlayButton), "clicked",
		       GTK_SIGNAL_FUNC(handle_play_callback), this);
    gtk_signal_connect(GTK_OBJECT(m_PauseButton), "clicked",
		       GTK_SIGNAL_FUNC(handle_pause_callback), this);
    gtk_signal_connect(GTK_OBJECT(m_StopButton), "clicked",
		       GTK_SIGNAL_FUNC(handle_stop_callback), this);
    
    // pack the first row of buttons into the box
    gtk_box_pack_start(GTK_BOX(button_box1), m_PlayButton,  FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box1), m_PauseButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box1), m_StopButton,  FALSE, FALSE, 0);

    // create the slider and add it to the box
    m_TrackSlider = gtk_hscale_new(NULL);
    gtk_signal_connect(GTK_OBJECT(m_TrackSlider),
		       "button_release_event",
		       GTK_SIGNAL_FUNC(handle_track_slider_button_released),
		       this);
    gtk_signal_connect(GTK_OBJECT(m_TrackSlider),
		       "button_press_event",
		       GTK_SIGNAL_FUNC(handle_track_slider_button_pressed),
		       this);

    gtk_widget_set_usize(m_TrackSlider, 
			 BT_PLAYER_TRACK_SLIDER_WIDTH,
			 BT_PLAYER_TRACK_SLIDER_HEIGHT);

    gtk_scale_set_draw_value(GTK_SCALE(m_TrackSlider), FALSE);
    gtk_range_set_update_policy(GTK_RANGE(m_TrackSlider), 
                                GTK_UPDATE_DISCONTINUOUS);
    m_TrackAdjustment = gtk_range_get_adjustment(GTK_RANGE(m_TrackSlider));
    m_TrackAdjustment->lower = 0.0;
    m_TrackAdjustment->upper = BT_PLAYER_TRACK_SLIDER_RANGE;
    gtk_adjustment_changed(m_TrackAdjustment);
    gtk_box_pack_start(GTK_BOX(button_box1), m_TrackSlider, TRUE, TRUE, 0);

    // create the second row of buttons and widgets
    m_SetInputButton = gtk_button_new_with_label("Set Input");
    m_InputNameEntry = gtk_entry_new();
    gtk_signal_connect(GTK_OBJECT(m_SetInputButton), "clicked",
		       GTK_SIGNAL_FUNC(handle_set_input_callback), this);

    // pack the second row of buttons
    gtk_box_pack_start(GTK_BOX(button_box2), m_InputNameEntry,TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box2), m_SetInputButton,FALSE, FALSE, 0);

    // create a box to hold the menu and the rest
    GtkWidget* main_box = gtk_vbox_new(FALSE, FALSE);

    // create a table for the status
    GtkWidget* status_table = gtk_table_new(3, 2, FALSE);
    m_TimeCodeLabel = gtk_label_new("00:00:00");
    gtk_table_attach_defaults(GTK_TABLE(status_table),
                              m_TimeCodeLabel,
                              1, 2, 1, 2);
    m_DecoderStateLabel = gtk_label_new("[STOPPED]");
    gtk_table_attach_defaults(GTK_TABLE(status_table),
                              m_DecoderStateLabel,
                              2, 3, 1, 2);

    // create a list for the properties
    m_PropertyList = gtk_clist_new(2);
    gtk_clist_set_column_width(GTK_CLIST(m_PropertyList), 0, 150);
    const gchar *column[2] = {NULL, "0"};
    column[0] = "Nominal Bitrate";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Average Bitrate";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Instant Bitrate";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Sample Rate";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Channels";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Stream Duration";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Stream Size";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));
    column[0] = "Data Type";
    column[1] = "Unknown";
    gtk_clist_append(GTK_CLIST(m_PropertyList), const_cast<gchar**>(column));

    // pack the menu and the button box vertically
    gtk_box_pack_start(GTK_BOX(main_box), m_MainMenu,     FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), button_box1,    FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), button_box2,    FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), status_table,   FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), m_PropertyList, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(m_MainWindow), main_box);
           
    // show the widgets
    gtk_widget_show(m_MainMenu);
    gtk_widget_show(m_PlayButton);
    gtk_widget_show(m_PauseButton);
    gtk_widget_show(m_StopButton);
    gtk_widget_show(m_SetInputButton);
    gtk_widget_show(m_InputNameEntry);
    gtk_widget_show(m_TrackSlider);
    gtk_widget_show(m_TimeCodeLabel);
    gtk_widget_show(m_DecoderStateLabel);
    gtk_widget_show(button_box1);
    gtk_widget_show(button_box2);
    gtk_widget_show(status_table);
    gtk_widget_show(m_PropertyList);
    gtk_widget_show(main_box);
    
    // show the window
    gtk_window_set_position(GTK_WINDOW(m_MainWindow), GTK_WIN_POS_CENTER);
    gtk_widget_show(m_MainWindow);

    // ask to be notified when there is a message from the decoder
    gdk_input_add(queue->GetEventFd(), 
                  GDK_INPUT_READ, 
                  handle_input_callback, 
                  (gpointer)this);
}

/*----------------------------------------------------------------------
|    BtPlayer::OnDecoderStateNotification
+---------------------------------------------------------------------*/
void 
BtPlayer::OnDecoderStateNotification(BLT_DecoderServer::State state)
{
    const char* label;

    switch (state) {
      case BLT_DecoderServer::STATE_STOPPED:
        label = "[STOPPED]";
        break;

      case BLT_DecoderServer::STATE_PLAYING:
        label = "[PLAYING]";
        break;

      case BLT_DecoderServer::STATE_PAUSED:
        label = "[PAUSED]";
        break;

      case BLT_DecoderServer::STATE_EOS:
        label = "[END OF STREAM]";
        break;

      default:
        label = "[UNKNOWN]";
        break;
    }

    gtk_label_set_text(GTK_LABEL(m_DecoderStateLabel), label);
}

/*----------------------------------------------------------------------
|    BtPlayer::OnStreamTimeCodeNotification
+---------------------------------------------------------------------*/
void 
BtPlayer::OnStreamTimeCodeNotification(BLT_TimeCode time_code)
{
    char label[32];
    sprintf(label, "%02d:%02d:%02d",
            time_code.h,
            time_code.m,
            time_code.s);
    gtk_label_set_text(GTK_LABEL(m_TimeCodeLabel), label);
}

/*----------------------------------------------------------------------
|    BtPlayer::OnStreamPositionNotification
+---------------------------------------------------------------------*/
void 
BtPlayer::OnStreamPositionNotification(BLT_StreamPosition& position)
{
    gfloat value = position.range ? 
        ((gfloat)m_TrackAdjustment->upper *
         ((gfloat)position.offset/(gfloat)position.range)) : 
         0.0;

    if (!m_TrackSliderIsActive) {
        gtk_adjustment_set_value(m_TrackAdjustment, value);
        gtk_adjustment_value_changed(m_TrackAdjustment);
    }
}

/*----------------------------------------------------------------------
|    BtPlayer::OnStreamInfoNotification
+---------------------------------------------------------------------*/
void 
BtPlayer::OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info)
{       
    char text[128];

    if (update_mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
        sprintf(text, "%d", info.nominal_bitrate);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 0, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
        sprintf(text, "%d", info.average_bitrate);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 1, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
        sprintf(text, "%d", info.instant_bitrate);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 2, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
        sprintf(text, "%d", info.sample_rate);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 3, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
        sprintf(text, "%d", info.channel_count);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 4, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_DURATION) {
        sprintf(text, "%" ATX_INT64_PRINTF_FORMAT "d", info.duration);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 5, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_SIZE) {
        sprintf(text, "%" ATX_INT64_PRINTF_FORMAT "d", info.size);
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 6, 1, text);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
        gtk_clist_set_text(GTK_CLIST(m_PropertyList), 7, 1, info.data_type);
    }
}

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    BLT_COMPILER_UNUSED(argc);
    BLT_COMPILER_UNUSED(argv);

    // initialize the GTK library
    gtk_init(&argc, &argv);
           
    // create a message queue
    NPT_SelectableMessageQueue* queue = new NPT_SelectableMessageQueue();

    // create a player
    BtPlayer* player = new BtPlayer(queue);

    // pass input arguments
    if (argc == 2) {
        player->SetInput(argv[1]);
        player->Play();
    }

    // main gtk loop
    gtk_main();
           
    // cleanup
    delete player;
    delete queue;

    return(0);
}
