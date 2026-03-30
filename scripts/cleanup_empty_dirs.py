#!/usr/bin/env python3
"""
cleanup_empty_dirs.py

Remove empty directories recursively under `src/`.

Usage:
  python scripts/cleanup_empty_dirs.py --root src --dry-run
"""
import argparse
import os


def find_empty_dirs(root):
    empty = []
    for dirpath, dirnames, filenames in os.walk(root, topdown=False):
        # skip hidden .git directory
        if '.git' in dirpath.split(os.sep):
            continue
        if not dirnames and not filenames:
            empty.append(dirpath)
    return empty


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--root', default='src')
    p.add_argument('--dry-run', action='store_true')
    args = p.parse_args()

    root = args.root
    if not os.path.isdir(root):
        print('Root not found or not a directory:', root)
        return

    empty = find_empty_dirs(root)
    if not empty:
        print('No empty directories found under', root)
        return

    for d in empty:
        print(('DRY: would remove' if args.dry_run else 'REMOVE:'), d)
        if not args.dry_run:
            try:
                os.rmdir(d)
            except Exception as e:
                print('Failed to remove', d, ':', e)


if __name__ == '__main__':
    main()
