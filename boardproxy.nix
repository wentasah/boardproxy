{ stdenv, lib, meson, ninja, pkg-config, libev, fmt, spdlog, boost, nix-gitignore, systemd }:
stdenv.mkDerivation {
  name = "boardproxy";
  src = nix-gitignore.gitignoreSource [] ./.;
  nativeBuildInputs = [ meson ninja pkg-config ];
  buildInputs = [ libev fmt spdlog boost systemd ];

  # Meson is no longer able to pick up Boost automatically.
  # https://github.com/NixOS/nixpkgs/issues/86131
  BOOST_INCLUDEDIR = "${lib.getDev boost}/include";
  BOOST_LIBRARYDIR = "${lib.getLib boost}/lib";

  # Export the same also to the shell started via shell.nix
  shellHook = ''
    export BOOST_INCLUDEDIR="${lib.getDev boost}/include"
    export BOOST_LIBRARYDIR="${lib.getLib boost}/lib"
  '';
}
