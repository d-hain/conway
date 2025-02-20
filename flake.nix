{
  description = "Conway's Game of Life";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
  };

  outputs = {
    self,
    nixpkgs,
    ...
  }: let
    game-name = "conway";
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    packages.${system} = {
      default = self.packages.${system}.${game-name};

      ${game-name} = pkgs.stdenv.mkDerivation {
        pname = game-name;
        version = "0.1";
        src = ./.;

        # Static linking
        raylib = pkgs.raylib.override {sharedLib = false;};

        nativeBuildInputs = with pkgs; [
          gcc
          raylib
          glfw
        ];

        buildPhase = ''
          cp -r $raylib/include .
          cp -r $raylib/lib .

          make release/${game-name}
        '';

        installPhase = ''
          mkdir -p $out/bin

          cp release/* $out/bin
        '';
      };

      "${game-name}-debug" = self.packages.${system}.${game-name}.overrideAttrs (old: {
        pname = "${game-name}-debug";
        dontStrip = true;
        enableDebugging = true;

        buildPhase = ''
          cp -r $raylib/include .
          cp -r $raylib/lib .

          make debug/${game-name}
        '';

        installPhase = ''
          mkdir -p $out/bin

          cp debug/* $out/bin
        '';
      });
    };

    # For completions in Neovim
    devShells.${system}.default = pkgs.mkShell {
      packages = with pkgs; [
        raylib
      ];
    };

    formatter.${system} = pkgs.alejandra;
  };
}
