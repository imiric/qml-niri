{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    quickshell = {
      url = "git+https://git.outfoxxed.me/outfoxxed/quickshell";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix.url = "github:numtide/treefmt-nix";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-parts,
      ...
    }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } (
      { lib, ... }:
      {
        imports = [ inputs.treefmt-nix.flakeModule ];
        systems = lib.intersectLists lib.systems.flakeExposed lib.platforms.linux;

        perSystem =
          {
            self',
            inputs',
            pkgs,
            ...
          }:
          {
            treefmt = {
              projectRootFile = "flake.nix";
              programs.nixfmt.enable = true;
            };

            packages = {
              default = self'.packages.qml-niri;
              qml-niri = pkgs.callPackage ./default.nix {
                version = self.sourceInfo.dirtyShortRev or self.sourceInfo.shortRev;
              };

              quickshell = self'.packages.quickshell-niri;
              quickshell-niri = inputs'.quickshell.packages.default.overrideAttrs (prevAttrs: {
                buildInputs = [ self'.packages.qml-niri ] ++ prevAttrs.buildInputs;
              });
            };
          };
      }
    );
}
