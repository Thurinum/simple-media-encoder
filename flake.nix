{
  description = "Dev environment for simple-media-encoder";

  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, flake-utils, nixpkgs }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
    in {
      packages = rec {
        SimpleMediaEncoder = pkgs.qt6Packages.callPackage ./build.nix {};
        default = SimpleMediaEncoder;
      };
      devShell =
        pkgs.mkShell {
          inputsFrom = [
            self.packages.${system}.default
          ];
          buildInputs = with pkgs; [
            lldb
            clang
            cmake

            # LSPs for Emacs or Neovim
            python3Minimal
            cmake-language-server
            clang-tools

            qt6.wrapQtAppsHook
            qt6.qtbase
            makeWrapper

            # runtime deps
            ffmpeg
          ];
          shellHook = ''
            fishdir=$(mktemp -d)
            makeWrapper "$(type -p fish)" "$fishdir/fish" "''${qtWrapperArgs[@]}"

            export CXX="clang++"
            export CC="clang"

            exec "$fishdir/fish"
        '';
        };
    });
}
