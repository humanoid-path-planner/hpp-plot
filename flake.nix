{
  description = "Graphical utilities for constraint graphs in hpp-manipulation";

  inputs.gepetto.url = "github:gepetto/nix";

  outputs =
    inputs:
    inputs.gepetto.lib.mkFlakoboros inputs (
      { lib, ... }:
      {
        overrideAttrs.hpp-plot =
          {
            drv-final,
            pkgs-final,
            ...
          }:
          {
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
            npmDeps = pkgs-final.fetchNpmDeps {
              name = "${drv-final.pname}-${drv-final.version}-npm-deps";
              src = drv-final.src + "/src/web_app/";
              hash = "sha256-GAYdugZFMygk0MXyXxf2wSsWRvn/aW4YeFH2v62IZjI=";
            };
          };
      }
    );
}
