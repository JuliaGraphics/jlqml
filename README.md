# JlQML

This is the C++ library component of the [QML.jl](https://github.com/barche/QML.jl) package.

To compile this, make sure that Qt, libcxxwrap-julia and Julia can be found by adding the relevant paths to `CMAKE_PREFIX_PATH`. You also need to build `libcxxwrap-julia` from source, using the instructions at: https://github.com/JuliaInterop/libcxxwrap-julia

Example sequence of commands to download the code and build it:

```bash
git clone https://github.com/barche/jlqml.git
mkdir jlqml-build
cd jlqml-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/cxxwrap;/path/to/qt ../jlqml
make
```

The path for CxxWrap can be obtained by running the following in Julia:

```julia
using CxxWrap
dirname(dirname(CxxWrap.libcxxwrap_julia))
```

After building jlqml, you also need to set up the `~/.julia/artifacts/Overrides.toml`, to prefer your locally built binaries over the standard binaries, for example:

```toml
[3eaa8342-bff7-56a5-9981-c04077f7cee7]
libcxxwrap_julia = "/home/user/src/build/libcxxwrap-julia"

[6b5019fb-a83d-5b4e-a9f7-678a36c28df7]
jlqml = "/home/user/src/build/jlqml"

[ea2cea3b-5b76-57ae-a6ef-0a8af62496e1]
Qt5Base = "/usr"

[c6373c32-5b88-5913-90f5-31d7686b42da]
Qt5Declarative = "/usr"

[3af4ccab-a251-578e-a514-ea85a0ba79ee]
Qt5Svg = "/usr"

[e4aecf45-a397-53cc-864f-87db395e0248]
Qt5QuickControls = "/usr"

[bf3ac11c-603e-589e-b4b7-e696ac65aa4a]
Qt5QuickControls2 = "/usr"
```

Then, in pkg mode (hit `]`):

```
add jlqml_jll
```

See the [QML.jl](https://github.com/barche/QML.jl) documentation for more info on using the QML.jl package.
