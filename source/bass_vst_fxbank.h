// FxBank.h: interface for the CFxBank class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FXBANK_H__11548EB6_7960_41DF_B4FA_C8966452FC43__INCLUDED_)
#define AFX_FXBANK_H__11548EB6_7960_41DF_B4FA_C8966452FC43__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "bass_vst_impl.h"

/*===========================================================================*/
/* structures for .fxb / .fxp files                                          */
/*===========================================================================*/

// These structures haven't been officially documented before VST SDK V2.2.
// Since then, they have undergone quite "interesting" changes in the SDK,
// which makes it quite a pain to maintain a link between them and VSTHost's
// file-handling classes. That is simply too dumb.
// From now on, I'll use my OWN structures to describe the contents of
// .fxb / .fxp files. In C++ style, which is 10 times easier to read
// and maintain. Of course, they'll match the latest available SDK.

/*****************************************************************************/
/* Constants used in the files - copied verbatim from vstfxstore.h           */
/*****************************************************************************/

#define cMagic 		'CcnK'
#define fMagic		'FxCk'
#define bankMagic	'FxBk'
#define chunkGlobalMagic	'FxCh'
#define chunkPresetMagic	'FPCh'
#define chunkBankMagic		'FBCh'

/*****************************************************************************/
/* SFxHeader : header for all chunks                                         */
/*****************************************************************************/

struct SFxHeader
{
	VstInt32 chunkMagic;                  /* 'CcnK' in any case                */
	VstInt32 byteSize;                    /* size of this chunk, excluding     */
	/* chunkMagic & byteSize             */
};

/*****************************************************************************/
/* SFxBase : base structure for all chunks                                   */
/*****************************************************************************/

struct SFxBase : public SFxHeader
{
	VstInt32 fxMagic;                     /* for programs:                     */
	/*   'FxCk' (regular) or             */
	/*   'FPCh' (opaque chunk)           */
	/* for banks:                        */
	/*   'FxBk' (regular) or             */
	/*   'FBCh' (opaque chunk)           */
	VstInt32 version;                     /* format version                    */
	/* for programs: 1                   */
	/* for banks: 1 or 2                 */
	VstInt32 fxID;                        /* fx unique ID                      */
	VstInt32 fxVersion;                   /* fx version                        */
};

/*****************************************************************************/
/* SFxProgramBase : base structure for all programs                          */
/*****************************************************************************/

struct SFxProgramBase : public SFxBase
{
	VstInt32 numParams;                   /* number of parameters              */
	char prgName[28];                     /* program name (ASCIIZ)             */
};

/*****************************************************************************/
/* SFxProgram : a program in single-parameter style                          */
/*****************************************************************************/

struct SFxProgram : public SFxProgramBase
{
	float params[1];                      /* in reality, a variable-sized array*/
	/* with numParams parameter values   */
};

/*****************************************************************************/
/* SFxProgramChunk : a program in chunk style                                */
/*****************************************************************************/

struct SFxProgramChunk : public SFxProgramBase
{
	VstInt32 size;                        /* size of program data in bytes     */
	char chunk[1];                        /* variable-sized opaque array       */
};

/*****************************************************************************/
/* SFxBankBase : base structure for a bank of programs                       */
/*****************************************************************************/

struct SFxBankBase : public SFxBase
{
	VstInt32 numPrograms;                 /* number of programs in bank        */
	VstInt32 currentProgram;              /* if version 2, current program,    */
	/* else irrelevant                   */
	char future[124];                     /* reserved (should be zero)         */
};

/*****************************************************************************/
/* SFxBank : structure for a bank of programs consisting of parameters       */
/*****************************************************************************/

struct SFxBank : public SFxBankBase
{
	SFxProgram programs[1];               /* variable number of programs       */
};

/*****************************************************************************/
/* SFxBankChunk : structure for a bank of programs as an opaque chunk        */
/*****************************************************************************/

struct SFxBankChunk : public SFxBankBase
{
	VstInt32 size;                        /* size of bank data in bytes        */
	char chunk[1];                        /* variable-sized opaque array       */
  };


class CFxBase
{
public:
	
protected:
	static bool NeedsBSwap;
	
protected:
	static void SwapBytes(float &f);
	static void SwapBytes(long &l);
#if defined(VST_2_4_EXTENSIONS)
	static void SwapBytes(VstInt32 &vi);
#endif
	
};


class CFxBank : public CFxBase
{
public:
    CFxBank(char *pszFile = 0);
    CFxBank(int nPrograms, int nParams);
    CFxBank(int nChunkSize);
    CFxBank(CFxBank const &org) { DoCopy(org); }
	virtual ~CFxBank();
    CFxBank & operator=(CFxBank const &org) { return DoCopy(org); }
public:
    bool SetSize(int nPrograms, int nParams);
    bool SetSize(int nChunkSize);
	bool LoadBank(char *pszFile);
	bool SaveBank(char *pszFile);
	void Unload();
    bool IsLoaded() { return !!bBank; }
    bool IsChunk() { return bChunk; }
	
    // access functions
public:
    long GetVersion() { if (!bBank) return 0; return ((SFxBase*)bBank)->version; }
    long GetFxID() { if (!bBank) return 0; return ((SFxBase*)bBank)->fxID; }
    void SetFxID(long id) { if (bBank) ((SFxBase*)bBank)->fxID = id; if (!bChunk) for (int i = GetNumPrograms() -1; i >= 0; i--) GetProgram(i)->fxID = id; }
    long GetFxVersion() { if (!bBank) return 0; return ((SFxBase*)bBank)->fxVersion; }
    void SetFxVersion(long v) { if (bBank) ((SFxBase*)bBank)->fxVersion = v; if (!bChunk) for (int i = GetNumPrograms() -1; i >= 0; i--) GetProgram(i)->fxVersion = v; }
    long GetNumPrograms() { if (!bBank) return 0; return ((SFxBankBase*)bBank)->numPrograms; }
    long GetNumParams() { if (bChunk) return 0; return GetProgram(0)->numParams; }
    long GetChunkSize() { if (!bChunk) return 0; return ((SFxBankChunk *)bBank)->size; }
    void *GetChunk() { if (!bChunk) return 0; return ((SFxBankChunk *)bBank)->chunk; }
    bool SetChunk(void *chunk) { if (!bChunk) return false; memcpy(((SFxBankChunk *)bBank)->chunk, chunk, ((SFxBankChunk *)bBank)->size); return true; }
	
	SFxProgram * GetProgram(int nProgNum);
	
    char * GetProgramName(int nProgram)
	{
		SFxProgram *p = GetProgram(nProgram);
		if (!p)
			return NULL;
		return p->prgName;
	}
    void SetProgramName(int nProgram, const char *name = "")
	{
		SFxProgram *p = GetProgram(nProgram);
		if (!p)
			return;
		strncpy(p->prgName, name, sizeof(p->prgName));
		p->prgName[sizeof(p->prgName)-1] = '\0';
	}
    float GetProgParm(int nProgram, int nParm)
	{
		SFxProgram *p = GetProgram(nProgram);
		if (!p || nParm > p->numParams)
			return 0;
#ifndef chunkGlobalMagic                /* VST SDK 2.4 rev2?                 */
		return p->content.params[nParm];
#else
		return p->params[nParm];
#endif
	}
    bool SetProgParm(int nProgram, int nParm, float val = 0.0)
	{
		SFxProgram *p = GetProgram(nProgram);
		if (!p || nParm > p->numParams)
			return false;
		if (val < 0.0)
			val = 0.0;
		if (val > 1.0)
			val = 1.0;
#ifndef chunkGlobalMagic                /* VST SDK 2.4 rev2?                 */
		p->content.params[nParm] = val;
#else
		p->params[nParm] = val;
#endif
		return true;
	}
	
protected:
	char szFileName[256];
	unsigned char * bBank;
	int nBankLen;
	bool bChunk;
	
protected:
    void Init();
	CFxBank & DoCopy(CFxBank const &org);
};

#endif // !defined(AFX_FXBANK_H__11548EB6_7960_41DF_B4FA_C8966452FC43__INCLUDED_)
