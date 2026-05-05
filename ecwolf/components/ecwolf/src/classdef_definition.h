/*
** classdef_definition.h
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
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

#ifndef __CLASSDEF_DEFINITION_H__
#define __CLASSDEF_DEFINITION_H__

#include "tarray.h"
#include "name.h"
#include "zstring.h"

class DObject;
class AActor;
class FArchive;
class Frame;
class Symbol;
class ExpressionNode;
class Type;
class Scanner;
struct StateDefinition;
struct ActionResult;
class CallArguments;
class ActionInfo;

class MetaTable
{
	public:
		MetaTable();
		MetaTable(const MetaTable &other);
		~MetaTable();

		enum Type
		{
			INTEGER,
			FIXED,
			STRING
		};

		int			GetMetaInt(uint32_t id, int def=0) const;
		fixed		GetMetaFixed(uint32_t id, fixed def=0) const;
		const char*	GetMetaString(uint32_t id) const;
		bool		IsInherited(uint32_t id);
		void		SetMetaInt(uint32_t id, int value);
		void		SetMetaFixed(uint32_t id, fixed value);
		void		SetMetaString(uint32_t id, const char* value);

		const MetaTable &operator= (const MetaTable &other) { CopyMeta(other); return *this; }

	private:
		class Data;

		Data	*head;
		Data	*FindMeta(uint32_t id) const;
		Data	*FindMetaData(uint32_t id);

		void	CopyMeta(const MetaTable &other);
		void	FreeTable();
};

class ClassDef
{
	public:
		ClassDef();
		~ClassDef();

		AActor					*CreateInstance() const;
		bool					IsAncestorOf(const ClassDef *child) const { return child->IsDescendantOf(this); }
		bool					IsDescendantOf(const ClassDef *parent) const;

		template<class T>
		static const ClassDef	*DeclareNativeClass(const char* className, const ClassDef **parent)
		{
			ClassDef **definitionLookup = ClassTable().CheckKey(className);
			ClassDef *definition = NULL;
			if(definitionLookup == NULL)
			{
				definition = new ClassDef();
				ClassTable()[className] = definition;
			}
			else
				definition = *definitionLookup;
			definition->Pointers = *T::__PointerOffsets == POINTER_END ? NULL : T::__PointerOffsets;
			definition->name = className;
			definition->parent = (const ClassDef *)parent;
			definition->size = sizeof(T);
			definition->defaultInstance = (DObject *) M_Malloc(definition->size);
			memset((void*)definition->defaultInstance, 0, definition->size);
			definition->ConstructNative = &T::__InPlaceConstructor;
			return definition;
		}

		typedef TMap<FName, ClassDef*>::ConstIterator	ClassIterator;
		typedef TMap<FName, ClassDef*>::ConstPair		ClassPair;
		static ClassIterator	GetClassIterator() { return ClassIterator(ClassTable()); }
		static unsigned int		GetNumClasses() { return ClassTable().CountUsed(); }

		static const ClassDef	*FindClass(unsigned int ednum);
		static const ClassDef	*FindClass(const FName &className);
		static const ClassDef	*FindClassTentative(const FName &className, const ClassDef *parent);
		static const ClassDef	*FindConversationClass(unsigned int convid);
		const ActionInfo		*FindFunction(const FName &function, int &specialNum) const;
		const Frame				*FindState(const FName &stateName) const;
		Symbol					*FindSymbol(const FName &symbol) const;
		AActor					*GetDefault() const { return (AActor*)defaultInstance; }
		const FName				&GetName() const { return name; }
		const ClassDef			*GetParent() const { return parent; }
		const ClassDef			*GetReplacement(bool respectMapinfo=true) const;
		size_t					GetSize() const { return size; }
		const Frame				*GetState(unsigned int index) const;
		bool					IsStateOwner(const Frame *frame) const;
		static void				DumpClasses();
		static void				LoadActors();
		static void				UnloadActors();

		unsigned int			ClassIndex;
		MetaTable				Meta;

		static bool	SetFlag(const ClassDef *newClass, AActor *instance, const FString &prefix, const FString &flagName, bool set);
	protected:
		friend class DObject;
		friend class StateLabel;
		friend class FDecorateParser;
		static const size_t POINTER_END;

		static bool SetProperty(ClassDef *newClass, const char* className, const char* propName, Scanner &sc);

		static void AddGlobalSymbol(Symbol *sym);
		void		BuildFlatPointers();
		const Frame *FindStateInList(const FName &stateName) const;
		void		FinalizeActorClass();
		bool		InitializeActorClass(bool isNative);
		void		InstallStates(const TArray<StateDefinition> &stateDefs);
		void		RegisterEdNum(unsigned int ednum);
		const Frame *ResolveStateIndex(unsigned int index) const;
		static TMap<FName, ClassDef *>	&ClassTable();
		static TArray<Symbol *> globalSymbols;

		bool			tentative;
		FName			name;
		const ClassDef	*parent;
		size_t			size;

		const ClassDef	*replacement;
		const ClassDef	*replacee;

		TMap<FName, unsigned int> stateList;
		TArray<Frame> frameList;

		TArray<ActionInfo *>	actions;
		TArray<Symbol *> symbols;

		const size_t	*Pointers;
		const size_t	*FlatPointers;

		DObject			*defaultInstance;
		DObject			*(*ConstructNative)(const ClassDef *, void *);

		static bool		bShutdown;
};

#endif
