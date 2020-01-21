#include <napi.h>
#include "model.h"
#include "decoder.h"

// apt install libatlas-dev libatlas-base-dev libfst-dev

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	Model::Init(env, exports);
	Decoder::Init(env, exports);
	return exports;
};

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)