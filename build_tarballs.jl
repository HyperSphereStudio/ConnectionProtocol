using BinaryBuilder, Pkg

name = "libsimplecommunicationencoder"
version = v"1.0.0"

path = "/workspace/ConnectionProtocol/src"
sources = [DirectorySource(path)]

script = raw"""
cd ${WORKSPACE}/srcdir
cmake -DCMAKE_INSTALL_PREFIX=${prefix} -DBINARY_DIR=${WORKSPACE}/destdir/bin -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel ${nproc}
cmake --install .
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
