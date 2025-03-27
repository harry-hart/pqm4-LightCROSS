{ pkgs ? import <nixpkgs> { } }:
  pkgs.mkShell {
    # nativeBuildInputs is usually what you want -- tools you need to run
    nativeBuildInputs = with pkgs.buildPackages; [ 
      python313
      gcc-arm-embedded
      tio
    ];

    packages = [
    (pkgs.python313.withPackages (python-pkgs: [
      python-pkgs.tqdm
      python-pkgs.pyserial
    ]))
  ];
}
