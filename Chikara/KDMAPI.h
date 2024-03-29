#pragma once
#ifndef KDMAPI_H
#include <Windows.h>
#include <cstdint>

//namespace KDMAPI 
//{
//  extern HMODULE dll_handle;
//
//  extern int(__cdecl* IsKDMAPIAvailable)();
//  extern int(__cdecl* InitializeKDMAPIStream)();
//  extern int(__cdecl* TerminateKDMAPIStream)();
//  extern uint32_t(__cdecl* SendDirectData)(unsigned long msg);
//  extern int(__cdecl* SendCustomEvent)(unsigned long eventtype, unsigned long chan, unsigned long param);
//  extern void(__cdecl* ResetKDMAPIStream)();
//
//  extern void Init();
//  extern void Destroy();
//}

//Stolen from bassmidi header file
#define MIDI_EVENT_NOTE				1
#define MIDI_EVENT_PROGRAM			2
#define MIDI_EVENT_CHANPRES			3
#define MIDI_EVENT_PITCH			4
#define MIDI_EVENT_PITCHRANGE		5
#define MIDI_EVENT_DRUMS			6
#define MIDI_EVENT_FINETUNE			7
#define MIDI_EVENT_COARSETUNE		8
#define MIDI_EVENT_MASTERVOL		9
#define MIDI_EVENT_BANK				10
#define MIDI_EVENT_MODULATION		11
#define MIDI_EVENT_VOLUME			12
#define MIDI_EVENT_PAN				13
#define MIDI_EVENT_EXPRESSION		14
#define MIDI_EVENT_SUSTAIN			15
#define MIDI_EVENT_SOUNDOFF			16
#define MIDI_EVENT_RESET			17
#define MIDI_EVENT_NOTESOFF			18
#define MIDI_EVENT_PORTAMENTO		19
#define MIDI_EVENT_PORTATIME		20
#define MIDI_EVENT_PORTANOTE		21
#define MIDI_EVENT_MODE				22
#define MIDI_EVENT_REVERB			23
#define MIDI_EVENT_CHORUS			24
#define MIDI_EVENT_CUTOFF			25
#define MIDI_EVENT_RESONANCE		26
#define MIDI_EVENT_RELEASE			27
#define MIDI_EVENT_ATTACK			28
#define MIDI_EVENT_DECAY			29
#define MIDI_EVENT_REVERB_MACRO		30
#define MIDI_EVENT_CHORUS_MACRO		31
#define MIDI_EVENT_REVERB_TIME		32
#define MIDI_EVENT_REVERB_DELAY		33
#define MIDI_EVENT_REVERB_LOCUTOFF	34
#define MIDI_EVENT_REVERB_HICUTOFF	35
#define MIDI_EVENT_REVERB_LEVEL		36
#define MIDI_EVENT_CHORUS_DELAY		37
#define MIDI_EVENT_CHORUS_DEPTH		38
#define MIDI_EVENT_CHORUS_RATE		39
#define MIDI_EVENT_CHORUS_FEEDBACK	40
#define MIDI_EVENT_CHORUS_LEVEL		41
#define MIDI_EVENT_CHORUS_REVERB	42
#define MIDI_EVENT_USERFX			43
#define MIDI_EVENT_USERFX_LEVEL		44
#define MIDI_EVENT_USERFX_REVERB	45
#define MIDI_EVENT_USERFX_CHORUS	46
#define MIDI_EVENT_DRUM_FINETUNE	50
#define MIDI_EVENT_DRUM_COARSETUNE	51
#define MIDI_EVENT_DRUM_PAN			52
#define MIDI_EVENT_DRUM_REVERB		53
#define MIDI_EVENT_DRUM_CHORUS		54
#define MIDI_EVENT_DRUM_CUTOFF		55
#define MIDI_EVENT_DRUM_RESONANCE	56
#define MIDI_EVENT_DRUM_LEVEL		57
#define MIDI_EVENT_DRUM_USERFX		58
#define MIDI_EVENT_SOFT				60
#define MIDI_EVENT_SYSTEM			61
#define MIDI_EVENT_TEMPO			62
#define MIDI_EVENT_SCALETUNING		63
#define MIDI_EVENT_CONTROL			64
#define MIDI_EVENT_CHANPRES_VIBRATO	65
#define MIDI_EVENT_CHANPRES_PITCH	66
#define MIDI_EVENT_CHANPRES_FILTER	67
#define MIDI_EVENT_CHANPRES_VOLUME	68
#define MIDI_EVENT_MOD_VIBRATO		69
#define MIDI_EVENT_MODRANGE			69
#define MIDI_EVENT_BANK_LSB			70
#define MIDI_EVENT_KEYPRES			71
#define MIDI_EVENT_KEYPRES_VIBRATO	72
#define MIDI_EVENT_KEYPRES_PITCH	73
#define MIDI_EVENT_KEYPRES_FILTER	74
#define MIDI_EVENT_KEYPRES_VOLUME	75
#define MIDI_EVENT_SOSTENUTO		76
#define MIDI_EVENT_MOD_PITCH		77
#define MIDI_EVENT_MOD_FILTER		78
#define MIDI_EVENT_MOD_VOLUME		79
#define MIDI_EVENT_VIBRATO_RATE		80
#define MIDI_EVENT_VIBRATO_DEPTH	81
#define MIDI_EVENT_VIBRATO_DELAY	82
#define MIDI_EVENT_MIXLEVEL			0x10000
#define MIDI_EVENT_TRANSPOSE		0x10001
#define MIDI_EVENT_SYSTEMEX			0x10002
#define MIDI_EVENT_SPEED			0x10004

#define MIDI_EVENT_END				0
#define MIDI_EVENT_END_TRACK		0x10003

#define MIDI_EVENT_NOTES			0x20000
#define MIDI_EVENT_VOICES			0x20001

#define MIDI_SYSTEM_DEFAULT			0
#define MIDI_SYSTEM_GM1				1
#define MIDI_SYSTEM_GM2				2
#define MIDI_SYSTEM_XG				3
#define MIDI_SYSTEM_GS				4

#endif