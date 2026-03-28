Import("env")

import os
import sys

if sys.platform.startswith("win"):
    env.Append(LIBS=["ws2_32"])
    env.Append(LINKFLAGS=["-static-libgcc", "-static-libstdc++"])

    compiler_path = env.subst("$CXX")
    compiler_dir = os.path.dirname(compiler_path) if compiler_path else ""
    if compiler_dir:
        env.PrependENVPath("PATH", compiler_dir)