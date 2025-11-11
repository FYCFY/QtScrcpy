#!/usr/bin/env python3
import argparse
import os
import shutil
import stat
import sys
import tempfile
import urllib.request
import zipfile
from typing import List

URLS = {
    "win": "https://dl.google.com/android/repository/platform-tools-latest-windows.zip",
    "mac": "https://dl.google.com/android/repository/platform-tools-latest-darwin.zip",
}


def download_zip(url: str, target: str) -> None:
    with urllib.request.urlopen(url) as response, open(target, "wb") as dst:
        shutil.copyfileobj(response, dst)


def extract_fastboot(zip_path: str, binary_name: str, dest_path: str) -> None:
    with zipfile.ZipFile(zip_path) as archive:
        member_name = None
        for name in archive.namelist():
            if not name.endswith(binary_name):
                continue
            if "platform-tools/" not in name:
                continue
            member_name = name
            break
        if not member_name:
            raise RuntimeError("fastboot binary not found in archive")
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        with archive.open(member_name) as src, open(dest_path, "wb") as dst:
            shutil.copyfileobj(src, dst)


def make_executable(path: str) -> None:
    mode = os.stat(path).st_mode
    os.chmod(path, mode | stat.S_IEXEC)


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description="Fetch official fastboot binary from Google Platform Tools.")
    parser.add_argument("--platform", choices=URLS.keys(), required=True, help="Target platform to download.")
    parser.add_argument("--dest", required=True, help="Directory to store the fastboot binary.")
    args = parser.parse_args(argv)

    url = URLS[args.platform]
    dest_dir = os.path.abspath(args.dest)
    os.makedirs(dest_dir, exist_ok=True)
    binary_name = "fastboot.exe" if args.platform == "win" else "fastboot"
    target_path = os.path.join(dest_dir, binary_name)

    print(f"[fetch_fastboot] downloading {url}")
    with tempfile.TemporaryDirectory() as tmpdir:
        zip_path = os.path.join(tmpdir, "platform-tools.zip")
        download_zip(url, zip_path)
        print(f"[fetch_fastboot] extracting {binary_name} -> {target_path}")
        extract_fastboot(zip_path, binary_name, target_path)

    if args.platform != "win":
        make_executable(target_path)

    print(f"[fetch_fastboot] fastboot saved to {target_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
