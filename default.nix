{
  pkgs ? import <nixpkgs> {}
}:
with pkgs;
let
  spdlog = stdenv.mkDerivation rec {
      pname = "spdlog";
      version = "1.8.1";

      src = fetchFromGitHub {
        owner  = "gabime";
        repo   = "spdlog";
        rev    = "v${version}";
        sha256 = "1n8vpa66mc3mp1fmnpk99cppp3krc4l2k563psz91d8x0xi629hk";
      };

      nativeBuildInputs = [ cmake ];

      cmakeFlags = [ "-DSPDLOG_BUILD_EXAMPLE=OFF" "-DSPDLOG_BUILD_BENCH=OFF" ];

      outputs = [ "out" "doc" ];

      postInstall = ''
        mkdir -p $out/share/doc/spdlog
        cp -rv ../example $out/share/doc/spdlog
      '';
    };
in
stdenv.mkDerivation {
  name = "boardproxy";
  src = ./.;
  nativeBuildInputs = [ meson ninja pkg-config ];
  buildInputs = [ asio fmt spdlog ];
}