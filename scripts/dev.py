"""Qt development commands. Uses only the Python standard library."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

from environment import PROJECT_ROOT, load_environment


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "action", nargs="?", default="build",
        choices=("configure", "build", "test", "run", "install", "package", "shell"),
    )
    parser.add_argument("--preset", choices=("debug", "release"), default="debug")
    args = parser.parse_args()
    if os.name != "nt":
        parser.error("This starter currently supports Windows LLVM-MinGW only.")

    try:
        env = load_environment()
        cmake_bin = Path(env["CMAKE_ROOT"]) / "bin"

        def run(program, *arguments):
            command = [str(program), *map(str, arguments)]
            print("+ " + subprocess.list2cmdline(command), flush=True)
            subprocess.run(command, cwd=PROJECT_ROOT, env=env, check=True)

        if args.action == "shell":
            shell = shutil.which("pwsh.exe", path=env["PATH"]) or shutil.which(
                "powershell.exe", path=env["PATH"]
            )
            if not shell:
                raise ValueError("PowerShell was not found on PATH")
            print("Qt environment ready. Run code . or CMake commands; exit returns to the original shell.")
            run(shell, "-NoLogo", "-NoProfile")
            return 0

        if args.action == "package":
            if not env.get("IFW_ROOT") or not (
                Path(env["IFW_ROOT"]) / "bin/binarycreator.exe"
            ).is_file():
                raise ValueError("Packaging requires IFW_ROOT pointing to Qt Installer Framework.")

        preset = "release" if args.action == "package" else args.preset
        run(cmake_bin / "cmake.exe", "--preset", preset)
        if args.action == "configure":
            return 0
        run(cmake_bin / "cmake.exe", "--build", "--preset", preset)
        if args.action == "test":
            run(cmake_bin / "ctest.exe", "--preset", preset)
        elif args.action == "run":
            run(PROJECT_ROOT / "build" / preset / "bin/QTdemo.exe")
        elif args.action == "install":
            run(cmake_bin / "cmake.exe", "--install", f"build/{preset}")
        elif args.action == "package":
            run(cmake_bin / "ctest.exe", "--preset", "release")
            run(cmake_bin / "cpack.exe", "--preset", "installer")
            run(cmake_bin / "cpack.exe", "--preset", "portable")
        return 0
    except subprocess.CalledProcessError as error:
        print(f"Command failed (exit {error.returncode})", file=sys.stderr)
        return 1
    except (OSError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main())
