/*
 * gtunet.c
 *
 */

#include <stdio.h>
#include <pthread.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

//#include "../os.h"
//#include "../des.h"
//#include "../md5.h"
//#include "../logs.h"
#include "../tunet.h"
#include "../dot1x.h"
#include "../ethcard.h"
#include "../userconfig.h"
//#include "../mytunet.h"
//#include "../util.h"
#include "../setting.h"
#include "../mytunetsvc.h"

#include "gtunet.h"
#include "gtunet-gtk.h"

pthread_t mytunet_service_tid;
USERCONFIG user_config;
gboolean save_password;
gboolean password_saved;
gboolean is_user_changed_password = TRUE;


void
on_button_quit_clicked (GtkWidget *widget,
                        gpointer data)
{
  mytunetsvc_set_stop_flag();

  if (mytunet_service_tid)
    {
      pthread_cancel(mytunet_service_tid);
      mytunet_service_tid = 0;
    }
  
  mytunetsvc_cleanup();
  puts("Exiting...");
  gtk_main_quit();
}

void
on_button_login_clicked (GtkWidget *widget,
                         gpointer data)
{
  char *user_name;
  int user_name_length;
  int ethercard_name_length;
  int eth_i;
  int lim_i;
  int limitation;
  gboolean set_dot1x;
  
  
  if (mytunet_service_tid)
    {
      return;
    }

  /* set user name */
  user_name_length = gtk_entry_get_text_length
    (GTK_ENTRY (entry_username));
  
  if ((user_name_length > (sizeof(user_config.szUsername) - 1))
      || (user_name_length == 0))
    {
      return;
    }
  user_name = (char *)gtk_entry_get_text(GTK_ENTRY (entry_username));
  userconfig_set_username(&user_config, user_name);

  textview_append("[CONFIG] Username:", user_name);
  
  
  /* set ethercard */
  eth_i = gtk_combo_box_get_active(combobox_adapter);
  ethercard_name_length = strlen(ethcards[eth_i].desc);
  if (ethercard_name_length > sizeof(user_config.szAdaptor) - 1)
    {
      return;
    }
  userconfig_set_adapter(&user_config, ethcards[eth_i].desc);
  textview_append("[CONFIG] Ethcard: ", ethcards[eth_i].desc);
  textview_append("[CONFIG] Ethcard MAC:", ethcards[eth_i].mac);
  textview_append("[CONFIG] Ethcard IP: ", ethcards[eth_i].ip);
  
  /* set limitation */
  lim_i = gtk_combo_box_get_active(GTK_COMBO_BOX (combobox_limitation));
  switch (lim_i)
    {
    case 0:
      {
        limitation = LIMITATION_CAMPUS;
        break;
      }
    case 1:
      {
        limitation = LIMITATION_DOMESTIC;
        break;
      }
    case 2:
      {
        limitation = LIMITATION_NONE;
        break;
      }
    default:
      {
        limitation = LIMITATION_DOMESTIC;
        break;
      }
    }
  userconfig_set_limitation(&user_config, limitation);
  textview_append("[CONFIG] Limitation:",
                  gtk_combo_box_get_active_text
                  (GTK_COMBO_BOX (combobox_limitation)));

  /* set whether use dot1x */
  set_dot1x = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON (checkbutton_zijing));
  printf("set_dot1x: %d\n", set_dot1x);
  userconfig_set_dot1x(&user_config, set_dot1x, FALSE);
  textview_append("[CONFIG] Use 802.1x:",
                  (set_dot1x? "Yes": "No"));

  /* set whether save password */
  save_password = gtk_toggle_button_get_active
    (GTK_TOGGLE_BUTTON (checkbutton_savepassword));

  is_user_changed_password = FALSE;
  gtk_entry_set_text(GTK_ENTRY (entry_password),
                     user_config.szMD5Password);
  is_user_changed_password = TRUE;

  /* save configuration */
  save_config(save_password);

  /* start mytunet service */
  mytunetsvc_clear_stop_flag();
  mytunetsvc_set_global_config_from(&user_config);

  if (pthread_create(&mytunet_service_tid,
                     NULL, mytunet_start, NULL))
    {
      puts("mytunet: service thread create error");
    }  
}

void
on_button_logout_clicked (GtkWidget *widget,
                          gpointer data)
{
  puts("Logout...");
  mytunetsvc_set_stop_flag();
  
  tunet_reset();
  dot1x_reset();

  if (mytunet_service_tid)
    {
      pthread_cancel(mytunet_service_tid);
      mytunet_service_tid = 0;
    }
  
  puts("Logout, OK!");
}

void *
mytunet_start (void *data)
{
  mytunetsvc_main(0, NULL);
  return NULL;
}

void
textview_append (const char *flag, const char *content)
{
  gtk_text_buffer_insert_at_cursor(textbuffer,
                                   flag,
                                   strlen(flag));
  
  gtk_text_buffer_insert_at_cursor(textbuffer,
                                   " ",
                                   1);
  
  gtk_text_buffer_insert_at_cursor(textbuffer,
                                   content,
                                   strlen(content));

  gtk_text_buffer_insert_at_cursor(textbuffer,
                                   "\n",
                                   1);
}

void
save_config (gboolean save_password)
{
  mytunetsvc_set_user_config_dot1x(user_config.bUseDot1x, FALSE);
  
  password_saved = save_password;
  
  if (save_password)
    {
      mytunetsvc_set_user_config(user_config.szUsername,
                                 user_config.szMD5Password,
                                 TRUE,
                                 user_config.szAdaptor,
                                 user_config.limitation,
                                 0);
    }
  else
    {
      mytunetsvc_set_user_config(user_config.szUsername,
                                 "",
                                 TRUE,
                                 user_config.szAdaptor,
                                 user_config.limitation,
                                 0);
    }

  setting_write_int(NULL, "savepassword", password_saved);
}

void
on_entry_password_changed (GtkWidget *widget,
                           gpointer data)
{
  char *password;

  if (!is_user_changed_password)
    {
      return;
    }
  
  password = (char *)gtk_entry_get_text(GTK_ENTRY (widget));

  userconfig_set_password(&user_config, password);
}
