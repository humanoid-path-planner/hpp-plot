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
            drv-prev,
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
            cmakeFlags = [
              (lib.cmakeBool "USE_JS" false) # build from nix not cmake
            ];
            postPatch = ''
              # prepare npm offline cache
              mkdir -p node_modules
              cd src/web_app
              cp package.json package-lock.json ../..
              ln -s ../../node_modules
              cd -
            '';
            nativeBuildInputs = drv-prev.nativeBuildInputs ++ [
              pkgs-final.npmHooks.npmConfigHook
              pkgs-final.nodejs
            ];
            npmDeps = pkgs-final.fetchNpmDeps {
              name = "${drv-final.pname}-${drv-final.version}-npm-deps";
              src = drv-final.src + "/src/web_app/";
              hash = "sha256-B8s+hhTn7CG3q8bx490SM8fKFAEGOmHX7u8JN/7qI94=";
            };
            preBuild = ''
              cd ../src/web_app
              npm --offline run build
              cd -
            '';
            postInstall = ''
              cp -r ../src/web_app/dist $out/share/hpp-plot/webapp
            '';
          };
      }
    );
}
