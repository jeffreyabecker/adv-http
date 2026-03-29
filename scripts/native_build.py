Import("env")

import os
import shutil
import sys


def _copy_runtime_dll(target, source, env):
    if not sys.platform.startswith("win"):
        return

    compiler_path = env.subst("$CXX")
    compiler_dir = os.path.dirname(compiler_path) if compiler_path else ""
    if not compiler_dir:
        return

    program_path = str(target[0])
    output_dir = os.path.dirname(program_path)
    for dll_name in ("libwinpthread-1.dll", "libgcc_s_seh-1.dll", "libstdc++-6.dll"):
        source_path = os.path.join(compiler_dir, dll_name)
        if os.path.exists(source_path):
            shutil.copy2(source_path, os.path.join(output_dir, dll_name))

if sys.platform.startswith("win"):
    env.Append(LIBS=["ws2_32"])
    env.Append(LINKFLAGS=["-static-libgcc", "-static-libstdc++"])

    compiler_path = env.subst("$CXX")
    compiler_dir = os.path.dirname(compiler_path) if compiler_path else ""
    if compiler_dir:
        env.PrependENVPath("PATH", compiler_dir)

env.AddPostAction("$PROGPATH", _copy_runtime_dll)