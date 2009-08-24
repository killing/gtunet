void on_button_quit_clicked (GtkWidget *widget,
                             gpointer data);

void on_button_login_clicked (GtkWidget *widget,
                              gpointer data);

void on_button_logout_clicked (GtkWidget *widget,
                               gpointer data);

void on_entry_password_changed (GtkWidget *widget,
                                gpointer data);

void * mytunet_start (void *data);

void textview_append (const char *flag, const char *content);

void save_config (gboolean save_password);

extern pthread_t mytunet_service_tid;
extern USERCONFIG user_config;
extern gboolean save_password;
extern gboolean password_saved;
