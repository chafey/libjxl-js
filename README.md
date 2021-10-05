# libjxl-js
JS/WASM build of libjxl (JPEG-XL)

## Try It Out!

Try it in your browser [here](https://chafey.github.io/libjxl-js/test/browser/index.html)

## Building

This project uses Docker to provide a consistent developer environment.

Create docker container 'jxlbuild'

> Docker/build.sh

Create shell inside jxlbuild container:

> Docker/docker.sh

This project uses git submodules to pull in libjxl.  If developing, initialize the git submodules first (do this outside the container):

```
> $ git submodule update --init --recursive
```

To build WASM:

```
> ./build.sh
```

To build native C/C++ version:
```
> ./build-native.sh
```