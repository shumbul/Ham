/*
 * Copyright 2010-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "code/Include.h"

#include <fstream>
#include <memory>
#include <sstream>

#include "code/Block.h"
#include "code/DumpContext.h"
#include "code/EvaluationContext.h"
#include "code/EvaluationException.h"
#include "data/FileStatus.h"
#include "data/TargetBinder.h"
#include "data/TargetPool.h"
#include "parser/Parser.h"
#include "util/Constants.h"


namespace ham {
namespace code {


Include::Include(Node* fileNames)
	:
	fFileNames(fileNames)
{
	fFileNames->AcquireReference();
}


Include::~Include()
{
	fFileNames->ReleaseReference();
}


StringList
Include::Evaluate(EvaluationContext& context)
{
	// check include depth
	size_t includeDepth = context.IncludeDepth();
	if (includeDepth >= util::kIncludeDepthLimit) {
		std::stringstream message;
		message << "Reached include depth limit ("
			<< util::kIncludeDepthLimit << ")";
		throw EvaluationException(message.str());
	}
	context.SetIncludeDepth(includeDepth + 1);

	StringList fileNames = fFileNames->Evaluate(context);
	if (!fileNames.IsEmpty()) {
		// Only the file referred to by the first element is included.

		// bind the target
		data::Target* target
			= context.Targets().LookupOrCreate(fileNames.ElementAt(0));
		String filePath;
		data::FileStatus fileStatus;
		data::TargetBinder::Bind(*context.GlobalVariables(), target, filePath,
			fileStatus);

		// open the file
		std::ifstream file(filePath.ToCString());
		if (file.fail()) {
			if (target->IsIgnoreIfMissing())
				return StringList::False();
			throw EvaluationException(
				std::string("include: Failed to open file \"")
					+ filePath.ToCString() + "\"");
		}

		// parse and evaluate it
		parser::Parser parser;
		parser.SetFileName(filePath.ToStlString());
		util::Reference<code::Block> block(parser.Parse(file), true);

		block->Evaluate(context);

		if (context.GetJumpCondition() == JUMP_CONDITION_JUMP_TO_EOF)
			context.SetJumpCondition(JUMP_CONDITION_NONE);
	}

	// reset include depth
	context.SetIncludeDepth(includeDepth);

	return StringList::False();
}


code::Node*
Include::Visit(NodeVisitor& visitor)
{
	if (visitor.VisitNode(this))
		return this;

	return fFileNames->Visit(visitor);
}


void
Include::Dump(DumpContext& context) const
{
	context << "Include(\n";
	context.BeginChildren();

	fFileNames->Dump(context);

	context.EndChildren();
	context << ")\n";
}


}	// namespace code
}	// namespace ham
