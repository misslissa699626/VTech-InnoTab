#ifndef AUDIOPHASETABLE_H
#define AUDIOPHASETABLE_H

// comment the line below for real time compute var phase parameter
#define AUDIO_USE_VARPHASE_TABLE 1

// comment first line below to use only default pitch bend table (sensitivity == 0 or sensitivity == 2)
// comment BOTH line below to real time compute ALL pitch bend parameter
// comment only the second line has no effect at all
#define AUDIO_USE_PITCHBEND_TABLE 1
#define AUDIO_USE_LIMITED_PITCHBEND_TABLE 1

// this line only have effect when AUDIO_USE_PITCHBEND_TABLE is not defined
// if both AUDIO_ENABLE_FIX_POINT_LIB and AUDIO_USE_LIMITED_PITCHBEND_TABLE are defined,
// only default pitch bend table (sensitivity == 0 or sensitivity == 2) value will be used, other pitch bend table value will be computed by external fix-point library "int fixExp(int)"
// if AUDIO_ENABLE_FIX_POINT_LIB is not defined,
// only default pitch bend table (sensitivity == 0 or sensitivity == 2) value will be used, other pitch bend table value will be treated as 1.0
#define AUDIO_ENABLE_FIX_POINT_LIB 1

unsigned Audio_computePhase(unsigned base_pitch, unsigned note_pitch, unsigned sampling_rate);
unsigned Audio_computePitchBendFactor(int sensitivity, int pitch_bend);

#endif