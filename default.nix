{ nixpkgs ? import <nixpkgs> {} }:
let
  inherit (nixpkgs) pkgs;
in pkgs.stdenv.mkDerivation {
  name = "picture-cc";
  buildInputs = with pkgs; [ gtk2 opencv gcc pkgconfig ];
}

