/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "parser/Parser.h"

#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <memory>

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
#include "parser/ParseException.h"


using parser::Parser;


namespace {
	struct ActionsFlagMap : std::map<std::string, uint32_t> {
		ActionsFlagMap()
		{
			(*this)["updated"] = code::kActionFlagUpdated;
			(*this)["together"] = code::kActionFlagTogether;
			(*this)["ignore"] = code::kActionFlagIgnore;
			(*this)["quietly"] = code::kActionFlagQuietly;
			(*this)["piecemeal"] = code::kActionFlagPiecemeal;
			(*this)["existing"] = code::kActionFlagExisting;
			(*this)["maxline"] = code::kActionFlagMaxLineFactor;
		}
	};

	static ActionsFlagMap sActionsFlags;
}


struct Parser::NodeListContainer {
	code::NodeList	fNodes;

	~NodeListContainer()
	{
		for (code::NodeList::iterator it = fNodes.begin(); it != fNodes.end();
				++it) {
			delete *it;
		}
	}

	void Add(code::Node* node)
	{
		try {
			fNodes.push_back(node);
		} catch (...) {
			delete node;
			throw;
		}
	}

	void Detach()
	{
		fNodes.clear();
	}
};


Parser::Parser()
{
}


void
Parser::Test(int argc, const char* const* argv)
{
#if 1
	std::istream& input = std::cin;
#else
	std::ifstream input("testdata/test1");
#endif

	std::noskipws(input);

	fLexer.Init(BaseIteratorType(input), BaseIteratorType());

	try {
		code::Block* block = _ParseFile();
		std::cout << "Parse tree:\n";
		code::DumpContext dumpContext;
		block->Dump(dumpContext);
		delete block;
	} catch (LexException& exception) {
		printf("Parser::Test(): %zu:%zu Lex exception: %s\n",
			exception.Position().Line() + 1, exception.Position().Column() + 1,
			exception.Message());
	} catch (ParseException& exception) {
		printf("Parser::Test(): %zu:%zu Parse exception: %s\n",
			exception.Position().Line() + 1, exception.Position().Column() + 1,
			exception.Message());
	} catch (...) {
		printf("Parser::Test(): Caught exception\n");
	}
}


code::Block*
Parser::_ParseFile()
{
	std::auto_ptr<code::Block> block(_ParseBlock());

	if (_Token() != TOKEN_EOF)
		_ThrowExpected("Expected statement or local variable declaration");

	return block.release();
}


code::Block*
Parser::_ParseBlock()
{
	std::auto_ptr<code::Block> block(new code::Block);

	while (true) {
		// local variable declaration or statement
		if (_Token() == TOKEN_LOCAL)
			*block += _ParseLocalVariableDeclaration();
		else if (code::Node* statement = _TryParseStatement())
			*block += statement;
		else
			break;
	}

	return block.release();
}


code::Node*
Parser::_TryParseStatement()
{
	switch (_Token().ID()) {
		case TOKEN_LEFT_BRACE:
		{
			// "{" block "}"
			_NextToken();
			std::auto_ptr<code::Block> result(_ParseBlock());
			_SkipToken(TOKEN_LEFT_BRACE,
				"Expected '}' at the end of the block");
			return result.release();
		}

		case TOKEN_INCLUDE:
		{
			// "include" list ";"
			_NextToken();
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_SEMICOLON,
				"Expected ';' at the end of the 'include' statement");

			// create node
			code::Node* result = new code::Include(list.get());
			list.release();
			return result;
		}

		case TOKEN_BREAK:
		{
			// "break" list ";"
			_NextToken();
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_SEMICOLON,
				"Expected ';' at the end of the 'break' statement");

			// create node
			code::Node* result = new code::Break(list.get());
			list.release();
			return result;
		}

		case TOKEN_CONTINUE:
		{
			// "continue" list ";"
			_NextToken();
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_SEMICOLON,
				"Expected ';' at the end of the 'continue' statement");

			// create node
			code::Node* result = new code::Continue(list.get());
			list.release();
			return result;
		}

		case TOKEN_RETURN:
		{
			// "return" list ";"
			_NextToken();
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_SEMICOLON,
				"Expected ';' at the end of the 'return' statement");

			// create node
			code::Node* result = new code::Return(list.get());
			list.release();
			return result;
		}

		case TOKEN_JUMPTOEOF:
		{
			// "jumptoeof" list ";"
			_NextToken();
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_SEMICOLON,
				"Expected ';' at the end of the 'jumptoeof' statement");

			// create node
			code::Node* result = new code::JumpToEof(list.get());
			list.release();
			return result;
		}

		case TOKEN_IF:
		{
			// "if" expression "{" block "}" [ "else" statement ]
			_NextToken();
			std::auto_ptr<code::Node> expression(_ParseExpression());
			_SkipToken(TOKEN_LEFT_BRACE, "Expected '{' after 'if' expression");
			std::auto_ptr<code::Node> block(_ParseBlock());
			_SkipToken(TOKEN_RIGHT_BRACE, "Expected '}' after 'if' block");

			// optional 'else' branch
			std::auto_ptr<code::Node> elseBlock;
			if (_TrySkipToken(TOKEN_ELSE)) {
				elseBlock.reset(_Expect(_TryParseStatement(),
					"Expected statement after 'else'"));
			}

			// create node
			code::Node* result = new code::If(expression.get(), block.get(),
				elseBlock.get());

			expression.release();
			block.release();
			elseBlock.release();

			return result;
		}

		case TOKEN_FOR:
		{
			// "for" argument "in" list "{" block "}"
			_NextToken();
			std::auto_ptr<code::Node> argument(_Expect(_TryParseArgument(),
				"Expected argument after 'for'"));
			_SkipToken(TOKEN_IN, "Expected 'in' after 'for' argument");
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_LEFT_BRACE, "Expected '{' after 'for' head");
			std::auto_ptr<code::Node> block(_ParseBlock());
			_SkipToken(TOKEN_RIGHT_BRACE, "Expected '}' after 'for' block");

			// create node
			code::Node* result = new code::For(argument.get(), list.get(),
				block.get());

			argument.release();
			list.release();
			block.release();

			return result;
		}

		case TOKEN_WHILE:
		{
			// "while" expression "{" block "}"
			_NextToken();
			std::auto_ptr<code::Node> expression(_ParseExpression());
			_SkipToken(TOKEN_LEFT_BRACE, "Expected '{' after 'while' head");
			std::auto_ptr<code::Node> block(_ParseBlock());
			_SkipToken(TOKEN_RIGHT_BRACE, "Expected '}' after 'while' block");

			// create node
			code::Node* result = new code::While(expression.get(), block.get());

			expression.release();
			block.release();

			return result;
		}

		case TOKEN_SWITCH:
		{
			// "switch" list "{" case* "}"
			_NextToken();
			std::auto_ptr<code::Node> list(_ParseList());
			_SkipToken(TOKEN_LEFT_BRACE, "Expected '{' after 'while' head");

			// create node
			std::auto_ptr<code::Switch> switchNode(
				new code::Switch(list.get()));
			list.release();

			// each case: "case" identifier ":" block
			while (_TrySkipToken(TOKEN_CASE)) {
				if (_Token() != TOKEN_STRING)
					_ThrowExpected("Expected pattern string after 'case'");

				data::String pattern = _Token();
				_NextToken();
				_SkipToken(TOKEN_COLON, "Expected ':' after 'case' pattern");
				std::auto_ptr<code::Node> block(_ParseBlock());

				// create node and add it
				code::Case* caseNode = new code::Case(pattern, block.get());
				block.release();

				switchNode->AddCase(caseNode);
			}

			_SkipToken(TOKEN_RIGHT_BRACE,
				"Expected '}' at the end of the 'switch' statement");

			return switchNode.release();
		}

		case TOKEN_ON:
		{
			// "on" argument statement
			_NextToken();
			std::auto_ptr<code::Node> argument(_Expect(_TryParseArgument(),
				"Expected argument after 'on'"));
			std::auto_ptr<code::Node> statement(_Expect(_TryParseStatement(),
				"Expected statement after 'on' argument"));

			// create node
			code::Node* result = new code::OnExpression(argument.get(),
				statement.get());

			argument.release();
			statement.release();

			return result;
		}

		case TOKEN_RULE:
		{
			// "rule" identifier [ identifier ( ":" identifier )*  ]
			// "{" block "}"
			_NextToken();

			if (_Token() != TOKEN_STRING)
				_ThrowExpected("Expected rule name string after 'rule'");
			data::String ruleName = _Token();
			_NextToken();

			// create node
			std::auto_ptr<code::RuleDefinition> rule(
				new code::RuleDefinition(ruleName));

			// get rule parameter names
			if (_Token() == TOKEN_STRING) {
				rule->AddParameterName(_Token());
				_NextToken();

				while (_TrySkipToken(TOKEN_COLON)) {
					if (_Token() != TOKEN_STRING) {
						_ThrowExpected(
							"Expected 'rule' parameter name after ':'");
					}

					rule->AddParameterName(_Token());
					_NextToken();
				}
			}

			_SkipToken(TOKEN_LEFT_BRACE, "Expected '{' after 'while' head");
			rule->SetBlock(_ParseBlock());
			_SkipToken(TOKEN_RIGHT_BRACE, "Expected '}' after 'while' block");

			return rule.release();
		}

		case TOKEN_ACTIONS:
		{
			// "actions" actionsFlags identifier [ "bind" list ]
			// "{" rawActions "}"
			_NextToken();

			// actionsflags: ( actionsFlag | ("maxline" string) )*
			uint32_t actionsFlags = 0;
			while (_Token() == TOKEN_STRING) {
				ActionsFlagMap::const_iterator it = sActionsFlags.find(
					_Token());
				if (it == sActionsFlags.end())
					break;

				_NextToken();

				uint32_t flag = it->second;
				if (flag == code::kActionFlagMaxLineFactor) {
					if (_Token() != TOKEN_STRING) {
						_ThrowExpected(
							"Expected number after 'actions' flag 'maxline'");
					}

					actionsFlags = (actionsFlags & code::kActionFlagMask)
						| (uint32_t)atoi(_Token().c_str())
							* code::kActionFlagMaxLineFactor;
					_NextToken();
				} else
					actionsFlags |= flag;
			}

			// rule name
			if (_Token() != TOKEN_STRING)
				_ThrowExpected("Expected 'actions' rule name");

			data::String ruleName = _Token();
			_NextToken();

			// bind list
			std::auto_ptr<code::Node> bindList;
			if (_Token() == TOKEN_BIND)
				bindList.reset(_ParseList());

			// the actions block
			if (_Token() != TOKEN_LEFT_BRACE)
				_ThrowExpected("Expected '{' after 'actions' head");

			data::String commands(fLexer.ScanActions());
			_NextToken();

			_SkipToken(TOKEN_RIGHT_BRACE, "Expected '}' after 'actions' block");

			// create node
			code::ActionsDefinition* actions = new code::ActionsDefinition(
				actionsFlags, ruleName, bindList.get(), commands);

			bindList.release();

			return actions;
		}

		default:
		{
			// 1. argument "on" list <assignmentOp> list ";"
			// 2. argument <assignmentOp> list ';'
			// 3. argument listOfLists ';'

			// each case starts with an argument
			std::auto_ptr<code::Node> argument(_TryParseArgument());
			if (argument.get() == NULL)
				return NULL;

			switch (_Token().ID()) {
				case TOKEN_ON:
				{
					// 1. argument "on" list <assignmentOp> list ";"
					_NextToken();
					std::auto_ptr<code::Node> onTargets(_ParseList());

					code::AssignmentOperator operatorType
						= _ParseAssignmentOperator();

					std::auto_ptr<code::Node> rhs(_ParseList());
					_SkipToken(TOKEN_SEMICOLON,
						"Expected ';' at the end of the assignment statement");

					// create node
					code::Node* result = new code::Assignment(argument.get(),
						operatorType, rhs.get(), onTargets.get());

					argument.release();
					rhs.release();
					onTargets.release();

					return result;
				}

				case TOKEN_ASSIGN:
				case TOKEN_ASSIGN_PLUS:
				case TOKEN_ASSIGN_DEFAULT:
				{
					// 2. argument <assignmentOp> list ';'
					code::AssignmentOperator operatorType
						= _ParseAssignmentOperator();

					std::auto_ptr<code::Node> rhs(_ParseList());
					_SkipToken(TOKEN_SEMICOLON,
						"Expected ';' at the end of the assignment statement");

					// create node
					code::Node* result = new code::Assignment(argument.get(),
						operatorType, rhs.get());

					argument.release();
					rhs.release();

					return result;
				}

				default:
				{
					// 3. argument listOfLists ';'
					NodeListContainer arguments;
					_ParseListOfLists(arguments);
					_SkipToken(TOKEN_SEMICOLON,
						"Expected ';' at the end of the rule invocation");

					// create node
					code::Node* result = new code::FunctionCall(argument.get(),
						arguments.fNodes);

					argument.release();
					arguments.Detach();

					return result;
				}
			}
		}
	}
}


code::Node*
Parser::_ParseLocalVariableDeclaration()
{
	// "local" list [ "=" list ] ";"

	// "local" keyword
	_SkipToken(TOKEN_LOCAL, "Expected 'local' keyword");

	// variable list
	std::auto_ptr<code::Node> variables(_ParseList());

	// optional "=" initializer
	std::auto_ptr<code::Node> initializer;
	if (_TrySkipToken(TOKEN_ASSIGN))
		initializer.reset(_ParseList());

	// skip ";"
	_SkipToken(TOKEN_SEMICOLON,
		"Expected ';' after local variable declaration");

	// create node
	code::LocalVariableDeclaration* result
		= new code::LocalVariableDeclaration(variables.get(),
			initializer.get());

	variables.release();
	initializer.release();

	return result;
}


code::List*
Parser::_ParseList()
{
	std::auto_ptr<code::List> list(new code::List);

	// zero or more arguments
	while (code::Node* argument = _TryParseArgument())
		*list += argument;

	return list.release();
}


code::Node*
Parser::_TryParseArgument()
{
	// either a bracket expression or a simple string
	if (_TrySkipToken(TOKEN_LEFT_BRACKET)) {
		std::auto_ptr<code::Node> result(_ParseBracketExpression());
		_SkipToken(TOKEN_RIGHT_BRACKET,
			"Expected ']' at the end of a bracket expression");
		return result.release();
	}

	if (_Token().IsStringOrKeyword()) {
		code::Node* result = new code::Leaf(_Token());
		_NextToken();
		return result;
	}

	return NULL;
}


code::Node*
Parser::_ParseBracketExpression()
{
	// either a bracketed "on" expression or a function call
	if (code::Node* node = _TryParseBracketOnExpression())
		return node;

	if (code::Node* node = _TryParseFunctionCall())
		return node;

	_ThrowExpected("Expected statement or local variable declaration");

	return NULL;
}


code::Node*
Parser::_TryParseBracketOnExpression()
{
	// "on" keyword
	if (!_TrySkipToken(TOKEN_ON))
		return NULL;

	std::auto_ptr<code::Node> onTarget(_Expect(_TryParseArgument(),
		"Expected target argument after 'on'"));

	// either a "return ..." or a function call
	std::auto_ptr<code::Node> expression;
	if (_TrySkipToken(TOKEN_RETURN)) {
		expression.reset(_ParseList());
	} else {
		expression.reset(_TryParseFunctionCall());
		if (expression.get() == NULL) {
			_ThrowExpected(
				"Expected 'return' or function call in 'on' expression");
		}
	}

	// create result
	code::Node* result = new code::OnExpression(onTarget.get(),
		expression.get());

	onTarget.release();
	expression.release();

	return result;
}


code::Node*
Parser::_TryParseFunctionCall()
{
	// the function name(s)
	std::auto_ptr<code::Node> function(_TryParseArgument());
	if (function.get() == NULL)
		return NULL;

	// list of lists of arguments
	NodeListContainer nodes;
	_ParseListOfLists(nodes);

	// create the node
	code::Node* result = new code::FunctionCall(function.get(), nodes.fNodes);

	function.release();
	nodes.Detach();

	return result;
}


void
Parser::_ParseListOfLists(NodeListContainer& nodes)
{
	// one list or more lists, separated by ":"
	nodes.Add(_ParseList());

	while (_TrySkipToken(TOKEN_COLON))
		nodes.Add(_ParseList());
}


code::Node*
Parser::_ParseExpression()
{
	// atom (<op> atom)*
	code::Node* result = _ParseAtom();
	code::Node* rhs;

	try {
		while (true) {
			rhs = NULL;

			switch (_Token().ID()) {
				case TOKEN_EQUAL:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::EqualExpression(result, rhs);
					break;

				case TOKEN_NOT_EQUAL:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::NotEqualExpression(result, rhs);
					break;

				case TOKEN_LESS_OR_EQUAL:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::LessOrEqualExpression(result, rhs);
					break;

				case TOKEN_LESS:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::LessExpression(result, rhs);
					break;

				case TOKEN_GREATER_OR_EQUAL:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::GreaterOrEqualExpression(result, rhs);
					break;

				case TOKEN_GREATER:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::GreaterExpression(result, rhs);
					break;

				case TOKEN_AND:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::AndExpression(result, rhs);
					break;

				case TOKEN_OR:
					_NextToken();
					rhs = _ParseAtom();
					result = new code::OrExpression(result, rhs);
					break;

				default:
					return result;
			}
		}
	} catch (...) {
		delete rhs;
		delete result;
		throw;
	}
}


code::Node*
Parser::_ParseAtom()
{
	switch (_Token().ID()) {
		case TOKEN_NOT:
		{
			// "!" atom
			_NextToken();
			std::auto_ptr<code::Node> atom(_ParseAtom());

			// create node
			code::Node* result = new code::NotExpression(atom.get());
			atom.release();
			return result;
		}

		case TOKEN_LEFT_PARENTHESIS:
		{
			// "(" expression ")"
			std::auto_ptr<code::Node> expression(_ParseExpression());
			_SkipToken(TOKEN_RIGHT_PARENTHESIS,
				"Expected ')' after expression");
			return expression.release();
		}

		default:
		{
			// argument [ "in" list ]
			std::auto_ptr<code::Node> argument(_Expect(_TryParseArgument(),
				"Expected argument"));

			// the "in" part is optional
			if (!_TrySkipToken(TOKEN_IN))
				return argument.release();

			std::auto_ptr<code::Node> list(_ParseList());

			// create node
			code::Node* result = new code::InListExpression(argument.get(),
				list.get());

			argument.release();
			list.release();
			return result;
		}
	}
}


code::AssignmentOperator
Parser::_ParseAssignmentOperator()
{
	switch (_Token().ID()) {
		case TOKEN_ASSIGN:
			_NextToken();
			return code::ASSIGNMENT_OPERATOR_ASSIGN;
		case TOKEN_ASSIGN_PLUS:
			_NextToken();
			return code::ASSIGNMENT_OPERATOR_APPEND;
		case TOKEN_ASSIGN_DEFAULT:
			_NextToken();
			return code::ASSIGNMENT_OPERATOR_DEFAULT;
		default:
			_ThrowExpected("Expected assignment operator");
			return code::ASSIGNMENT_OPERATOR_DEFAULT;
	}
}


void
Parser::_Throw(const char* message)
{
	throw ParseException(message, fLexer.CurrentTokenPosition());
}


void
Parser::_ThrowExpected(const char* expected)
{
	std::string message(expected);
	if (_Token() == TOKEN_EOF)
		message += ". Encountered end of input";
	else
		message += ". Got '" + _Token() + "'";

	_Throw(message.c_str());
}