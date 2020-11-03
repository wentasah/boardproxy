{ stdenv, meson, ninja, pkg-config, libev, fmt, spdlog, nix-gitignore }:
stdenv.mkDerivation {
  name = "boardproxy";
  src = nix-gitignore.gitignoreSource [] ./.;
  nativeBuildInputs = [ meson ninja pkg-config ];
  buildInputs = [ libev fmt spdlog ];
}
