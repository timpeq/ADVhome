import os
Import("env")

def merge_bin(source, target, env):
    # ESP32-S3 typical offsets:
    # Bootloader: 0x0
    # Partitions: 0x8000
    # Firmware: 0x10000
    
    platform = env.PioPlatform()
    esptool_path = os.path.join(platform.get_package_dir("tool-esptoolpy"), "esptool.py")
    
    build_dir = env.subst("$BUILD_DIR")
    merged_bin = os.path.join(build_dir, "firmware_merged.bin")
    
    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    firmware = os.path.join(build_dir, "firmware.bin")
    
    # We use ESP32-S3 chip type for Cardputer
    cmd = [
        env.subst("$PYTHONEXE"), esptool_path,
        "--chip", "esp32s3",
        "merge_bin",
        "-o", merged_bin,
        "--flash_mode", "dio",
        "--flash_size", "8MB",
        "0x0", bootloader,
        "0x8000", partitions,
        "0x10000", firmware
    ]
    
    print("\n--- Generating Merged Binary for M5Burner/Full Flashing ---")
    import subprocess
    subprocess.run(cmd, check=True)
    print(f"--> Merged binary created at: {merged_bin}\n")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)
