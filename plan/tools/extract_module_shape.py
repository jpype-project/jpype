#!/usr/bin/env python3
"""Draft an SPI WrapperService manifest from a real Python module's class
hierarchy, instead of hand-writing it (see plan/IO.md, plan/SPI.md).

Dev-time tool, not shipped. Walks a module's classes via `inspect`,
reconstructs the desired Java-interface inheritance chain from each class's
`__mro__`, and groups results by `__module__` -- surfacing splits like
`io` (public facade) vs `_io` (C accelerator, where the concrete classes
actually live) that a hand-written manifest would otherwise miss silently.

Usage:
    python3 plan/tools/extract_module_shape.py io
    python3 plan/tools/extract_module_shape.py io --json
"""
import argparse
import importlib
import inspect
import json
import sys


def java_name(py_class_name: str) -> str:
    """Heuristic: `_RawIOBase` / `RawIOBase` -> `PyRawIOBase`."""
    return "Py" + py_class_name.lstrip("_")


def interface_chain(cls: type) -> list[str]:
    """MRO, minus `object`, mapped to draft Java interface names, deduped
    in MRO order (a class and its "public" and "private" mro duplicates --
    e.g. `_io.BytesIO`'s mro includes `_io._BufferedIOBase`, not
    `io.BufferedIOBase` -- collapse to the same drafted name)."""
    seen = set()
    chain = []
    for c in cls.__mro__:
        if c is object:
            continue
        name = java_name(c.__name__)
        if name not in seen:
            seen.add(name)
            chain.append(name)
    return chain


def own_public_methods(cls: type) -> list[str]:
    """Methods/properties introduced or overridden at this class level
    (present in cls.__dict__), public API surface only."""
    names = []
    for name, member in vars(cls).items():
        if name.startswith("_") and name not in (
            "__enter__", "__exit__", "__iter__", "__next__", "__del__",
        ):
            continue
        if inspect.isfunction(member) or isinstance(member, (staticmethod, classmethod)):
            names.append(name)
        elif isinstance(member, property):
            names.append(name + " (property)")
    return sorted(names)


def collect_classes(module) -> dict[str, type]:
    found = {}
    for name in dir(module):
        obj = getattr(module, name)
        if inspect.isclass(obj):
            found[name] = obj
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("module", help="module to introspect, e.g. io")
    parser.add_argument("--json", action="store_true",
                         help="emit the {module_name: {class: [interfaces]}} "
                              "manifest as JSON instead of the human report")
    args = parser.parse_args()

    module = importlib.import_module(args.module)
    classes = collect_classes(module)

    # Group by the *reporting* module (__module__), which is where the
    # `_io` vs `io` split shows up -- this is the exact grouping a
    # WrapperService.getModuleManifest(moduleName) call needs to key on.
    by_module: dict[str, dict[str, type]] = {}
    for name, cls in classes.items():
        by_module.setdefault(cls.__module__, {})[name] = cls

    manifest: dict[str, dict[str, list[str]]] = {}
    for mod_name, mod_classes in by_module.items():
        manifest[mod_name] = {
            cls_name: interface_chain(cls)
            for cls_name, cls in mod_classes.items()
        }

    if args.json:
        json.dump(manifest, sys.stdout, indent=2, sort_keys=True)
        print()
        return 0

    print(f"# Draft WrapperService manifest for `{args.module}`\n")
    if len(by_module) > 1:
        print(f"** Module split detected: classes reported under "
              f"{sorted(by_module.keys())} **")
        print("A single WrapperService.getModuleManifest(name) call per "
              "module name is needed for EACH of these -- registering only "
              f"under \"{args.module}\" will miss the others.\n")

    for mod_name, mod_classes in sorted(by_module.items()):
        print(f"## module={mod_name!r}  ({len(mod_classes)} classes)\n")
        for cls_name, cls in sorted(mod_classes.items()):
            chain = interface_chain(cls)
            print(f"- {cls_name}")
            print(f"    interfaces: {' -> '.join(chain)}")
            own = own_public_methods(cls)
            if own:
                print(f"    own methods: {', '.join(own)}")
        print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
