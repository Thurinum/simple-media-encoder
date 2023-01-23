# Efficient Video Encoder
Re-encode and compress your media files quickly and efficiently with this convenient tool. Uses FFMPEG and the Qt toolkit under GPL v3.

<div>
  <img height=400 src=https://user-images.githubusercontent.com/43908636/214160823-a1646833-ca03-4a9b-86e6-f64c2e8dac92.png />
  <img height=400 src=https://user-images.githubusercontent.com/43908636/214160857-1eafac6e-8e98-4775-9305-baf9ed480c13.png />
</div>  

The default interface allows anyone to quickly re-encode their media files.  
So-called *expert mode* gives advanced users a bit more control over the exported result.
  
## Features
With this tool, you may:

- Re-encode **any file type** supported by FFMPEG;
- Choose whether to export **video, audio, or both**;
- Quickly select a **quality preset**, or manually choose **your codecs**;
- Choose *audio bitrate*;
- **Re-scale video** with automatic aspect ratio adjustment;
- Manually adjust **aspect ratio**;
- Specify a **target file size** for easy compression of large files;
- Change the **video and audio speed** or manually set a video framerate;
- Specify **custom FFMPEG arguments** for advanced use.

Furthermore, you may:

- Choose to auto-fill all fields when a file is selected;
- Delete the input file on success;
- Decide whether to warn when a file is about to be overwritten;
- Specify the output name as a file name or as a suffix to the input name;
- View detailed statistics about your media file *(to be implemented)*.

As well as customize behavior on successful re-encoding:

- Open the resulting media in the file explorer;
- View the resulting media in the default media player;
- Auto-close the utility for a one-shot export.

Conveniently, all these parameters are saved when the tool is closed.

## How to install
### Binaries
Binary releases should be available before March 2023.

### Source
This tool uses the Qt Framework for application development.

## On Windows
If Qt is not already installed on your system:
- Download the Qt Open Source installer <a href=https://www.qt.io/download-qt-installer>here</a>
- Proceed 

## Technologies used
- ffmpeg and ffprobe
- The Qt framework (Qt Widgets)

## Special thanks to
Theodore l'Heureux (for his consulting and beta-testing)

## About stability
This is a beta, and the tool will face difficulties opening *certain files*.  
Should this happen to you, feel free to create an issue on this repository so I can investigate the problem quicker.  
I'm aware of these issues and will be working on fixing them within a few weeks.  

Thanks for reading through this and cheers!
