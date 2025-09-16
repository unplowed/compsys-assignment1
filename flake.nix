{
  description = "simple gcc flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs =
    {
      nixpkgs,
      ...
    }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [
        "aarch64-linux"
        "x86_64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              gcc
              valgrind
            ];
          };
        }
      );

      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.stdenv.mkDerivation rec {
            pname = "file";
            version = "v0.0.1";
            src = ./file.c;
            phases = [
              "buildPhase"
            ];

            buildPhase = ''
              mkdir -p $out/bin
              gcc $src -o $out/bin/${pname}
            '';

            meta.mainProgram = "file";
          };
        }
      );

      apps = forAllSystems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          test = {
            type = "app";
            program = toString (
              pkgs.writeShellScript "compile-and-run.sh" (
                let
                  files = [
                    "empty"
                    "ascii"
                    "latin1"
                    "eggplant"
                    "file"
                    "priviledged"
                  ];
                in
                ''
                  gcc file.c -o file
                  touch priviledged
                  chmod -r priviledged
                  ${builtins.concatStringsSep "\n" (
                    map (x: ''
                      echo "Testing file ${x}"
                      ./file ${x}
                      echo " "
                    '') files
                  )}
                  rm priviledged
                ''
              )
            );
          };
        }
      );

      formatter = forAllSystems (system: nixpkgs.legacyPackages.${system}.nixfmt-rfc-style);
    };
}
