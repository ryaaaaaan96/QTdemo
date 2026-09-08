"""Build a child-process environment without changing the parent environment."""

import json
import os
from pathlib import Path
import sys

PROJECT_ROOT = Path(__file__).resolve().parents[1]
TOOL_NAMES = ("QT_ROOT", "LLVM_ROOT", "CMAKE_ROOT", "NINJA_ROOT", "IFW_ROOT", "IFW_VERSION")


def load_environment(root=PROJECT_ROOT):
    env = os.environ.copy()
    config_path = root / "tools.local.json"
    if config_path.is_file():
        config = json.loads(config_path.read_text(encoding="utf-8-sig"))
        if not isinstance(config, dict):
            raise ValueError("tools.local.json must contain a JSON object")
        for name in TOOL_NAMES:
            value = config.get(name)
            if value is not None and not isinstance(value, str):
                raise ValueError(f"{name} must be a string")
            if value:
                env[name] = value

    required = {
        "QT_ROOT": "bin/qtpaths6.exe",
        "LLVM_ROOT": "bin/clang++.exe",
        "CMAKE_ROOT": "bin/cmake.exe",
        "NINJA_ROOT": "ninja.exe",
    }
    for name, executable in required.items():
        if not env.get(name) or not (Path(env[name]) / executable).is_file():
            raise ValueError(
                f"Invalid {name}. Copy tools.example.json to tools.local.json "
                "and set your tool paths, or set environment variables."
            )

    paths = [
        str(Path(sys.executable).parent),
        str(Path(env["LLVM_ROOT"]) / "bin"),
        str(Path(env["QT_ROOT"]) / "bin"),
        str(Path(env["CMAKE_ROOT"]) / "bin"),
        env["NINJA_ROOT"],
        env.get("PATH", ""),
    ]
    env["PATH"] = os.pathsep.join(paths)
    return env
