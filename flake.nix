{
  description = "Graphical utilities for constraint graphs in hpp-manipulation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/refs/pull/362956/head";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];
      perSystem =
        { pkgs, self', ... }:
        {
          devShells.default = pkgs.mkShell { inputsFrom = [ self'.packages.default ]; };
          packages = {
            default = self'.packages.hpp-plot;
            hpp-plot = pkgs.python3Packages.hpp-plot.overrideAttrs (_: {
              src = pkgs.lib.fileset.toSource {
                root = ./.;
                fileset = pkgs.lib.fileset.unions [
                  ./bin
                  ./cmake_modules
                  ./CMakeLists.txt
                  ./doc
                  ./include
                  ./package.xml
                  ./plugins
                  ./src
                ];
              };
            });
          };
        };
    };
}
