{
  description = "PlatformIO project for ADVhome on M5Stack Cardputer";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        
        myPython = pkgs.python3.withPackages (ps: with ps; [ pyserial ]);
        
        deployScript = pkgs.writeShellScriptBin "deploy" ''
          set -e
          echo "Building Firmware..."
          pio run
          echo "Uploading directly to M5Launcher OTA_3 slot (0x5e0000)..."
          
          # Find esptool.py from platformio packages
          ESPTOOL=$(find ~/.platformio/packages -name "esptool.py" | head -n 1)
          if [ -z "$ESPTOOL" ]; then
            echo "Error: esptool.py not found in ~/.platformio/packages"
            exit 1
          fi
          
          echo "Waiting for device on /dev/ttyACM0 to become available (plug it in or reset)..."
          while [ ! -e /dev/ttyACM0 ]; do
            sleep 0.5
          done
          echo "Device found!"
          
          # Run upload using the nix python3
          ${myPython}/bin/python3 "$ESPTOOL" --port /dev/ttyACM0 write_flash 0x5e0000 .pio/build/m5stack-stamps3/firmware.bin
          echo "Success! Please reset your device."
        '';
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            platformio
            myPython
            deployScript
          ];
          
          shellHook = ''
            echo "ADVhome PlatformIO Environment"
            echo "Run 'pio run' to build the project."
            echo "Run 'deploy' to build and flash directly to M5Launcher."
          '';
        };
      }
    );
}
