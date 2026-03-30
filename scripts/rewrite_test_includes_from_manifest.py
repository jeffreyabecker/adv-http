#!/usr/bin/env python3
"""
rewrite_test_includes_from_manifest.py

Use the move manifest to rewrite include paths in `test/` files.
This specifically handles cases where test includes point to `src/httpadv/v1/...`
and need to be redirected according to the manifest (e.g., compat -> transport).

Run with --dry-run to preview, --apply to write changes.
"""
import argparse
import csv
from pathlib import Path
import re


def read_manifest(path):
    mapping = {}
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            src = row['source'].strip()
            tgt = row['target'].strip()
            if src.startswith('src/'):
                src = src[len('src/'):] 
            if tgt.startswith('src/'):
                tgt = tgt[len('src/'):] 
            mapping[src] = tgt
    return mapping


INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*[<\"])([^>\"]+)([>\"])')


def find_test_files(root='test'):
    exts = {'.cpp', '.c', '.cc', '.h', '.hpp'}
    for p in Path(root).rglob('*'):
        if p.is_file() and p.suffix.lower() in exts:
            yield p


def apply_mapping_to_include(inc, mapping):
    # normalize leading ../ or ./
    prefix = ''
    norm = inc
    while norm.startswith('../') or norm.startswith('./'):
        if norm.startswith('../'):
            prefix += '../'
            norm = norm[len('../'):]
        else:
            prefix += './'
            norm = norm[len('./'):]

    # if norm starts with src/httpadv/v1/, strip that
    if norm.startswith('src/httpadv/v1/'):
        candidate = norm[len('src/httpadv/v1/'):]
    elif norm.startswith('src/'):
        candidate = norm[len('src/'):]
    else:
        candidate = norm

    if candidate in mapping:
        new_rel = mapping[candidate]
        # produce a replacement that keeps same relative prefix (../..)
        return prefix + 'src/' + new_rel
    return None


def run(manifest_path='docs/backlog/move-manifest.csv', dry_run=True):
    mapping = read_manifest(manifest_path)
    any_changes = False
    for f in find_test_files():
        text = f.read_text(encoding='utf-8')
        out_lines = []
        changed = False
        for line in text.splitlines():
            m = INCLUDE_RE.match(line)
            if m:
                inc = m.group(2)
                new_inc = apply_mapping_to_include(inc, mapping)
                if new_inc:
                    print(f"{f}: would replace '{inc}' -> '{new_inc}'")
                    out_lines.append(m.group(1) + new_inc + m.group(3))
                    changed = True
                    any_changes = True
                    continue
            out_lines.append(line)
        if changed and not dry_run:
            f.write_text('\n'.join(out_lines) + '\n', encoding='utf-8')

    if not any_changes:
        print('No test include references found for manifest-based rewrite.')


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--manifest', default='docs/backlog/move-manifest.csv')
    p.add_argument('--dry-run', action='store_true')
    p.add_argument('--apply', action='store_true')
    args = p.parse_args()
    run(args.manifest, dry_run=not args.apply)


if __name__ == '__main__':
    main()
