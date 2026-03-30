#!/usr/bin/env python3
"""
rewrite_test_includes.py

Updates `#include` paths inside `test/` files using the move manifest.
Runs in dry-run by default and prints planned replacements.
"""
import argparse
import csv
import re
from pathlib import Path

INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*[<\"])([^>\"]+)([>\"])')


def read_manifest(path):
    pairs = []
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            src = row['source'].strip()
            tgt = row['target'].strip()
            # normalize to paths relative to repo root without leading src/
            if src.startswith('src/'):
                src_rel = src[len('src/'):]
            else:
                src_rel = src
            if tgt.startswith('src/'):
                tgt_rel = tgt[len('src/'):]
            else:
                tgt_rel = tgt
            pairs.append((src_rel, tgt_rel))
    return dict(pairs)


def find_test_files(root):
    exts = {'.cpp', '.c', '.cc', '.h', '.hpp'}
    for p in Path(root).rglob('*'):
        if p.is_file() and p.suffix.lower() in exts:
            yield p


def run(manifest_path, dry_run=True):
    repo_root = Path(__file__).resolve().parent.parent
    manifest = repo_root.joinpath(manifest_path)
    if not manifest.exists():
        print('Manifest not found:', manifest)
        return 1
    repl = read_manifest(str(manifest))

    any_changes = False
    for f in find_test_files(repo_root.joinpath('test')):
        text = f.read_text(encoding='utf-8')
        out = []
        changed = False
        for line in text.splitlines():
            m = INCLUDE_RE.match(line)
            if m:
                inc = m.group(2)
                # normalize leading relative segments (../../)
                norm = inc
                while norm.startswith('../') or norm.startswith('./'):
                    if norm.startswith('../'):
                        norm = norm[len('../'):]
                    elif norm.startswith('./'):
                        norm = norm[len('./'):]
                new_inc = None
                # direct match (e.g., compat/ByteStream.h)
                if inc in repl:
                    new_inc = repl[inc]
                # matches starting with src/ or src/httpadv/v1/
                elif inc.startswith('src/'):
                    candidate = inc[len('src/'):]
                    if candidate in repl:
                        new_inc = repl[candidate]
                elif inc.startswith('src/httpadv/v1/'):
                    candidate = inc[len('src/httpadv/v1/'):]
                    if candidate in repl:
                        new_inc = repl[candidate]

                if new_inc:
                    # ensure we output a path relative to repo root as tests expect
                    print(f"{f}: would replace include '{inc}' -> '{new_inc}'")
                    changed = True
                    any_changes = True
                    out.append(m.group(1) + new_inc + m.group(3))
                    continue
            out.append(line)
        if changed and not dry_run:
            f.write_text('\n'.join(out) + '\n', encoding='utf-8')

    if not any_changes:
        print('No test include references found for the manifest mappings.')
    return 0


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--manifest', default='docs/backlog/move-manifest.csv')
    p.add_argument('--dry-run', action='store_true')
    args = p.parse_args()
    return run(args.manifest, dry_run=args.dry_run)


if __name__ == '__main__':
    raise SystemExit(main())
