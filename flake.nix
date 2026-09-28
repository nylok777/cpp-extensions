{
  description = "C++ standard library extensions.";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" "x86_64-darwin" ];
      perSystem = { config, self', inputs', pkgs, system, ... }: {
        packages.cpp-extensions = pkgs.callPackage ./package.nix { cxxStandard = "23"; };

		devShells.default = pkgs.mkShell {
		  packages = with pkgs; [ cmake gtest ];
		};
      };
    };
}
