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

        # The Cardputer is a shared M5Launcher install, so the app slot ADVhome
        # belongs in is one of several. flash_slot.py reads the device's own
        # partition table and resolves the slot by label rather than trusting a
        # hardcoded offset, which is wrong the moment the layout changes and
        # silently eats a neighbouring firmware when it is.
        waitForPort = ''
          PORT="''${ADVHOME_PORT:-/dev/ttyACM0}"
          ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
          cd "$ROOT"

          echo "Waiting for device on $PORT (plug it in or reset)..."
          while [ ! -e "$PORT" ]; do
            sleep 0.5
          done
          echo "Device found."
        '';

        deployScript = pkgs.writeShellScriptBin "deploy" ''
          set -e
          ROOT=$(git rev-parse --show-toplevel 2>/dev/null || pwd)
          cd "$ROOT"

          echo "Building firmware..."
          pio run

          ${waitForPort}

          exec ${myPython}/bin/python3 "$ROOT/tools/flash_slot.py" \
            --port "$PORT" "$@"
        '';

        ptableScript = pkgs.writeShellScriptBin "ptable" ''
          set -e
          ${waitForPort}

          exec ${myPython}/bin/python3 "$ROOT/tools/flash_slot.py" \
            --port "$PORT" --show "$@"
        '';
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            platformio
            myPython
            git
            deployScript
            ptableScript
          ];

          shellHook = ''
            echo "ADVhome PlatformIO Environment"
            echo "  pio run   - build the firmware"
            echo "  ptable    - show the connected device's partition table"
            echo "  deploy    - build, then flash into the 'advhom' app slot"
            echo ""
            echo "  deploy --dry-run   resolve the slot and check the fit only"
            echo "  deploy --slot NAME target a different app slot"
            echo "  ADVHOME_PORT=...   use a port other than /dev/ttyACM0"
          '';
        };
      }
    );
}
