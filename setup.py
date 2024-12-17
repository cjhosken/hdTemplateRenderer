import os
import subprocess
import sys
import urllib.request
import tarfile

def pip_installs(packages):
    subprocess.check_call([sys.executable, "-m", "pip", "install", "--upgrade", "pip"])

    for package in packages:
        print(f"Installing {package}...")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])

def build_usd(usd_version="24.11", build_dir="build", install_dir=os.path.join("deps", "usd"), args=[]):
    print("\n\n")
    print(f"---- Building USD {usd_version} ----")
    print(f"Python Version: {sys.version}")
    print(f"USD Custom Build Args: {args}")
    print(f"Build directory: {os.path.abspath(build_dir)}")
    print(f"Install directory: {os.path.abspath(install_dir)}")
    print("\n\n")

    pip_installs([
        "PySide6",
        "PyOpenGL"
        ])

    usd_url = f"https://github.com/PixarAnimationStudios/OpenUSD/archive/refs/tags/v{usd_version}.tar.gz"
    usd_archive = f"OpenUSD-v{usd_version}.tar.gz"
    
    os.makedirs(build_dir, exist_ok=True)
    
    archive_path = os.path.join(build_dir, usd_archive)
    if not os.path.exists(archive_path):
        print(f"Downloading OpenUSD from {usd_url}...")
        urllib.request.urlretrieve(usd_url, archive_path)

    extracted_dir = os.path.join(build_dir, f"OpenUSD-{usd_version}")
    if not os.path.exists(extracted_dir):
        print(f"Extracting {usd_archive}...")
        with tarfile.open(archive_path, "r:gz") as tar:
            tar.extractall(path=build_dir)
            
    if not os.path.exists(extracted_dir):
        print(f"Error: Extracted directory {extracted_dir} not found.")
        sys.exit(1)

    build_script = os.path.join(extracted_dir, "build_scripts", "build_usd.py")

    if not os.path.exists(build_script):
        print(f"Error: {build_script} not found.")
        sys.exit(1)

    build_command = [
        sys.executable,
        build_script,
        install_dir
    ]

    for arg in args:
        build_command.append(arg)

    print(f"Running {build_command}...")

    try:
        subprocess.check_call(build_command)
        print("Build completed!")
        os.system('rm -rf build/*')

    except subprocess.CalledProcessError as e:
        print(f"Error during build: {e}")
    

def main():
    version="24.11"
    
    args = [
        "--build-shared",
        "--openvdb",
        "--openimageio",
        "--opencolorio",
        "--ptex",
        "--alembic",
        "--no-hdf5",
        "--no-draco",
        "--embree",
        "--no-tests",
        "--no-prman"
    ]

    build_dir="build"
    install_dir=os.path.join("lib", "usd")

    build_usd(usd_version=version, build_dir=build_dir, install_dir=install_dir, args=args)

if __name__ == "__main__":
    main()