#!/usr/bin/env python3
"""
fix_test_relative_includes.py

Updates relative include paths in `test/` files that point to `src/` to the new `src/httpadv/v1/` layout.
This is a focused, mechanical rewrite: it replaces occurrences of
  ../../src/
  ../../../src/
with the corresponding httpadv path.

Run as: python scripts/fix_test_relative_includes.py --apply (default is dry-run)
"""
import argparse
from pathlib import Path


REPLACEMENTS = [
    ('../../src/', '../../src/httpadv/v1/'),
    ('../../../src/', '../../../src/httpadv/v1/'),
    ('#include "src/', '#include "src/httpadv/v1/'),
]


def find_test_files(root='test'):
    exts = {'.cpp', '.c', '.cc', '.h', '.hpp'}
    for p in Path(root).rglob('*'):
        if p.is_file() and p.suffix.lower() in exts:
            yield p


def process_file(path, apply=False):
    text = path.read_text(encoding='utf-8')
    new = text
    for old, newprefix in REPLACEMENTS:
        new = new.replace(old, newprefix)
    if new != text:
        if apply:
            path.write_text(new, encoding='utf-8')
            print(f'APPLIED: {path}')
        else:
            print(f'WOULD CHANGE: {path}')
        return True
    return False


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--apply', action='store_true')
    args = p.parse_args()

    any_changed = False
    for f in find_test_files():
        changed = process_file(f, apply=args.apply)
        any_changed = any_changed or changed
    if not any_changed:
        print('No test include patterns found to update.')


if __name__ == '__main__':
    main()
