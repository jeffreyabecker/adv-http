# llhttp Vendor Notes

This directory is a vendored copy of `llhttp` that is kept in-tree.

## Provenance

- Upstream project: `nodejs/llhttp`
- Intended version: `9.3.1`
- Source type: generated release snapshot

This copy is not treated as a git submodule and is not expected to match the upstream git tag tree exactly.
The files used by this project come from the release-produced `llhttp` source snapshot, which includes generated artifacts that are not all present in upstream source control in the same form.

## Project Policy

- Keep `llhttp` checked into this repository as a vendored dependency.
- Do not replace this directory with a submodule.
- Do not make builds depend on fetching `llhttp` from git at build time.
- Preserve the local include path shape so the rest of the project does not need churn when `llhttp` is refreshed.

## Updating

When updating this dependency:

1. Start from the upstream release-generated `llhttp` snapshot for the target version.
2. Replace the local vendored files with that snapshot.
3. Reapply any project-local compatibility fixes that are still needed.
4. Update this file if the vendored version or update policy changes.

## Local Modifications

This repository may carry small local compatibility adjustments on top of the release snapshot when required for embedded targets or toolchain compatibility.
Those changes should remain minimal and reviewable.