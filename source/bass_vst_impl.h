 
/*****************************************************************************
 *  BASS_VST
 *****************************************************************************
 *
 *  File:       bass_vst_impl.c
 *  Authors:    Bjoern Petersen
 *  Purpose:    All defines and headers needed for the BASS_VST implementation,
 *				this is an internal header!
 *
 *	Version History:
 *	22.04.2006	Created in this form (bp)
 *
 *  (C) Bjoern Petersen Software Design and Development
 *  
 *****************************************************************************/



#ifndef __BASS_VST_IMPL_H__
#define __BASS_VST_IMPL_H__



/*****************************************************************************
 *  Common Stuff and includes
 *****************************************************************************/


#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <pthread.h>
#define CRITICAL_SECTION				pthread_mutex_t
#define InitializeCriticalSection(a)	pthread_mutex_init(a, NULL)
#define TryEnterCriticalSection			!pthread_mutex_trylock
#define EnterCriticalSection			pthread_mutex_lock
#define LeaveCriticalSection			pthread_mutex_unlock
#define DeleteCriticalSection			pthread_mutex_destroy	
typedef CFBundleRef HINSTANCE;
#elif __linux__
#include <dlfcn.h>
#include <pthread.h>
#define CRITICAL_SECTION				pthread_mutex_t
#define InitializeCriticalSection(a)	pthread_mutex_init(a, NULL)
#define TryEnterCriticalSection			!pthread_mutex_trylock
#define EnterCriticalSection			pthread_mutex_lock
#define LeaveCriticalSection			pthread_mutex_unlock
#define DeleteCriticalSection			pthread_mutex_destroy
typedef void* HINSTANCE;
#else
#include <windows.h>
#include <crtdbg.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif


// BASS includes
#define BASSDEF(f) (WINAPI f)	
#define BASSSCOPE
#include "bass/bass.h"
#include "bass/bass-addon.h"
#include "bass/bassmidi.h"

// BASS VST includes
#ifndef _WIN32
#pragma GCC visibility push(default)
#endif
#include "bass_vst.h"
#ifndef _WIN32
#pragma GCC visibility pop
#endif

// Fix build aeffect.h on linux
#ifdef __linux__
#define __cdecl __attribute__((__cdecl__))
#endif

// VST DSK includes
#include "vstsdk24/aeffectx.h"


// internal includes
#include "sjhash.h"
#include "bass_vst_version.h"

// in BASS 2.4 the user pointer are void*, in older versions, DWORD was used
#if BASSVERSION >= 0x204
#define USERPTR void*
#else
#define USERPTR DWORD
#endif


/*****************************************************************************
 *  Plugins
 *****************************************************************************/

typedef struct
{
	// are we an effect or an instrument?
	DWORD				type;
	#define				VSTeffect		0
	#define				VSTinstrument	1

	// the vstHandle as returned from BASS_VST_ChannelSetDSP() or BASS_VST_ChannelCreate().
	DWORD				vstHandle;

	// the underlying channel as given to BASS_VST_ChannelSetDSP() or created by BASS_VST_ChannelCreate().
	// for instruments created by the latter, this value is equal to vstHandle!
	DWORD				channelHandle;

	// effect-only stuff: dspHandle is the handle returned from BASS_ChannelSetDSP.  0 for
	// unchanneled effect and for VST instruments.
	HDSP				dspHandle;

	// the underlying DLL
	DWORD				createFlags;
	HINSTANCE			hinst;

	// the underlying VST object
	AEffect*			aeffect;
	#define				canDoubleReplacing(a) ( ((a)->aeffect->flags&effFlagsCanDoubleReplacing)!=0 && (a)->aeffect->processDoubleReplacing!=NULL )

	// handing parameters and programs
	int 				numDefaultValues;  // we cache this value as some plugins change aeffect->numParams :-(
	float*				defaultValues;     // only set at loading time caching the initial param values
	int 				numLastValues;
	float*				lastValues;
	float*				tempProgramValueBuf;
	char				tempProgramNameBuf[128]; // normally kVstMaxProgNameLen+1 (=24+1) should be enough, be a little safer
	char*				tempChunkData;

	// process handling
	#define				MAX_CHANS 32
	float*				buffersIn[MAX_CHANS];
	float*				buffersOut[MAX_CHANS];
	long				bytesPerInOutBuffer;

	float*				bufferTemp;
	long				bytesTempBuffer;

	bool				effOpenCalled;
	bool				effStartProcessCalled;

	long				effBlockSize;
	
	// bypass handling
	BOOL				doBypass;

	// idle stuff
	#define				NEEDS_EDIT_IDLE			0x01
	#define				NEEDS_IDLE_OUTSIDE_EDIT 0x02
	int					needsIdle;

	// editor stuff
	bool				editorIsOpen;
	DWORD				editorScope;

	// callbacks
	VSTPROC*			callback;
	void*				callbackUserData;

	// pending MIDI events, they're sended just before processReplacing is called
	#define				MAX_MIDI_EVENTS 2048
	VstEvents*			midiEventsCurr;
	VstEvents*			midiEventsPrev;
	CRITICAL_SECTION	midiCritical_;

	
	// static vstTimeInfo structre, "static" as the pointer may be needed "a little bit longer"
	VstTimeInfo			vstTimeInfo;

	// misc
	#define				MAX_FWD 128
	DWORD				forwardDataToOtherVstHandles[MAX_FWD];
	int					forwardDataToOtherCnt;

	CRITICAL_SECTION	vstCritical_;

	// do not use directly! always use the BASS_VST_LOCKER!
	long				handleUsage;

	// pluginID for shell plugin
	long				pluginID;

	//// vst plugin path
	//char pluginPath[2048];
} BASS_VST_PLUGIN;



// handle stuff
void				initHandleHandling();
void				exitHandleHandling();

BASS_VST_PLUGIN*	createHandle(DWORD type, DWORD handle); // initalized the reference counting to 1; if handle is 0, a new handle is created
BASS_VST_PLUGIN*	refHandle(DWORD handle);
BOOL				unrefHandle(DWORD handle);	// if a handle has no more references, it is destroyed!

BOOL				tryEnterVstCritical(BASS_VST_PLUGIN*);
void				enterVstCritical(BASS_VST_PLUGIN*);
void				leaveVstCritical(BASS_VST_PLUGIN*);

// idle stuff
extern sjhash			s_idleHash;
extern CRITICAL_SECTION	s_idleCritical;
extern sjhash			s_unloadPendingInstances;
extern long				s_unloadPendingCountdown;
void					idleDo();
void					updateIdleTimers(BASS_VST_PLUGIN*); // call this if needsIdle has changed
void					createIdleTimers();
void					killIdleTimers();

#define					IDLE_FREQ 50 /*ms = 20Hz*/
#define					IDLE_UNLOAD_PENDING_COUNTDOWN (10000/*10 seconds*/ / IDLE_FREQ)



// buffers
void					freeChansBuffers(BASS_VST_PLUGIN*);
void					freeTempBuffer(BASS_VST_PLUGIN*);

bool					openProcess(BASS_VST_PLUGIN*, BASS_VST_PLUGIN* info_);
bool					closeProcess(BASS_VST_PLUGIN*);
void CALLBACK			doEffectProcess(HDSP handle, DWORD channel, void* buffer, DWORD length, USERPTR user);
DWORD CALLBACK			doInstrumentProcess(HSTREAM vstHandle, void* buffer, DWORD length, USERPTR user);

int						validateLastValues(BASS_VST_PLUGIN*);


// forwarding
void					checkForwarding();
extern CRITICAL_SECTION	s_forwardCritical;

// misc
void					callMainsChanged(BASS_VST_PLUGIN* this_, long blockSize);
long					fileSelOpen(BASS_VST_PLUGIN* this_, VstFileSelect* vstFs);
void					fileSelClose(BASS_VST_PLUGIN* this_, VstFileSelect* vstFs);

// Effect bank files.
int					EffGetChunk(BASS_VST_PLUGIN* this_, void **ptr, bool isPreset = false);
int					EffSetChunk(BASS_VST_PLUGIN* this_, void *data, long byteSize, bool isPreset = false);

#endif /* __BASS_VST_IMPL_H__ */
