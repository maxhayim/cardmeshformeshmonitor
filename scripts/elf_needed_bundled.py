"""Minimal ELF64 .dynamic section parser: lists DT_NEEDED entries and
computes the full transitive closure of shared library dependencies for a
binary, resolving each name against a given library directory.

Used by package_deb.py to determine exactly which .so files must be bundled
alongside the cross-compiled cardmesh binary.
"""
import os
import struct

# Provided by the target's base glibc install; never bundled ourselves.
BASE_SYSTEM_LIBS = {
    "libc.so.6",
    "ld-linux-aarch64.so.1",
    "libm.so.6",
    "libdl.so.2",
    "libpthread.so.0",
    "librt.so.1",
    "libresolv.so.2",
}


def parse_needed(path):
    with open(path, "rb") as f:
        data = f.read()

    assert data[:4] == b"\x7fELF", f"not an ELF file: {path}"
    endian = "<" if data[5] == 1 else ">"
    e_shoff = struct.unpack_from(endian + "Q", data, 0x28)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(endian + "HHH", data, 0x3A)

    def section(i):
        off = e_shoff + i * e_shentsize
        name, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from(
            endian + "IIQQQQIIQQ", data, off
        )
        return dict(name=name, type=typ, offset=offset, size=size, entsize=entsize, link=link)

    shstr = section(e_shstrndx)

    def strat(base, off):
        end = data.index(b"\x00", base + off)
        return data[base + off : end].decode()

    dynsec = dynstr = None
    for i in range(e_shnum):
        s = section(i)
        if strat(shstr["offset"], s["name"]) == ".dynamic":
            dynsec = s
        if strat(shstr["offset"], s["name"]) == ".dynstr":
            dynstr = s

    if dynsec is None:
        return []

    needed = []
    entsize = 16
    count = dynsec["size"] // entsize
    for i in range(count):
        tag, val = struct.unpack_from(endian + "qQ", data, dynsec["offset"] + i * entsize)
        if tag == 0:  # DT_NULL
            break
        if tag == 1:  # DT_NEEDED
            needed.append(strat(dynstr["offset"], val))
    return needed


def parse_soname(path):
    """Returns the library's own DT_SONAME, or None if it has none."""
    with open(path, "rb") as f:
        data = f.read()

    endian = "<" if data[5] == 1 else ">"
    e_shoff = struct.unpack_from(endian + "Q", data, 0x28)[0]
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from(endian + "HHH", data, 0x3A)

    def section(i):
        off = e_shoff + i * e_shentsize
        name, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from(
            endian + "IIQQQQIIQQ", data, off
        )
        return dict(name=name, type=typ, offset=offset, size=size, entsize=entsize, link=link)

    shstr = section(e_shstrndx)

    def strat(base, off):
        end = data.index(b"\x00", base + off)
        return data[base + off : end].decode()

    dynsec = dynstr = None
    for i in range(e_shnum):
        s = section(i)
        if strat(shstr["offset"], s["name"]) == ".dynamic":
            dynsec = s
        if strat(shstr["offset"], s["name"]) == ".dynstr":
            dynstr = s

    if dynsec is None:
        return None

    entsize = 16
    count = dynsec["size"] // entsize
    for i in range(count):
        tag, val = struct.unpack_from(endian + "qQ", data, dynsec["offset"] + i * entsize)
        if tag == 0:  # DT_NULL
            break
        if tag == 14:  # DT_SONAME
            return strat(dynstr["offset"], val)
    return None


def compute_closure(binary_path, libdir):
    """Returns {soname: resolved_real_path} for every non-base-system shared
    library needed by `binary_path`, transitively."""
    resolved = {}
    queue = list(parse_needed(binary_path))
    seen = set(queue)

    while queue:
        name = queue.pop(0)
        if name in BASE_SYSTEM_LIBS:
            continue
        path = os.path.join(libdir, name)
        if not os.path.exists(path):
            raise FileNotFoundError(f"required shared library not found in sysroot: {name}")
        resolved[name] = os.path.realpath(path)
        for dep in parse_needed(path):
            if dep not in seen:
                seen.add(dep)
                queue.append(dep)

    return resolved
