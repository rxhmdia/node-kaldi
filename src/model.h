#ifndef INC_MODEL
#define INC_MODEL

#include <napi.h>
#include "nnet3.h"

class Model : public Napi::ObjectWrap<Model> {
	public:
		static Napi::Object Init(Napi::Env env, Napi::Object exports);
		Model(const Napi::CallbackInfo& info);
		kaldi::NNet3OnlineModelWrapper* GetInternalInstance();

	private:
		static Napi::FunctionReference constructor;
		kaldi::NNet3OnlineModelWrapper *model_;
};

#endif /* INC_MODEL */