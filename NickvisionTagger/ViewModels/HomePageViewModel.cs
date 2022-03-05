﻿using Avalonia.Controls;
using Avalonia.Media.Imaging;
using FluentAvalonia.UI.Controls;
using Nickvision.Avalonia.MVVM;
using Nickvision.Avalonia.MVVM.Commands;
using Nickvision.Avalonia.MVVM.Messaging;
using Nickvision.Avalonia.MVVM.Services;
using NickvisionTagger.Extensions;
using NickvisionTagger.Models;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Threading.Tasks;

namespace NickvisionTagger.ViewModels;

public class HomePageViewModel : ViewModelBase
{
    private readonly ServiceCollection _serviceCollection;
    private bool _tagFilenameEnabled;
    private string _tagFilename;
    private string _tagTitle;
    private string _tagArtist;
    private string _tagAlbum;
    private string _tagYear;
    private string _tagTrack;
    private string _tagAlbumArtist;
    private string _tagComposer;
    private string _tagGenre;
    private string _tagComment;
    private string _tagDuration;
    private string _tagFileSize;
    private Bitmap? _tagAlbumArt;
    private string _tagPath;
    private string _fttFormatString;
    private string _ttfFormatString;

    public MusicFolder MusicFolder { get; init; }
    public ObservableCollection<string> FormatStrings { get; init; }
    public ObservableCollection<MusicFile> SelectedMusicFiles { get; init; }
    public DelegateAsyncCommand<object?> OpenedCommand { get; init; }
    public DelegateAsyncCommand<object?> OpenMusicFolderCommand { get; init; }
    public DelegateAsyncCommand<object?> RefreshMusicFolderCommand { get; init; }
    public DelegateAsyncCommand<object?> CloseMusicFolderCommand { get; set; }
    public DelegateAsyncCommand<object?> SaveTagsCommand { get; init; }
    public DelegateAsyncCommand<object?> RemoveTagsCommand { get; init; }
    public DelegateAsyncCommand<object?> InsertAlbumArtCommand { get; init; }
    public DelegateAsyncCommand<object?> FilenameToTagCommand { get; init; }
    public DelegateAsyncCommand<object?> TagToFilenameCommand { get; init; }
    public DelegateCommand<IList?> DataMusicFiles_SelectionChangedCommand { get; init; }

    public HomePageViewModel(ServiceCollection serviceCollection)
    {
        Title = "Home";
        _serviceCollection = serviceCollection;
        _tagFilenameEnabled = true;
        _tagFilename = "";
        _tagTitle = "";
        _tagArtist = "";
        _tagAlbum = "";
        _tagYear = "";
        _tagTrack = "";
        _tagAlbumArtist = "";
        _tagComposer = "";
        _tagGenre = "";
        _tagComment = "";
        _tagDuration = "";
        _tagFileSize = "";
        _tagAlbumArt = null;
        _tagPath = "";
        _fttFormatString = "%artist%- %title%";
        _ttfFormatString = "%artist%- %title%";
        MusicFolder = new MusicFolder();
        FormatStrings = new ObservableCollection<string>() { "%artist%- %title%", "%title%- %artist%", "%title%" };
        SelectedMusicFiles = new ObservableCollection<MusicFile>();
        OpenedCommand = new DelegateAsyncCommand<object?>(Opened);
        OpenMusicFolderCommand = new DelegateAsyncCommand<object?>(OpenMusicFolder);
        RefreshMusicFolderCommand = new DelegateAsyncCommand<object?>(RefreshMusicFolder, () => MusicFolder.Path != "No Folder Open");
        CloseMusicFolderCommand = new DelegateAsyncCommand<object?>(CloseMusicFolder, () => MusicFolder.Path != "No Folder Open");
        SaveTagsCommand = new DelegateAsyncCommand<object?>(SaveTags, () => SelectedMusicFiles.Count > 0);
        RemoveTagsCommand = new DelegateAsyncCommand<object?>(RemoveTags, () => SelectedMusicFiles.Count > 0);
        InsertAlbumArtCommand = new DelegateAsyncCommand<object?>(InsertAlbumArt, () => SelectedMusicFiles.Count > 0);
        FilenameToTagCommand = new DelegateAsyncCommand<object?>(FilenameToTag, () => SelectedMusicFiles.Count > 0);
        TagToFilenameCommand = new DelegateAsyncCommand<object?>(TagToFilename, () => SelectedMusicFiles.Count > 0);
        DataMusicFiles_SelectionChangedCommand = new DelegateCommand<IList?>(DataMusicFiles_SelectionChanged);
        Messenger.Default.Register("HomeSettingsUpdate", SettingsUpdate);
    }

    public bool TagFilenameEnabled
    {
        get => _tagFilenameEnabled;

        set => SetProperty(ref _tagFilenameEnabled, value);
    }

    public string TagFilename
    {
        get => _tagFilename;

        set => SetProperty(ref _tagFilename, value);
    }

    public string TagTitle
    {
        get => _tagTitle;

        set => SetProperty(ref _tagTitle, value);
    }

    public string TagArtist
    {
        get => _tagArtist;

        set => SetProperty(ref _tagArtist, value);
    }

    public string TagAlbum
    {
        get => _tagAlbum;

        set => SetProperty(ref _tagAlbum, value);
    }

    public string TagYear
    {
        get => _tagYear;

        set => SetProperty(ref _tagYear, value);
    }

    public string TagTrack
    {
        get => _tagTrack;

        set => SetProperty(ref _tagTrack, value);
    }

    public string TagAlbumArtist
    {
        get => _tagAlbumArtist;

        set => SetProperty(ref _tagAlbumArtist, value);
    }

    public string TagComposer
    {
        get => _tagComposer;

        set => SetProperty(ref _tagComposer, value);
    }

    public string TagGenre
    {
        get => _tagGenre;

        set => SetProperty(ref _tagGenre, value);
    }

    public string TagComment
    {
        get => _tagComment;

        set => SetProperty(ref _tagComment, value);
    }

    public string TagDuration
    {
        get => _tagDuration;

        set => SetProperty(ref _tagDuration, value);
    }

    public string TagFileSize
    {
        get => _tagFileSize;

        set => SetProperty(ref _tagFileSize, value);
    }

    public Bitmap? TagAlbumArt
    {
        get => _tagAlbumArt;

        set => SetProperty(ref _tagAlbumArt, value);
    }

    public string TagPath
    {
        get => _tagPath;

        set => SetProperty(ref _tagPath, value);
    }

    public string FttFormatString
    {
        get => _fttFormatString;

        set => SetProperty(ref _fttFormatString, value);
    }

    public string TtfFormatString
    {
        get => _ttfFormatString;

        set => SetProperty(ref _ttfFormatString, value);
    }

    private async Task Opened(object? parameter)
    {
        var config = await Configuration.LoadAsync();
        if (config.RememberLastOpenedFolder)
        {
            MusicFolder.Path = config.LastOpenedFolder;
            RefreshMusicFolderCommand.RaiseCanExecuteChanged();
            CloseMusicFolderCommand.RaiseCanExecuteChanged();
            await RefreshMusicFolder(null);
        }
    }

    private async Task OpenMusicFolder(object? parameter)
    {
        var result = await _serviceCollection.GetService<IIOService>()?.ShowOpenFolderDialogAsync("Select Music Folder")!;
        if (result != null)
        {
            MusicFolder.Path = result;
            RefreshMusicFolderCommand.RaiseCanExecuteChanged();
            CloseMusicFolderCommand.RaiseCanExecuteChanged();
            var config = await Configuration.LoadAsync();
            if (config.RememberLastOpenedFolder)
            {
                config.LastOpenedFolder = MusicFolder.Path;
                await config.SaveAsync();
            }
            await RefreshMusicFolder(null);
        }
    }

    private async Task RefreshMusicFolder(object? parameter) => await _serviceCollection.GetService<IProgressDialogService>()?.ShowAsync("Loading music files...", async () => await MusicFolder.ReloadFilesAsync())!;

    private async Task CloseMusicFolder(object? parameter)
    {
        MusicFolder.CloseFolder();
        RefreshMusicFolderCommand.RaiseCanExecuteChanged();
        CloseMusicFolderCommand.RaiseCanExecuteChanged();
        var config = await Configuration.LoadAsync();
        if (config.RememberLastOpenedFolder)
        {
            config.LastOpenedFolder = MusicFolder.Path;
            await config.SaveAsync();
        }
    }

    private async Task SaveTags(object? parameter)
    {
        await _serviceCollection.GetService<IProgressDialogService>()?.ShowAsync("Saving tags...", async () =>
        {
            await Task.Run(() =>
            {
                foreach (var musicFile in SelectedMusicFiles)
                {
                    if (TagFilename != musicFile.Filename && TagFilename != "<keep>")
                    {
                        musicFile.Filename = TagFilename;
                    }
                    if (TagTitle != "<keep>")
                    {
                        musicFile.Title = TagTitle;
                    }
                    if (TagArtist != "<keep>")
                    {
                        musicFile.Artist = TagArtist;
                    }
                    if (TagAlbum != "<keep>")
                    {
                        musicFile.Album = TagAlbum;
                    }
                    if (TagYear != "<keep>")
                    {
                        musicFile.Year = string.IsNullOrEmpty(TagYear) ? null : int.Parse(TagYear);
                    }
                    if (TagTrack != "<keep>")
                    {
                        musicFile.Track = string.IsNullOrEmpty(TagTrack) ? null : int.Parse(TagYear);
                    }
                    if (TagAlbumArtist != "<keep>")
                    {
                        musicFile.AlbumArtist = TagAlbumArtist;
                    }
                    if (TagComposer != "<keep>")
                    {
                        musicFile.Composer = TagComposer;
                    }
                    if (TagGenre != "<keep>")
                    {
                        musicFile.Genre = TagGenre;
                    }
                    if (TagComment != "<keep>")
                    {
                        musicFile.Comment = TagComment;
                    }
                    if (TagAlbumArt != null)
                    {
                        musicFile.AlbumArt = TagAlbumArt;
                    }
                    musicFile.SaveTag();
                }
            });
        })!;
        await RefreshMusicFolder(null);
    }

    private async Task RemoveTags(object? parameter)
    {
        await _serviceCollection.GetService<IProgressDialogService>()?.ShowAsync("Removing tags...", async () =>
        {
            await Task.Run(() =>
            {
                foreach (var musicFile in SelectedMusicFiles)
                {
                    musicFile.RemoveTag();
                }
            });
        })!;
        await RefreshMusicFolder(null);
    }

    private async Task InsertAlbumArt(object? parameter)
    {
        var fileFilters = new List<FileDialogFilter>();
        fileFilters.Add(new FileDialogFilter()
        {
            Name = "Supported Images",
            Extensions = new List<string>() { "jpg", "png" }
        });
        var result = await _serviceCollection.GetService<IIOService>()?.ShowOpenFileDialogAsync("Open Album Art Image", fileFilters)!;
        if (result != null)
        {
            TagAlbumArt = new Bitmap(result[0]);
            await SaveTags(null);
        }
    }

    private async Task FilenameToTag(object? parameter)
    {
        int success = 0;
        await _serviceCollection.GetService<IProgressDialogService>()?.ShowAsync("Converting filenames to tags...", async () =>
        {
            await Task.Run(() =>
            {
                foreach (var musicFile in SelectedMusicFiles)
                {
                    try
                    {
                        musicFile.FilenameToTag(FttFormatString);
                        success++;
                    }
                    catch { }
                }
            });
        })!;
        _serviceCollection.GetService<IInfoBarService>()?.ShowCloseableNotification("Conversion Complete", $"Converted {success} out of {SelectedMusicFiles.Count} filenames to tags successfully.", InfoBarSeverity.Success);
        await RefreshMusicFolder(null);
    }

    private async Task TagToFilename(object? parameter)
    {
        int success = 0;
        await _serviceCollection.GetService<IProgressDialogService>()?.ShowAsync("Converting tags to filenames...", async () =>
        {
            await Task.Run(() =>
            {
                foreach (var musicFile in SelectedMusicFiles)
                {
                    try
                    {
                        musicFile.TagToFilename(TtfFormatString);
                        success++;
                    }
                    catch { }
                }
            });
        })!;
        _serviceCollection.GetService<IInfoBarService>()?.ShowCloseableNotification("Conversion Complete", $"Converted {success} out of {SelectedMusicFiles.Count} tags to filenames successfully.", InfoBarSeverity.Success);
        await RefreshMusicFolder(null);
    }

    private void DataMusicFiles_SelectionChanged(IList? selectedItems)
    {
        SelectedMusicFiles.Clear();
        if (selectedItems != null)
        {
            foreach (var item in selectedItems)
            {
                SelectedMusicFiles.Add((MusicFile)item);
            }
        }
        SaveTagsCommand.RaiseCanExecuteChanged();
        RemoveTagsCommand.RaiseCanExecuteChanged();
        InsertAlbumArtCommand.RaiseCanExecuteChanged();
        FilenameToTagCommand.RaiseCanExecuteChanged();
        TagToFilenameCommand.RaiseCanExecuteChanged();
        TagFilenameEnabled = true;
        TagPath = "<keep>";
        if (SelectedMusicFiles.Count == 0)
        {
            TagFilename = "";
            TagTitle = "";
            TagArtist = "";
            TagAlbum = "";
            TagYear = "";
            TagTrack = "";
            TagAlbumArtist = "";
            TagComposer = "";
            TagGenre = "";
            TagComment = "";
            TagDuration = "";
            TagFileSize = "";
            TagAlbumArt = null;
        }
        else if (SelectedMusicFiles.Count == 1)
        {
            var musicFile = SelectedMusicFiles[0];
            TagFilename = musicFile.Filename;
            TagTitle = musicFile.Title;
            TagArtist = musicFile.Artist;
            TagAlbum = musicFile.Album;
            TagYear = musicFile.Year == null ? "" : musicFile.Year.ToString();
            TagTrack = musicFile.Track == null ? "" : musicFile.Track.ToString();
            TagAlbumArtist = musicFile.AlbumArtist;
            TagAlbumArtist = musicFile.Composer;
            TagGenre = musicFile.Genre;
            TagComment = musicFile.Comment;
            TagDuration = musicFile.DurationAsString;
            TagFileSize = musicFile.FileSizeAsString;
            TagAlbumArt = musicFile.AlbumArt;
            TagPath = musicFile.Path;
        }
        else
        {
            var firstMusicFile = SelectedMusicFiles[0];
            TagFilenameEnabled = false;
            TagFilename = "<keep>";
            var haveSameTitle = true;
            var haveSameArtist = true;
            var haveSameAlbum = true;
            var haveSameYear = true;
            var haveSameTrack = true;
            var haveSameAlbumArtist = true;
            var haveSameComposer = true;
            var haveSameGenre = true;
            var haveSameComment = true;
            var haveSameAlbumArt = true;
            var totalDuration = 0;
            long totalFileSize = 0;
            foreach (var musicFile in SelectedMusicFiles)
            {
                if (firstMusicFile.Title != musicFile.Title)
                {
                    haveSameTitle = false;
                }
                if (firstMusicFile.Artist != musicFile.Artist)
                {
                    haveSameArtist = false;
                }
                if (firstMusicFile.Album != musicFile.Album)
                {
                    haveSameAlbum = false;
                }
                if (firstMusicFile.Year != musicFile.Year)
                {
                    haveSameYear = false;
                }
                if (firstMusicFile.Track != musicFile.Track)
                {
                    haveSameTrack = false;
                }
                if (firstMusicFile.AlbumArtist != musicFile.AlbumArtist)
                {
                    haveSameAlbumArtist = false;
                }
                if (firstMusicFile.Composer != musicFile.Composer)
                {
                    haveSameComposer = false;
                }
                if (firstMusicFile.Genre != musicFile.Genre)
                {
                    haveSameGenre = false;
                }
                if (firstMusicFile.Comment != musicFile.Comment)
                {
                    haveSameComment = false;
                }
                if (firstMusicFile.AlbumArt != musicFile.AlbumArt)
                {
                    haveSameAlbumArt = false;
                }
                totalDuration += musicFile.Duration;
                totalFileSize += musicFile.FileSize;
            }
            TagTitle = haveSameTitle ? firstMusicFile.Title : "<keep>";
            TagArtist = haveSameArtist ? firstMusicFile.Artist : "<keep>";
            TagAlbum = haveSameAlbum ? firstMusicFile.Album : "<keep>";
            TagYear = haveSameYear ? (firstMusicFile.Year == null ? "" : firstMusicFile.Year.ToString()) : "<keep>";
            TagTrack = haveSameTrack ? (firstMusicFile.Track == null ? "" : firstMusicFile.Track.ToString()) : "<keep>";
            TagAlbumArtist = haveSameAlbumArtist ? firstMusicFile.AlbumArtist : "<keep>";
            TagComposer = haveSameComposer ? firstMusicFile.Composer : "<keep>";
            TagGenre = haveSameGenre ? firstMusicFile.Genre : "<keep>";
            TagComment = haveSameComment ? firstMusicFile.Comment : "<keep>";
            TagDuration = totalDuration.DurationToString();
            TagFileSize = totalFileSize.FileSizeToString();
            TagAlbumArt = haveSameAlbumArt ? firstMusicFile.AlbumArt : null;
        }
    }

    private async Task SettingsUpdate(object? parameter)
    {
        var config = await Configuration.LoadAsync();
        if(MusicFolder.IncludeSubfolders != config.IncludeSubfolders)
        {
            MusicFolder.IncludeSubfolders = config.IncludeSubfolders;
            await RefreshMusicFolder(null);
        }
        if (!config.RememberLastOpenedFolder)
        {
            config.LastOpenedFolder = "No Folder Opened";
            await config.SaveAsync();
        }
    }
}
