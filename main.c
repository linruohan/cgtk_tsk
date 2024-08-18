#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <libadwaita-1/adwaita.h>

G_MODULE_EXPORT void sync_btn_clicked_cb(GtkButton *button, GtkEntry *input) {
    GtkEntryBuffer *input_buffer = gtk_entry_get_buffer(input);
    const char *text = gtk_entry_buffer_get_text(input_buffer);
    printf("%s\n", text);
}


static void active_refresh_time(GtkButton *button, GtkEntryBuffer *raw_data) {
    const int64_t time_now = g_get_real_time();
    GDateTime *now = g_date_time_new_now_local();
    const gchar *formatted = g_date_time_format(now, "%Y-%m-%d %H:%M:%S");
    gtk_entry_buffer_set_text(raw_data, formatted, strlen(formatted));
}

static void
activate(GtkApplication *app) {
    GtkBuilder *builder = gtk_builder_new_from_file("../ui.glade");
    GObject *window = gtk_builder_get_object(builder, "main_window");
    gtk_window_set_application(GTK_WINDOW(window), app);


    gtk_window_set_title(GTK_WINDOW(window), "test Title");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    gtk_window_present(GTK_WINDOW(window));
}

static void
activate_cb(GtkApplication *app) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "test Title");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);

    // GtkWidget *label = gtk_label_new("Hello World");
    GtkWidget *refresh_time = gtk_button_new_with_label("点击");
    GtkEntryBuffer *raw_data = gtk_entry_buffer_new(NULL, 0);
    g_signal_connect(refresh_time, "clicked", G_CALLBACK (active_refresh_time), raw_data);

    GtkWidget *output = gtk_text_new_with_buffer(raw_data);

    GtkWidget *box = (gtk_box_new(GTK_ORIENTATION_VERTICAL, 10));

    gtk_box_append(GTK_BOX(box), output);
    gtk_box_append(GTK_BOX(box), refresh_time);

    // gtk_window_set_child(GTK_WINDOW(window), label);
    gtk_window_set_child(GTK_WINDOW(window), box);
    gtk_window_present(GTK_WINDOW(window));
}


int
main(const int argc, char *argv[]) {
    g_autoptr(AdwApplication) app = NULL;

    app = adw_application_new("com.github.linruohan", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}

