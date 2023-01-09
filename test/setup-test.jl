depot = get(ENV, "JULIA_DEPOT_PATH", "")
if endswith(depot, "test-depot")
  artifacts = joinpath(depot, "artifacts")
  mkpath(artifacts)
  open(joinpath(artifacts, "Overrides.toml"), "w") do f
    println(f, "[3eaa8342-bff7-56a5-9981-c04077f7cee7]")
    println(f, "libcxxwrap_julia = \"@CXXWRAP_ROOT@\"")
  end
end
envdir = mktempdir()
import Pkg
Pkg.activate(envdir)
Pkg.add("CxxWrap")
using CxxWrap
