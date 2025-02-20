#include "mainwindow.hpp"
#include <algorithm>
#include <filesystem>
#include <regex>
#include <utility>
#include "preferencesdialog.hpp"
#include "shortcutsdialog.hpp"
#include "../controls/comboboxdialog.hpp"
#include "../controls/entrydialog.hpp"
#include "../controls/messagedialog.hpp"
#include "../controls/progressdialog.hpp"
#include "../../helpers/mediahelpers.hpp"
#include "../../helpers/stringhelpers.hpp"
#include "../../helpers/translation.hpp"
#include "../../models/musicfolder.hpp"
#include "../../models/tagmap.hpp"

using namespace NickvisionTagger::Controllers;
using namespace NickvisionTagger::Helpers;
using namespace NickvisionTagger::Models;
using namespace NickvisionTagger::UI::Controls;
using namespace NickvisionTagger::UI::Views;

/**
 * Sets a GtkImage's source from the TagLib::ByteVector
 *
 * @param image The GtkImage
 * @param byteVector The TagLib::ByteVector representing the image
 */
void gtk_image_set_from_byte_vector(GtkImage* image, const TagLib::ByteVector& byteVector)
{
    if(byteVector.isEmpty())
    {
        gtk_image_clear(image);
    }
    else
    {
        GdkPixbufLoader* pixbufLoader{gdk_pixbuf_loader_new()};
        gdk_pixbuf_loader_write(pixbufLoader, (unsigned char*)byteVector.data(), byteVector.size(), nullptr);
        gtk_image_set_from_pixbuf(image, gdk_pixbuf_loader_get_pixbuf(pixbufLoader));
        gdk_pixbuf_loader_close(pixbufLoader, nullptr);
        g_object_unref(pixbufLoader);
    }
}

MainWindow::MainWindow(GtkApplication* application, const MainWindowController& controller) : m_controller{ controller }, m_isSelectionOccuring{ false }, m_gobj{ adw_application_window_new(application) }
{
    //Window Settings
    gtk_window_set_default_size(GTK_WINDOW(m_gobj), 900, 700);
    if(m_controller.getIsDevVersion())
    {
        gtk_style_context_add_class(gtk_widget_get_style_context(m_gobj), "devel");
    }
    g_signal_connect(m_gobj, "close_request", G_CALLBACK((bool (*)(GtkWindow*, gpointer))[](GtkWindow*, gpointer data) -> bool { return reinterpret_cast<MainWindow*>(data)->onCloseRequest(); }), this);
    //Header Bar
    m_headerBar = adw_header_bar_new();
    m_adwTitle = adw_window_title_new(m_controller.getAppInfo().getShortName().c_str(), m_controller.getMusicFolderPath().c_str());
    adw_header_bar_set_title_widget(ADW_HEADER_BAR(m_headerBar), m_adwTitle);
    //Open Music Folder Button
    m_btnOpenMusicFolder = gtk_button_new();
    GtkWidget* btnOpenMusicFolderContent{ adw_button_content_new() };
    adw_button_content_set_icon_name(ADW_BUTTON_CONTENT(btnOpenMusicFolderContent), "folder-open-symbolic");
    adw_button_content_set_label(ADW_BUTTON_CONTENT(btnOpenMusicFolderContent), _("Open"));
    gtk_button_set_child(GTK_BUTTON(m_btnOpenMusicFolder), btnOpenMusicFolderContent);
    gtk_widget_set_tooltip_text(m_btnOpenMusicFolder, _("Open Music Folder (Ctrl+O)"));
    gtk_actionable_set_action_name(GTK_ACTIONABLE(m_btnOpenMusicFolder), "win.openMusicFolder");
    adw_header_bar_pack_start(ADW_HEADER_BAR(m_headerBar), m_btnOpenMusicFolder);
    //Reload Music Folder Button
    m_btnReloadMusicFolder = gtk_button_new();
    gtk_button_set_icon_name(GTK_BUTTON(m_btnReloadMusicFolder), "view-refresh-symbolic");
    gtk_widget_set_tooltip_text(m_btnReloadMusicFolder, _("Reload Music Folder (F5)"));
    gtk_widget_set_visible(m_btnReloadMusicFolder, false);
    gtk_actionable_set_action_name(GTK_ACTIONABLE(m_btnReloadMusicFolder), "win.reloadMusicFolder");
    adw_header_bar_pack_start(ADW_HEADER_BAR(m_headerBar), m_btnReloadMusicFolder);
    //Menu Help Button
    m_btnMenuHelp = gtk_menu_button_new();
    GMenu* menuHelp{ g_menu_new() };
    g_menu_append(menuHelp, _("Preferences"), "win.preferences");
    g_menu_append(menuHelp, _("Keyboard Shortcuts"), "win.keyboardShortcuts");
    g_menu_append(menuHelp, std::string(StringHelpers::format(_("About %s"), m_controller.getAppInfo().getShortName().c_str())).c_str(), "win.about");
    gtk_menu_button_set_direction(GTK_MENU_BUTTON(m_btnMenuHelp), GTK_ARROW_NONE);
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(m_btnMenuHelp), G_MENU_MODEL(menuHelp));
    gtk_widget_set_tooltip_text(m_btnMenuHelp, _("Main Menu"));
    adw_header_bar_pack_end(ADW_HEADER_BAR(m_headerBar), m_btnMenuHelp);
    g_object_unref(menuHelp);
    //Header End Separator
    m_sepHeaderEnd = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_style_context_add_class(gtk_widget_get_style_context(m_sepHeaderEnd), "spacer");
    adw_header_bar_pack_end(ADW_HEADER_BAR(m_headerBar), m_sepHeaderEnd);
    //Apply Button
    m_btnApply = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(m_btnApply), _("Apply"));
    gtk_widget_set_tooltip_text(m_btnApply, _("Apply Changes To Tag (Ctrl+S)"));
    gtk_widget_set_visible(m_btnApply, false);
    gtk_actionable_set_action_name(GTK_ACTIONABLE(m_btnApply), "win.apply");
    gtk_style_context_add_class(gtk_widget_get_style_context(m_btnApply), "suggested-action");
    adw_header_bar_pack_end(ADW_HEADER_BAR(m_headerBar), m_btnApply);
    //Menu Tag Actions Button
    m_btnMenuTagActions = gtk_menu_button_new();
    GMenu* menuTagActions{ g_menu_new() };
    GMenu* menuAlbumArt{ g_menu_new() };
    GMenu* menuOtherActions{ g_menu_new() };
    g_menu_append(menuAlbumArt, _("Insert Album Art"), "win.insertAlbumArt");
    g_menu_append(menuAlbumArt, _("Remove Album Art"), "win.removeAlbumArt");
    g_menu_append(menuOtherActions, _("Convert Filename to Tag"), "win.filenameToTag");
    g_menu_append(menuOtherActions, _("Convert Tag to Filename"), "win.tagToFilename");
    g_menu_append(menuTagActions, _("Discard Unapplied Changes"), "win.discardUnappliedChanges");
    g_menu_append(menuTagActions, _("Delete Tags"), "win.deleteTags");
    g_menu_append_section(menuTagActions, nullptr, G_MENU_MODEL(menuAlbumArt));
    g_menu_append_section(menuTagActions, nullptr, G_MENU_MODEL(menuOtherActions));
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(m_btnMenuTagActions), "document-properties-symbolic");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(m_btnMenuTagActions), G_MENU_MODEL(menuTagActions));
    m_popoverListMusicFiles = gtk_popover_menu_new_from_model(G_MENU_MODEL(menuTagActions));
    gtk_widget_set_tooltip_text(m_btnMenuTagActions, _("Tag Actions"));
    gtk_widget_set_visible(m_btnMenuTagActions, false);
    adw_header_bar_pack_end(ADW_HEADER_BAR(m_headerBar), m_btnMenuTagActions);
    g_object_unref(menuAlbumArt);
    g_object_unref(menuOtherActions);
    g_object_unref(menuTagActions);
    //Menu Web Services Button
    m_btnMenuWebServices = gtk_menu_button_new();
    GMenu* menuWebServices{ g_menu_new() };
    g_menu_append(menuWebServices, _("Download MusicBrainz Metadata"), "win.downloadMusicBrainzMetadata");
    g_menu_append(menuWebServices, _("Submit to AcoustId"), "win.submitToAcoustId");
    gtk_menu_button_set_icon_name(GTK_MENU_BUTTON(m_btnMenuWebServices), "web-browser-symbolic");
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(m_btnMenuWebServices), G_MENU_MODEL(menuWebServices));
    gtk_widget_set_tooltip_text(m_btnMenuWebServices, _("Web Services"));
    gtk_widget_set_visible(m_btnMenuWebServices, false);
    adw_header_bar_pack_end(ADW_HEADER_BAR(m_headerBar), m_btnMenuWebServices);
    g_object_unref(menuWebServices);
    //Toast Overlay
    m_toastOverlay = adw_toast_overlay_new();
    gtk_widget_set_hexpand(m_toastOverlay, true);
    gtk_widget_set_vexpand(m_toastOverlay, true);
    //No Files Status Page
    m_pageStatusNoFiles = adw_status_page_new();
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_pageStatusNoFiles), "org.nickvision.tagger-symbolic");
    adw_status_page_set_title(ADW_STATUS_PAGE(m_pageStatusNoFiles), _("No Music Files Found"));
    adw_status_page_set_description(ADW_STATUS_PAGE(m_pageStatusNoFiles), _("Open a folder (or drag one into the app) with music files inside to get started."));
    //Tagger Flap Page
    m_pageFlapTagger = adw_flap_new();
    adw_flap_set_flap_position(ADW_FLAP(m_pageFlapTagger), GTK_PACK_END);
    adw_flap_set_reveal_flap(ADW_FLAP(m_pageFlapTagger), false);
    adw_flap_set_fold_policy(ADW_FLAP(m_pageFlapTagger), ADW_FLAP_FOLD_POLICY_NEVER);
    //Box Search
    m_boxSearch = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    //Text Search Music Files
    m_txtSearchMusicFiles = gtk_search_entry_new();
    gtk_widget_set_hexpand(m_txtSearchMusicFiles, true);
    g_object_set(m_txtSearchMusicFiles, "placeholder-text", _("Search for filename (type ! to activate advanced search)..."), nullptr);
    gtk_box_append(GTK_BOX(m_boxSearch), m_txtSearchMusicFiles);
    g_signal_connect(m_txtSearchMusicFiles, "search-changed", G_CALLBACK((void (*)(GtkSearchEntry*, gpointer))[](GtkSearchEntry*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtSearchMusicFilesChanged(); }), this);
    //Button Advanced Search Info
    m_btnAdvancedSearchInfo = gtk_button_new();
    //gtk_style_context_remove_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_btnAdvancedSearchInfo)), "circular");
    gtk_button_set_icon_name(GTK_BUTTON(m_btnAdvancedSearchInfo), "help-faq-symbolic");
    gtk_widget_set_tooltip_text(m_btnAdvancedSearchInfo, _("Advanced Search Info"));
    gtk_widget_set_visible(m_btnAdvancedSearchInfo, false);
    gtk_actionable_set_action_name(GTK_ACTIONABLE(m_btnAdvancedSearchInfo), "win.advancedSearchInfo");
    gtk_box_append(GTK_BOX(m_boxSearch), m_btnAdvancedSearchInfo);
    //List Music Files
    m_listMusicFiles = gtk_list_box_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(m_listMusicFiles), "boxed-list");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(m_listMusicFiles), GTK_SELECTION_MULTIPLE);
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(m_listMusicFiles), false);
    g_signal_connect(m_listMusicFiles, "selected-rows-changed", G_CALLBACK((void (*)(GtkListBox*, gpointer))[](GtkListBox*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onListMusicFilesSelectionChanged(); }), this);
    //List Music Files Popover
    gtk_widget_set_parent(m_popoverListMusicFiles, m_listMusicFiles);
    gtk_popover_set_position(GTK_POPOVER(m_popoverListMusicFiles), GTK_POS_BOTTOM);
    gtk_popover_set_has_arrow(GTK_POPOVER(m_popoverListMusicFiles), false);
    gtk_widget_set_halign(m_popoverListMusicFiles, GTK_ALIGN_START);
    //List Music Files Right Click
    m_gestureListMusicFiles = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(m_gestureListMusicFiles), 3);
    gtk_gesture_single_set_exclusive(GTK_GESTURE_SINGLE(m_gestureListMusicFiles), true);
    gtk_widget_add_controller(m_listMusicFiles, GTK_EVENT_CONTROLLER(m_gestureListMusicFiles));
    g_signal_connect(m_gestureListMusicFiles, "pressed", G_CALLBACK((void (*)(GtkGesture*, int, double, double, gpointer))[](GtkGesture*, int n_press, double x, double y, gpointer data) { reinterpret_cast<MainWindow*>(data)->onListMusicFilesRightClicked(n_press, x, y); }), this);
    //Tagger Flap Content
    m_scrollTaggerContent = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(m_scrollTaggerContent, true);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrollTaggerContent), m_listMusicFiles);
    m_boxTaggerContent = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(m_boxTaggerContent, 10);
    gtk_widget_set_margin_top(m_boxTaggerContent, 10);
    gtk_widget_set_margin_end(m_boxTaggerContent, 10);
    gtk_widget_set_margin_bottom(m_boxTaggerContent, 10);
    gtk_box_append(GTK_BOX(m_boxTaggerContent), m_boxSearch);
    gtk_box_append(GTK_BOX(m_boxTaggerContent), m_scrollTaggerContent);
    adw_flap_set_content(ADW_FLAP(m_pageFlapTagger), m_boxTaggerContent);
    //Tagger Flap Separator
    m_sepTagger = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    adw_flap_set_separator(ADW_FLAP(m_pageFlapTagger), m_sepTagger);
    //Album Art Stack
    m_stackAlbumArt = adw_view_stack_new();
    gtk_widget_set_halign(m_stackAlbumArt, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(m_stackAlbumArt, 280, 280);
    //No Album Art
    m_btnNoAlbumArt = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(m_btnNoAlbumArt), "card");
    g_signal_connect(m_btnNoAlbumArt, "clicked", G_CALLBACK((void (*)(GtkButton*, gpointer))[](GtkButton*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onInsertAlbumArt(); }), this);
    m_statusNoAlbumArt = adw_status_page_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(m_statusNoAlbumArt), "compact");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_statusNoAlbumArt), "image-missing-symbolic");
    gtk_button_set_child(GTK_BUTTON(m_btnNoAlbumArt), m_statusNoAlbumArt);
    adw_view_stack_add_named(ADW_VIEW_STACK(m_stackAlbumArt), m_btnNoAlbumArt, "noImage");
    //Album Art
    m_btnAlbumArt = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(m_btnAlbumArt), "card");
    g_signal_connect(m_btnAlbumArt, "clicked", G_CALLBACK((void (*)(GtkButton*, gpointer))[](GtkButton*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onInsertAlbumArt(); }), this);
    m_frmAlbumArt = gtk_frame_new(nullptr);
    m_imgAlbumArt = gtk_image_new();
    gtk_frame_set_child(GTK_FRAME(m_frmAlbumArt), m_imgAlbumArt);
    gtk_button_set_child(GTK_BUTTON(m_btnAlbumArt), m_frmAlbumArt);
    adw_view_stack_add_named(ADW_VIEW_STACK(m_stackAlbumArt), m_btnAlbumArt, "image");
    //Keep Album Art
    m_btnKeepAlbumArt = gtk_button_new();
    gtk_widget_set_tooltip_text(m_btnKeepAlbumArt, _("Selected files have different album art images."));
    gtk_style_context_add_class(gtk_widget_get_style_context(m_btnKeepAlbumArt), "card");
    g_signal_connect(m_btnKeepAlbumArt, "clicked", G_CALLBACK((void (*)(GtkButton*, gpointer))[](GtkButton*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onInsertAlbumArt(); }), this);
    m_statusKeepAlbumArt = adw_status_page_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(m_statusKeepAlbumArt), "compact");
    adw_status_page_set_icon_name(ADW_STATUS_PAGE(m_statusKeepAlbumArt), "folder-music-symbolic");
    gtk_button_set_child(GTK_BUTTON(m_btnKeepAlbumArt), m_statusKeepAlbumArt);
    adw_view_stack_add_named(ADW_VIEW_STACK(m_stackAlbumArt), m_btnKeepAlbumArt, "keepImage");
    //Properties Group
    m_adwGrpProperties = adw_preferences_group_new();
    //Filename
    m_txtFilename = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtFilename), _("Filename"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtFilename);
    g_signal_connect(m_txtFilename, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Title
    m_txtTitle = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtTitle), _("Title"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtTitle);
    g_signal_connect(m_txtTitle, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Artist
    m_txtArtist = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtArtist), _("Artist"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtArtist);
    g_signal_connect(m_txtArtist, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Album
    m_txtAlbum = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtAlbum), _("Album"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtAlbum);
    g_signal_connect(m_txtAlbum, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Year
    m_txtYear = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtYear), _("Year"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtYear);
    g_signal_connect(m_txtYear, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Track
    m_txtTrack = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtTrack), _("Track"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtTrack);
    g_signal_connect(m_txtTrack, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Album Artist
    m_txtAlbumArtist = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtAlbumArtist), _("Album Artist"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtAlbumArtist);
    g_signal_connect(m_txtAlbumArtist, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Genre
    m_txtGenre = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtGenre), _("Genre"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtGenre);
    g_signal_connect(m_txtGenre, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Comment
    m_txtComment = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtComment), _("Comment"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtComment);
    g_signal_connect(m_txtComment, "changed", G_CALLBACK((void (*)(GtkEditable*, gpointer))[](GtkEditable*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTxtTagPropertyChanged(); }), this);
    //Duration
    m_txtDuration = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtDuration), _("Duration"));
    gtk_editable_set_editable(GTK_EDITABLE(m_txtDuration), false);
    gtk_editable_set_text(GTK_EDITABLE(m_txtDuration), "00:00:00");
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtDuration);
    //Chromaprint Fingerprint
    m_txtChromaprintFingerprint = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtChromaprintFingerprint), _("Fingerprint"));
    gtk_editable_set_editable(GTK_EDITABLE(m_txtChromaprintFingerprint), false);
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtChromaprintFingerprint);
    //File Size
    m_txtFileSize = adw_entry_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_txtFileSize), _("File Size"));
    gtk_editable_set_editable(GTK_EDITABLE(m_txtFileSize), false);
    gtk_editable_set_text(GTK_EDITABLE(m_txtFileSize), _("0 MB"));
    adw_preferences_group_add(ADW_PREFERENCES_GROUP(m_adwGrpProperties), m_txtFileSize);
    //Tagger Flap Flap
    m_scrollTaggerFlap = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(m_scrollTaggerFlap, true);
    m_boxTaggerFlap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 40);
    gtk_widget_set_margin_start(m_boxTaggerFlap, 80);
    gtk_widget_set_margin_top(m_boxTaggerFlap, 20);
    gtk_widget_set_margin_end(m_boxTaggerFlap, 80);
    gtk_widget_set_margin_bottom(m_boxTaggerFlap, 20);
    gtk_box_append(GTK_BOX(m_boxTaggerFlap), m_stackAlbumArt);
    gtk_box_append(GTK_BOX(m_boxTaggerFlap), m_adwGrpProperties);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(m_scrollTaggerFlap), m_boxTaggerFlap);
    adw_flap_set_flap(ADW_FLAP(m_pageFlapTagger), m_scrollTaggerFlap);
    //View Stack
    m_viewStack = adw_view_stack_new();
    adw_view_stack_add_named(ADW_VIEW_STACK(m_viewStack), m_pageStatusNoFiles, "pageNoFiles");
    adw_view_stack_add_named(ADW_VIEW_STACK(m_viewStack), m_pageFlapTagger, "pageTagger");
    adw_toast_overlay_set_child(ADW_TOAST_OVERLAY(m_toastOverlay), m_viewStack);
    //Main Box
    m_mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(m_mainBox), m_headerBar);
    gtk_box_append(GTK_BOX(m_mainBox), m_toastOverlay);
    adw_application_window_set_content(ADW_APPLICATION_WINDOW(m_gobj), m_mainBox);
    //Send Toast Callback
    m_controller.registerSendToastCallback([&](const std::string& message) { adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(m_toastOverlay), adw_toast_new(message.c_str())); });
    //Music Folder Updated Callback
    m_controller.registerMusicFolderUpdatedCallback([&](bool sendToast) { onMusicFolderUpdated(sendToast); });
    //Music Files Saved Updated Callback
    m_controller.registerMusicFilesSavedUpdatedCallback([&]() { onMusicFilesSavedUpdated(); });
    //Open Music Folder Action
    m_actOpenMusicFolder = g_simple_action_new("openMusicFolder", nullptr);
    g_signal_connect(m_actOpenMusicFolder, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onOpenMusicFolder(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actOpenMusicFolder));
    gtk_application_set_accels_for_action(application, "win.openMusicFolder", new const char*[2]{ "<Ctrl>o", nullptr });
    //Reload Music Folder Action
    m_actReloadMusicFolder = g_simple_action_new("reloadMusicFolder", nullptr);
    g_signal_connect(m_actReloadMusicFolder, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onReloadMusicFolder(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actReloadMusicFolder));
    gtk_application_set_accels_for_action(application, "win.reloadMusicFolder", new const char*[2]{ "F5", nullptr });
    //Apply Action
    m_actApply = g_simple_action_new("apply", nullptr);
    g_signal_connect(m_actApply, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onApply(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actApply));
    gtk_application_set_accels_for_action(application, "win.apply", new const char*[2]{ "<Ctrl>s", nullptr });
    //Discard Unapplied Changes Action
    m_actDiscardUnappliedChanges = g_simple_action_new("discardUnappliedChanges", nullptr);
    g_signal_connect(m_actDiscardUnappliedChanges, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onDiscardUnappliedChanges(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actDiscardUnappliedChanges));
    gtk_application_set_accels_for_action(application, "win.discardUnappliedChanges", new const char*[2]{ "<Ctrl>z", nullptr });
    //Delete Tags Action
    m_actDeleteTags = g_simple_action_new("deleteTags", nullptr);
    g_signal_connect(m_actDeleteTags, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onDeleteTags(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actDeleteTags));
    gtk_application_set_accels_for_action(application, "win.deleteTags", new const char*[2]{ "Delete", nullptr });
    //Insert Album Art
    m_actInsertAlbumArt = g_simple_action_new("insertAlbumArt", nullptr);
    g_signal_connect(m_actInsertAlbumArt, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onInsertAlbumArt(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actInsertAlbumArt));
    gtk_application_set_accels_for_action(application, "win.insertAlbumArt", new const char*[2]{ "<Ctrl><Shift>o", nullptr });
    //Remove Album Art
    m_actRemoveAlbumArt = g_simple_action_new("removeAlbumArt", nullptr);
    g_signal_connect(m_actRemoveAlbumArt, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onRemoveAlbumArt(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actRemoveAlbumArt));
    gtk_application_set_accels_for_action(application, "win.removeAlbumArt", new const char*[2]{ "<Ctrl>Delete", nullptr });
    //Filename to Tag
    m_actFilenameToTag = g_simple_action_new("filenameToTag", nullptr);
    g_signal_connect(m_actFilenameToTag, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onFilenameToTag(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actFilenameToTag));
    gtk_application_set_accels_for_action(application, "win.filenameToTag", new const char*[2]{ "<Ctrl>f", nullptr });
    //Tag to Filename
    m_actTagToFilename = g_simple_action_new("tagToFilename", nullptr);
    g_signal_connect(m_actTagToFilename, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onTagToFilename(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actTagToFilename));
    gtk_application_set_accels_for_action(application, "win.tagToFilename", new const char*[2]{ "<Ctrl>t", nullptr });
    //Download MusicBrainz Metadata
    m_actDownloadMusicBrainzMetadata = g_simple_action_new("downloadMusicBrainzMetadata", nullptr);
    g_signal_connect(m_actDownloadMusicBrainzMetadata, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onDownloadMusicBrainzMetadata(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actDownloadMusicBrainzMetadata));
    gtk_application_set_accels_for_action(application, "win.downloadMusicBrainzMetadata", new const char*[2]{ "<Ctrl>m", nullptr });
    //Submit to AcoustId
    m_actSubmitToAcoustId = g_simple_action_new("submitToAcoustId", nullptr);
    g_signal_connect(m_actSubmitToAcoustId, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onSubmitToAcoustId(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actSubmitToAcoustId));
    gtk_application_set_accels_for_action(application, "win.submitToAcoustId", new const char*[2]{ "<Ctrl>u", nullptr });
    //Preferences Action
    m_actPreferences = g_simple_action_new("preferences", nullptr);
    g_signal_connect(m_actPreferences, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onPreferences(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actPreferences));
    gtk_application_set_accels_for_action(application, "win.preferences", new const char*[2]{ "<Ctrl>comma", nullptr });
    //Keyboard Shortcuts Action
    m_actKeyboardShortcuts = g_simple_action_new("keyboardShortcuts", nullptr);
    g_signal_connect(m_actKeyboardShortcuts, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onKeyboardShortcuts(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actKeyboardShortcuts));
    gtk_application_set_accels_for_action(application, "win.keyboardShortcuts", new const char*[2]{ "<Ctrl>question", nullptr });
    //About Action
    m_actAbout = g_simple_action_new("about", nullptr);
    g_signal_connect(m_actAbout, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onAbout(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actAbout));
    gtk_application_set_accels_for_action(application, "win.about", new const char*[2]{ "F1", nullptr });
    //Advanced Search Info Action
    m_actAdvancedSearchInfo = g_simple_action_new("advancedSearchInfo", nullptr);
    g_signal_connect(m_actAdvancedSearchInfo, "activate", G_CALLBACK((void (*)(GSimpleAction*, GVariant*, gpointer))[](GSimpleAction*, GVariant*, gpointer data) { reinterpret_cast<MainWindow*>(data)->onAdvancedSearchInfo(); }), this);
    g_action_map_add_action(G_ACTION_MAP(m_gobj), G_ACTION(m_actAdvancedSearchInfo));
    //Drop Target
    m_dropTarget = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
    g_signal_connect(m_dropTarget, "drop", G_CALLBACK((int (*)(GtkDropTarget*, const GValue*, gdouble, gdouble, gpointer))[](GtkDropTarget*, const GValue* value, gdouble, gdouble, gpointer data) -> int { return reinterpret_cast<MainWindow*>(data)->onDrop(value); }), this);
    gtk_widget_add_controller(m_gobj, GTK_EVENT_CONTROLLER(m_dropTarget));
}

GtkWidget* MainWindow::gobj()
{
    return m_gobj;
}

void MainWindow::start()
{
    gtk_widget_show(m_gobj);
    m_controller.startup();
}

bool MainWindow::onCloseRequest()
{
    if(!m_controller.getCanClose())
    {
        MessageDialog messageDialog{ GTK_WINDOW(m_gobj), _("Apply Changes?"), _("Some music files still have changes waiting to be applied. Would you like to apply those changes to the file or discard them?"), _("Cancel"), _("Discard"), _("Apply") };
        MessageDialogResponse response{ messageDialog.run() };
        if(response == MessageDialogResponse::Cancel)
        {
            return true;
        }
        else if(response == MessageDialogResponse::Suggested)
        {
            onApply();
        }
    }
    gtk_list_box_unselect_all(GTK_LIST_BOX(m_listMusicFiles));
    gtk_widget_unparent(m_popoverListMusicFiles);
    return false;
}

void MainWindow::onMusicFolderUpdated(bool sendToast)
{
    adw_window_title_set_subtitle(ADW_WINDOW_TITLE(m_adwTitle), m_controller.getMusicFolderPath().c_str());
    gtk_widget_set_visible(m_btnReloadMusicFolder, !m_controller.getMusicFolderPath().empty());
    gtk_list_box_unselect_all(GTK_LIST_BOX(m_listMusicFiles));
    for(GtkWidget* row : m_listMusicFilesRows)
    {
        gtk_list_box_remove(GTK_LIST_BOX(m_listMusicFiles), row);
    }
    m_listMusicFilesRows.clear();
    ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Loading music files..."), [&]() { m_controller.reloadMusicFolder(); } };
    progressDialog.run();
    std::size_t musicFilesCount{ m_controller.getMusicFiles().size() };
    adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_viewStack), musicFilesCount > 0 ? "pageTagger" : "pageNoFiles");
    for(const std::shared_ptr<MusicFile>& musicFile : m_controller.getMusicFiles())
    {
        GtkWidget* row{ adw_action_row_new() };
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), std::regex_replace(musicFile->getFilename(), std::regex("\\&"), "&amp;").c_str());
        gtk_list_box_append(GTK_LIST_BOX(m_listMusicFiles), row);
        m_listMusicFilesRows.push_back(row);
        g_main_context_iteration(g_main_context_default(), false);
    }
    if(musicFilesCount > 0 && sendToast)
    {
        adw_toast_overlay_add_toast(ADW_TOAST_OVERLAY(m_toastOverlay), adw_toast_new(StringHelpers::format(_("Loaded %d music files."), musicFilesCount).c_str()));
    }
}

void MainWindow::onMusicFilesSavedUpdated()
{
    size_t i{ 0 };
    for(bool saved : m_controller.getMusicFilesSaved())
    {
        adw_action_row_set_icon_name(ADW_ACTION_ROW(m_listMusicFilesRows[i]), !saved ? "document-modified-symbolic" : "");
        i++;
    }
}

void MainWindow::onOpenMusicFolder()
{
    GtkFileChooserNative* openFolderDialog{ gtk_file_chooser_native_new(_("Open Music Folder"), GTK_WINDOW(m_gobj), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Open"), _("_Cancel")) };
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(openFolderDialog), true);
    g_signal_connect(openFolderDialog, "response", G_CALLBACK((void (*)(GtkNativeDialog*, gint, gpointer))([](GtkNativeDialog* dialog, gint response_id, gpointer data)
    {
        if(response_id == GTK_RESPONSE_ACCEPT)
        {
            MainWindow* mainWindow{ reinterpret_cast<MainWindow*>(data) };
            GFile* file{ gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)) };
            mainWindow->m_controller.openMusicFolder(g_file_get_path(file));
            g_object_unref(file);
        }
        g_object_unref(dialog);
    })), this);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(openFolderDialog));
}

void MainWindow::onReloadMusicFolder()
{
    if(!m_controller.getCanClose())
    {
        MessageDialog messageDialog{ GTK_WINDOW(m_gobj), _("Apply Changes?"), _("Some music files still have changes waiting to be applied. Would you like to apply those changes to the file or discard them?"), _("Cancel"), _("Discard"), _("Apply") };
        MessageDialogResponse response{ messageDialog.run() };
        if(response == MessageDialogResponse::Suggested)
        {
            onApply();
        }
        if(response != MessageDialogResponse::Cancel)
        {
            onMusicFolderUpdated(true);
        }
    }
    else
    {
        onMusicFolderUpdated(true);
    }
}

void MainWindow::onApply()
{
    ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Saving tags..."), [&]() { m_controller.saveTags(); } };
    progressDialog.run();
    onTxtSearchMusicFilesChanged();
}

void MainWindow::onDiscardUnappliedChanges()
{
    ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Discarding unapplied changes..."), [&]() { m_controller.discardUnappliedChanges(); } };
    progressDialog.run();
    onListMusicFilesSelectionChanged();
}

void MainWindow::onDeleteTags()
{
    ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Deleting tags..."), [&]() { m_controller.deleteTags(); } };
    progressDialog.run();
    onListMusicFilesSelectionChanged();
}

void MainWindow::onInsertAlbumArt()
{
    GtkFileChooserNative* openPictureDialog{ gtk_file_chooser_native_new(_("Insert Album Art"), GTK_WINDOW(m_gobj), GTK_FILE_CHOOSER_ACTION_OPEN, _("_Open"), _("_Cancel")) };
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(openPictureDialog), true);
    GtkFileFilter* imageFilter{ gtk_file_filter_new() };
    gtk_file_filter_add_mime_type(imageFilter, "image/*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(openPictureDialog), imageFilter);
    g_object_unref(imageFilter);
    g_signal_connect(openPictureDialog, "response", G_CALLBACK((void (*)(GtkNativeDialog*, gint, gpointer))([](GtkNativeDialog* dialog, gint response_id, gpointer data)
    {
        if(response_id == GTK_RESPONSE_ACCEPT)
        {
            MainWindow* mainWindow{ reinterpret_cast<MainWindow*>(data) };
            GFile* file{ gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)) };
            std::string path{ g_file_get_path(file) };
            ProgressDialog progressDialog{ GTK_WINDOW(mainWindow->m_gobj), _("Inserting album art..."), [mainWindow, path]() { mainWindow->m_controller.insertAlbumArt(path); } };
            progressDialog.run();
            mainWindow->onListMusicFilesSelectionChanged();
            g_object_unref(file);
        }
        g_object_unref(dialog);
    })), this);
    gtk_native_dialog_show(GTK_NATIVE_DIALOG(openPictureDialog));
}

void MainWindow::onRemoveAlbumArt()
{
    ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Removing album art..."), [&]() { m_controller.removeAlbumArt(); } };
    progressDialog.run();
    onListMusicFilesSelectionChanged();
}

void MainWindow::onFilenameToTag()
{
    ComboBoxDialog formatStringDialog{ GTK_WINDOW(m_gobj), _("Filename to Tag"), _("Please select a format string."), _("Format String"), { "%artist%- %title%", "%title%- %artist%", "%track%- %title%", "%title%" } };
    std::string formatString = formatStringDialog.run();
    if(!formatString.empty())
    {
        ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Converting filenames to tags..."), [&, formatString]() { m_controller.filenameToTag(formatString); } };
        progressDialog.run();
        onListMusicFilesSelectionChanged();
    }
}

void MainWindow::onTagToFilename()
{
    ComboBoxDialog formatStringDialog{ GTK_WINDOW(m_gobj), _("Tag to Filename"), _("Please select a format string."), _("Format String"), { "%artist%- %title%", "%title%- %artist%", "%track%- %title%", "%title%" } };
    std::string formatString = formatStringDialog.run();
    if(!formatString.empty())
    {
        ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Converting tags to filenames..."), [&, formatString]() { m_controller.tagToFilename(formatString); } };
        progressDialog.run();
        size_t i{ 0 };
        for(const std::shared_ptr<MusicFile>& musicFile : m_controller.getMusicFiles())
        {
            if(std::string(adw_preferences_row_get_title(ADW_PREFERENCES_ROW(m_listMusicFilesRows[i]))) != musicFile->getFilename())
            {
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_listMusicFilesRows[i]), std::regex_replace(musicFile->getFilename(), std::regex("\\&"), "&amp;").c_str());
            }
            i++;
        }
    }
}

void MainWindow::onDownloadMusicBrainzMetadata()
{
    ProgressDialog progressDialog{ GTK_WINDOW(m_gobj), _("Downloading MusicBrainz metadata...\n<small>(This may take a while)</small>"), [&]() { m_controller.downloadMusicBrainzMetadata(); } };
    progressDialog.run();
    onListMusicFilesSelectionChanged();
}

void MainWindow::onSubmitToAcoustId()
{
    //Check for one file selected
    if(m_controller.getSelectedMusicFilesCount() > 1)
    {
        MessageDialog messageDialog{ GTK_WINDOW(m_gobj), _("Too Many Files Selected"), _("Only one file can be submitted to AcoustId at a time. Please select only one file and try again."), _("OK") };
        messageDialog.run();
        return;
    }
    //Check for valid AcoustId User API Key
    bool validAcoustIdUserAPIKey{ false };
    ProgressDialog progressDialogChecking{ GTK_WINDOW(m_gobj), _("Checking AcoustId user api key..."), [&]() { validAcoustIdUserAPIKey = m_controller.checkIfAcoustIdUserAPIKeyValid(); } };
    progressDialogChecking.run();
    if(!validAcoustIdUserAPIKey)
    {
        MessageDialog messageDialog{ GTK_WINDOW(m_gobj), _("Invalid User API Key"), _("The AcoustId User API Key is invalid.\nPlease provide a valid api key in Preferences."), _("OK") };
        messageDialog.run();
        return;
    }
    //Get MusicBrainz Recording Id
    EntryDialog entryDialog{ GTK_WINDOW(m_gobj), _("Submit to AcoustId"), _("AcoustId can associate a song's fingerprint with a MusicBrainz Recording Id for easy identification.\n\nIf you have a MusicBrainz Recording Id for this song, please provide it below.\n\nIf no id is provided, Tagger will submit your tag's metadata in association with the fingerprint instead."), _("MusicBrainz Recording Id") };
    std::string result{ entryDialog.run() };
    ProgressDialog progressDialogSubmitting{ GTK_WINDOW(m_gobj), _("Submitting metadata to AcoustId..."), [&, result]() { m_controller.submitToAcoustId(result); } };
    progressDialogSubmitting.run();
}

void MainWindow::onPreferences()
{
    PreferencesDialog preferencesDialog{ GTK_WINDOW(m_gobj), m_controller.createPreferencesDialogController() };
    preferencesDialog.run();
    m_controller.onConfigurationChanged();
}

void MainWindow::onKeyboardShortcuts()
{
    ShortcutsDialog shortcutsDialog{ GTK_WINDOW(m_gobj) };
    shortcutsDialog.run();
}

void MainWindow::onAbout()
{
    adw_show_about_window(GTK_WINDOW(m_gobj),
                          "application-name", m_controller.getAppInfo().getShortName().c_str(),
                          "application-icon", (m_controller.getAppInfo().getId() + (m_controller.getIsDevVersion() ? "-devel" : "")).c_str(),
                          "version", m_controller.getAppInfo().getVersion().c_str(),
                          "comments", m_controller.getAppInfo().getDescription().c_str(),
                          "developer-name", "Nickvision",
                          "license-type", GTK_LICENSE_GPL_3_0,
                          "copyright", "(C) Nickvision 2021-2022",
                          "website", m_controller.getAppInfo().getGitHubRepo().c_str(),
                          "issue-url", m_controller.getAppInfo().getIssueTracker().c_str(),
                          "support-url", m_controller.getAppInfo().getSupportUrl().c_str(),
                          "developers", new const char*[3]{ "Nicholas Logozzo https://github.com/nlogozzo", "Contributors on GitHub ❤️ https://github.com/nlogozzo/NickvisionTagger/graphs/contributors", nullptr },
                          "designers", new const char*[2]{ "Nicholas Logozzo https://github.com/nlogozzo", nullptr },
                          "artists", new const char*[3]{ "David Lapshin https://github.com/daudix-UFO", "noëlle https://github.com/jannuary", nullptr },
                          "release-notes", m_controller.getAppInfo().getChangelog().c_str(),
                          nullptr);
}

void MainWindow::onAdvancedSearchInfo()
{
    MessageDialog messageDialog{ GTK_WINDOW(m_gobj), _("Advanced Search"), _(R"(Advanced Search is a powerful feature provided by Tagger that allows users to search files' tag contents for certain values, using a powerful tag syntax:

    !prop1="value1";prop2="value2"
    Where prop1, prop2 are valid tag properties and value1, value2 are the values to search wrapped in quotes.
    Each property is separated by a comma. Notice how the last property does not end in a comma.

    Valid Properties:
    - filename
    - title
    - artist
    - album
    - year
    - track
    - albumartist
    - genre
    - comment

    Syntax Checking:
    - If the syntax of your string is valid, the textbox will turn green and will filter the listbox with your search
    - If the syntax of your string is invalid, the textbox will turn red and will not filter the listbox

    Examples:
    !artist=""
    This search string will filter the listbox to contain music files who's artist is empty

    !genre="";year="2022"
    This search string will filter the listbox to contain music files who's genre is empty and who's year is 2022
    (Year and Track properties will validate if the value string is a number).

    !title="";artist="bob"
    This search string will filter the listbox to contain music files who's title is empty and who's artist is bob

    * Advanced Search is case insensitive *)"), _("OK") };
    gtk_widget_set_size_request(messageDialog.gobj(), 600, -1);
    messageDialog.run();
}

bool MainWindow::onDrop(const GValue* value)
{
    void* file{ g_value_get_object(value) };
    std::string path{ g_file_get_path(G_FILE(file)) };
    if(std::filesystem::is_directory(path))
    {
        m_controller.openMusicFolder(path);
        return true;
    }
    return false;
}

void MainWindow::onTxtSearchMusicFilesChanged()
{
    std::string* searchEntry{ new std::string(gtk_editable_get_text(GTK_EDITABLE(m_txtSearchMusicFiles))) };
    std::transform(searchEntry->begin(), searchEntry->end(), searchEntry->begin(), ::tolower);
    if(searchEntry->substr(0, 1) == "!")
    {
        gtk_widget_set_visible(m_btnAdvancedSearchInfo, true);
        std::pair<bool, std::vector<std::string>> result{ m_controller.advancedSearch(*searchEntry) };
        if(!result.first)
        {
            gtk_style_context_remove_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_txtSearchMusicFiles)), "success");
            gtk_style_context_add_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_txtSearchMusicFiles)), "error");
            delete searchEntry;
            gtk_list_box_set_filter_func(GTK_LIST_BOX(m_listMusicFiles), [](GtkListBoxRow*, gpointer) -> int
            {
                return true;
            }, nullptr, nullptr);
            return;
        }
        gtk_style_context_remove_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_txtSearchMusicFiles)), "error");
        gtk_style_context_add_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_txtSearchMusicFiles)), "success");
        delete searchEntry;
        gtk_list_box_set_filter_func(GTK_LIST_BOX(m_listMusicFiles), [](GtkListBoxRow* row, gpointer data) -> int
        {
            std::vector<std::string>* filenames{ reinterpret_cast<std::vector<std::string>*>(data) };
            if(filenames->size() == 0)
            {
                return false;
            }
            std::string rowFilename{ adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row)) };
            std::transform(rowFilename.begin(), rowFilename.end(), rowFilename.begin(), ::tolower);
            return std::find(filenames->begin(), filenames->end(), rowFilename) != filenames->end();
        }, new std::vector<std::string>(result.second), [](gpointer data)
        {
            std::vector<std::string>* filenames{ reinterpret_cast<std::vector<std::string>*>(data) };
            delete filenames;
        });
    }
    else
    {
        gtk_widget_set_visible(m_btnAdvancedSearchInfo, false);
        gtk_style_context_remove_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_txtSearchMusicFiles)), "success");
        gtk_style_context_remove_class(GTK_STYLE_CONTEXT(gtk_widget_get_style_context(m_txtSearchMusicFiles)), "error");
        gtk_list_box_set_filter_func(GTK_LIST_BOX(m_listMusicFiles), [](GtkListBoxRow* row, gpointer data) -> int
        {
            std::string* searchEntry{ reinterpret_cast<std::string*>(data) };
            std::string rowFilename{ adw_preferences_row_get_title(ADW_PREFERENCES_ROW(row)) };
            std::transform(rowFilename.begin(), rowFilename.end(), rowFilename.begin(), ::tolower);
            bool found{ false };
            if(searchEntry->empty() || rowFilename.find(*searchEntry) != std::string::npos)
            {
                found = true;
            }
            return found;
        }, searchEntry, [](gpointer data)
        {
            std::string* searchEntry{ reinterpret_cast<std::string*>(data) };
            delete searchEntry;
        });
    }
}

void MainWindow::onListMusicFilesSelectionChanged()
{
    m_isSelectionOccuring = true;
    //Update Selected Music Files
    std::vector<int> selectedIndexes;
    GList* selectedRows{ gtk_list_box_get_selected_rows(GTK_LIST_BOX(m_listMusicFiles)) };
    for(GList* list{ selectedRows }; list; list = list->next)
    {
        selectedIndexes.push_back(gtk_list_box_row_get_index(GTK_LIST_BOX_ROW(list->data)));
    }
    m_controller.updateSelectedMusicFiles(selectedIndexes);
    //Update UI
    gtk_widget_set_visible(m_btnApply, true);
    gtk_widget_set_visible(m_btnMenuTagActions, true);
    gtk_widget_set_visible(m_btnMenuWebServices, true);
    adw_flap_set_reveal_flap(ADW_FLAP(m_pageFlapTagger), true);
    gtk_editable_set_editable(GTK_EDITABLE(m_txtFilename), true);
    if(selectedIndexes.size() == 0)
    {
        gtk_widget_set_visible(m_btnApply, false);
        gtk_widget_set_visible(m_btnMenuTagActions, false);
        gtk_widget_set_visible(m_btnMenuWebServices, false);
        adw_flap_set_reveal_flap(ADW_FLAP(m_pageFlapTagger), false);
        gtk_editable_set_text(GTK_EDITABLE(m_txtSearchMusicFiles), "");
    }
    else if(selectedIndexes.size() > 1)
    {
        gtk_editable_set_editable(GTK_EDITABLE(m_txtFilename), false);
    }
    TagMap tagMap{ m_controller.getSelectedTagMap() };
    gtk_editable_set_text(GTK_EDITABLE(m_txtFilename), tagMap.getFilename().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtTitle), tagMap.getTitle().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtArtist), tagMap.getArtist().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtAlbum), tagMap.getAlbum().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtYear), tagMap.getYear().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtTrack), tagMap.getTrack().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtAlbumArtist), tagMap.getAlbumArtist().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtGenre), tagMap.getGenre().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtComment), tagMap.getComment().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtDuration), tagMap.getDuration().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtChromaprintFingerprint), tagMap.getFingerprint().c_str());
    gtk_editable_set_text(GTK_EDITABLE(m_txtFileSize), tagMap.getFileSize().c_str());
    if(tagMap.getAlbumArt() == "hasArt")
    {
        adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_stackAlbumArt), "image");
        gtk_image_set_from_byte_vector(GTK_IMAGE(m_imgAlbumArt), m_controller.getFirstSelectedMusicFile()->getAlbumArt());
    }
    else if(tagMap.getAlbumArt() == "keepArt")
    {
        adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_stackAlbumArt), "keepImage");
        gtk_image_clear(GTK_IMAGE(m_imgAlbumArt));
    }
    else
    {
        adw_view_stack_set_visible_child_name(ADW_VIEW_STACK(m_stackAlbumArt), "noImage");
        gtk_image_clear(GTK_IMAGE(m_imgAlbumArt));
    }
    m_isSelectionOccuring = false;
}

void MainWindow::onListMusicFilesRightClicked(int n_press, double x, double y)
{
    GdkEventSequence* sequence{ gtk_gesture_single_get_current_sequence(GTK_GESTURE_SINGLE(m_gestureListMusicFiles)) };
    GdkEvent* event{ gtk_gesture_get_last_event(m_gestureListMusicFiles, sequence) };
    if(n_press != 1 || !gdk_event_triggers_context_menu(event))
    {
        return;
    }
    gtk_gesture_set_sequence_state(m_gestureListMusicFiles, sequence, GTK_EVENT_SEQUENCE_CLAIMED);
    if(!gtk_widget_get_visible(m_btnMenuTagActions))
    {
        return;
    }
    GdkRectangle rect{ (int)x, (int)y, 1, 1 };
    gtk_popover_set_pointing_to(GTK_POPOVER(m_popoverListMusicFiles), &rect);
    gtk_popover_popup(GTK_POPOVER(m_popoverListMusicFiles));
}

void MainWindow::onTxtTagPropertyChanged()
{
    if(!m_isSelectionOccuring)
    {
        TagMap tagMap;
        tagMap.setFilename(gtk_editable_get_text(GTK_EDITABLE(m_txtFilename)));
        tagMap.setTitle(gtk_editable_get_text(GTK_EDITABLE(m_txtTitle)));
        tagMap.setArtist(gtk_editable_get_text(GTK_EDITABLE(m_txtArtist)));
        tagMap.setAlbum(gtk_editable_get_text(GTK_EDITABLE(m_txtAlbum)));
        tagMap.setYear(gtk_editable_get_text(GTK_EDITABLE(m_txtYear)));
        tagMap.setTrack(gtk_editable_get_text(GTK_EDITABLE(m_txtTrack)));
        tagMap.setAlbumArtist(gtk_editable_get_text(GTK_EDITABLE(m_txtAlbumArtist)));
        tagMap.setGenre(gtk_editable_get_text(GTK_EDITABLE(m_txtGenre)));
        tagMap.setComment(gtk_editable_get_text(GTK_EDITABLE(m_txtComment)));
        m_controller.updateTags(tagMap);
        size_t i{ 0 };
        for(const std::shared_ptr<MusicFile>& musicFile : m_controller.getMusicFiles())
        {
            if(std::string(adw_preferences_row_get_title(ADW_PREFERENCES_ROW(m_listMusicFilesRows[i]))) != musicFile->getFilename())
            {
                adw_preferences_row_set_title(ADW_PREFERENCES_ROW(m_listMusicFilesRows[i]), std::regex_replace(musicFile->getFilename(), std::regex("\\&"), "&amp;").c_str());
            }
            i++;
        }
    }
}

