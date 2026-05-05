/*
** dobject.cpp
** Implements the base class DObject, which most other classes derive from
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#include <stdlib.h>
#include <string.h>

#include "dobject.h"
#include "actordef.h"
#include "wl_agent.h"
#include "thingdef.h"
#include "zdoomsupport.h"

const ClassDef *DObject::__StaticClass = ClassDef::DeclareNativeClass<DObject>("DObject", NULL);
const size_t DObject::__PointerOffsets[] = { ~(size_t)0 };
DObject *DObject::__InPlaceConstructor(const ClassDef *classDef, void *mem)
{
	return new ((EInPlace *)mem) DObject(classDef);
}

DObject::DObject ()
: Class(0), ObjNext(NULL), GCNext(NULL), ObjectFlags(0)
{
	ObjectFlags = GC::CurrentWhite & OF_WhiteBits;
	ObjNext = GC::Root;
	GC::Root = this;
}

DObject::DObject (const ClassDef *inClass)
: Class(inClass), ObjNext(NULL), GCNext(NULL), ObjectFlags(0)
{
	ObjectFlags = GC::CurrentWhite & OF_WhiteBits;
	ObjNext = GC::Root;
	GC::Root = this;
}

DObject::~DObject ()
{
	if (!(ObjectFlags & OF_Cleanup))
	{
		DObject **probe;
		const ClassDef *type = GetClass();

		// Find all pointers that reference this object and NULL them.
		StaticPointerSubstitution(this, NULL);

		// Now unlink this object from the GC list.
		for (probe = &GC::Root; *probe != NULL; probe = &((*probe)->ObjNext))
		{
			if (*probe == this)
			{
				*probe = ObjNext;
				if (&ObjNext == GC::SweepPos)
				{
					GC::SweepPos = probe;
				}
				break;
			}
		}

		// If it's gray, also unlink it from the gray list.
		if (this->IsGray())
		{
			for (probe = &GC::Gray; *probe != NULL; probe = &((*probe)->GCNext))
			{
				if (*probe == this)
				{
					*probe = GCNext;
					break;
				}
			}
		}
	}
}

void DObject::Destroy ()
{
	ObjectFlags = (ObjectFlags & ~OF_Fixed) | OF_EuthanizeMe;
}

size_t DObject::PropagateMark()
{
	const ClassDef *info = GetClass();
	if (info && !ClassDef::bShutdown)
	{
		const size_t *offsets = info->FlatPointers;
		if (offsets == NULL)
		{
			const_cast<ClassDef *>(info)->BuildFlatPointers();
			offsets = info->FlatPointers;
		}
		if (offsets)
		{
			while (*offsets != ~(size_t)0)
			{
				GC::Mark((DObject **)((BYTE *)this + *offsets));
				offsets++;
			}
		}
		return info->GetSize();
	}
	return 0;
}

size_t DObject::PointerSubstitution (DObject *old, DObject *notOld)
{
	const ClassDef *info = GetClass();
	const size_t *offsets = info->FlatPointers;
	size_t changed = 0;
	if (offsets == NULL)
	{
		const_cast<ClassDef *>(info)->BuildFlatPointers();
		offsets = info->FlatPointers;
	}
	if (offsets)
	{
		while (*offsets != ~(size_t)0)
		{
			if (*(DObject **)((BYTE *)this + *offsets) == old)
			{
				*(DObject **)((BYTE *)this + *offsets) = notOld;
				changed++;
			}
			offsets++;
		}
	}
	return changed;
}

size_t DObject::StaticPointerSubstitution (DObject *old, DObject *notOld)
{
	DObject *probe;
	size_t changed = 0;

	// Go through all objects.
	for (probe = GC::Root; probe != NULL; probe = probe->ObjNext)
	{
		changed += probe->PointerSubstitution(old, notOld);
	}

	return changed;
}

void DObject::SerializeUserVars(FArchive &arc)
{
}

void DObject::Serialize (FArchive &arc)
{
	ObjectFlags |= OF_SerialSuccess;
}

void DObject::CheckIfSerialized () const
{
	if (!(ObjectFlags & OF_SerialSuccess))
	{
		I_Error (
			"BUG: %s::Serialize\n"
			"(or one of its superclasses) needs to call\n"
			"Super::Serialize\n",
			GetClass()->GetName().GetChars());
	}
}

namespace GC
{
	void Barrier(DObject *pointing, DObject *pointed)
	{
		if (pointed->IsWhite())
		{
			if (pointing == NULL || pointing->IsBlack())
			{
				pointed->White2Gray();
				pointed->GCNext = Gray;
				Gray = pointed;
			}
		}
	}
}
