{
  "targets": [
    {
      "target_name": "kaldi",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [
        "./src/nnet3.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "/opt/kaldi/src"
      ],
      "ldflags": [
        "-rdynamic",
        "-ldl",
        "-lm",
        "-lpthread"
      ],
      "cflags_cc": [
        "-isystem /opt/kaldi/tools/openfst-1.6.7/include",
        "-std=c++11",
        "-Wall",
        "-Wno-sign-compare",
        "-Wno-unused-local-typedefs",
        "-Wno-deprecated-declarations",
        "-Winit-self",
        "-DKALDI_DOUBLEPRECISION=0",
        "-DHAVE_EXECINFO_H=1",
        "-DHAVE_CXXABI_H",
        "-DHAVE_ATLAS",
        "-m64",
        "-msse",
        "-msse2",
        "-pthread",
        "-g",
        "-frtti"
      ],
      "library_dirs": [
        "/opt/kaldi/src/lib"
      ],
      "libraries": [
        "-latlas",
        "-lfst",
        "/opt/kaldi/src/online2/libkaldi-online2.so",
        "/opt/kaldi/src/ivector/libkaldi-ivector.so",
        "/opt/kaldi/src/nnet3/libkaldi-nnet3.so",
        "/opt/kaldi/src/chain/libkaldi-chain.so",
        "/opt/kaldi/src/nnet2/libkaldi-nnet2.so",
        "/opt/kaldi/src/cudamatrix/libkaldi-cudamatrix.so",
        "/opt/kaldi/src/decoder/libkaldi-decoder.so",
        "/opt/kaldi/src/lat/libkaldi-lat.so",
        "/opt/kaldi/src/fstext/libkaldi-fstext.so",
        "/opt/kaldi/src/hmm/libkaldi-hmm.so",
        "/opt/kaldi/src/feat/libkaldi-feat.so",
        "/opt/kaldi/src/transform/libkaldi-transform.so",
        "/opt/kaldi/src/gmm/libkaldi-gmm.so",
        "/opt/kaldi/src/tree/libkaldi-tree.so",
        "/opt/kaldi/src/util/libkaldi-util.so",
        "/opt/kaldi/src/matrix/libkaldi-matrix.so",
        "/opt/kaldi/src/base/libkaldi-base.so",
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
