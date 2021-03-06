/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HAM_CODE_EVALUATION_EXCEPTION_H
#define HAM_CODE_EVALUATION_EXCEPTION_H


#include "util/TextFileException.h"


namespace ham {
namespace code {


class EvaluationException : public util::TextFileException {
public:
	EvaluationException(const std::string& message = std::string(),
		const std::string& fileName = std::string(),
		const util::TextFilePosition& position = util::TextFilePosition())
		:
		util::TextFileException(message, position)
	{
	}
};


}	// namespace code
}	// namespace ham


#endif	// HAM_CODE_EVALUATION_EXCEPTION_H
