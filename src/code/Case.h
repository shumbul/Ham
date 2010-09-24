/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HAM_CODE_CASE_H
#define HAM_CODE_CASE_H


#include "code/Node.h"


namespace code {


class Case : public Node {
public:
								Case(const String& pattern, Node* block);

	virtual	StringList			Evaluate(EvaluationContext& context);

	virtual	void				Dump(DumpContext& context) const;

private:
			String				fPattern;
			Node*				fBlock;
};


}	// namespace code


#endif	// HAM_CODE_CASE_H
