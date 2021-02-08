{
  pkgs ? import <nixpkgs> {}
}:
let
  myBoost = pkgs.boost.overrideAttrs (oldAttrs: {
    postFixup = "echo NO POST_FIXUP"; # Do not remove header prefix - it makes debugging harder
  });
in
pkgs.callPackage ./boardproxy.nix { }
