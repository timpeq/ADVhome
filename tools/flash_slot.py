#!/usr/bin/env python3
"""Flash the built firmware into the app partition it actually belongs in.

This Cardputer is a shared M5Launcher install: several unrelated firmwares
each live in their own app slot and the launcher picks between them at boot.
A hardcoded flash offset is wrong the moment that layout changes -- and a
wrong offset does not fail, it silently overwrites whichever neighbouring
slot happens to be there.

So: read the partition table off the device, resolve the slot by name, check
the image fits, and only then write.
"""

import argparse
import os
import struct
import subprocess
import sys
import tempfile

PTABLE_OFFSET = 0x8000
PTABLE_SIZE = 0xC00  # ESP-IDF reserves 3072 bytes = 96 entries
ENTRY_MAGIC = 0x50AA
MD5_MAGIC = 0xEBEB

TYPE_APP = 0
TYPE_NAMES = {0: "app", 1: "data"}
SUBTYPE_NAMES = {
    0: {0: "factory", 0x10: "ota_0", 0x11: "ota_1", 0x12: "ota_2",
        0x13: "ota_3", 0x14: "ota_4", 0x20: "test"},
    1: {0: "otadata", 1: "phy", 2: "nvs", 3: "coredump", 4: "nvs_keys",
        5: "efuse", 0x81: "phy", 0x82: "fat", 0x83: "spiffs"},
}


class Partition:
    def __init__(self, ptype, subtype, offset, size, label):
        self.type = ptype
        self.subtype = subtype
        self.offset = offset
        self.size = size
        self.label = label

    @property
    def is_app(self):
        return self.type == TYPE_APP

    def subtype_name(self):
        return SUBTYPE_NAMES.get(self.type, {}).get(self.subtype, str(self.subtype))

    def type_name(self):
        return TYPE_NAMES.get(self.type, str(self.type))


def parse_table(blob):
    """Decode a raw partition table image into Partition entries."""
    parts = []
    for offset in range(0, len(blob), 32):
        entry = blob[offset:offset + 32]
        if len(entry) < 32:
            break
        magic = struct.unpack("<H", entry[:2])[0]
        if magic == MD5_MAGIC:
            continue  # checksum entry, not a partition
        if magic != ENTRY_MAGIC:
            break  # first blank/garbage entry ends the table
        ptype, subtype = entry[2], entry[3]
        part_offset, size = struct.unpack("<II", entry[4:12])
        label = entry[12:28].rstrip(b"\x00").decode("utf-8", "replace")
        parts.append(Partition(ptype, subtype, part_offset, size, label))
    return parts


def print_table(parts, highlight=None):
    print(f"  {'name':<10} {'type':<5} {'subtype':<9} {'offset':>9} "
          f"{'size':>10} {'end':>9}")
    for p in parts:
        marker = " <--" if highlight is not None and p is highlight else ""
        print(f"  {p.label:<10} {p.type_name():<5} {p.subtype_name():<9} "
              f"{hex(p.offset):>9} {p.size:>10} {hex(p.offset + p.size):>9}{marker}")


def find_esptool():
    root = os.path.expanduser("~/.platformio/packages")
    for dirpath, _dirnames, filenames in os.walk(root):
        if "esptool.py" in filenames:
            return os.path.join(dirpath, "esptool.py")
    return None


def run_esptool(esptool, port, chip, args, quiet=False):
    cmd = [sys.executable, esptool, "--chip", chip, "--port", port] + args
    result = subprocess.run(
        cmd,
        stdout=subprocess.PIPE if quiet else None,
        stderr=subprocess.STDOUT if quiet else None,
    )
    if result.returncode != 0:
        if quiet and result.stdout:
            sys.stderr.write(result.stdout.decode("utf-8", "replace"))
        raise SystemExit(f"error: esptool failed ({' '.join(args[:1])})")
    return result


def read_table(esptool, port, chip):
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as handle:
        path = handle.name
    try:
        run_esptool(esptool, port, chip,
                    ["read_flash", hex(PTABLE_OFFSET), hex(PTABLE_SIZE), path],
                    quiet=True)
        with open(path, "rb") as handle:
            return parse_table(handle.read())
    finally:
        os.unlink(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--port", default=os.environ.get("ADVHOME_PORT", "/dev/ttyACM0"))
    parser.add_argument("--chip", default="esp32s3")
    parser.add_argument("--slot", default=os.environ.get("ADVHOME_SLOT", "advhom"),
                        help="app partition label to flash into (default: advhom)")
    parser.add_argument("--firmware", default=".pio/build/m5stack-stamps3/firmware.bin")
    parser.add_argument("--esptool", default=None)
    parser.add_argument("--show", action="store_true",
                        help="print the device's partition table and exit")
    parser.add_argument("--dry-run", action="store_true",
                        help="resolve the slot and check the fit, but do not write")
    args = parser.parse_args()

    esptool = args.esptool or find_esptool()
    if not esptool:
        raise SystemExit("error: esptool.py not found under ~/.platformio/packages")

    print(f"Reading partition table from {args.port} at {hex(PTABLE_OFFSET)}...")
    parts = read_table(esptool, args.port, args.chip)

    if not parts:
        raise SystemExit(
            "error: no valid partition table on the device.\n"
            "       The table at 0x8000 is blank or corrupt. ptable.bin in the\n"
            "       repo is a known-good copy of this device's layout; restoring\n"
            "       it is a deliberate, destructive step -- do it by hand:\n"
            f"         esptool.py --chip {args.chip} --port {args.port} "
            "write_flash 0x8000 ptable.bin")

    if args.show:
        print(f"\nPartition table on {args.port}:")
        print_table(parts)
        return

    target = next((p for p in parts if p.label == args.slot and p.is_app), None)
    if target is None:
        print(f"\nPartition table on {args.port}:")
        print_table(parts)
        app_slots = [p.label for p in parts if p.is_app]
        raise SystemExit(
            f"\nerror: no app partition labelled '{args.slot}' on this device.\n"
            f"       app slots present: {', '.join(app_slots) or '(none)'}\n"
            f"       Pick one with --slot NAME or ADVHOME_SLOT=NAME.")

    if not os.path.exists(args.firmware):
        raise SystemExit(f"error: firmware not found: {args.firmware}")
    size = os.path.getsize(args.firmware)

    print(f"\nPartition table on {args.port}:")
    print_table(parts, highlight=target)

    used = 100.0 * size / target.size
    print(f"\nSlot '{target.label}': {target.size} bytes at {hex(target.offset)}")
    print(f"Firmware:      {size} bytes ({used:.1f}% of the slot)")

    if size > target.size:
        raise SystemExit(
            f"\nerror: firmware is {size - target.size} bytes too large for "
            f"'{target.label}'.\n"
            "       Refusing to write -- this would overflow into the next "
            "partition.")

    if args.dry_run:
        print("\nDry run; nothing written.")
        return

    print(f"\nWriting {args.firmware} to {hex(target.offset)}...")
    run_esptool(esptool, args.port, args.chip,
                ["write_flash", hex(target.offset), args.firmware])
    print(f"\nSuccess. Reset the device and pick '{target.label}' in M5Launcher.")


if __name__ == "__main__":
    main()
