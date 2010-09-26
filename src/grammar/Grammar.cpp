/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "grammar/Grammar.h"

#include "code/ActionsDefinition.h"
#include "code/Assignment.h"
#include "code/BinaryExpression.h"
#include "code/Block.h"
#include "code/Case.h"
#include "code/DumpContext.h"
#include "code/For.h"
#include "code/FunctionCall.h"
#include "code/If.h"
#include "code/Include.h"
#include "code/InListExpression.h"
#include "code/Jump.h"
#include "code/Leaf.h"
#include "code/List.h"
#include "code/LocalVariableDeclaration.h"
#include "code/NotExpression.h"
#include "code/OnExpression.h"
#include "code/RuleDefinition.h"
#include "code/Switch.h"
#include "code/While.h"
#include "grammar/Iterator.h"
#include "grammar/Skipper.h"


namespace grammar {


namespace phoenix = boost::phoenix;
namespace ascii = boost::spirit::ascii;


static const uint32_t kActionFlagUpdated		= 0x01;
static const uint32_t kActionFlagTogether		= 0x02;
static const uint32_t kActionFlagIgnore			= 0x04;
static const uint32_t kActionFlagQuietly		= 0x08;
static const uint32_t kActionFlagPiecemeal		= 0x10;
static const uint32_t kActionFlagExisting		= 0x20;
static const uint32_t kActionFlagMaxLineFactor	= 0x40;


template<>
Grammar<IteratorType, Skipper<IteratorType> >::Grammar()
	:
	Grammar::base_type(fStart),
	fStart(std::string("start")),
	fBlock(std::string("block")),
	fLocalVariableDeclaration(std::string("localVariableDeclaration")),
	fRuleDefinition(std::string("ruleDefinition")),
	fActionsDefinition(std::string("actionsDefinition")),
	fIfStatement(std::string("ifStatement")),
	fForStatement(std::string("forStatement")),
	fWhileStatement(std::string("whileStatement")),
	fSwitchStatement(std::string("switchStatement")),
	fCaseStatement(std::string("caseStatement")),
	fStatement(std::string("statement")),
	fFunctionCall(std::string("functionCall")),
	fOnStatement(std::string("onStatement")),
	fListOfLists(std::string("listOfLists")),
	fList(std::string("list")),
	fActionsFlags(std::string("actionsFlags")),
	fActionsBindList(std::string("actionsBindList")),
	fActions(std::string("actions")),
	fAtom(std::string("atom")),
	fExpression(std::string("expression")),
	fArgument(std::string("argument")),
	fBracketOnExpression(std::string("bracketOnExpression")),
	fBracketExpression(std::string("bracketExpression")),
	fIdentifier(std::string("identifier")),
	fString()
{
	using qi::eps;
	using qi::_val;
	using qi::_1;
	using qi::_2;
	using qi::_3;
	using qi::_4;
	using phoenix::new_;

	fAssignmentOperator.add
		("=", code::ASSIGNMENT_OPERATOR_ASSIGN)
		("+=", code::ASSIGNMENT_OPERATOR_APPEND)
		("?=", code::ASSIGNMENT_OPERATOR_DEFAULT)
	;

	fActionFlag.add
		("updated",		kActionFlagUpdated)
		("together",	kActionFlagTogether)
		("ignore",		kActionFlagIgnore)
		("quietly",		kActionFlagQuietly)
		("piecemeal",	kActionFlagPiecemeal)
		("existing",	kActionFlagExisting)
	;

	_InitActionsDefinition();
	_InitArgument();
	_InitExpression();
	_InitRuleDefinition();
	_InitStatement();

#if 0
	fKeywords.add
		()
		;
"local"
"include"
"jumptoeof"
"on"
"break"
"continue"
"return"
"for"
"in"
"switch"
"if"
"else"
"while"
"rule"
"actions"
#endif

	fIdentifier = fString ;
		// TODO: This should also exclude keywords!

	fList = eps
			[ _val = new_<code::List>() ]
		>> *(fArgument
				[ *_val += _1 ]
			);

	fListOfLists = fList % ':';

	fFunctionCall
		= fArgument
				[ _val = new_<code::FunctionCall>(_1) ]
			>> -(fList
					[ bind(&code::FunctionCall::AddArgument, _val, _1) ]
				>> *(':' >> fList
						[ bind(&code::FunctionCall::AddArgument, _val, _1) ]
					)
				)
// TODO: Parse fListOfLists here, too.
	;

	fBlock
		= eps
				[ _val = new_<code::Block>() ]
			>> *(fStatement
					[ *_val += _1 ]
				| fLocalVariableDeclaration
					[ *_val += _1 ]
				)
	;

	fLocalVariableDeclaration
		= "local" >> fList
				[ _val = new_<code::LocalVariableDeclaration>(_1) ]
			>> -('=' >> fList
					[ bind(&code::LocalVariableDeclaration::SetInitializer,
						_val, _1) ]
				)
			>> ';'
	;

	fIfStatement
		= ("if" >> fExpression >> '{' >> fBlock >> '}')
				[ _val = new_<code::If>(_1, _2) ]
			>> -("else" >> fStatement
					[ bind(&code::If::SetElseBlock, _val, _1) ]
				)
	;

	fForStatement
		= ("for" >> fArgument >> "in" >> fList >> '{' >> fBlock >> '}')
				[ _val = new_<code::For>(_1, _2, _3) ]
	;

	fWhileStatement
		= ("while" >> fExpression >> '{' >> fBlock >> '}')
				[ _val = new_<code::While>(_1, _2) ]
	;

	fOnStatement
		= ("on" >> fArgument >> fStatement)
				[ _val = new_<code::OnExpression>(_1, _2) ]
	;

	fCaseStatement
		= ("case" >> fIdentifier >> ':' >> fBlock)
				[ _val = new_<code::Case>(_1, _2) ]
	;

	fSwitchStatement
		= ("switch" >> fList)
				[ _val = new_<code::Switch>(_1) ]
			>> '{'
			>> *(fCaseStatement
					[ bind(&code::Switch::AddCase, _val, _1) ]
				)
			>> '}'
	;

	fStart = fBlock;

#if 0
	qi::debug(fStart);
	qi::debug(fBlock);
	qi::debug(fLocalVariableDeclaration);
	qi::debug(fRuleDefinition);
	qi::debug(fActionsDefinition);
	qi::debug(fIfStatement);
	qi::debug(fForStatement);
	qi::debug(fWhileStatement);
	qi::debug(fSwitchStatement);
	qi::debug(fCaseStatement);
	qi::debug(fStatement);
	qi::debug(fFunctionCall);
	qi::debug(fOnStatement);
	qi::debug(fListOfLists);
	qi::debug(fList);
	qi::debug(fActionsFlags);
	qi::debug(fActionsBindList);
	qi::debug(fActions);
	qi::debug(fAtom);
	qi::debug(fExpression);
	qi::debug(fBracketExpression);
	qi::debug(fBracketOnExpression);
	qi::debug(fArgument);
	qi::debug(fIdentifier);
#endif
}


} // namespace grammar