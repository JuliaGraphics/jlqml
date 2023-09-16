depot = get(ENV, "JULIA_DEPOT_PATH", "")

cxxwrap_root = "@CXXWRAP_ROOT@"

if endswith(depot, "test-depot")
  artifacts = joinpath(depot, "artifacts")
  mkpath(artifacts)
  open(joinpath(artifacts, "Overrides.toml"), "w") do f
    println(f, "[3eaa8342-bff7-56a5-9981-c04077f7cee7]")
    println(f, "libcxxwrap_julia = \"$cxxwrap_root\"")
  end
end

import Downloads, TOML

function libcxxwrap_jll_version()
  versions_toml_uri = "https://raw.githubusercontent.com/JuliaRegistries/General/master/L/libcxxwrap_julia_jll/Versions.toml"
  buf = IOBuffer()
  Downloads.download(versions_toml_uri, buf)
  seekstart(buf)
  versions = TOML.parse(buf)
  lastversion = sort(collect(keys(versions)))[end]
  baseversion = split(lastversion, '+')[1]
  return VersionNumber(baseversion)
end

envdir = mktempdir()
import Pkg
Pkg.activate(envdir)
Pkg.add("CxxWrap")
using CxxWrap
