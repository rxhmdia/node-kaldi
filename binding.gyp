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
        "/usr/lib"
      ],
      "libraries": [
        "-latlas",
        '-lfst',
        "/usr/lib/libkaldi-online2.so",
        "/usr/lib/libkaldi-ivector.so",
        "/usr/lib/libkaldi-nnet3.so",
        "/usr/lib/libkaldi-chain.so",
        "/usr/lib/libkaldi-nnet2.so",
        "/usr/lib/libkaldi-cudamatrix.so",
        "/usr/lib/libkaldi-decoder.so",
        "/usr/lib/libkaldi-lat.so",
        "/usr/lib/libkaldi-fstext.so",
        "/usr/lib/libkaldi-hmm.so",
        "/usr/lib/libkaldi-feat.so",
        "/usr/lib/libkaldi-transform.so",
        "/usr/lib/libkaldi-gmm.so",
        "/usr/lib/libkaldi-tree.so",
        "/usr/lib/libkaldi-util.so",
        "/usr/lib/libkaldi-matrix.so",
        "/usr/lib/libkaldi-base.so",
      ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
