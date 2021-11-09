{ stdenv, lib, meson, ninja, pkg-config, libev, fmt, spdlog, boost, nix-gitignore, systemd, python3Packages, netcat }:
stdenv.mkDerivation {
  name = "boardproxy";
  src = if builtins.pathExists ./.git then
    builtins.fetchGit { url = ./.; }
  else
    nix-gitignore.gitignoreSource [] ./.;

  mesonFlags = [
    "-Dsystemdsystemunitdir=${placeholder "out"}/lib/systemd/system"
  ];

  nativeBuildInputs = [ meson ninja pkg-config ];
  buildInputs = [ libev fmt spdlog boost systemd ];

  doCheck = true;
  checkInputs = [ python3Packages.pytest netcat ];

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
