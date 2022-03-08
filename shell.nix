{ pkgs ? import <nixpkgs> {} }:

with pkgs;
mkShell {
  inputsFrom = [
    (import ./default.nix { inherit pkgs; })
  ];
  buildInputs = [
     ccls
     python3Packages.python-lsp-server
  ];
}
