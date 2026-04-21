{
  description = "Graphical utilities for constraint graphs in hpp-manipulation";

  inputs.gepetto.url = "github:gepetto/nix";

  outputs =
    inputs:
    inputs.gepetto.lib.mkFlakoboros inputs (
      { lib, ... }:
      {
        overrideAttrs.hpp-plot = {
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
    );
}
