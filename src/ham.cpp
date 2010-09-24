#define BOOST_SPIRIT_DEBUG 1

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_container.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>

#include <boost/fusion/include/at.hpp>

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


// TODO: Remove!
using code::DumpContext;
using code::EvaluationContext;
using code::List;
using code::ListList;
using code::Node;
using code::EqualExpression;
using code::NotEqualExpression;
using code::LessExpression;
using code::LessOrEqualExpression;
using code::GreaterExpression;
using code::GreaterOrEqualExpression;
using code::AndExpression;
using code::OrExpression;
using code::NotExpression;
using code::InListExpression;
using code::OnExpression;
using code::Break;
using code::Continue;
using code::Return;
using code::JumpToEof;
using code::FunctionCall;
using code::Block;
using code::Include;
using code::For;
using code::While;
using code::If;
using code::Case;
using code::Switch;
using code::RuleDefinition;
using code::Assignment;
using code::AssignmentOperator;
using code::ASSIGNMENT_OPERATOR_ASSIGN;
using code::ASSIGNMENT_OPERATOR_APPEND;
using code::ASSIGNMENT_OPERATOR_DEFAULT;
using code::LocalVariableDeclaration;
using code::ActionsDefinition;


namespace ascii = boost::spirit::ascii;
namespace fusion = boost::fusion;
namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;


static const uint32_t kActionFlagUpdated		= 0x01;
static const uint32_t kActionFlagTogether		= 0x02;
static const uint32_t kActionFlagIgnore			= 0x04;
static const uint32_t kActionFlagQuietly		= 0x08;
static const uint32_t kActionFlagPiecemeal		= 0x10;
static const uint32_t kActionFlagExisting		= 0x20;
static const uint32_t kActionFlagMaxLineFactor	= 0x40;


template <typename Iterator>
struct HamSkipParser : qi::grammar<Iterator> {
	HamSkipParser() : HamSkipParser::base_type(fStart)
	{
		using qi::char_;
		using qi::lit;
		using qi::space;

		fStart = space | lit('#') >> *(char_ - '\n' - '\r');
	}

private:
	qi::rule<Iterator> fStart;
};













template <typename Iterator, typename Skipper>
struct HamParser : qi::grammar<Iterator, Node*(), Skipper> {
	HamParser()
		:
		HamParser::base_type(fStart),
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
		fBracketExpression(std::string("bracketExpression")),
		fFunctionCall(std::string("functionCall")),
		fBracketOnExpression(std::string("bracketOnExpression")),
		fOnStatement(std::string("onStatement")),
		fExpression(std::string("expression")),
		fListOfLists(std::string("listOfLists")),
		fList(std::string("list")),
		fAtom(std::string("atom")),
		fArgument(std::string("argument")),
		fActionsFlags(std::string("actionsFlags")),
		fActionsBindList(std::string("actionsBindList")),
		fActions(std::string("actions")),
		fIdentifier(std::string("identifier")),
		fString(std::string("string")),
		fSubString(std::string("subString")),
		fQuotedChar(std::string("quotedChar")),
		fUnquotedChar(std::string("unquotedChar")),
		fEscapedChar(std::string("escapedChar"))
	{
		using qi::eoi;
		using qi::eps;
		using qi::lexeme;
		using qi::lit;
		using qi::raw;
		using qi::space;
		using qi::_val;
		using qi::_1;
		using qi::_2;
		using qi::_3;
		using qi::_4;
		using qi::_r1;
		using qi::_r2;
		using qi::_r3;
		using phoenix::begin;
		using phoenix::construct;
		using phoenix::end;
		using phoenix::new_;
		using phoenix::static_cast_;
		using ascii::char_;

		fAssignmentOperator.add
			("=", ASSIGNMENT_OPERATOR_ASSIGN)
			("+=", ASSIGNMENT_OPERATOR_APPEND)
			("?=", ASSIGNMENT_OPERATOR_DEFAULT)
		;

		fListDelimiter.add
			(":", 0)
			(";", 1)
			("]", 2)
			("=", 3)
			("+=", 3)
			("?=", 3)
			("||", 3)
			("|", 3)
			("&&", 3)
			("&", 3)
			("!=", 3)
			(")", 3)
			("<", 3)
			("<=", 3)
			(">", 3)
			(">=", 3)
			("{", 3)
			("}", 3)
		;

		fActionFlag.add
			("updated",		kActionFlagUpdated)
			("together",	kActionFlagTogether)
			("ignore",		kActionFlagIgnore)
			("quietly",		kActionFlagQuietly)
			("piecemeal",	kActionFlagPiecemeal)
			("existing",	kActionFlagExisting)
		;



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

		fQuotedChar = fEscapedChar | (char_ - '"');
		fUnquotedChar = fEscapedChar | (char_ - '"' - space);
		fEscapedChar = '\\' >> char_;

		fSubString = ('"' >> *fQuotedChar >> '"') | +fUnquotedChar;
		fString
			= !(fListDelimiter >> (space | eoi))
				>> +fSubString
					[ _val += _1 ]
		;
		fIdentifier
			= !(fListDelimiter >> (space | eoi))
				>> +fSubString
					[ _val += _1 ]
		;
			// TODO: This should also exclude keywords!

		fArgument
			= ("[" >> fBracketExpression >> "]")
					[ _val = _1 ]
			| fString
					[ _val = new_<code::Leaf>(_1) ];

		fList = eps
				[ _val = new_<List>() ]
			>> *(fArgument
					[ *_val += _1 ]
				);

		fListOfLists = fList % ':';

		fAtom
 			= '!' >> fAtom
 					[ _val = new_<NotExpression>(_1) ]
			| ('(' >> fExpression >> ')')
					[ _val = _1 ]
			| fArgument
					[ _val = _1 ]
				>> -("in" >> fList)
					[ _val = new_<InListExpression>(_val, _1) ]
		;

		fExpression
			= fAtom
					[ _val = _1 ]
				>> *(
					'=' >> fAtom
						[ _val = new_<EqualExpression>(_val, _1) ]
				|	"!=" >> fAtom
						[ _val = new_<NotEqualExpression>(_val, _1) ]
				|	"<=" >> fAtom
						[ _val = new_<LessOrEqualExpression>(_val, _1) ]
				|	'<' >> fAtom
						[ _val = new_<LessExpression>(_val, _1) ]
				|	">=" >> fAtom
						[ _val = new_<GreaterOrEqualExpression>(_val, _1) ]
				|	'>' >> fAtom
						[ _val = new_<GreaterExpression>(_val, _1) ]
				|	"&&" >> fAtom
						[ _val = new_<AndExpression>(_val, _1) ]
				|	'&' >> fAtom
						[ _val = new_<AndExpression>(_val, _1) ]
				|	"||" >> fAtom
						[ _val = new_<OrExpression>(_val, _1) ]
				|	"|" >> fAtom
						[ _val = new_<OrExpression>(_val, _1) ]
				)
		;

		fBracketOnExpression
			= "on" >> fArgument
					[ _val = new_<OnExpression>(_1) ]
				>> (
					("return" >> fList)
						[ bind(&OnExpression::SetExpression, _val, _1) ]
				|	fFunctionCall
						[ bind(&OnExpression::SetExpression, _val, _1) ]
				)
		;
			// Unfortunately [ _val = new_<OnExpression>(_1, _2) ] doesn't
			// work here, since the variant isn't converted automatically to
			// Node*.

		fBracketExpression
			= fBracketOnExpression
			| fFunctionCall
		;

		fFunctionCall
			= fArgument
					[ _val = new_<FunctionCall>(_1) ]
				>> -(fList
						[ bind(&FunctionCall::AddArgument, _val, _1) ]
					>> *(':' >> fList
							[ bind(&FunctionCall::AddArgument, _val, _1) ]
						)
					)
// TODO: Parse fListOfLists here, too.
		;

		fStatement
			= ('{' >> fBlock >> '}')
					[ _val = _1 ]
			| ("include" >> fList >> ';')
					[ _val = new_<Include>(_1) ]
			| ("break" >> fList >> ';')
					[ _val = new_<Break>(_1) ]
			| ("continue" >> fList >> ';')
					[ _val = new_<Continue>(_1) ]
			| ("return" >> fList >> ';')
					[ _val = new_<Return>(_1) ]
			| ("jumptoeof" >> fList >> ';')
					[ _val = new_<JumpToEof>(_1) ]
			| fIfStatement
					[ _val = _1 ]
			| fForStatement
					[ _val = _1 ]
			| fWhileStatement
					[ _val = _1 ]
			| fSwitchStatement
					[ _val = _1 ]
			| fOnStatement
					[ _val = _1 ]
			| fRuleDefinition
					[ _val = _1 ]
			| fActionsDefinition
					[ _val = _1 ]
			| fArgument [_val = _1]
				>> (("on" >> fList >> fAssignmentOperator >> fList)
						[ _val = new_<Assignment>(_val, _2, _3, _1) ]
					| (fAssignmentOperator >> fList)
						[ _val = new_<Assignment>(_val, _1, _2) ]
					| fListOfLists
						[ _val = new_<FunctionCall>(_val, _1) ]
				) >> ';'
		;

		fBlock
			= eps
					[ _val = new_<Block>() ]
				>> *(fStatement
						[ *_val += _1 ]
					| fLocalVariableDeclaration
						[ *_val += _1 ]
					)
		;

		fLocalVariableDeclaration
			= "local" >> fList
					[ _val = new_<LocalVariableDeclaration>(_1) ]
				>> -('=' >> fList
						[ bind(&LocalVariableDeclaration::SetInitializer,
							_val, _1) ]
					)
				>> ';'
		;

		fIfStatement
			= ("if" >> fExpression >> '{' >> fBlock >> '}')
					[ _val = new_<If>(_1, _2) ]
				>> -("else" >> fStatement
						[ bind(&If::SetElseBlock, _val, _1) ]
					)
		;

		fForStatement
			= ("for" >> fArgument >> "in" >> fList >> '{' >> fBlock >> '}')
					[ _val = new_<For>(_1, _2, _3) ]
		;

		fWhileStatement
			= ("while" >> fExpression >> '{' >> fBlock >> '}')
					[ _val = new_<While>(_1, _2) ]
		;

		fOnStatement
			= ("on" >> fArgument >> fStatement)
					[ _val = new_<OnExpression>(_1, _2) ]
		;

		fCaseStatement
			= ("case" >> fIdentifier >> ':' >> fBlock)
					[ _val = new_<Case>(_1, _2) ]
		;

		fSwitchStatement
			= ("switch" >> fList)
					[ _val = new_<Switch>(_1) ]
				>> '{'
				>> *(fCaseStatement
						[ bind(&Switch::AddCase, _val, _1) ]
					)
				>> '}'
		;

		fRuleDefinition
			= ("rule" >> fIdentifier)
					[ _val = new_<RuleDefinition>(_1)]
				>> -(fIdentifier
						[ bind(&RuleDefinition::AddParameterName, _val, _1) ]
					>> *(':'
						>> fIdentifier
								[ bind(&RuleDefinition::AddParameterName,
									_val, _1) ]
						)
					)
				>> '{'
				>> fBlock
						[ bind(&RuleDefinition::SetBlock, _val, _1) ]
				>> '}'
		;

		fActionsDefinition
			= ("actions" >> fActionsFlags >> fIdentifier >> fActionsBindList
				>> '{'
				>> raw[fActions]
				>> '}')
						[ _val = new_<ActionsDefinition>(_1, _2, _3,
							construct<std::string>(begin(_4), end(_4))) ]
		;

		fActionsFlags
			= *(fActionFlag
					[ _val |= _1 ]
// TODO: Support "maxline ARG"!
				)
		;

		fActionsBindList
			= -("bind" >> fList)
		;

		fActions
			= *(+(char_ - '{' - '}')
				| '{' >> fActions >> '}'
				)
// TODO: This is a very crude approximation. Jam does a mite better, since it
// does at least recognize (full line) comments. This should be improved!
		;

		fStart = fBlock;

#if 1
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
		qi::debug(fBracketExpression);
		qi::debug(fFunctionCall);
		qi::debug(fBracketOnExpression);
		qi::debug(fOnStatement);
		qi::debug(fExpression);
		qi::debug(fListOfLists);
		qi::debug(fList);
		qi::debug(fActionsFlags);
		qi::debug(fActionsBindList);
		qi::debug(fActions);
		qi::debug(fAtom);
		qi::debug(fArgument);
		qi::debug(fIdentifier);
		qi::debug(fString);
		qi::debug(fSubString);
#endif
	}

private:
	qi::rule<Iterator, Node*(), Skipper> fStart;
	qi::rule<Iterator, Block*(), Skipper> fBlock;
	qi::rule<Iterator, LocalVariableDeclaration*(), Skipper>
		fLocalVariableDeclaration;
	qi::rule<Iterator, RuleDefinition*(), Skipper> fRuleDefinition;
	qi::rule<Iterator, ActionsDefinition*(), Skipper> fActionsDefinition;
	qi::rule<Iterator, If*(), Skipper> fIfStatement;
	qi::rule<Iterator, For*(), Skipper> fForStatement;
	qi::rule<Iterator, While*(), Skipper> fWhileStatement;
	qi::rule<Iterator, Switch*(), Skipper> fSwitchStatement;
	qi::rule<Iterator, Case*(), Skipper> fCaseStatement;
	qi::rule<Iterator, Node*(), Skipper> fStatement;
	qi::rule<Iterator, Node*(), Skipper> fBracketExpression;
	qi::rule<Iterator, FunctionCall*(), Skipper> fFunctionCall;
	qi::rule<Iterator, OnExpression*(), Skipper> fBracketOnExpression;
	qi::rule<Iterator, OnExpression*(), Skipper> fOnStatement;
	qi::rule<Iterator, Node*(), Skipper> fExpression;
	qi::rule<Iterator, ListList(), Skipper> fListOfLists;
	qi::rule<Iterator, List*(), Skipper> fList;
	qi::rule<Iterator, Node*(), Skipper> fAtom;
	qi::rule<Iterator, Node*(), Skipper> fArgument;
	qi::rule<Iterator, uint32_t(), Skipper> fActionsFlags;
	qi::rule<Iterator, List*(), Skipper> fActionsBindList;
	qi::rule<Iterator, qi::unused_type> fActions;
	qi::rule<Iterator, String()> fIdentifier;
	qi::rule<Iterator, String()> fString;
	qi::rule<Iterator, String()> fSubString;
	qi::rule<Iterator, char()> fQuotedChar;
	qi::rule<Iterator, char()> fUnquotedChar;
	qi::rule<Iterator, char()> fEscapedChar;
	qi::symbols<char, unsigned> fListDelimiter;
	qi::symbols<char, uint32_t> fActionFlag;
	qi::symbols<char, AssignmentOperator> fAssignmentOperator;
};


int
main(int argc, const char* const* argv)
{
	std::noskipws(std::cin);

	typedef std::istream_iterator<char> BaseIteratorType;

	// convert input iterator to forward iterator, usable by spirit parser
#if 1
	typedef boost::spirit::multi_pass<BaseIteratorType> IteratorType;
	IteratorType begin = boost::spirit::make_default_multi_pass(
		BaseIteratorType(std::cin));
	IteratorType end;

#else
//const char* kTestString = "SubDir HAM_TOP src ;";
//const char* kTestString = "1 in x y z = a = B";
//const char* kTestString = "! x < 1 in a = B";
//const char* kTestString = "! [ myRule x : y z ] < 1 in a = B";
const char* kTestString =
"{ }\n"
"include foo ;\n"
"for myVar in $(bar) {\n"
"	if $(a) in $(something) $(else) {\n"
"		break ;\n"
"	}\n"
"	if $(foo) {\n"
"		continue ab ;\n"
"		while $(a) < $(b) {\n"
"			return c d ef ;\n"
"		}\n"
"	} else {\n"
"		jumptoeof xyz ;\n"
"	}\n"
"}\n"
"switch $(HAIKU_DISTRO_COMPATIBILITY) {\n"
"	case official	: {\n"
"	}\n"
"}\n"
/*
"switch $(HAIKU_DISTRO_COMPATIBILITY) {\n"
"	case official	: {\n"
"		HAIKU_DEFINES += HAIKU_DISTRO_COMPATIBILITY_OFFICIAL ;\n"
"		HAIKU_INCLUDE_TRADEMARKS = \"\" ;\n"
"	}\n"
"	case compatible	: {\n"
"		HAIKU_DEFINES += HAIKU_DISTRO_COMPATIBILITY_COMPATIBLE ;\n"
"		HAIKU_INCLUDE_TRADEMARKS = \"\" ;\n"
"	}\n"
"	case \"default\"	: {\n"
"		HAIKU_DEFINES += HAIKU_DISTRO_COMPATIBILITY_DEFAULT ;\n"
"		HAIKU_INCLUDE_TRADEMARKS = ;\n"
"	}\n"
"	case *			: Exit \"Invalid value for HAIKU_DISTRO_COMPATIBILITY:\"\n"
"						$(HAIKU_DISTRO_COMPATIBILITY) ;\n"
"}\n"
*/
;
	typedef const char* IteratorType;
	IteratorType begin = kTestString;
	IteratorType end = begin + strlen(begin);
#endif

	std::cout << "Parsing: \"";
	for (IteratorType it = begin; it != end; ++it)
		std::cout << *it;
	std::cout << "\" ...\n";

	Node* result = NULL;
	bool r = qi::phrase_parse(begin, end,
		HamParser<IteratorType, HamSkipParser<IteratorType> >(),
		HamSkipParser<IteratorType>(), result);

	if (r && begin == end) {
		std::cout << "-------------------------\n";
		std::cout << "Parsing succeeded\n";
		DumpContext dumpContext;
		result->Dump(dumpContext);
		std::cout << "-------------------------\n";
	} else {
		String rest(begin, end);
		std::cout << "-------------------------\n";
		std::cout << "Parsing failed\n";
		std::cout << "stopped at: \"" << rest << "\"\n";
		std::cout << "-------------------------\n";
if (result != NULL) {
DumpContext dumpContext;
result->Dump(dumpContext);
}
	}

	return 0;
}
