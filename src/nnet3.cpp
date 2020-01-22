
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "lat/confidence.h"
#include "util/kaldi-thread.h"
#include "nnet3/nnet-utils.h"
#include "decoder/grammar-fst.h"
#include "util/kaldi-table.h"
#include "lat/sausages.h"

#include "nnet3.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <string>
#include <numeric>

namespace kaldi {

    // Model wrapper
    NNet3OnlineModelWrapper::NNet3OnlineModelWrapper(
      std::string &nnet3_rxfilename,
      std::string &fst_rxfilename,
      std::string &word_syms_filename
    )
    {
        using namespace kaldi;
        using namespace fst;

        typedef kaldi::int32 int32;
        typedef kaldi::int64 int64;

        KALDI_LOG << "Creating model...";

        // Options and configuration
        KALDI_LOG << "Loading feature configuration...";

        // Apply options
        feature_config.mfcc_config                  = "/lm/online/conf/mfcc.conf";
        feature_config.ivector_extraction_config    = "/lm/online/conf/ivector_extractor.conf";

        // One of these breaks stuff
        // feature_config.feature_type                 = "mfcc";
        // feature_config.cmvn_config                  = "/lm/online/conf/online_cmvn.conf";
        // feature_config.global_cmvn_stats_rxfilename = "/lm/online/ivector_extractor/global_cmvn.stats";

        KALDI_LOG << "Loading decodable configuration...";

        decodable_opts.frame_subsampling_factor = 3;
        decodable_opts.acoustic_scale           = 0.83;
        decodable_opts.frames_per_chunk         = 250;

        // Create acoustic model
        KALDI_LOG << "Loading acoustic model: " << nnet3_rxfilename;
        {
          bool binary;
          Input ki(nnet3_rxfilename, &binary);
          trans_model.Read(ki.Stream(), binary);
          am_nnet.Read(ki.Stream(), binary);
          SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
          SetDropoutTestMode(true, &(am_nnet.GetNnet()));
          nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
        }

        // this object contains precomputed stuff that is used by all decodable
        // objects.  It takes a pointer to am_nnet because if it has iVectors it has
        // to modify the nnet to accept iVectors at intervals.
        // nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts, &am_nnet);

        // Create GrammarFST
        KALDI_LOG << "Loading GrammarFST: " << fst_rxfilename;
        ReadKaldiObject(fst_rxfilename, &decode_fst);

        // Read words symbol table
        KALDI_LOG << "Loading word symbols table: " << word_syms_filename;
        if (!word_syms_filename.empty())
          if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
            KALDI_ERR << "Could not read symbol table from file "
                      << word_syms_filename;

        KALDI_LOG << "Loaded " << word_syms->NumSymbols();

        // Create feature pipeline
        KALDI_LOG << "Loading feature pipeline info...";
        feature_info = new OnlineNnet2FeaturePipelineInfo(this->feature_config);

        KALDI_LOG << "Model loaded";
    }

    NNet3OnlineModelWrapper::~NNet3OnlineModelWrapper() {
        delete &feature_info;
        delete &decode_fst;
    }


    WordResult::WordResult (std::string word, BaseFloat confidence) {
      word_ = word;
      confidence_ = confidence;
    }

    std::string WordResult::getWord() {
      return word_;
    }

    BaseFloat WordResult::getConfidence() {
      return confidence_;
    }

    /*
     * NNet3OnlineDecoderWrapper
     */

    NNet3OnlineDecoderWrapper::NNet3OnlineDecoderWrapper(NNet3OnlineModelWrapper *aModel) : model(aModel) {

        KALDI_LOG << "Creating decoder...";

        decoder            = nullptr;
        // silence_weighting  = nullptr;
        feature_pipeline   = nullptr;
        // adaptation_state   = nullptr;

        decodable_info = new nnet3::DecodableNnetSimpleLoopedInfo(model->decodable_opts, &model->am_nnet);

        // Decoding
        decoder_opts.beam = 16.0;
        decoder_opts.lattice_beam = 5.0;
        decoder_opts.max_active = 2000;
        // decoder_opts.min_active = 10;

        // Endpointing
        endpoint_opts.silence_phones = "1:2:3:4:5:6:7:8:9:10:11:12:13:14:15";

        OnlineEndpointRule rule1_opts;
        rule1_opts.max_relative_cost=10000000000.0;
        rule1_opts.min_trailing_silence=4.0;
        rule1_opts.min_utterance_length=0.0;
        rule1_opts.must_contain_nonsilence=false;
        endpoint_opts.rule1 = rule1_opts;

        OnlineEndpointRule rule2_opts;
        rule2_opts.max_relative_cost=5.5;
        rule2_opts.min_trailing_silence=0.7;
        rule2_opts.min_utterance_length=0.0;
        rule2_opts.must_contain_nonsilence=true;
        endpoint_opts.rule1 = rule2_opts;

        OnlineEndpointRule rule3_opts;
        rule3_opts.max_relative_cost=8.0;
        rule3_opts.min_trailing_silence=1.0;
        rule3_opts.min_utterance_length=0.0;
        rule3_opts.must_contain_nonsilence=true;
        endpoint_opts.rule1 = rule3_opts;

        OnlineEndpointRule rule4_opts;
        rule4_opts.max_relative_cost=2.5;
        rule4_opts.min_trailing_silence=0.5;
        rule4_opts.min_utterance_length=0.0;
        rule4_opts.must_contain_nonsilence=true;
        endpoint_opts.rule1 = rule4_opts;

        OnlineEndpointRule rule5_opts;
        rule5_opts.max_relative_cost=10000000000.0;
        rule5_opts.min_trailing_silence=0.0;
        rule5_opts.min_utterance_length=10.0;
        rule5_opts.must_contain_nonsilence=true;
        endpoint_opts.rule1 = rule5_opts;

        // // Create MBR
        KALDI_LOG << "Loading MBR...";
        MinimumBayesRiskOptions mbr_opts;

        KALDI_LOG << "alloc: OnlineIvectorExtractorAdaptationState";

        adaptation_state  = new OnlineIvectorExtractorAdaptationState (model->feature_info->ivector_extractor_info);

        KALDI_LOG << "alloc: OnlineSilenceWeighting";

        silence_weighting = new OnlineSilenceWeighting (model->trans_model,
                                                        model->feature_info->silence_weighting_config,
                                                        model->decodable_opts.frame_subsampling_factor);

    }

    NNet3OnlineDecoderWrapper::~NNet3OnlineDecoderWrapper() {
        free_decoder();
        delete silence_weighting;
        delete adaptation_state;
        delete decodable_info;
    }

    void NNet3OnlineDecoderWrapper::start_decoding(void) {

        // KALDI_LOG << "start_decoding..." ;
        // KALDI_LOG << "max_active  :" << decoder_opts.max_active;
        // KALDI_LOG << "min_active  :" << decoder_opts.min_active;
        // KALDI_LOG << "beam        :" << decoder_opts.beam;
        // KALDI_LOG << "lattice_beam:" << decoder_opts.lattice_beam;

        free_decoder();

        t = clock();

        // KALDI_LOG << "alloc: OnlineNnet2FeaturePipeline";

        feature_pipeline = new OnlineNnet2FeaturePipeline(*model->feature_info);
        feature_pipeline->SetAdaptationState(*adaptation_state);

        // KALDI_LOG << "alloc: SingleUtteranceNnet3DecoderTpl";

        decoder = new SingleUtteranceNnet3DecoderTpl<fst::GrammarFst>(
          decoder_opts,
          model->trans_model,
          *decodable_info,
          model->decode_fst,
          feature_pipeline
        );

        decoder->InitDecoding(0);

        // KALDI_LOG << "start_decoding...done" ;
    }

    void NNet3OnlineDecoderWrapper::free_decoder(void) {
        // KALDI_LOG << "free_decoder";
        delete decoder ;
        decoder = nullptr;
        delete feature_pipeline ;
        feature_pipeline = nullptr;
    }

    // Returns:
    // false if no endpoint detected
    // true if endpoint detected
    bool NNet3OnlineDecoderWrapper::chunk(BaseFloat samp_freq, Vector<BaseFloat> &wave_part) {

        if (!decoder) {
            start_decoding();
        }

        // KALDI_LOG << "AcceptWaveform [sample_rate: " << samp_freq << "]";
        feature_pipeline->AcceptWaveform(samp_freq, wave_part);

        // KALDI_LOG << "Wave accepted";
        if (silence_weighting->Active() && feature_pipeline->IvectorFeature()) {
            silence_weighting->ComputeCurrentTraceback(decoder->Decoder());
            silence_weighting->GetDeltaWeights(feature_pipeline->NumFramesReady(),
                                               &delta_weights);
            feature_pipeline->IvectorFeature()->UpdateFrameWeights(delta_weights);
        }

        // KALDI_LOG << "Advancing decoding...";
        decoder->AdvanceDecoding();
        // KALDI_LOG << "Decoding advanced";

        if (decoder->EndpointDetected(endpoint_opts)) {
          // KALDI_LOG << "Endpoint detected!";
          return true;
        }

        return false;
    }

    void NNet3OnlineDecoderWrapper::get_result(std::string &text, double &likelihood, double &time_taken, std::vector<WordResult> &word_confidences) {

        feature_pipeline->InputFinished();

        decoder->AdvanceDecoding();
        decoder->FinalizeDecoding();

        // KALDI_LOG << "Frames decoded: " << decoder->NumFramesDecoded();

        if (decoder->NumFramesDecoded() > 0) {

          // KALDI_LOG << "Getting lattice";
          CompactLattice lat;
          decoder->GetLattice(true, &lat);

          // WriteCompactLattice(std::cout, false, lat);

          // Text
          text = LatticeToString(lat, *model->word_syms);

          // // Confidences
          MinimumBayesRisk *mbr = nullptr;
          mbr = new MinimumBayesRisk(lat, mbr_opts);

          const int decimals = 5;
          const std::vector<BaseFloat> &conf                         = mbr->GetOneBestConfidences();
          const std::vector<int32> &words                            = mbr->GetOneBest();

          // const BaseFloat &minBayesRisk                              = 1 - exp( mbr->GetBayesRisk() * -1 );

          // KALDI_LOG << "Got words " << words.size();
          // KALDI_LOG << "Got conf " << conf.size();

          // const std::vector<std::pair<BaseFloat, BaseFloat> > &times = mbr->GetOneBestTimes();
          // const BaseFloat &minBayesRisk                              = 1 - exp( mbr->GetBayesRisk() * -1 );

          // KALDI_ASSERT(conf.size() == words.size() && words.size() == times.size());
          KALDI_ASSERT(conf.size() == words.size());

          // // Sentence confidence
          // int32 num_paths;
          // std::vector<int32> best_sentence, second_best_sentence;
          // BaseFloat sentenceConfidence;
          // sentenceConfidence = SentenceLevelConfidence(lat, &num_paths,
          //   &best_sentence,
          //   &second_best_sentence);

          // KALDI_LOG << "Best: " << best_sentence;
          // KALDI_LOG << "Second: " << second_best_sentence;

          // KALDI_LOG << "Text: " << text;

          // Likelihood
          // likelihood = minBayesRisk; //1 - exp(sentenceConfidence * -1);

          // Time
          t = clock() - t;
          time_taken = ((double)t)/CLOCKS_PER_SEC;
          double totalConfidence = 0;

          for (size_t i = 0; i < words.size(); i++) {
            // Get the word symbol (# -> word)
            const std::string &word = model->word_syms->Find(words[i]);
            const BaseFloat confidence = RoundFloat(conf[i], decimals);

            totalConfidence += confidence;

            kaldi::WordResult res = WordResult(word, confidence);

            word_confidences.push_back(res);
          }

          likelihood = totalConfidence / words.size();

          // KALDI_LOG << "Decoded utterance '" << text << "' in " << time_taken << "s with confidence " << likelihood;
        }

        free_decoder();
    }

    std::string NNet3OnlineDecoderWrapper::LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms) {
      LatticeWeight weight;
      std::vector<int32> alignment;
      std::vector<int32> words;
      GetLinearSymbolSequence(lat, &alignment, &words, &weight);

      std::ostringstream msg;
      for (size_t i = 0; i < words.size(); i++) {
        std::string s = word_syms.Find(words[i]);
        if (s.empty()) {
          KALDI_WARN << "Word-id " << words[i] << " not in symbol table.";
          msg << "<#" << std::to_string(i) << "> ";
        } else
          msg << s << " ";
      }
      return msg.str();
    }

    std::string NNet3OnlineDecoderWrapper::LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms) {
      if (clat.NumStates() == 0) {
        KALDI_WARN << "Empty lattice.";
        return "";
      }
      CompactLattice best_path_clat;
      CompactLatticeShortestPath(clat, &best_path_clat);

      Lattice best_path_lat;
      ConvertLattice(best_path_clat, &best_path_lat);
      return LatticeToString(best_path_lat, word_syms);
    }

    double NNet3OnlineDecoderWrapper::RoundFloat(BaseFloat number, int decimals) {
      return round(number * pow(10, decimals)) / pow(10, decimals);
    }
}