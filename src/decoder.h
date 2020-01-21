#include <napi.h>
#include "model.h"
#include "nnet3.h"

class Decoder : public Napi::ObjectWrap<Decoder> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		Decoder(const Napi::CallbackInfo& info);
		Napi::Value PushChunk(const Napi::CallbackInfo& info);
		Napi::Value GetResult(const Napi::CallbackInfo& info);

	private:
		static Napi::FunctionReference constructor;
		Model *aModel;
		kaldi::NNet3OnlineDecoderWrapper *decoder_;
};