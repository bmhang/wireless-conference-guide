
// ----- WAVE File ------ //

// source: https://github.com/nanoant/mjpeg/blob/master/riff.h
#define WAVE_FORMAT_PCM			0x0001 /* Microsoft Corporation */
#define WAVE_FORMAT_ALAW		0x0006 /* Microsoft Corporation */
#define WAVE_FORMAT_MULAW		0x0007 /* Microsoft Corporation */

// source: https://ccrma.stanford.edu/courses/422/projects/WAVEFormat/
typedef struct WAVE_filehdr_s
{
	char	chunk_id[4];		// 0x52494646 = "RIFF"
	int32_t	chunk_size;
	char	format[4];		// 0x57415645 = "WAVE"
	char	subchunk1_id[4];	// 0x666d74 = "fmt"
	int32_t	subchunk1_size;
	int16_t	audio_format;		// PCM = 1 (i.e. Linear quantization) and other values indicate some form of compression.
	int16_t	channels;		// Mono = 1, Stereo = 2, etc.
	int32_t	sampleRate;		// sampleRate denotes the sampling rate.
	int32_t	byte_rate;		// sampleRate * NumChannels * BitsPerSample/8
	int16_t	block_align;		// The number of bytes for one sample and from all channels
	int16_t	bits_per_sample;	// 8 bits = 8, 16 bits = 16, etc
	char	subchunk2_id[4];	// 0x64617461 = "data"
	int32_t	subchunk2_size;
} WAVE_filehdr_t;
