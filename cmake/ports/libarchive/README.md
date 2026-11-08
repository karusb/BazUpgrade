# Temporary libarchive security update

Derived from the public Microsoft vcpkg libarchive port at 6fdd23fa0a9479a9f9a1c48178696f60cf39255d, retaining its MIT license in VCPKG-LICENSE.txt. The upstream version and archive SHA-512 are updated. Existing bcrypt, libmd, iconv and XML pkg-config hunks are rebased onto upstream's changed CMake context; build/dependency fixes retain their original behavior.

Upstream libarchive 3.8.11 is a security/bugfix release. The vcpkg baseline still resolves 3.8.9. This overlay pins the newer public source without changing every other dependency. Remove it after updating to an upstream baseline that includes this or a newer reviewed version.

Source: https://github.com/libarchive/libarchive/releases/tag/v3.8.11
