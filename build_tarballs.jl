using BinaryBuilder, Pkg

name = "libsimplecommunicationencoder"
version = v"1.0.0"

path = "/workspace/ConnectionProtocol/src"
sources = [DirectorySource(path)]

script = raw"""
cd ${WORKSPACE}/srcdir
meson --cross-file="${MESON_TARGET_TOOLCHAIN}" --buildtype=release
"""

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line
platforms = [Platform("x86_64", "windows"; )]

println("Selected Platforms [no-apple]:", platforms)

# The products that we will ensure are always built
products = [
    LibraryProduct("libsimplecommunicationencoder", :libsimplecommunicationencoder)
]

x11_platforms = filter(p -> Sys.islinux(p) || Sys.isfreebsd(p), platforms)

# Dependencies that must be installed before this package can be built
dependencies = []

# Build the tarballs, and possibly a `build.jl` as well.
build_tarballs(ARGS, name, version, sources, script, platforms, products, dependencies)
