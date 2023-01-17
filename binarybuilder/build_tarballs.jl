# Adapted from the Yggdrasil build script
# Note that this script can accept some limited command-line arguments, run
# `julia build_tarballs.jl --help` to see a usage message.
using BinaryBuilder, Pkg, TOML

# See https://github.com/JuliaLang/Pkg.jl/issues/2942
# Once this Pkg issue is resolved, this must be removed
uuid = Base.UUID("a83860b7-747b-57cf-bf1f-3e79990d037f")
delete!(Pkg.Types.get_last_stdlibs(v"1.6.3"), uuid)

GITHUB_REF_NAME = haskey(ENV, "GITHUB_REF_NAME") ? ENV["GITHUB_REF_NAME"] : ""

do_deploy(refname) = (refname == "main")

function getversion(cmakefile)
    a = b = c = 0
    for l in readlines(cmakefile)
        if startswith(l, "set(JlQML_VERSION")
            a, b, c = parse.(Int, split(replace(split(l)[end], ")" => ""),'.'))
        end
    end
    return VersionNumber(a, b, c)
end

basepath = dirname(@__DIR__)

name = "jlqml"
version = getversion(joinpath(basepath, "CMakeLists.txt"))

julia_versions = do_deploy(GITHUB_REF_NAME) ? [v"1.6.3", v"1.8.0", v"1.10.0"] : [v"1.8.0"]

# Collection of sources required to complete build
sources = [
    DirectorySource(basepath; target = name)
]

# Bash recipe for building across all platforms
script = raw"""
# Override compiler ID to silence the horrible "No features found" cmake error
if [[ $target == *"apple-darwin"* ]]; then
  macos_extra_flags="-DCMAKE_CXX_COMPILER_ID=AppleClang -DCMAKE_CXX_COMPILER_VERSION=10.0.0 -DCMAKE_CXX_STANDARD_COMPUTED_DEFAULT=11"
fi

mkdir build
cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_FIND_ROOT_PATH=${prefix} \
    -DCMAKE_INSTALL_PREFIX=$prefix \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
    -DJulia_PREFIX=${prefix} \
    $macos_extra_flags \
    ../jlqml/
VERBOSE=ON cmake --build . --config Release --target install -- -j${nproc}
install_license $WORKSPACE/srcdir/jlqml*/LICENSE.md
"""

# These are the platforms we will build for by default, unless further
# platforms are passed in on the command line

function libjulia_platforms(julia_version)
    platforms = supported_platforms(; experimental = julia_version ≥ v"1.7")

    filter!(p -> libc(p) != "musl" && arch(p) != "i686" && !contains(arch(p), "arm") && arch(p) != "aarch64" && arch(p) != "powerpc64le" && !Sys.isfreebsd(p), platforms)

    for p in platforms
        p["julia_version"] = string(julia_version)
    end

    platforms = expand_cxxstring_abis(platforms)

    filter!(p -> cxxstring_abi(p) != "cxx03", platforms)

    return platforms
end

platforms = vcat(libjulia_platforms.(julia_versions)...)
if !do_deploy(GITHUB_REF_NAME)
    filter!(Sys.islinux, platforms)
end

# The products that we will ensure are always built
products = [
    LibraryProduct("libjlqml", :libjlqml),
]

# Dependencies that must be installed before this package can be built
dependencies = [
    Dependency("libcxxwrap_julia_jll"),
    Dependency("Qt6Base_jll", v"6.3.0"; compat="6.3.0"),
    Dependency("Qt6ShaderTools_jll", v"6.3.0"; compat="6.3.0"),
    Dependency("Qt6Declarative_jll"),
    HostBuildDependency("Qt6Declarative_jll"),
    Dependency("Qt6Svg_jll"),
    BuildDependency("Libglvnd_jll"),
    BuildDependency("libjulia_jll"),
]

GITHUB_REF_NAME = haskey(ENV, "GITHUB_REF_NAME") ? ENV["GITHUB_REF_NAME"] : ""
deployingargs = deepcopy(ARGS)
if !isempty(GITHUB_REF_NAME) && !do_deploy(GITHUB_REF_NAME)
    push!(deployingargs, "--deploy=local")
end

# Build the tarballs, and possibly a `build.jl` as well.
build_tarballs(deployingargs, name, version, sources, script, platforms, products, dependencies;
    preferred_gcc_version = v"9", julia_compat = "1.6")

if do_deploy(GITHUB_REF_NAME)
    const testreg = "https://github.com/barche/CxxWrapTestRegistry.git"

    const repo = "barche/jlqml_jll.jl"

    Pkg.Registry.add(RegistrySpec(url = testreg))
    Pkg.develop(PackageSpec(url = "https://github.com/$(repo).git"))

    jlqml_jll_project_toml = TOML.parsefile(joinpath(Pkg.devdir(), "jlqml_jll", "Project.toml"))
    build_version = parse(VersionNumber, jlqml_jll_project_toml["version"])
    if version == VersionNumber(build_version.major, build_version.minor, build_version.patch)
        build_version = VersionNumber(build_version.major, build_version.minor, build_version.patch, (), build_version.build .+ 1)
    else
        build_version = version
    end

    mktemp() do jsonfile, _
        push!(deployingargs, "--meta-json=$jsonfile")
        build_tarballs(deployingargs, name, version, sources, script, platforms, products, dependencies;
            preferred_gcc_version = v"9", julia_compat = "1.6")

        json = String(read(jsonfile))
        buff = IOBuffer(strip(json))
        objs = []
        while !eof(buff)
            push!(objs, BinaryBuilder.JSON.parse(buff))
        end
        json_obj = objs[1]

        BinaryBuilder.cleanup_merged_object!(json_obj)
        json_obj["dependencies"] = BinaryBuilder.AbstractDependency[dep for dep in json_obj["dependencies"] if !isa(dep, BuildDependency)]

        tag = "$(name)-v$(build_version)"
        upload_prefix = "https://github.com/$(repo)/releases/download/$(tag)"

        download_dir = "products"
        BinaryBuilder.rebuild_jll_package(json_obj; download_dir, upload_prefix, build_version)
        BinaryBuilder.push_jll_package(name, build_version; deploy_repo = repo)
        BinaryBuilder.upload_to_github_releases(repo, tag, download_dir; verbose = true)
    end

    Pkg.develop("jlqml_jll")
    Pkg.add([PackageSpec(name="Qt6Base_jll", version=v"6.3.0"), PackageSpec(name="Qt6ShaderTools_jll", version=v"6.3.0")])
    import jlqml_jll
    using LocalRegistry
    register(jlqml_jll, registry = testreg)
end
