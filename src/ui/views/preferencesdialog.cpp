#include "preferencesdialog.hpp"

using namespace NickvisionTagger::Controllers;
using namespace NickvisionTagger::UI::Views;

PreferencesDialog::PreferencesDialog(GtkWindow* parent, const PreferencesDialogController& controller) : m_controller{ controller }, m_gobj{ adw_window_new() }
{
    //Window Settings
    gtk_window_set_transient_for(GTK_WINDOW(m_gobj), parent);
    gtk_window_set_default_size(GTK_WINDOW(m_gobj), 800, 600);
    gtk_window_set_modal(GTK_WINDOW(m_gobj), true);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(m_gobj), false);
    gtk_window_set_hide_on_close(GTK_WINDOW(m_gobj), true);
    //Header Bar
    m_headerBar = adw_header_bar_new();
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(m_headerBar), adw_window_title_new("Preferences", nullptr));
    //User Interface Group
    m_grpUserInterface = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(m_grpUserInterface), "User Interface");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(m_grpUserInterface), "Customize the application's user interface.");
    //Theme Row
    m_rowTheme = adw_combo_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_rowTheme), "Theme");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(m_rowTheme), "A theme change will be applied once the dialog is closed.");
    adw_combo_row_set_model(ADW_COMBO_ROW(m_rowTheme), G_LIST_MODEL(gtk_string_list_new(new const char*[4]{ "System", "Light", "Dark", nullptr })));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_grpUserInterface), m_rowTheme);
    //Music Folder Group
    m_grpMusicFolder = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(m_grpMusicFolder), "Music Folder");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(m_grpMusicFolder), "Customize music folder settings.");
    //Include Subfolders Row
    m_rowIncludeSubfolders = adw_action_row_new();
    m_switchIncludeSubfolders = gtk_switch_new();
    gtk_widget_set_valign(m_switchIncludeSubfolders, GTK_ALIGN_CENTER);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_rowIncludeSubfolders), "Include Subfolders");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(m_rowIncludeSubfolders), "If checked, subfolders will be included when scanning for music files in a music folder.");
    adw_action_row_add_suffix(ADW_ACTION_ROW(m_rowIncludeSubfolders), m_switchIncludeSubfolders);
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(m_rowIncludeSubfolders), m_switchIncludeSubfolders);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_grpMusicFolder), m_rowIncludeSubfolders);
    //Remember Last Opened Folder Row
    m_rowRememberLastOpenedFolder = adw_action_row_new();
    m_switchRememberLastOpenedFolder = gtk_switch_new();
    gtk_widget_set_valign(m_switchRememberLastOpenedFolder, GTK_ALIGN_CENTER);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_rowRememberLastOpenedFolder), "Remember Last Opened Folder");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(m_rowRememberLastOpenedFolder), "If checked, the last opened music folder will be remembered and opened again when Tagger starts.");
    adw_action_row_add_suffix(ADW_ACTION_ROW(m_rowRememberLastOpenedFolder), m_switchRememberLastOpenedFolder);
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(m_rowRememberLastOpenedFolder), m_switchRememberLastOpenedFolder);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_grpMusicFolder), m_rowRememberLastOpenedFolder);
    //Music File Group
    m_grpMusicFile = adw_preferences_group_new();
    adw_preferences_group_set_title(ADW_PREFERENCES_GROUP(m_grpMusicFile), "Music File");
    adw_preferences_group_set_description(ADW_PREFERENCES_GROUP(m_grpMusicFile), "Customize music file settings.");
    //Preserve Modification Time Stamp Row
    m_rowPreserveModificationTimeStamp = adw_action_row_new();
    m_switchPreserveModificationTimeStamp = gtk_switch_new();
    gtk_widget_set_valign(m_switchPreserveModificationTimeStamp, GTK_ALIGN_CENTER);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_rowPreserveModificationTimeStamp), "Preserve Modification Time Stamp");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(m_rowPreserveModificationTimeStamp), "If checked, a music file's modification time stamp will not be updated when the tag is edited'.");
    adw_action_row_add_suffix(ADW_ACTION_ROW(m_rowPreserveModificationTimeStamp), m_switchPreserveModificationTimeStamp);
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(m_rowPreserveModificationTimeStamp), m_switchPreserveModificationTimeStamp);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_grpMusicFile), m_rowPreserveModificationTimeStamp);
    //Overwrite Tag With MusicBrainz
    m_rowOverwriteTagWithMusicBrainz = adw_action_row_new();
    m_switchOverwriteTagWithMusicBrainz = gtk_switch_new();
    gtk_widget_set_valign(m_switchOverwriteTagWithMusicBrainz, GTK_ALIGN_CENTER);
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_rowOverwriteTagWithMusicBrainz), "Overwrite Tag With MusicBrainz");
    adw_action_row_set_subtitle(ADW_ACTION_ROW(m_rowOverwriteTagWithMusicBrainz), "If checked, Tagger will overwrite a tag's properties with the resulting data from MusicBrainz. Else, Tagger will preserve already filled-in properties and fill in only empty properties in a tag with the data from MusicBrainz.");
    adw_action_row_add_suffix(ADW_ACTION_ROW(m_rowOverwriteTagWithMusicBrainz), m_switchOverwriteTagWithMusicBrainz);
    adw_action_row_set_activatable_widget(ADW_ACTION_ROW(m_rowOverwriteTagWithMusicBrainz), m_switchOverwriteTagWithMusicBrainz);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_grpMusicFile), m_rowOverwriteTagWithMusicBrainz);
    //Page
    m_page = adw_preferences_page_new();
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_page), ADW_PREFERENCES_GROUP(m_grpUserInterface));
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_page), ADW_PREFERENCES_GROUP(m_grpMusicFolder));
    adw_preferences_page_add(ADW_PREFERENCES_PAGE(m_page), ADW_PREFERENCES_GROUP(m_grpMusicFile));
    //Main Box
    m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(m_mainBox), m_headerBar);
    gtk_box_append(GTK_BOX(m_mainBox), m_page);
    adw_window_set_content(ADW_WINDOW(m_gobj), m_mainBox);
    //Load Configuration
    adw_combo_row_set_selected(ADW_COMBO_ROW(m_rowTheme), m_controller.getThemeAsInt());
    gtk_switch_set_active(GTK_SWITCH(m_switchIncludeSubfolders), m_controller.getIncludeSubfolders());
    gtk_switch_set_active(GTK_SWITCH(m_switchRememberLastOpenedFolder), m_controller.getRememberLastOpenedFolder());
    gtk_switch_set_active(GTK_SWITCH(m_switchPreserveModificationTimeStamp), m_controller.getPreserveModificationTimeStamp());
    gtk_switch_set_active(GTK_SWITCH(m_switchOverwriteTagWithMusicBrainz), m_controller.getOverwriteTagWithMusicBrainz());
}

GtkWidget* PreferencesDialog::gobj()
{
    return m_gobj;
}

void PreferencesDialog::run()
{
    gtk_widget_show(m_gobj);
    while(gtk_widget_is_visible(m_gobj))
    {
        g_main_context_iteration(g_main_context_default(), false);
    }
    m_controller.setTheme(adw_combo_row_get_selected(ADW_COMBO_ROW(m_rowTheme)));
    m_controller.setIncludeSubfolders(gtk_switch_get_active(GTK_SWITCH(m_switchIncludeSubfolders)));
    m_controller.setRememberLastOpenedFolder(gtk_switch_get_active(GTK_SWITCH(m_switchRememberLastOpenedFolder)));
    m_controller.setPreserveModificationTimeStamp(gtk_switch_get_active(GTK_SWITCH(m_switchPreserveModificationTimeStamp)));
    m_controller.setOverwriteTagWithMusicBrainz(gtk_switch_get_active(GTK_SWITCH(m_switchOverwriteTagWithMusicBrainz)));
    m_controller.saveConfiguration();
    if(m_controller.getThemeAsInt() == 0)
    {
        adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_PREFER_LIGHT);
    }
    else if(m_controller.getThemeAsInt() == 1)
    {
        adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_FORCE_LIGHT);
    }
    else if(m_controller.getThemeAsInt() == 2)
    {
        adw_style_manager_set_color_scheme(adw_style_manager_get_default(), ADW_COLOR_SCHEME_FORCE_DARK);
    }
    gtk_window_destroy(GTK_WINDOW(m_gobj));
}
