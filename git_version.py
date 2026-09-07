import subprocess
Import("env")

try:
    # Get the git commit hash/tag
    ret = subprocess.run(["git", "describe", "--always", "--dirty", "--tags"], stdout=subprocess.PIPE, text=True, check=True)
    git_version = ret.stdout.strip()
except Exception:
    git_version = "unknown"

# Define the macro
env.Append(BUILD_FLAGS=[f'-D ADVHOME_VERSION=\\"{git_version}\\"'])
