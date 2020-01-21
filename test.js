const Kaldi	= require('./build/Release/kaldi.node')
const File	= require('fs')
const Wav	= require('wavefile').WaveFile;
const Glob	= require('glob')

const m = new Kaldi.Model()
console.log(m)

const d = new Kaldi.Decoder(m)
console.log(d)

Glob(process.argv[2], (err, files) => {
	files.forEach(file => {
		console.log('=====================================================')
		console.log(file)
		const wavFile = new Wav(File.readFileSync(file))

		// wavFile.toSampleRate(16000, {method: "sinc"});
		// wavFile.toBitDepth("16");
		const samples = wavFile.getSamples(false, Float32Array)
		// console.log(samples)
		// console.log(typeof samples);
		const endpointDetected = d.pushChunk(16000.0, samples.length, samples)
		if ( endpointDetected ) console.log('>>>>>>>>>>>>>>>>>>>> ENDPOINT')
		const res = d.getResult()
		console.log(res.text.replace(/#nonterm[^ ]+/gi, '').replace(/ +/g, ' ').trim(), res.likelihood)
	})
})

module.exports = Kaldi