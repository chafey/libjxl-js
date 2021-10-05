docker run -it --rm \
  -v $HOME/src/github/chafey/libjxl-js:/libjxl-js -w /libjxl-js \
  emscripten/emsdk:2.0.31 bash