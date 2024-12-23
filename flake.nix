{
    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
        flake-utils.url = "github:numtide/flake-utils";
        flake-utils.inputs.nixpkgs.follows = "nixpkgs";
    };

    outputs = { nixpkgs, flake-utils, ... } @ inputs: flake-utils.lib.eachDefaultSystem (system:
        let
            pkgs = import nixpkgs { inherit system; };
            inherit (pkgs) lib stdenv mkShell;
        in {
            devShells.default = mkShell {
                shellHook = "export DEVSHELL_PS1_PREFIX='Nyax'";
                nativeBuildInputs = with pkgs; [
                    libisoburn # xorisso
                    nasm
                    mtools
                    curl
                    gnumake
                    gdb
                    clang_19
                    clang-tools_19
                    qemu_full
                ];
            };
        }
    );
}