{
  description = "Graphical utilities for constraint graphs in hpp-manipulation";

  inputs = {
    gepetto.url = "github:gepetto/nix";
    gazebros2nix.follows = "gepetto/gazebros2nix";
    flake-parts.follows = "gepetto/flake-parts";
    nixpkgs.follows = "gepetto/nixpkgs";
    nix-ros-overlay.follows = "gepetto/nix-ros-overlay";
    systems.follows = "gepetto/systems";
    treefmt-nix.follows = "gepetto/treefmt-nix";
  };

  outputs =
    inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } (
      { lib, ... }:
      {
        systems = import inputs.systems;
        imports = [
          inputs.gepetto.flakeModule
          {
            gazebros2nix.overrides.hpp-plot = _final: {
              src = lib.fileset.toSource {
                root = ./.;
                fileset = lib.fileset.unions [
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
            };
          }
        ];
      }
    );
}
