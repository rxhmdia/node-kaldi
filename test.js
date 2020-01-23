const Kaldi	= require('./build/Release/kaldi.node')
const File	= require('fs')
const Path	= require('path')
const Wav	= require('wavefile').WaveFile;
const Glob	= require('glob')

const testsGlob = Path.resolve(process.cwd(), process.argv[2])

const m = new Kaldi.OnlineNNet3Model({
	model: '/lm/model/final.mdl',
	graph: '/lm/graph/HCLG.fst',
	words: '/lm/graph/words.txt',
	mfcc_config: '/lm/online/conf/mfcc.conf',
	ivector_extraction_config: '/lm/online/conf/ivector_extractor.conf',
	global_cmvn_stats: '/lm/online/ivector_extractor/global_cmvn.stats',
	frame_subsampling_factor: 3,
	acoustic_scale: 0.83,
	frames_per_chunk: 250
})
let d = new Kaldi.OnlineNNet3GrammarDecoder(m, {
	beam: 16.0,
	lattice_beam: 5.0,
	max_active: 2500,
	min_active: 100,
	endpointing: {
		silence_phones: '1:2:3:4:5:6:7:8:9:10:11:12:13:14:15'
	}
})

Glob(testsGlob, (err, files) => {

	files.forEach(file => {
		console.log('=====================================================')
		console.log(file)
		const wavFile = new Wav(File.readFileSync(file))
		// wavFile.toSampleRate(16000, {method: "sinc"});
		// wavFile.toBitDepth("16");
		const samples = wavFile.getSamples(false, Float32Array)

		const endpointDetected = d.pushChunk(16000.0, samples.length, samples)

		if ( endpointDetected ) console.log('>>>>>>>>>>>>>>>>>>>> ENDPOINT')

		const res = d.getResult()

		// console.dir(res, { depth: null })

		console.log(res.text.replace(/#nonterm[^ ]+/gi, '').replace(/ +/ig, ' ').trim(), res.likelihood, res.seconds)
	})

	delete d
	d = null

})