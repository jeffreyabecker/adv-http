#!/usr/bin/env python3
"""
rewrite_includes.py (workspace copy)
"""
import argparse
import csv
import os
import re
from pathlib import Path


def read_manifest(path):
    pairs = []
    with open(path, newline='') as f:
        reader = csv.DictReader(f)
        for row in reader:
            src = row['source'].strip()
            tgt = row['target'].strip()
            pairs.append((src, tgt))
    return pairs


INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*[<\"])([^>\"]+)([>\"])')


def find_files(root):
    exts = {'.h', '.hpp', '.c', '.cc', '.cpp'}
    for p in Path(root).rglob('*'):
        if p.is_file() and p.suffix.lower() in exts:
            yield p


def plan_replacements(manifest_pairs):
    repl = {}
    for src, tgt in manifest_pairs:
        old = src
        if old.startswith('src/'):
            old = old[len('src/'):]
        new = tgt
        if new.startswith('src/'):
            new = new[len('src/'):]
        repl[old] = new
    return repl


def process_file(path, repl_map, dry_run=True):
    changed = False
    text = path.read_text(encoding='utf-8')
    lines = text.splitlines()
    out_lines = []
    for line in lines:
        m = INCLUDE_RE.match(line)
        if m:
            inc = m.group(2)
            if inc in repl_map:
                new_inc = repl_map[inc]
                new_line = m.group(1) + new_inc + m.group(3)
                out_lines.append(new_line)
                changed = True
                if dry_run:
                    print(f"{path}: would replace include '{inc}' -> '{new_inc}'")
                continue
        out_lines.append(line)
    if changed and not dry_run:
        path.write_text('\n'.join(out_lines) + '\n', encoding='utf-8')
    return changed


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--manifest', required=True)
    p.add_argument('--dry-run', action='store_true')
    args = p.parse_args()

    repo_root = Path(__file__).resolve().parent.parent
    manifest = repo_root.joinpath(args.manifest)
    if not manifest.exists():
        print('Manifest not found:', manifest)
        return

    pairs = read_manifest(str(manifest))
    repl_map = plan_replacements(pairs)

    any_changes = False
    for f in find_files(repo_root):
        changed = process_file(f, repl_map, dry_run=args.dry_run)
        any_changes = any_changes or changed

    if not any_changes:
        print('No include references found for the manifest mappings.')


if __name__ == '__main__':
    main()
