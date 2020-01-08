# JlQML

This is the C++ library component of the [QML.jl](https://github.com/barche/QML.jl) package.

To compile this, make sure that Qt, libcxxwrap-julia and Julia can be found by adding the relevant paths to `CMAKE_PREFIX_PATH`.

Example sequence of commands to download the code and build it:

```bash
git clone https://github.com/barche/jlqml.git
mkdir jlqml-build
cd jlqml-build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/cxxwrap ../jlqml
make
```

The path for CxxWrap can be obtained by running the following in Julia:

```julia
using CxxWrap
dirname(dirname(CxxWrap.libcxxwrap_julia))
```

After building jlqml, add QML.jl in Julia with the environment variable `JLQML_DIR` set to the full build dir path:

```julia
ENV["JLQML_DIR"] = "/path/to/jlqml-build"
```

Then, in pkg mode (hit `]`):

```
add QML#master
```

See the [QML.jl](https://github.com/barche/QML.jl) README for more info on using the QML.jl package.

## Qt compilation

Using Qt packages from the Linux distribution is easiest, but if you need to compile Qt itself from source, the following configuration command should result in a Qt build that is compatible with QML.jl:

```bash
../qt5/configure -L $prefix/lib -I $prefix/include -prefix $prefix -opensource -confirm-license -skip qtactiveqt -skip qtandroidextras -skip qtcanvas3d -skip qtconnectivity -skip qtdatavis3d -skip qtdoc -skip qtgamepad -skip qtnetworkauth -skip qtpurchasing -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin -skip qtwebsockets -skip qtwebview -skip qtwinextras -release
```