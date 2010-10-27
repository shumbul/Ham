/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef HAM_PARSER_PARSER_H
#define HAM_PARSER_PARSER_H


#include <iterator>

#include "code/Defs.h"
#include "parser/Lexer.h"


namespace code {
	class Block;
	class List;
	class Node;
}


namespace parser {


class Parser {
public:
			class Listener;

public:
								Parser();

			void				Test(int argc, const char* const* argv);

private:
			typedef std::istream_iterator<char> BaseIteratorType;
			typedef parser::Lexer<BaseIteratorType> LexerType;

			struct NodeListContainer;
			struct ListenerNotifier;
			struct DumpListener;

private:
			code::Block*		_ParseFile();
			code::Block*		_ParseBlock();
			code::Node*			_TryParseStatement();
			code::Node*			_ParseLocalVariableDeclaration();
			code::List*			_ParseList();
			code::Node*			_TryParseArgument(bool allowKeyword = false);
			code::Node*			_ParseBracketExpression();
			code::Node*			_TryParseBracketOnExpression();
			code::Node*			_TryParseFunctionCall();
			void				_ParseListOfLists(NodeListContainer& nodes);
			code::Node*			_ParseExpression();
			code::Node*			_ParseAtom();

	inline	code::AssignmentOperator _ParseAssignmentOperator();

			void				_Throw(const char* message);
			void				_ThrowExpected(const char* expected);

			const Token&		_Token() const
									{ return fLexer.CurrentToken(); }
			const Token&		_NextToken();
	inline	bool				_TrySkipToken(TokenID id);
	inline	void				_SkipToken(TokenID id, const char* expected);

	template<typename NodeType>
	inline	NodeType*			_Expect(NodeType* node, const char* expected);

private:
			LexerType	fLexer;
			Listener*	fListener;
};


class Parser::Listener {
public:
	virtual						~Listener();

	virtual	void				NonterminalStart(const char* name);
	virtual	void				NonterminalEnd(const char* name);
	virtual	void				NextToken(const Token& token);
};


const Token&
Parser::_NextToken()
{
	const Token& token = fLexer.NextToken();

	if (fListener != NULL)
		fListener->NextToken(token);

	return token;
}


bool
Parser::_TrySkipToken(TokenID id)
{
	if (_Token() != id)
		return false;

	_NextToken();
	return true;
}


void
Parser::_SkipToken(TokenID id, const char* expected)
{
	if (!_TrySkipToken(id))
		_ThrowExpected(expected);
}


template<typename NodeType>
NodeType*
Parser::_Expect(NodeType* node, const char* expected)
{
	if (node == NULL)
		_ThrowExpected(expected);
	return node;
}


} // namespace parser


#endif	// HAM_PARSER_PARSER_H
