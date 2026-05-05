/*
** thingdef.h
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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
**
*/

#ifndef __THINGDEF_H__
#define __THINGDEF_H__

#include "zstring.h"
#include "tarray.h"
#include "classdef_definition.h"
#include "actordef.h"

class FName;
class AActor;
class Frame;
class ExpressionNode;
class Type;
class Scanner;

class StateLabel
{
	public:
		StateLabel() : isDefault(false), isRelative(false) {}
		StateLabel(const FString &str, const ClassDef *parent, bool noRelative=false);
		StateLabel(Scanner &sc, const ClassDef *parent, bool noRelative=false);

		const Frame	*Resolve() const;
		const Frame	*Resolve(AActor *owner, const Frame *caller, const Frame *def=NULL) const;
		void	Parse(Scanner &sc, const ClassDef *parent, bool noRelative=false);

	private:
		const ClassDef	*cls;
		FString 		label;
		unsigned short	offset;
		bool			isDefault;
		bool			isRelative;
};

class CallArguments
{
	public:
		class Value
		{
			public:
				enum
				{
					VAL_INTEGER,
					VAL_DOUBLE,
					VAL_STRING,
					VAL_STATE
				} useType;
				bool isExpression;

				ExpressionNode	*expr;
				union
				{
					int64_t	i;
					double	d;
				}				val;
				FString			str;
				StateLabel		label;

				Value() : useType(VAL_INTEGER), isExpression(false), expr(NULL) { val.i = 0; }
		};

		~CallArguments();

		void		AddArgument(const Value &val);
		void		ShrinkToFit() { args.ShrinkToFit(); }
		int			Count() const { return args.Size(); }
		void		Evaluate(AActor *self);
		const Value	&operator[] (unsigned int idx) const { return args[idx]; }

	private:
		TArray<Value> args;
};

class ActionInfo
{
	public:
		ActionInfo(ActionPtr func, const FName &name);

		const Type *ArgType(unsigned int n) const { return types[MIN(n, maxArgs-1)]; }

		ActionPtr func;
		const FName name;

		unsigned int					minArgs;
		unsigned int					maxArgs;
		bool							varArgs;
		TArray<CallArguments::Value>	defaults;
		TArray<const Type *>			types;
};

typedef TArray<ActionInfo *> ActionTable;

#ifdef __cplusplus
#define ACTION_EXTERN extern "C"
#else
#define ACTION_EXTERN extern
#endif

#define ACTION_FUNCTION(func) \
	ACTION_EXTERN bool __AF_##func(AActor *, AActor *, const Frame *, const CallArguments &, struct ActionResult *); \
	static const ActionInfo __attribute__((used)) __AI_##func(__AF_##func, #func); \
	ACTION_EXTERN bool __AF_##func(AActor *self, AActor *stateOwner, const Frame *caller, const CallArguments &args, struct ActionResult *result)
#define ACTION_ALIAS(func, alias) \
	ACTION_FUNCTION(alias) \
	{ \
		return __AF_##func(self, stateOwner, caller, args, result); \
	}
#define CALL_ACTION(func, self) \
	{ \
		__AF_##func(self, self, NULL, CallArguments(), NULL); \
	}
#define ACTION_PARAM_COUNT args.Count()
#define ACTION_PARAM_BOOL(name, num) \
	bool name = args[num].val.i ? true : false
#define ACTION_PARAM_INT(name, num) \
	int name = static_cast<int>(args[num].val.i)
#define ACTION_PARAM_DOUBLE(name, num) \
	double name = args[num].val.d
#define ACTION_PARAM_FIXED(name, num) \
	fixed name = static_cast<fixed>(args[num].val.d*FRACUNIT)
#define ACTION_PARAM_STRING(name, num) \
	FString name = args[num].str
#define ACTION_PARAM_STATE(name, num, def) \
	const Frame *name = args[num].label.Resolve(stateOwner, caller, def)

class SymbolInfo
{
	public:
		static const SymbolInfo *LookupSymbol(const ClassDef *cls, FName var);

		SymbolInfo(const ClassDef *cls, const FName var, const int offset);

		const ClassDef	* const cls;
		const FName		var;
		const int		offset;
};

#define DEFINE_SYMBOL(cls, var) \
	static const SymbolInfo __SI_##var(NATIVE_CLASS(cls), #var, typeoffsetof(A##cls,var));

struct StateDefinition;

struct PropertyParam
{
	bool isExpression;
	union
	{
		ExpressionNode	*expr;
		char			*s;
		double			f;
		int64_t			i;
	};
};
typedef void (*PropHandler)(ClassDef *info, AActor *defaults, const unsigned int PARAM_COUNT, PropertyParam* params);
#define HANDLE_PROPERTY(property) void __Handler_##property(ClassDef *cls, AActor *defaults, const unsigned int PARAM_COUNT, PropertyParam* params)
struct PropDef
{
	public:
		const ClassDef* const	&className;
		const char* const		prefix;
		const char* const		name;
		const char* const		params;
		PropHandler				handler;
};

typedef TArray<Symbol *> SymbolTable;

extern const PropDef properties[];

#endif /* __THINGDEF_H__ */
