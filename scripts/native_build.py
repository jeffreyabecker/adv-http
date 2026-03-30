Import("env")

import os
import shutil
import sys


def _resolve_compiler_dir(build_env):
    compiler_path = build_env.subst("$CXX")
    if compiler_path:
        if os.path.isabs(compiler_path):
            compiler_dir = os.path.dirname(compiler_path)
            if compiler_dir:
                return compiler_dir

        resolved_path = shutil.which(compiler_path)
        if resolved_path:
            compiler_dir = os.path.dirname(resolved_path)
            if compiler_dir:
                return compiler_dir

    return ""


def _copy_runtime_dll(target, source, env):
    if not sys.platform.startswith("win"):
        return

    compiler_dir = _resolve_compiler_dir(env)
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

    compiler_dir = _resolve_compiler_dir(env)
    if compiler_dir:
        env.PrependENVPath("PATH", compiler_dir)

env.AddPostAction("$PROGPATH", _copy_runtime_dll)