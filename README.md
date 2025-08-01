# Simple Media Encoder

Compress and convert your media files in a breeze with this convenient tool! Perfect for sending to Discord or
Messenger, or reducing the size of music to listen on the go. Uses FFmpeg and Qt Widgets under GPL v3.

<div>
  <img width=300 src=https://github.com/user-attachments/assets/f5400a8e-f3cd-4516-baa7-a4a289e37528 />
  <img width=520 src=https://github.com/user-attachments/assets/a05aefc7-1132-4c93-a08d-a3c338066ca6 />  
</div>  
<br/>

The default interface allows anyone to quickly re-encode media files.  
The so-called "expert" interface gives advanced users a bit more control over the exported result.

## Compatibility

- Tested on Windows 11, Windows 10, and Ubuntu. Earlier versions of Windows are not supported.
- Should work on Mac as well. Let me know if you encounter issues! More testing is underway.

## Features

With this tool, one may:

- Re-encode **any file type** supported by FFmpeg;
- Choose whether to export **video, audio, or both**;
- Quickly select a **quality preset**, or manually tune **your settings**;
- Choose **audio bitrate**;
- **Re-scale video** with automatic aspect ratio adjustment, or manually adjust the **aspect ratio**;
- Change the **video and audio speed** or manually set a video framerate;
- Specify **custom FFmpeg arguments** for advanced use.

Furthermore, one may:

- Choose to **auto-fill** all fields when a file is selected;
- **Delete** the input file on success;
- Decide whether to **warn** when a file is about to be overwritten;
- Specify the **output name** as a file name or as a suffix to the input name;
- View **detailed statistics** about your media file *(to be implemented)*.

As well as customize behavior on successful re-encoding:

- Open the resulting media in the **file explorer**;
- View the resulting media in the **default media player**;
- **Auto-close the utility** for a one-shot export.

Conveniently, all these parameters are **saved** when the tool is closed.

## How to install

### Binaries

A [binary release](https://github.com/Thurinum/simple-media-encoder/releases) is available for Windows!

### Source

This tool uses the Qt for Application Development framework, under the GNU GPL v3 open source license.  
It must be installed for compiling SME.

#### On Windows

1. If Qt is not already installed on your system:
    - Download the Qt Open Source installer [here](https://www.qt.io/download-qt-installer) for easy setup of the
      framework;
    - Proceed with the installation of Qt (you will need a free Qt developer account):
        - Ideally, select version >= 6.7.
        - Under "Developer and Designer tools", select the latest MinGW compiler.
        - Compilation has been tested with the CLion and Qt Creator IDEs.
          <br><br>
2. If FFmpeg is not already installed and in your PATH:
    - You must manually download and install the ffmpeg.exe and ffprobe.exe binaries.
        - You may find links to mirrors [here](https://ffmpeg.org/download.html#build-windows).
        - Extract the archive and place the two binaries (and associated libraries if present) inside the `bin`
          directory (or add them to your PATH).
3. Build the project:
    - Run the cmake project in your IDE (tested with CLion and Qt Creator).

#### On Linux (with apt)

```bash
sudo apt update -y
sudo apt upgrade -y
sudo apt install git build-essential cmake qt6-base-dev qt6-base-dev-tools -y

git clone https://github.com/Thurinum/simple-media-encoder.git
cd simple-media-encoder

cmake .
make
bin/SimpleMediaEncoder
```

## Technologies used

- ffmpeg and ffprobe
- The Qt framework (Qt Widgets)

## Special thanks to

Theodore l'Heureux (for his consulting and beta-testing)
William Boulanger (for his advice and knowledge on audio theory)

## Found an issue?

This is a beta, and the tool may face difficulties opening certain files.  
Should you encounter such a problematic file, feel free to file a Github issue; it will help finding the problem.
Any bug reports are also greatly appreciated!

Thanks for reading through this and cheers!
