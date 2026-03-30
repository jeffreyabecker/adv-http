#!/usr/bin/env python3
"""
apply_move_manifest.py (workspace copy)
Mechanical move helper for the namespacing refactor.
"""
import argparse
import csv
import os
import subprocess
import sys


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def run(cmd):
    print("$", " ".join(cmd))
    subprocess.check_call(cmd)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--manifest", required=True)
    p.add_argument("--dry-run", action="store_true")
    args = p.parse_args()

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    manifest = os.path.abspath(args.manifest)

    moves = []
    with open(manifest, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            src = os.path.join(repo_root, row['source'].strip())
            tgt = os.path.join(repo_root, row['target'].strip())
            if not os.path.exists(src):
                print(f"SKIP: source missing: {src}")
                continue
            tgt_dir = os.path.dirname(tgt)
            ensure_dir(tgt_dir)
            moves.append((src, tgt))

    if not moves:
        print("No moves to perform.")
        return

    for src, tgt in moves:
        rel_src = os.path.relpath(src, repo_root)
        rel_tgt = os.path.relpath(tgt, repo_root)
        if args.dry_run:
            print(f"DRY RUN: git mv -- " + rel_src + " -> " + rel_tgt)
        else:
            run(["git", "mv", rel_src, rel_tgt])

    print("Done. Next: run include-rewrite pass using the same manifest to update #include paths.")


if __name__ == '__main__':
    main()
