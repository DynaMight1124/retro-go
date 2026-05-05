/*
** dobjgc.cpp
** The garbage collector. Based largely on Lua's.
**
**---------------------------------------------------------------------------
** Copyright 2008 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include "actor.h"
#include "dobject.h"
#include "templates.h"
#include "m_alloc.h"
#include "thinker.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_net.h"
#include "id_ca.h"
#include "thingdef.h"

// MACROS ------------------------------------------------------------------

#define DEFAULT_GCPAUSE		150
#define DEFAULT_GCMUL		400
#define GCSTEPSIZE		1024u
#define GCSWEEPMAX		40
#define GCSWEEPCOST		10
#define GCFINALIZECOST	100

// PUBLIC DATA DEFINITIONS -------------------------------------------------

namespace GC
{
size_t AllocBytes;
size_t Threshold;
size_t Estimate;
DObject *Gray;
DObject *Root;
DObject **SweepPos;
uint32 CurrentWhite = OF_White0 | OF_Fixed;
EGCState State = GCS_Pause;
int Pause = DEFAULT_GCPAUSE;
int StepMul = DEFAULT_GCMUL;
size_t Dept;

// CODE --------------------------------------------------------------------

void SetThreshold()
{
	Threshold = (Estimate / 100) * Pause;
#ifdef ESP_PLATFORM
	// Cap the GC growth to 64KB on ESP32 to prevent OOM panics
	// because the 4MB PSRAM cannot accommodate a 50% growth (1.5MB+) over the 3MB baseline.
	if (Threshold > Estimate + 65536)
		Threshold = Estimate + 65536;
#endif
}

size_t PropagateMark()
{
	DObject *obj = Gray;
	obj->Gray2Black();
	Gray = obj->GCNext;
	return obj->PropagateMark();
}

DObject **SweepList(DObject **p, size_t count, size_t *finalize_count)
{
	DObject *curr;
	int deadmask = OtherWhite();
	size_t finalized = 0;

	while ((curr = *p) != NULL && count-- > 0)
	{
		if (!(curr->ObjectFlags & OF_Fixed) && ((curr->ObjectFlags ^ OF_WhiteBits) & deadmask))
		{
			curr->MakeWhite();
			p = &curr->ObjNext;
		}
		else
		{
			*p = curr->ObjNext;
			if (curr->ObjectFlags & OF_Fixed)
			{
				// This should not happen
				curr->MakeWhite();
				p = &curr->ObjNext;
			}
			else
			{
				curr->ObjectFlags |= OF_Cleanup;
				delete curr;
				finalized++;
			}
		}
	}
	if (finalize_count != NULL)
	{
		*finalize_count = finalized;
	}
	return p;
}

void Mark(DObject **obj)
{
	DObject *lobj = *obj;
	if (lobj != NULL)
	{
		if (lobj->ObjectFlags & OF_EuthanizeMe)
		{
			*obj = (DObject *)NULL;
		}
		else if (lobj->IsWhite())
		{
			lobj->White2Gray();
			lobj->GCNext = Gray;
			Gray = lobj;
		}
	}
}

static void MarkRoot()
{
	Gray = NULL;
	thinkerList.MarkRoots();
	for(unsigned int i = 0;i < Net::InitVars.numPlayers;++i)
		players[i].PropagateMark();
	if(map)
		map->PropagateMark();
	
	// Use the Mark that takes a pointer-to-pointer
	DObject *screenObj = (DObject *)screen;
	Mark(&screenObj);

	State = GCS_Propagate;
}

static void Atomic()
{
	CurrentWhite = OtherWhite();
	SweepPos = &Root;
	State = GCS_Sweep;
	Estimate = AllocBytes;
}

static size_t SingleStep()
{
	switch (State)
	{
	case GCS_Pause:
		MarkRoot();
		return 0;

	case GCS_Propagate:
		if (Gray != NULL)
		{
			return PropagateMark();
		}
		else
		{
			Atomic();
			return 0;
		}

	case GCS_Sweep: {
		size_t old = AllocBytes;
		size_t finalize_count;
		SweepPos = SweepList(SweepPos, GCSWEEPMAX, &finalize_count);
		if (*SweepPos == NULL)
		{
			State = GCS_Finalize;
		}
		Estimate -= old - AllocBytes;
		return (GCSWEEPMAX - finalize_count) * GCSWEEPCOST + finalize_count * GCFINALIZECOST;
	  }

	case GCS_Finalize:
		State = GCS_Pause;
		Dept = 0;
		return 0;

	default:
		return 0;
	}
}

void Step()
{
	size_t lim = (GCSTEPSIZE/100) * StepMul;
	size_t olim;
	if (lim == 0)
	{
		lim = (~(size_t)0) / 2;
	}
	Dept += AllocBytes - Threshold;
	do
	{
		olim = lim;
		lim -= SingleStep();
	} while (olim > lim && State != GCS_Pause);
	if (State != GCS_Pause)
	{
		if (Dept < GCSTEPSIZE)
		{
			Threshold = AllocBytes + GCSTEPSIZE;
		}
		else
		{
			Dept -= GCSTEPSIZE;
			Threshold = AllocBytes;
		}
	}
	else
	{
		SetThreshold();
	}
}

void FullGC()
{
	if (State <= GCS_Propagate)
	{
		SweepPos = &Root;
		Gray = NULL;
		State = GCS_Sweep;
	}
	while (State != GCS_Finalize)
	{
		SingleStep();
	}
	MarkRoot();
	while (State != GCS_Pause)
	{
		SingleStep();
	}
	SetThreshold();
}

void FreeAll()
{
	State = GCS_Sweep;
	CurrentWhite = OF_White0 | OF_White1;
	SweepPos = &Root;
	while (*SweepPos != NULL)
	{
		SweepPos = SweepList(SweepPos, ~(size_t)0, NULL);
	}
}

void DelSoftRootHead() {}
void AddSoftRoot(DObject *obj) {}
void DelSoftRoot(DObject *obj) {}

}
