/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "code/LocalVariableDeclaration.h"

#include "code/DumpContext.h"
#include "code/EvaluationContext.h"


using code::LocalVariableDeclaration;


LocalVariableDeclaration::LocalVariableDeclaration(List* variables)
	:
	fVariables(variables),
	fInitializer(NULL)
{
}


StringList
LocalVariableDeclaration::Evaluate(EvaluationContext& context)
{
	// TODO: Implement!
	return kFalseStringList;
}


void
LocalVariableDeclaration::Dump(DumpContext& context) const
{
	context << "LocalVariableDeclaration(\n";
	context.BeginChildren();

	fVariables->Dump(context);

	if (fInitializer != NULL)
		fInitializer->Dump(context);

	context.EndChildren();
	context << ")\n";
}
