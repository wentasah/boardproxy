let
  nixpkgs-versions = [
    "release-21.05"
    "nixos-unstable"
  ];
  build = branch:
    let pkgs = import (builtins.fetchTarball {
          # Descriptive name to make the store path easier to identify
          name = "nixpkgs-${branch}";
          url = "https://github.com/NixOS/nixpkgs/archive/refs/heads/${branch}.tar.gz";
        }) {};
    in
      pkgs.callPackage ./boardproxy.nix { version = "nixpkgs-${branch}"; };
in
map build nixpkgs-versions
