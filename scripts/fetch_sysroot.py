#!/usr/bin/env python3
"""Downloads and extracts the Debian bookworm (12) arm64 packages needed to
cross-compile and link CardMesh's device build (libcurl + its full transitive
dependency chain, plus sqlite3), into a local sysroot directory.

This exists because the CardputerZero AppBuilder toolchain isn't available in
this environment -- see docs/DEVICE_BUILD.md for why this approach was used
and what it does and doesn't verify.

Usage: python3 scripts/fetch_sysroot.py <output-dir>
  Produces <output-dir>/root/usr/{include,lib/aarch64-linux-gnu}/...
"""
import os
import subprocess
import sys
import urllib.request

DEBIAN_BASE = "https://deb.debian.org/debian/"

# name -> path relative to DEBIAN_BASE. Versions pinned to what was current
# in the bookworm (12) suite at the time this was written; re-resolve via
# https://deb.debian.org/debian/dists/bookworm/main/binary-arm64/Packages.xz
# if a package here 404s.
PACKAGES = {
    "libcurl4": "pool/main/c/curl/libcurl4_7.88.1-10+deb12u15_arm64.deb",
    "libcurl4-openssl-dev": "pool/main/c/curl/libcurl4-openssl-dev_7.88.1-10+deb12u15_arm64.deb",
    "libssl3": "pool/main/o/openssl/libssl3_3.0.20-1~deb12u2_arm64.deb",
    "libssl-dev": "pool/main/o/openssl/libssl-dev_3.0.20-1~deb12u2_arm64.deb",
    "libsqlite3-0": "pool/main/s/sqlite3/libsqlite3-0_3.40.1-2+deb12u2_arm64.deb",
    "libsqlite3-dev": "pool/main/s/sqlite3/libsqlite3-dev_3.40.1-2+deb12u2_arm64.deb",
    "zlib1g": "pool/main/z/zlib/zlib1g_1.2.13.dfsg-1_arm64.deb",
    "zlib1g-dev": "pool/main/z/zlib/zlib1g-dev_1.2.13.dfsg-1_arm64.deb",
    "libidn2-0": "pool/main/libi/libidn2/libidn2-0_2.3.3-1+b1_arm64.deb",
    "libpsl5": "pool/main/libp/libpsl/libpsl5_0.21.2-1_arm64.deb",
    "librtmp1": "pool/main/r/rtmpdump/librtmp1_2.4+20151223.gitfa8646d.1-2+b2_arm64.deb",
    "libssh2-1": "pool/main/libs/libssh2/libssh2-1_1.10.0-3+b1_arm64.deb",
    "libnghttp2-14": "pool/main/n/nghttp2/libnghttp2-14_1.52.0-1+deb12u3_arm64.deb",
    "libbrotli1": "pool/main/b/brotli/libbrotli1_1.0.9-2+b6_arm64.deb",
    "libgnutls30": "pool/main/g/gnutls28/libgnutls30_3.7.9-2+deb12u7_arm64.deb",
    "libunistring2": "pool/main/libu/libunistring/libunistring2_1.0-2_arm64.deb",
    "libgssapi-krb5-2": "pool/main/k/krb5/libgssapi-krb5-2_1.20.1-2+deb12u5_arm64.deb",
    "libkrb5-3": "pool/main/k/krb5/libkrb5-3_1.20.1-2+deb12u5_arm64.deb",
    "libk5crypto3": "pool/main/k/krb5/libk5crypto3_1.20.1-2+deb12u5_arm64.deb",
    "libkrb5support0": "pool/main/k/krb5/libkrb5support0_1.20.1-2+deb12u5_arm64.deb",
    "libkeyutils1": "pool/main/k/keyutils/libkeyutils1_1.6.3-2_arm64.deb",
    "libcom-err2": "pool/main/e/e2fsprogs/libcom-err2_1.47.0-2+b2_arm64.deb",
    "libp11-kit0": "pool/main/p/p11-kit/libp11-kit0_0.24.1-2_arm64.deb",
    "libtasn1-6": "pool/main/libt/libtasn1-6/libtasn1-6_4.19.0-2+deb12u1_arm64.deb",
    "libffi8": "pool/main/libf/libffi/libffi8_3.4.4-1_arm64.deb",
    "libldap-2.5-0": "pool/main/o/openldap/libldap-2.5-0_2.5.13+dfsg-5_arm64.deb",
    "libsasl2-2": "pool/main/c/cyrus-sasl2/libsasl2-2_2.1.28+dfsg-10_arm64.deb",
    "libgmp10": "pool/main/g/gmp/libgmp10_6.2.1+dfsg1-1.1_arm64.deb",
    "libhogweed6": "pool/main/n/nettle/libhogweed6_3.8.1-2_arm64.deb",
    "libnettle8": "pool/main/n/nettle/libnettle8_3.8.1-2_arm64.deb",
    "libzstd1": "pool/main/libz/libzstd/libzstd1_1.5.4+dfsg2-5_arm64.deb",
}


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)

    out_dir = os.path.abspath(sys.argv[1])
    debs_dir = os.path.join(out_dir, "debs")
    root_dir = os.path.join(out_dir, "root")
    work_dir = os.path.join(out_dir, "work")
    os.makedirs(debs_dir, exist_ok=True)
    os.makedirs(root_dir, exist_ok=True)
    os.makedirs(work_dir, exist_ok=True)

    for name, rel_path in PACKAGES.items():
        dest = os.path.join(debs_dir, os.path.basename(rel_path))
        if os.path.exists(dest):
            print(f"skip (cached): {name}")
            continue
        print(f"fetch: {name}")
        urllib.request.urlretrieve(DEBIAN_BASE + rel_path, dest)

    for deb_name in sorted(os.listdir(debs_dir)):
        deb_path = os.path.join(debs_dir, deb_name)
        extract_dir = os.path.join(work_dir, deb_name)
        os.makedirs(extract_dir, exist_ok=True)
        subprocess.run(["ar", "x", deb_path], cwd=extract_dir, check=True)

        data_files = [f for f in os.listdir(extract_dir) if f.startswith("data.tar")]
        if not data_files:
            raise RuntimeError(f"no data.tar.* found in {deb_name}")
        subprocess.run(["tar", "-xf", os.path.join(extract_dir, data_files[0]), "-C", root_dir], check=True)
        print(f"extracted: {deb_name}")

    # Debian packages ship files under /lib/<triplet>/..., which is a symlink
    # to /usr/lib/<triplet>/... on a real merged-/usr system; our tar
    # extraction doesn't have that symlink, so merge manually.
    lib_dir = os.path.join(root_dir, "lib")
    usr_lib_dir = os.path.join(root_dir, "usr", "lib")
    if os.path.isdir(lib_dir):
        subprocess.run(["cp", "-a", lib_dir + "/.", usr_lib_dir + "/"], check=True)

    print(f"\nSysroot ready at: {root_dir}")
    print("Next: python3 scripts/build_arm64.py " + root_dir)


if __name__ == "__main__":
    main()
