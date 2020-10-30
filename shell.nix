{ pkgs ? import <nixpkgs> {} }:

with pkgs;
mkShell {
  inputsFrom = [
    (import ./default.nix { inherit pkgs; })
  ];
  buildInputs = [
     ccls
     ccache
  ];

  # Meson is no longer able to pick up Boost automatically.
  # https://github.com/NixOS/nixpkgs/issues/86131
  BOOST_INCLUDEDIR = "${stdenv.lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${stdenv.lib.getLib boost}/lib";
}
