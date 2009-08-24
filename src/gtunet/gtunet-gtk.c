/*
 * gtunet-gtk.c
 *
 */

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "../ethcard.h"
#include "../mytunetsvc.h"

#include "gtunet-gtk.h"
#include "gtunet.h"

/*
 * callback functions
 */

gboolean window_delete_event (GtkWidget *window,
                              GdkEvent *event,
                              gpointer data);
gboolean about_dialog_delete_event (GtkWidget *about_dialog,
                                    GdkEvent *event,
                                    gpointer data);
void on_button_about_clicked (GtkWidget *widget,
                              gpointer data);
void about_dialog_hide (GtkWidget *widget,
                        gpointer data);


void on_tray_icon_clicked (GtkStatusIcon *status_icon, 
                           gpointer data);
void on_tray_icon_menu (GtkStatusIcon *status_icon,
                        guint button, 
                        guint activate_time,
                        gpointer data);

  
/*
 * non-libglade widgets initialization
 */

static GtkStatusIcon *create_tray_icon ();
static GtkMenu *create_tray_menu ();
static GtkComboBox *create_combo_box_adapter (GladeXML *xml);

/*
 * main function
 */

GladeXML *xml;
GtkWidget *window;
GtkWidget *about_dialog;
GtkWidget *button_about;
GtkWidget *button_quit;
GtkWidget *combobox_limitation;
GtkWidget *image_status;

GtkWidget *entry_username;
GtkWidget *entry_password;
GtkWidget *checkbutton_savepassword;
GtkWidget *checkbutton_zijing;
GtkWidget *textview_log;

GtkStatusIcon *tray_icon;
GtkMenu *tray_menu;
GtkComboBox *combobox_adapter;
GtkTextBuffer *textbuffer;

ETHCARD_INFO ethcards[16];
int ethcard_count;

int
main(int argc, char *argv[])
{
  int i;
  
  
  ethcard_count = (int)get_ethcards(ethcards, sizeof(ethcards));
  
  
  gtk_init(&argc, &argv);

  
  /* load the interface */
  xml = glade_xml_new("gtunet.glade", NULL, NULL);
  
  
  /* get widgets from xml */
  window = glade_xml_get_widget(xml, "window");
  
  about_dialog = glade_xml_get_widget(xml, "aboutdialog");
    
  button_about = glade_xml_get_widget(xml, "button_about");
  
  button_quit = glade_xml_get_widget(xml, "button_quit");

  combobox_limitation = glade_xml_get_widget(xml, "combobox_limitation");
  gtk_combo_box_set_active((GtkComboBox *)combobox_limitation, 1);
  
  image_status = glade_xml_get_widget(xml, "image_status");
  gtk_image_set_from_file((GtkImage *)image_status, "resource/image1.png");
  
  entry_username = glade_xml_get_widget(xml, "entry_username");
  
  entry_password = glade_xml_get_widget(xml, "entry_password");

  checkbutton_savepassword = glade_xml_get_widget(xml,
                                                  "checkbutton_savepassword");
  checkbutton_zijing = glade_xml_get_widget(xml,
                                            "checkbutton_zijing");

  textview_log = glade_xml_get_widget(xml, "textview_log");
  textbuffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview_log));
  
  /* create widgets from code */
  tray_icon = create_tray_icon();
  
  tray_menu = create_tray_menu();
  
  combobox_adapter = create_combo_box_adapter(xml);
  for (i = 0; i < ethcard_count; i++)
    {
      gtk_combo_box_append_text((GtkComboBox *)combobox_adapter,
                                ethcards[i].desc);
    }
  if (ethcard_count != 0)
    {
      gtk_combo_box_set_active(combobox_adapter, 0);
    }
  
  
  /* connect the signals in gtk window */
  g_signal_connect(G_OBJECT (window),
                   "delete_event",
                   G_CALLBACK (window_delete_event),
                   NULL);
  
  glade_xml_signal_connect_data(xml,
                                "on_button_about_clicked",
                                G_CALLBACK (on_button_about_clicked),
                                about_dialog);

  glade_xml_signal_connect_data(xml,
                                "on_button_quit_clicked",
                                G_CALLBACK (on_button_quit_clicked),
                                NULL);
  
  glade_xml_signal_connect_data(xml,
                                "on_button_login_clicked",
                                G_CALLBACK (on_button_login_clicked),
                                NULL);

  glade_xml_signal_connect_data(xml,
                                "on_button_logout_clicked",
                                G_CALLBACK (on_button_logout_clicked),
                                NULL);

  glade_xml_signal_connect(xml,
                           "on_entry_password_changed",
                           G_CALLBACK (on_entry_password_changed));
  
  /* connect signals in about_dialog */
  g_signal_connect(G_OBJECT (about_dialog),
                   "delete_event",
                   G_CALLBACK (about_dialog_delete_event),
                   NULL);
  glade_xml_signal_connect(xml,
                           "on_aboutdialog_response",
                           G_CALLBACK (about_dialog_hide));
  glade_xml_signal_connect(xml,
                           "on_aboutdialog_close",
                           G_CALLBACK (about_dialog_hide));
  
  
  /* connect signals in tray_icon */
  g_signal_connect(G_OBJECT(tray_icon),
                   "activate", 
                   G_CALLBACK(on_tray_icon_clicked),
                   window);
  g_signal_connect(G_OBJECT(tray_icon), 
                   "popup-menu",
                   G_CALLBACK(on_tray_icon_menu),
                   tray_menu);

  
  /* show the window */
  gtk_widget_show(window);
  
  
  /* start mytunet service */
  mytunetsvc_set_transmit_log_func(MYTUNETSVC_TRANSMIT_LOG_POSIX);
  mytunetsvc_init();
  
  /* start the event loop */
  gtk_main();

  return 0;
}


/*
 * function definitions
 */
void
about_dialog_hide (GtkWidget *widget,
                   gpointer data)
{
  gtk_widget_hide(widget);
}

gboolean
window_delete_event (GtkWidget *window,
                     GdkEvent *event,
                     gpointer data)
{
  gtk_widget_hide(window);
  return TRUE;
}

gboolean
about_dialog_delete_event (GtkWidget *about_dialog,
                           GdkEvent *event,
                           gpointer data)
{
  gtk_widget_hide(about_dialog);
  return TRUE;
}

void
on_button_about_clicked (GtkWidget *widget,
                         gpointer data)
{
  gtk_widget_show(data);
}

GtkStatusIcon *
create_tray_icon ()
{
  GtkStatusIcon *tray_icon;

  tray_icon = gtk_status_icon_new();
  gtk_status_icon_set_from_file(tray_icon,
                                "resource/image1.png");
  gtk_status_icon_set_tooltip(tray_icon, 
                              "gtunet");
  gtk_status_icon_set_visible(tray_icon, TRUE);
  
  return tray_icon;
}

void
on_tray_icon_clicked (GtkStatusIcon *status_icon, 
                      gpointer user_data)
{
  GtkWidget *window;
  window = (GtkWidget *)user_data;

  if (GTK_WIDGET_VISIBLE (window))
    {
      gtk_widget_hide(window);
    }
  else
    {
      gtk_widget_show(window);
    }
}

void
on_tray_icon_menu (GtkStatusIcon *status_icon,
                   guint button, 
                   guint activate_time,
                   gpointer data)
{
  gtk_menu_popup(data,
                 NULL,
                 NULL,
                 gtk_status_icon_position_menu,
                 status_icon,
                 button,
                 activate_time);
}

GtkMenu *
create_tray_menu ()
{
  GtkWidget *tray_menu;
  GtkWidget *tray_menu_quit;
  
  tray_menu_quit = gtk_menu_item_new_with_label("Quit");

  g_signal_connect(G_OBJECT(tray_menu_quit),
                   "activate",
                   G_CALLBACK (on_button_quit_clicked),
                   NULL);
  
  tray_menu = gtk_menu_new();
  
  gtk_menu_shell_append((GtkMenuShell *)tray_menu, tray_menu_quit);
  gtk_widget_show(tray_menu_quit);

  return (GtkMenu *)tray_menu;
}

GtkComboBox *
create_combo_box_adapter (GladeXML *xml)
{
  GtkWidget *combobox_adapter;
  GtkWidget *table2;
  
  table2 = glade_xml_get_widget(xml, "table2");
  combobox_adapter = gtk_combo_box_new_text();
  
  gtk_table_attach_defaults((GtkTable *)table2,
                            combobox_adapter,
                            1,
                            2,
                            0,
                            1);

  gtk_widget_show(combobox_adapter);
  
  return (GtkComboBox *)combobox_adapter;
}
