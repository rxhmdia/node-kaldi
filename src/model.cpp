#include "model.h"

Napi::FunctionReference Model::constructor;

Napi::Object Model::Init(Napi::Env env, Napi::Object exports) {

	Napi::HandleScope scope(env);

	// Model
	Napi::Function func = DefineClass(env, "Model", {

	});

	constructor = Napi::Persistent(func);
	constructor.SuppressDestruct();

	exports.Set("Model", func);

	return exports;
}

Model::Model(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Model>(info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	std::string model_file = "/lm/model/final.mdl";
	std::string graph_file = "/lm/graph/HCLG.fst";
	std::string words_file = "/lm/graph/words.txt";

	this->model_ = new kaldi::NNet3OnlineModelWrapper(model_file, graph_file, words_file);
}

kaldi::NNet3OnlineModelWrapper* Model::GetInternalInstance() {
	return this->model_;
}