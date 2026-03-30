#!/usr/bin/env python3
"""
fix_double_httpadv_path.py

Fixes accidental duplicated include paths like
  src/httpadv/v1/httpadv/v1/...
to
  src/httpadv/v1/...

Run as: python scripts/fix_double_httpadv_path.py --apply
"""
from pathlib import Path
import argparse


def find_files(root='.', exts=None):
    if exts is None:
        exts = {'.h', '.hpp', '.c', '.cc', '.cpp'}
    for p in Path(root).rglob('*'):
        if p.is_file() and p.suffix.lower() in exts:
            yield p


def process(path, apply=False):
    text = path.read_text(encoding='utf-8')
    new = text.replace('httpadv/v1/httpadv/v1/', 'httpadv/v1/')
    if new != text:
        if apply:
            path.write_text(new, encoding='utf-8')
            print('APPLIED:', path)
        else:
            print('WOULD CHANGE:', path)
        return True
    return False


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--apply', action='store_true')
    args = p.parse_args()

    any_changed = False
    for f in find_files('.'):
        changed = process(f, apply=args.apply)
        any_changed = any_changed or changed
    if not any_changed:
        print('No duplicated httpadv paths found.')


if __name__ == '__main__':
    main()
