{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    platformio
  ];
  
  shellHook = ''
    echo "ADVhome PlatformIO Environment"
    echo "Run 'pio run' to build the project."
  '';
}
