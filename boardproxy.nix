{ stdenv, meson, ninja, pkg-config, libev, fmt, spdlog, boost, nix-gitignore }:
stdenv.mkDerivation {
  name = "boardproxy";
  src = nix-gitignore.gitignoreSource [] ./.;
  nativeBuildInputs = [ meson ninja pkg-config ];
  buildInputs = [ libev fmt spdlog boost ];

  # Meson is no longer able to pick up Boost automatically.
  # https://github.com/NixOS/nixpkgs/issues/86131
  BOOST_INCLUDEDIR = "${stdenv.lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${stdenv.lib.getLib boost}/lib";

  # Export the same also to the shell started via shell.nix
  shellHook = ''
    export BOOST_INCLUDEDIR="${stdenv.lib.getDev boost}/include"
    export BOOST_LIBRARYDIR="${stdenv.lib.getLib boost}/lib"
  '';
}
