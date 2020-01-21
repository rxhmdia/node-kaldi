#ifndef INC_NNET3
#define INC_NNET3

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

namespace kaldi {
    class NNet3OnlineModelWrapper {
    friend class NNet3OnlineDecoderWrapper;
    public:

        NNet3OnlineModelWrapper(
              std::string &nnet3_rxfilename,
              std::string &fst_rxfilename,
              std::string &word_syms_filename
        );
        ~NNet3OnlineModelWrapper();

    private:

        fst::SymbolTable                          *word_syms;

        OnlineNnet2FeaturePipelineConfig           feature_config;

        OnlineNnet2FeaturePipelineInfo            *feature_info;

        nnet3::AmNnetSimple                        am_nnet;
        nnet3::NnetSimpleLoopedComputationOptions  decodable_opts;

        TransitionModel                            trans_model;
        fst::GrammarFst                            decode_fst;
        // std::string                               *ie_conf_filename;

        // std::vector<std::vector<int32> >           word_alignment_lexicon;
    };


    class WordResult {
    public:
        WordResult(
            std::string word,
            BaseFloat confidence
        );

        std::string getWord();
        BaseFloat getConfidence();

    private:
        std::string word_;
        BaseFloat confidence_;
    };

    class NNet3OnlineDecoderWrapper {
    public:

        NNet3OnlineDecoderWrapper(
            NNet3OnlineModelWrapper *aModel
        );
        ~NNet3OnlineDecoderWrapper();

        bool chunk(BaseFloat samp_freq, Vector<BaseFloat> &wave_part);

        void get_result(std::string &text, double &likelihood, double &time, std::vector<WordResult> &word_confidences);
        // bool               get_word_alignment(std::vector<string> &words,
        //                                       std::vector<int32>  &times,
        //                                       std::vector<int32>  &lengths);

    private:

        void start_decoding(void);
        void free_decoder(void);
        std::string LatticeToString(const Lattice &lat, const fst::SymbolTable &word_syms);
        std::string LatticeToString(const CompactLattice &clat, const fst::SymbolTable &word_syms);
        double RoundFloat(BaseFloat number, int decimals);

        clock_t t;

        LatticeFasterDecoderConfig                      decoder_opts;
        OnlineEndpointConfig                            endpoint_opts;

        OnlineNnet2FeaturePipelineConfig           feature_config;

        OnlineNnet2FeaturePipelineInfo            *feature_info;
        OnlineNnet2FeaturePipeline                      *feature_pipeline;
        nnet3::DecodableNnetSimpleLoopedInfo            *decodable_info;

        NNet3OnlineModelWrapper                         *model;

        MinimumBayesRiskOptions                         mbr_opts;

        OnlineIvectorExtractorAdaptationState     *adaptation_state;
        OnlineSilenceWeighting                    *silence_weighting;
        SingleUtteranceNnet3DecoderTpl<fst::GrammarFst> *decoder;

        std::vector<std::pair<int32, BaseFloat> >  delta_weights;
        // int32                                      tot_frames, tot_frames_decoded;

        // decoding result:
        // CompactLattice                             best_path_clat;

    };
}

#endif /* INC_NNET3 */