{ stdenv
, qtbase
, full
, cmake
, wrapQtAppsHook
, ffmpeg
, lib
, writeText
, clangStdenv
}:
let
  version = "0.11.0";
  simpleMediaEncoder-dist = stdenv.mkDerivation {
    pname = "SimpleMediaEncoder-dist";
    inherit version;

    src = ./.;
    stdenv = clangStdenv;

    buildInputs = [
      qtbase
      full
    ];

    nativeBuildInputs = [
      cmake
      wrapQtAppsHook
    ];

    # Here, we're in the build directory so ../ is src
    installPhase = ''
      mkdir -p $out/share/applications
      cp -a ../bin $out/bin

      wrapProgram $out/bin/SimpleMediaEncoder \
        --prefix PATH : ${lib.makeBinPath [ ffmpeg ]}
    '';
  };
  desktopFile = writeText "SimpleMediaEncoder.desktop" ''
    [Desktop Entry]
    Version=${version}
    Encoding=UTF-8
    Name=SimpleMediaEncoder
    GenericName=Simple media encoder
    Type=Application
    Categories=Multimedia;AudioVideo;Video;
    MimeType=video/x-msvideo;video/x-matroska;video/webm;video/mpeg;video/mp4;
    Terminal=false
    StartupNotify=true
    Exec=${simpleMediaEncoder-dist}/bin/SimpleMediaEncoder %f
    Icon=svp-manager4.png
  '';
in
stdenv.mkDerivation {
  name = "SimpleMediaEncoder";
  inherit (simpleMediaEncoder-dist) version;

  phases = ["installPhase"];
  installPhase = ''
    mkdir -p $out/share/applications
    ln -s ${simpleMediaEncoder-dist}/bin $out/bin
    ln -s ${desktopFile} $out/share/applications/SimpleMediaEncoder.desktop
  '';

  meta = with lib; {
    description = "Compress and convert your media files in a breeze with this convenient tool! Perfect for sending to Discord or Messenger, or reducing the size of music to listen on the go. Uses FFmpeg and Qt Widgets under GPL v3.";
    homepage = "https://github.com/willbou1/simple-media-encoder";
    platforms = ["x86_64-linux"];
    license = licenses.gpl3;
  };
}
