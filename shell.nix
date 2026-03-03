# Use this file with nix-shell or similar tools; see https://nixos.org/
with import <nixpkgs> {};
mkShell { buildInputs = import ./lib/github.com/abxh/lys/build-inputs.nix pkgs; }
