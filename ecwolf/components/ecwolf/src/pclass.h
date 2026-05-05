#ifndef __PCLASS_H__
#define __PCLASS_H__

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

typedef TArray<ActionInfo *> ActionTable;
typedef TArray<Symbol *> SymbolTable;

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

		static void				DumpClasses();
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
		static void				LoadActors();
		bool					IsStateOwner(const Frame *frame) const;
		static void				UnloadActors();

		unsigned int			ClassIndex;
		MetaTable				Meta;

		static bool	SetFlag(const ClassDef *newClass, AActor *instance, const FString &prefix, const FString &flagName, bool set);
		static bool SetProperty(ClassDef *newClass, const char* className, const char* propName, Scanner &sc);

	protected:
		friend class DObject;
		static const size_t POINTER_END;

		static void AddGlobalSymbol(Symbol *sym);
		void		BuildFlatPointers();
		const Frame *FindStateInList(const FName &stateName) const;
		void		FinalizeActorClass();
		bool		InitializeActorClass(bool isNative);
		void		InstallStates(const TArray<StateDefinition> &stateDefs);
		void		RegisterEdNum(unsigned int ednum);
		const Frame *ResolveStateIndex(unsigned int index) const;

		static TMap<FName, ClassDef *>	&ClassTable();
		static SymbolTable				globalSymbols;

		bool			tentative;
		FName			name;
		const ClassDef	*parent;
		size_t			size;

		const ClassDef	*replacement;
		const ClassDef	*replacee;

		TMap<FName, unsigned int> stateList;
		TArray<Frame> frameList;

		ActionTable		actions;
		SymbolTable		symbols;

		const size_t	*Pointers;
		const size_t	*FlatPointers;

		DObject			*defaultInstance;
		DObject			*(*ConstructNative)(const ClassDef *, void *);

		static bool		bShutdown;
};

#endif
