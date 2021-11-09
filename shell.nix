{ pkgs ? import <nixpkgs> {} }:

with pkgs;
mkShell {
  inputsFrom = [
    (import ./default.nix { inherit pkgs; })
  ];
  buildInputs = [
     ccls
     ccache
     python38Packages.python-language-server
  ];
}
