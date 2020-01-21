#include "decoder.h"

Napi::FunctionReference Decoder::constructor;

Napi::Object Decoder::Init(Napi::Env env, Napi::Object exports) {

	Napi::HandleScope scope(env);

	// Decoder
	Napi::Function func = DefineClass(env, "Decoder", {
		InstanceMethod("pushChunk", &Decoder::PushChunk),
		InstanceMethod("getResult", &Decoder::GetResult)
	});

	constructor = Napi::Persistent(func);
	constructor.SuppressDestruct();

	exports.Set("Decoder", func);

	return exports;
}

Decoder::Decoder(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Decoder>(info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	Model* model = Napi::ObjectWrap<Model>::Unwrap( info[0].As<Napi::Object>() );

	this->decoder_ = new kaldi::NNet3OnlineDecoderWrapper( model->GetInternalInstance() );
}

Napi::Value Decoder::PushChunk(const Napi::CallbackInfo& info) {
	using namespace kaldi;
	using fst::VectorFst;

	Napi::Env env = info.Env();
  	Napi::HandleScope scope(env);

  	const BaseFloat sample_rate = info[0].As<Napi::Number>().FloatValue();
  	const int32 num_frames = info[1].As<Napi::Number>().Uint32Value();
  	const Napi::Float32Array frames = info[2].As<Napi::Float32Array>();

	Vector<BaseFloat> buf;

	buf.Resize(static_cast<MatrixIndexT>(num_frames));

	for (int i = 0; i < num_frames; i++) {
		buf(i) = static_cast<BaseFloat>(frames[i]);
	}

  	bool endpoint_detected = this->decoder_->chunk(sample_rate, buf);

	return Napi::Boolean::New(env, endpoint_detected);
}

Napi::Value Decoder::GetResult(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	using kaldi::WordResult;

	std::string text = "";
	double likelihood = 1.0;
	double time_taken = 0.0;
	std::vector<WordResult> word_confidences;

	this->decoder_->get_result(text, likelihood, time_taken, word_confidences);

	Napi::Object response = Napi::Object::New(env);

	response.Set("text", text);
	response.Set("likelihood", likelihood);
	response.Set("time", time_taken);

	Napi::Array words = Napi::Array::New(env, word_confidences.size());

	for (size_t i = 0; i < word_confidences.size(); i++) {
		std::string word = word_confidences[i].getWord();
		double confidence = word_confidences[i].getConfidence();

		Napi::Object item = Napi::Object::New(env);
		item.Set("word", word);
		item.Set("confidence", confidence);

		words[i] = item;
	}

	response.Set("words", words);

	return response;
}
