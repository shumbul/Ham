/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HAM_CODE_EVALUATION_CONTEXT_H
#define HAM_CODE_EVALUATION_CONTEXT_H


#include "data/VariableScope.h"


namespace data {
	class TargetPool;
}


namespace code {


class EvaluationContext {
public:
								EvaluationContext(
									data::VariableDomain& globalVariables,
									data::TargetPool& targets);

			data::VariableDomain* GlobalVariables()
									{ return &fGlobalVariables; }
			data::VariableScope* GlobalScope() const
									{ return fGlobalScope; }
			void				SetGlobalScope(data::VariableScope* scope)
									{ fGlobalScope = scope; }

			data::VariableScope* LocalScope() const
									{ return fLocalScope; }
			void				SetLocalScope(data::VariableScope* scope)
									{ fLocalScope = scope; }

			data::TargetPool&	Targets() const	{ return fTargets; }

private:
			data::VariableDomain& fGlobalVariables;
			data::VariableScope	fRootScope;
			data::VariableScope* fGlobalScope;
			data::VariableScope* fLocalScope;
			data::TargetPool&	fTargets;
};


}	// namespace code


#endif	// HAM_CODE_EVALUATION_CONTEXT_H
