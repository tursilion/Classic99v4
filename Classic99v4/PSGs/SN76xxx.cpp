// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)
// Tursi's own from-scratch TMS9919/SN76xxx emulation

/*
From Matt - check this decrement:
[2:15 PM] dnotq: The counters don't consider the 0-count as it's own state.  It was very interesting that I literally took the AY up-counters, changed them to count down (changed ++ to -), and changed the reset when count >= period condition to load-period when count = 0, and they just worked.
[2:16 PM] dnotq: It took me a while to realize this, since when you mentioned 0-count is maximum period, that threw me for a bit.
[2:17 PM] tursilion: yeah, I said that. But I think the SN was the first and everyone else cloned them ;)
[2:17 PM] dnotq: But the "compare-to-0 and load" is done during the period of the count.  Easier to show than to explain in English.
[2:18 PM] dnotq: It was just neat to see the same mechanism work in both directions.  Everyone else is loading the count - 1 and crap like that into their counters, or making special cases for the 0 count, etc.
[2:19 PM] tursilion: huh, weird. I don't understand your "during the period of the count" bit, as you implied ;)
[2:21 PM] dnotq: It means that the reset-and-count happens in the same time period as a single count.  So when you hit zero, the period is loaded, then decremented immediately.  So the counter being "zero" is never try for a full count period.
[2:21 PM] dnotq: This is what allows a count of 1 to actually cut the frequency in half.
[2:22 PM] dnotq: If zero was true for a full count cycle, a count of one would be: 1, 0 (toggle), 1, 0 (toggle).  And that is actually a divide by 2.
[2:26 PM] dnotq: It is done in the real IC via the asynchronous nature of the transistors, and the way any signal transition can be a clock, set, or reset signal.  But I reproduced the functionality in synchronous HDL and the counters just work as they should.  No special tests, edge cases, loading or comparing period-1, etc.
[2:27 PM] tursilion: ah, I see. That makes sense :) I should check if Classic99 does that right
*/

// Testing on a real TI:
// The sound chip itself outputs an always positive sound wave peaking roughly at 2.8v.
// Output values are roughly 1v peak-to-peak (I measured 0 attenuation at roughly 720mV)
// However, it's hard to get good readings in the TI console due to lots of high frequency
// noise, which is also roughly 1v peak to peak and starts at around 1khz or 2khz
// The center point at output was closer to 1v, nevertheless, it's all positive.
// Two interesting points - rather than a center point, the maximum voltage appeared
// to be consistent, and the minimum voltage crept higher as attenuation increased.
// The other interesting point is that MESS seems to output all negative voltage,
// but in the end where in the range it lands doesn't matter so much (in fact my
// positive/negative swing probably doesn't even matter, but we can fix it).
// I measured these approximate values. They are less accurate as they get smaller
// because of the high noise (a sound chip out of circuit on an AVR or such
// would be a good experiment). But we can line them up with the approximate
// scale and see if it makes sense:
//
// From 0-15, I read (mV):
// 720, 600, 488, 407, 384, 328, 272, 232,
// 176, 160, 136, 120, 112, 104, 72, 0
//
// Again, measured by hand in a noisy system, so just use as very loose confirmation
// data, rather than anything exact.
//
// As a potential thought -- centering our output on zero instead of making it all positive
// means we play more nicely with the Windows system (ie: no click, no DC offset), and
// so it may be worth leaving it that way. It should still sound the same.
//

// Based on documentation on the SN76489 from SMSPOWER
// and datasheets for the SN79489/SN79486/SN79484,
// and recordings from an actual TI99 console (gasp!)
//
// Decay to zero is roughly 1% of maximum every 500uS
// My math suggests that the volume would thus decay by
// 0.01 every 1431.818 clocks (real clocks, not clock/16)
//
// The clock dividers in this file don't look correct according
// to the datasheets, but till I find more accurate details
// on the 9919 itself I'll leave them be. And they work.

// The 15 bit shift register is more or less simulated here -
// I need to do some measurements on the real TI and see if
// I can figure out from the doc what the real chip is doing --
// by using the user-controlled rate I can set the shift register
// to a desired speed and thus measure it.

// One interesting note from this... for getting "bass" from the
// periodic register, the periodic output is going to be
// ticking at (frequency count / 2) / 15 
// The tick count for tones is 111860.78125Hz (exactly on NTSC - are PAL consoles detuned?)
// The tick count for periodic noise if 15-bit is about 3728.6927
//
// It's a little harder to measure the white noise register, we'd have
// to work out where the tap points are by looking at the pattern.
//
// Note that technically, the datasheet says the chip needs 32 cycles to
// deal with the audio data, but we assume immediate. Also, note that
// the dB attenuation is +/- 1dB, but we assume perfect accuracy. In short,
// the chip is really pretty crummy, but we emulate a perfect one.
// Note anyway that 32 cycles at 3.58MHz is 8.9uS - just /slightly/
// slower than the fastest 9900 write cycle (MOVB Rn,*Rm = 26 cycles = 8.63uS)
// On the later chip that took a 500kHz clock (not used in the TI?)
// only 4 clocks were needed, so it was 8.0uS to respond.
// This means the normally used sound chip can potentially /just barely/
// be overrun by the CPU, but we should be fine with this emulation.
// (In truth, the time it takes to fetch the next instruction should make it
// impossible to overrun.)
//
// Futher note: the TI CPU is halted by the sound chip during the write,
// so not only is overrun impossible, but the system is halted for a significant
// time. This is now emulated on the CPU side.
//
// Another neat tidbit from RL -- don't track the fractional loss from
// the timing counters -- your ear can hear when they are added in, and
// you get periodic noise, depending on the frequency being played!
//
// Nice notes at http://www.smspower.org/maxim/docs/SN76489.txt

// TODO: Speech and SID need to be implemented as separate objects

#include <cstdio>
#include "SN76xxx.h"
#include "../EmulatorSupport/automutex.h"

// logarithmic scale (linear isn't right!)
// the SMS Power example, (I convert below to percentages)
static int sms_volume_table[16]={
       32767, 26028, 20675, 16422, 13045, 10362,  8231,  6538,
        5193,  4125,  3277,  2603,  2067,  1642,  1304,     0
};

// (15 bit)
static const int LFSRReset = 0x4000;

SN76xxx::SN76xxx(Classic99System *theCore)
	: Classic99Peripheral(theCore)
	, Classic99AudioSrc()
	, enableBackgroundHum(false)
    , CRU_TOGGLES(0.0)
    , nDACLevel(0.0)
    , dac_pos(0)
    , dacupdatedistance(0.0)
    , dacramp(0.0)
    , FADECLKTICK(0.001/9.0)
    , nClock(3579545)	// NTSC, PAL may be at 3546893? - this is divided by 16 to tick
	, nCounter()
    , nNoisePos(1)
    , LFSR(LFSRReset)
    , nRegister()
    , nVolume()
    , max_volume(0)
    , AudioSampleRate(44100)
    , nTappedBits(0x0003)
	, latch_byte(0)
	, nVolumeTable()
{
	// special init for noise counter
	nCounter[3] = 1;
	// set outputs to maximum
	for (int idx=0; idx<4; ++idx) {
		nFade[idx] = 1.0;
		nOutput[idx] = 1.0;
	}

	csAudioBuf = al_create_mutex_recursive();
}

SN76xxx::~SN76xxx() {
	cleanup();
}

// we receive a pointer to a buffer to fill with current audio data
// buf - pointer to data - we expect 16-bit mono
// bufSize - size of buf in bytes (for sanity checking)
// samples - number of samples to generate
void SN76xxx::fillAudioBuffer(void *inBuf, int bufSize, int nSamples) {
	// fill the output audio buffer with signed 16-bit values
	// nClock is the input clock frequency, which runs through a divide by 16 counter
	// The frequency does not divide exactly by 16
	// AudioSampleRate is the frequency at which we actually output bytes
	// multiplying values by 1000 to improve accuracy of the final division (so ms instead of sec)
	double nTimePerClock=1000.0/(nClock/16.0);
	double nTimePerSample=1000.0/(double)AudioSampleRate;
	int nClocksPerSample = (int)(nTimePerSample / nTimePerClock + 0.5);		// +0.5 to round up if needed
	int newdacpos = 0;
	int inSamples = nSamples;
	short *buf = (short*)inBuf;

	// sanity check the buffer
	if (nSamples*2 != bufSize) {
		debug_write("Improper buffer size - fail audio");
		return;
	}

	while (nSamples) {
		// emulate drift to zero
		for (int idx=0; idx<4; idx++) {
			if (nFade[idx] > 0.0) {
				nFade[idx]-=FADECLKTICK*nClocksPerSample;
				if (nFade[idx] < 0.0) nFade[idx]=0.0;
			} else {
				nFade[idx]=0.0;
			}
		}

		// tone channels

		for (int idx=0; idx<3; idx++) {
            // Further Testing with the chip that SMS Power's doc covers (SN76489)
            // 0 outputs a 1024 count tone, just like the TI, but 1 DOES output a flat line.
            // On the TI (SN76494, I think), 1 outputs the highest pitch (count of 1)
            // However, my 99/4 pics show THAT machine with an SN76489! 
            // My plank TI has an SN94624 (early name? TMS9919 -> SN94624 -> SN76494 -> SN76489)
            // And my 2.2 QI console has an SN76494!
            // So maybe we can't say with certainty which chip is in which machine?
            // Myths and legends:
            // - SN76489 grows volume from 2.5v down to 0 (matches my old scopes of the 494), but SN76489A grows volume from 0 up.
            // - SN76496 is the same as the SN7689A but adds the Audio In pin (all TI used chips have this, even the older ones)
            // So right now, I believe there are two main versions, differing largely by the behaviour of count 0x001:
            // Original (high frequency): TMS9919, SN94624, SN76494?
            // New (flat line): SN76489, SN76489A, SN76496
			nCounter[idx]-=nClocksPerSample;
			while (nCounter[idx] <= 0) {    // TODO: should be able to do this without a loop, it would be faster (well, in the rare cases it needs to loop)!
				nCounter[idx]+=(nRegister[idx]?nRegister[idx]:0x400);
				nOutput[idx]*=-1.0;
				nFade[idx]=1.0;
			}
			// A little check to eliminate high frequency tones
			// If the frequency is greater than 1/2 the sample rate,
			// then mute it (we'll do that with the nFade value.) 
			// Noises can't get higher than audible frequencies (even with high user defined rates),
			// so we don't need to worry about them.
			if ((nRegister[idx] != 0) && (nRegister[idx] <= (int)(111860.0/(double)(AudioSampleRate/2)))) {
				// this would be too high a frequency, so we'll merge it into the DAC channel (elsewhere)
				// and kill this channel. The reason is that the high frequency ends up
				// creating artifacts with the lower frequency output rate, and you don't
				// get an inaudible tone but awful noise
				nFade[idx]=0.0;
			}
		}

		// noise channel 
		nCounter[3]-=nClocksPerSample;
		while (nCounter[3] <= 0) {
			switch (nRegister[3]&0x03) {
				case 0: nCounter[3]+=0x10; break;
				case 1: nCounter[3]+=0x20; break;
				case 2: nCounter[3]+=0x40; break;
				// even when the count is zero, the noise shift still counts
				// down, so counting down from 0 is the same as wrapping up to 0x400
				// same is with the tone above :)
				case 3: nCounter[3]+=(nRegister[2]?nRegister[2]:0x400); break;		// is never zero!
			}
			nNoisePos*=-1;
			double nOldOut=nOutput[3];
			// Shift register is only kicked when the 
			// Noise output sign goes from negative to positive
			if (nNoisePos > 0) {
				int in=0;
				if (nRegister[3]&0x4) {
					// white noise - actual tapped bits uncertain?
					// This doesn't currently look right.. need to
					// sample a full sequence of TI white noise at
					// a known rate and study the pattern.
					if (parity(LFSR&nTappedBits)) in=0x4000;
					if (LFSR&0x01) {
						// the SMSPower documentation says it never goes negative,
						// but (my very old) recordings say white noise does goes negative,
						// and periodic noise doesn't. Need to sit down and record these
						// noises properly and see what they really do. 
						// For now I am going to swing negative to play nicely with
						// the tone channels. 
						// It's possible the chip does not go negative but the TI's audio
						// amp causes an offset?
						// TODO: I need to verify noise vs tone on a clean system.
						// need to test for 0 because periodic noise sets it
						if (nOutput[3] == 0.0) {
							nOutput[3] = 1.0;
						} else {
							nOutput[3]*=-1.0;
						}
					}
				} else {
					// periodic noise - tap bit 0 (again, BBC Micro)
					// Compared against TI samples, this looks right
					if (LFSR&0x0001) {
						in=0x4000;	// (15 bit shift)
						// TODO: verify periodic noise as well as white noise
						// always positive
						nOutput[3]=1.0;
					} else {
						nOutput[3]=0.0;
					}
				}
				LFSR>>=1;
				LFSR|=in;
			}
			if (nOldOut != nOutput[3]) {
				nFade[3]=1.0;
			}
		}

		// write sample
		nSamples--;
		double output;

		// using division (single voices are quiet!)
		// write out one sample
		output = nOutput[0]*nVolumeTable[nVolume[0]]*nFade[0] +
				nOutput[1]*nVolumeTable[nVolume[1]]*nFade[1] +
				nOutput[2]*nVolumeTable[nVolume[2]]*nFade[2] +
				nOutput[3]*nVolumeTable[nVolume[3]]*nFade[3];
//				+ ((dac_buffer[newdacpos++] / 255.0)*dacramp);
		// output is now between 0.0 and 4.0, may be positive or negative
		// would be 5.0 with the DAC active
		output/=4.0;	// you aren't supposed to do this when mixing. Sorry. :)
#if 0
		// TODO: DAC needs to be tied to the CPU to work
		if (newdacpos >= dac_pos) {
			// not enough DAC samples!
			newdacpos--;
		}
#endif

		short nSample=(short)((double)0x7fff*output); 
		*(buf++)=nSample; 
	}

#if 0
	// roll the dac output buffer
	EnterCriticalSection(&csAudioBuf);
	if (inSamples < dac_pos) {
		memmove(dac_buffer, &dac_buffer[newdacpos], dac_pos-newdacpos);
		dac_pos-=newdacpos;
        if (dacramp < 1.0) {
            dacramp+=0.01;  // slow, slow ramp in
            if (dacramp > 1.0) dacramp = 1.0;
        }
	} else {
		dac_pos = 0;
	}
	LeaveCriticalSection(&csAudioBuf);
#endif
}

void SN76xxx::write(int addr, bool isIO, volatile long &cycles, MEMACCESSTYPE rmw, uint8_t data) {
	(void)addr;
	(void)isIO;

	// 'data' contains the byte currently being written to the sound chip
	// all functions are 1 or 2 bytes long, as follows					
	//
	// BYTE		BIT		PURPOSE											
	//	1		0		always '1' (latch bit)							
	//			1-3		Operation:	000 - tone 1 frequency				
	//								001 - tone 1 volume					
	//								010 - tone 2 frequency				
	//								011 - tone 2 volume					
	//								100 - tone 3 frequency				
	//								101 - tone 3 volume					
	//								110 - noise control					
	//								111 - noise volume					
	//			4-7		Least sig. frequency bits for tone, or volume	
	//					setting (0-F), or type of noise.				
	//					(volume F is off)								
	//					Noise set:	4 - always 0						
	//								5 - 0=periodic noise, 1=white noise 
	//								6-7 - shift rate from table, or 11	
	//									to get rate from voice 3.		
	//	2		0-1		Always '0'. This byte only used for frequency	
	//			2-7		Most sig. frequency bits						
	//
	// Commands are instantaneous

	if (rmw != ACCESS_FREE) {
        // sound chip writes eat ~28 additional cycles (verified hardware, reads do not)
		cycles += 28;
	}

	// Latch anytime the high bit is set
	// This byte still immediately changes the channel
	if (data&0x80) {
		latch_byte=data;
	}

	switch (data&0xf0)									// check command
	{	
	case 0x90:											// Voice 1 vol
	case 0xb0:											// Voice 2 vol
	case 0xd0:											// Voice 3 vol
	case 0xf0:											// Noise volume
		nVolume[(data&0x60)>>5]=data&0xf;
		break;

	case 0xe0:
		nRegister[3]=data&0x07;							// update noise
		resetNoise();
		break;

//	case 0x80:											// Voice 1 frequency
//	case 0xa0:											// Voice 2 frequency
//	case 0xc0:											// Voice 3 frequency
	default:											// Any other byte
		int nChan=(latch_byte&0x60)>>5;
		if (data&0x80) {
			// latch write - least significant bits of a tone register
			// (definately not noise, noise was broken out earlier)
			nRegister[nChan] &= 0xfff0;
			nRegister[nChan] |= data&0x0f;
			// don't update the counters, let them run out on their own
		} else {
			// latch bit clear - data to whatever is latched
			// TODO: re-verify this on hardware, it doesn't agree with the SMS Power doc
			// as far as the volume and noise goes!
			if (latch_byte&0x10) {
				// volume register
				nVolume[nChan]=data&0xf;
			} else if (nChan==3) {
				// noise register
				nRegister[3]=data&0x07;
				resetNoise();
			} else {
				// tone generator - most significant bits
				nRegister[nChan] &= 0xf;
				nRegister[nChan] |= (data&0x3f)<<4;
				// don't update the counters, let them run out on their own
			}
		}
		break;
	}
}

bool SN76xxx::init(int idx) {
	setIndex("SN76xxx", idx);

	// get a stream running
	stream = theCore->getSpeaker()->requestStream(this);
	if ((nullptr != stream) && (nullptr != stream->stream)) {
		al_attach_audio_stream_to_mixer(stream->stream, al_get_default_mixer());
	}

	// init the emulation
	sound_init(AudioSampleRate);	// TODO: need configuration input
	return true;
}

bool SN76xxx::operate(double timestamp) {
	// There's actually nothing to do here - we'll operate based on samples instead
	return true;
}

bool SN76xxx::cleanup() {
	// don't remove the stream, it's an auto
	return true;
}

void SN76xxx::getDebugSize(int &x, int &y) {
	// TODO: it might be nice to make the debug option a bitmap instead of text - then we just render
	// it. This would allow graphics as well as text - for instance the sound could actually display
	// a recent waveform, VDP could show graphical tables... to the same, multiple bitmaps per object
	// would also make sense. But then the user just selects the bitmaps they want, and can move
	// them around as needed.

	// for now, just the registers
	x=11;
	y=2;
}
void SN76xxx::getDebugWindow(char *buffer) {
	// buffer should be sized to (x+2)*y (to allow \r\n endings)
	sprintf(buffer, "%03X %03X %03X %01X\r\n %1X  %1X  %1X  %1X", 
		nRegister[0], nRegister[1], nRegister[2], nRegister[3],
		nVolume[0], nVolume[1], nVolume[2], nVolume[3]);
}

// save and restore state - return size of 0 if no save, and return false if either act fails catastrophically
int SN76xxx::saveStateSize() {
	return 1+8+8+1024*1024+4+8+8+8+4+4*4+4+2+4*4+4*4+8*4+4+4+4+4+8*4;
}
bool SN76xxx::saveState(unsigned char *buffer) {
    saveStateVal(buffer, enableBackgroundHum);
    saveStateVal(buffer, CRU_TOGGLES);
    saveStateVal(buffer, nDACLevel);

    memcpy(buffer, dac_buffer, 1024*1024);

    saveStateVal(buffer, dac_pos);
    saveStateVal(buffer, dacupdatedistance);
    saveStateVal(buffer, dacramp);
    saveStateVal(buffer, FADECLKTICK);
    saveStateVal(buffer, nClock);

    for (int idx=0; idx<4; ++idx) {
	saveStateVal(buffer, nCounter[idx]);
    }

    saveStateVal(buffer, nNoisePos);
    saveStateVal(buffer, LFSR);

    for (int idx=0; idx<4; ++idx) {
	saveStateVal(buffer, nRegister[idx]);
    }
    for (int idx=0; idx<4; ++idx) {
	saveStateVal(buffer, nVolume[idx]);
    }
    for (int idx=0; idx<4; ++idx) {
	saveStateVal(buffer, nFade[idx]);
    }

    saveStateVal(buffer, max_volume);
    saveStateVal(buffer, AudioSampleRate);
    saveStateVal(buffer, nTappedBits);
    saveStateVal(buffer, latch_byte);

    for (int idx=0; idx<4; ++idx) {
	saveStateVal(buffer, nOutput[idx]);
    }

    return true;
}
bool SN76xxx::restoreState(unsigned char *buffer) {
    loadStateVal(buffer, enableBackgroundHum);
    loadStateVal(buffer, CRU_TOGGLES);
    loadStateVal(buffer, nDACLevel);

    memcpy(dac_buffer, buffer, 1024*1024);

    loadStateVal(buffer, dac_pos);
    loadStateVal(buffer, dacupdatedistance);
    loadStateVal(buffer, dacramp);
    loadStateVal(buffer, FADECLKTICK);
    loadStateVal(buffer, nClock);

    for (int idx=0; idx<4; ++idx) {
	loadStateVal(buffer, nCounter[idx]);
    }

    loadStateVal(buffer, nNoisePos);
    loadStateVal(buffer, LFSR);

    for (int idx=0; idx<4; ++idx) {
 	loadStateVal(buffer, nRegister[idx]);
    }
    for (int idx=0; idx<4; ++idx) {
 	loadStateVal(buffer, nVolume[idx]);
    }
    for (int idx=0; idx<4; ++idx) {
	loadStateVal(buffer, nFade[idx]);
    }

    loadStateVal(buffer, max_volume);
    loadStateVal(buffer, AudioSampleRate);
    loadStateVal(buffer, nTappedBits);
    loadStateVal(buffer, latch_byte);

    for (int idx=0; idx<4; ++idx) {
   	loadStateVal(buffer, nOutput[idx]);
    }

    return true;
}

void SN76xxx::resetNoise() {
	// reset shift register
	LFSR=LFSRReset;
	switch (nRegister[3]&0x03) {
		// these values work but check the datasheet dividers
		case 0: nCounter[3]=0x10; break;
		case 1: nCounter[3]=0x20; break;
		case 2: nCounter[3]=0x40; break;
		// even when the count is zero, the noise shift still counts
		// down, so counting down from 0 is the same as wrapping up to 0x400
		case 3: nCounter[3]=(nRegister[2]?nRegister[2]:0x400); break;		// is never zero!
	}
}

// return 1 or 0 depending on odd parity of set bits
// function by Dave aka finaldave. Input value should
// be no more than 16 bits.
int SN76xxx::parity(int val) {
	val^=val>>8;
	val^=val>>4;
	val^=val>>2;
	val^=val>>1;
	return val&1;
};

// prepare the sound emulation. 
// freq - output sound frequency in hz
void SN76xxx::sound_init(int freq) {
	// I don't want max to clip, so I bias it slightly low
	// use the SMS power Logarithmic table
	for (int idx=0; idx<16; idx++) {
		// this magic number makes maximum volume (32767) become about 0.9375
		nVolumeTable[idx]=(double)(sms_volume_table[idx])/34949.3333;	
	}

    resetDAC();

    // and set up the audio rate
	AudioSampleRate=freq;
}

// TODO: how shall we do volume?
void SN76xxx::SetSoundVolumes() {
	// set overall volume (this is not a sound chip effect, it's directly related to DirectSound)
	// it sets the maximum volume of all channels
	// this uses max_volume
}

// TODO: same as SetSoundVolumes, but mute it (don't change max_volume)
void SN76xxx::MuteAudio() {
}


void SN76xxx::resetDAC() { 
	// empty the buffer and reset the pointers
    MuteAudio();

	{
		autoMutex lock(csAudioBuf);
		memset(&dac_buffer[0], (unsigned char)(nDACLevel*255), sizeof(dac_buffer));
		dac_pos=0;
		dacupdatedistance=0.0;
        dacramp=0.0;
	}

	SetSoundVolumes();
}

void updateDACBuffer(int nCPUCycles) {
	// TODO: implement DAC based on CPU time
	// Maybe we can put nCPUCycles into InterestingData and reset it here, same with the CRU stuff
#if 0
	static int totalCycles = 0;

	if (max_cpf < DEFAULT_60HZ_CPF) {
		totalCycles = 0;
		return;	// don't do it if running slow
	}

	// because it is an int (nominally 3,000,000 - this makes the slider work)
	int CPUCYCLES = max_cpf * hzRate;

	totalCycles+=nCPUCycles;
	double fdist = (double)totalCycles / ((double)CPUCYCLES/AudioSampleRate);
	if (fdist < 1.0) return;				// don't even bother
	dacupdatedistance += fdist;
	totalCycles = 0;		// we used them all.

	int distance = (int)dacupdatedistance;
	dacupdatedistance -= distance;
	if (dac_pos + distance >= sizeof(dac_buffer)) {
        if (ThrottleMode == THROTTLE_NORMAL) {
    		debug_write("DAC Buffer overflow...");
        }
		dac_pos=0;
		return;
	}
	int average = 1;
	double value = nDACLevel;

	// emulate noise
	if (enableBackgroundHum) {
		if (VDPS&VDPS_INT) value += 0.05;	// interrupt line (TODO: this is the internal bit, need to account for the mask)
		value += CRU_TOGGLES;
	}
	CRU_TOGGLES = 0;

	for (int idx=0; idx<3; idx++) {
		// A little check for eliminate high frequency tones
		// If the frequency is greater than 1/2 the sample rate, it's DAC.
		// Noises can't get higher than audible frequencies (even with high user defined rates),
		// so we don't need to worry about them.
		if ((nRegister[idx] != 0) && (nRegister[idx] <= (int)(111860.0/(double)(AudioSampleRate/2)))) {
			// this would be too high a frequency, so we'll merge it into the DAC channel
			// and kill this channel. The reason is that the high frequency ends up
			// creating artifacts with the lower frequency output rate, and you don't
			// get an inaudible tone but awful noise
			value += nVolumeTable[nVolume[idx]];	// not strictly right, the high frequency output adds some distortion. But close enough.
			average++;
		}
	}
	value /= average;
	unsigned char out = (unsigned char)(value * 255);
	EnterCriticalSection(&csAudioBuf);
		memset(&dac_buffer[dac_pos], out, distance);
		dac_pos+=distance;
	LeaveCriticalSection(&csAudioBuf);
#endif
}
