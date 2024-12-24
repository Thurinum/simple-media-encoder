{
  description = "Dev environment for simple-media-encoder";

  inputs.devshell.url = "github:numtide/devshell";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, flake-utils, devshell, nixpkgs }:
    flake-utils.lib.eachDefaultSystem (system: {
      devShell =
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [ devshell.overlays.default ];
          };
        in
        pkgs.devshell.mkShell {
            packages = with pkgs; [
              lldb

              # LSPs for Emacs or Neovim
              cmake-language-server

              # build dependencies
              cmake
              gnumake
              clang
              qt6.full

              # runtime dependencies
              ffmpeg
            ];
        };
    });
}
