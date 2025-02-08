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
    name = "conway";
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    packages.${system}.default = pkgs.stdenv.mkDerivation {
      pname = name;
      version = "0.1.0";
      src = ./.;

      # Static linking
      raylib = pkgs.raylib.override { sharedLib = false; };

      nativeBuildInputs = with pkgs; [
        gcc
        raylib
        glfw
      ];

      buildPhase = ''
        cp -r $raylib/include .
        cp -r $raylib/lib .

        make target/$pname
      '';

      installPhase = ''
        mkdir -p $out/bin
        cp target/* $out/bin
      '';
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
