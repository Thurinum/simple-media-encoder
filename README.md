# Simple Media Encoder
Re-encode and compress your media files quickly and efficiently with this convenient tool. Uses FFMPEG and the Qt toolkit under GPL v3.

<div>
  <img height=400 src=https://user-images.githubusercontent.com/43908636/214199765-8b79ab79-b069-48e8-b41d-de67893a2b1f.png />
  <img height=400 src=https://user-images.githubusercontent.com/43908636/214199849-85893425-2850-44c1-92f8-ff6c2edcb525.png />  
</div>  
  
The default interface allows anyone to quickly re-encode media files.  
The so-called "expert" interface gives advanced users a bit more control over the exported result.
  
## Features
With this tool, one may:

- Re-encode **any file type** supported by FFMPEG;
- Choose whether to export **video, audio, or both**;
- Quickly select a **quality preset**, or manually choose **your codecs**;
- Choose *audio bitrate*;
- **Re-scale video** with automatic aspect ratio adjustment;
- Manually adjust **aspect ratio**;
- Specify a **target file size** for easy compression of large files;
- Change the **video and audio speed** or manually set a video framerate;
- Specify **custom FFMPEG arguments** for advanced use.

Furthermore, one may:

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
Binary releases should be available before February 2023.

### Source
This tool uses the Qt for Application Development framework, under the GNU GPL v3 open source license.  
It must be installed for compiling SME.

### On Windows
If Qt is not already installed on your system:
- Download the Qt Open Source installer <a href=https://www.qt.io/download-qt-installer>here</a> for easy setup of the framework;
- Proceed with the installation of Qt (you will need a free Qt developer account):
  - Ideally, select version >= 6.3.
  - Under "Developer and Designer tools", choose the Qt Creator IDE (unless you intend to use CLI) and the latest MinGW compiler.
Once Qt is installed:
- Open Qt Creator and clone this repository in `File -> New Project... -> Import project`.
- Press `Ctrl+R` to run the project.
Before you get going:
- On Windows, you must manually download and install the ffmpeg.exe and ffprobe.exe binaries.
  - You may find links to mirrors <a href=https://ffmpeg.org/download.html#build-windows>here</a>.
  - Extract the archive and place the two binaries (and associated libraries if present) inside the `bin` directory (or add them to your PATH).
  
### On Linux
If the Qt development libraries are not already installed on your system:
- Install Qt libraries version >= 6.3 and qmake
  - From your package manager (recommended, although some sources may provided outdated Qt packages).
  - With the Open Source installer (see instructions for Windows above).
If ffmpeg is not already installed on your system (and in your PATH):
- Install ffmpeg with your package manager.
Once everything is installed:
- Clone this repository.
  - Run `qmake` on the .pro file to generate makefiles
  - Run `make -j32` to compile.
  - The executable is output in the bin/ directory.
  
## Technologies used
- ffmpeg and ffprobe
- The Qt framework (Qt Widgets)

## Special thanks to
Theodore l'Heureux (for his consulting and beta-testing)

## Found an issue?
This is a beta, and the tool may face difficulties opening certain files.  
Should you encounter such a problematic file, feel free to send it over alongside a Github issue; it will help finding the problem.
Any bug reports are also greatly appreciated!

Thanks for reading through this and cheers!
