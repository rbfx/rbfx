/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

%{
#include "dLSCstdafx.h"
#include "dLittleScriptParser.h"

//
// Newton Script lexer 
// loosely based on a subset of Java and C sharp
//
%}

WhiteSpace			[ \t\n\r]+
AnyButAstr			[^\*]
AnyButSlash			[^\/]
Comment1			[\/][\/].*
Comment2			[\/][\*]({AnyButAstr}|[\*]{AnyButSlash})*[\*][\/]
Comment				({Comment1}|{Comment2})

Indentifier			[a-zA-Z_][0-9a-zA-Z_]*

Integer				[0-9]+
Float				{Integer}[\.][0-9]+(e{Integer})?
string				["][^"]*["]
dimensionOp			[\[][ \t\n\r]*[\]]

%%
{WhiteSpace}		{/* skip is a white space*/}
{Comment}			{/* skip commnets */}

{Indentifier}		{m_tokenString = dString ('_') + m_tokenString; return dLittleScriptParser::_IDENTIFIER;}
{dimensionOp}		{return dLittleScriptParser::_OP_DIM;}

"\;"				{return ';';}
"\:"				{return ':';}
"\,"				{return ',';}
"\{"				{return '{';}
"\}"				{return '}';}
"\!"				{return '!';}
"\~"				{return '~';}
"\&"				{return '&';}
"\="				{return '=';}
"\/"				{return '/';}
"\%"				{return '%';}
"\+"				{return '+';}
"\-"				{return '-';}
"\*"				{return '*';}
"\."				{return '.';}
"\?"				{return '?';}
"\["				{return '[';}
"\]"				{return ']';}
"\("				{return '(';}
"\)"				{return ')';}
"\^"				{return '^';}
"\|"				{return '|';}
"\<"				{return '<';}
"\>"				{return '>';}

"\+\+"				{return dLittleScriptParser::_OP_INC;}
"\-\-"				{return dLittleScriptParser::_OP_DEC;}

"\*\="				{return dLittleScriptParser::_ASS_MUL;}
"\/\="				{return dLittleScriptParser::_ASS_DIV;}
"\%\="				{return dLittleScriptParser::_ASS_MOD;}
"\+\="				{return dLittleScriptParser::_ASS_ADD;}
"\-\="				{return dLittleScriptParser::_ASS_SUB;}
"\<\<\="			{return dLittleScriptParser::_ASS_SHL;}
"\>\>\="			{return dLittleScriptParser::_ASS_SHR;}
"\&\="				{return dLittleScriptParser::_ASS_AND;}
"\^\="				{return dLittleScriptParser::_ASS_XOR;}
"\|\="				{return dLittleScriptParser::_ASS_OR;}
"\<\="				{return dLittleScriptParser::_LESS_EQUAL;}
"\>\="				{return dLittleScriptParser::_GREATHER_EQUAL;}
"\=\="				{return dLittleScriptParser::_IDENTICAL;}
"\!\="				{return dLittleScriptParser::_DIFFERENT;}
"\&\&"				{return dLittleScriptParser::_LOGIC_AND;}
"\|\|"				{return dLittleScriptParser::_LOGIC_OR;}

// "::"				{return dLittleScriptParser::_DOUBLE_COLOM;}

// "enum"			{return dLittleScriptParser::_ENUM;}



// "operator"		{return dLittleScriptParser::_OPERATOR;}
// "sizeof"			{return dLittleScriptParser::_SIZEOF;}
// "cast"			{return dLittleScriptParser::_CAST;}
// "gui"			{return dLittleScriptParser::_GUI;}


// "extends"		{return dLittleScriptParser::_EXTENDS;}

// "base"			{return dLittleScriptParser::_BASE;}


{Integer}			{return dLittleScriptParser::_INTEGER_CONST;}
{Float}				{return dLittleScriptParser::_FLOAT_CONST;}
//{string}			{return dLittleScriptParser::_STRING_VALUE;}	

"if"				{return dLittleScriptParser::_IF;}
"else"				{return dLittleScriptParser::_ELSE;}
"switch"			{return dLittleScriptParser::_SWITCH;}
"new"				{return dLittleScriptParser::_NEW;}
"case"				{return dLittleScriptParser::_CASE;}
"default"			{return dLittleScriptParser::_DEFAULT;}
"do"				{return dLittleScriptParser::_DO;}
"for"				{return dLittleScriptParser::_FOR;}
"while"				{return dLittleScriptParser::_WHILE;}
"break"				{return dLittleScriptParser::_BREAK;}
"continue"			{return dLittleScriptParser::_CONTINUE;}
"return"			{return dLittleScriptParser::_RETURN;}

"void"				{return dLittleScriptParser::_VOID;}
"byte"				{return dLittleScriptParser::_BYTE;}
"int"				{return dLittleScriptParser::_INT;}
"short"				{return dLittleScriptParser::_SHORT;}
"long"				{return dLittleScriptParser::_LONG;}
"const"			    {return dLittleScriptParser::_CONST;}
"bool"				{return dLittleScriptParser::_BOOLEAN;}
"float"				{return dLittleScriptParser::_FLOAT;}
"double"			{return dLittleScriptParser::_DOUBLE;}

"this"				{return dLittleScriptParser::_THIS;}
"class"				{return dLittleScriptParser::_CLASS;}
"import"			{return dLittleScriptParser::_IMPORT;}
"package"			{return dLittleScriptParser::_PACKAGE;}
"final"				{return dLittleScriptParser::_FINAL;}
"public"			{return dLittleScriptParser::_PUBLIC;}
"private"			{return dLittleScriptParser::_PRIVATE;}
"static"			{return dLittleScriptParser::_STATIC;}
"native"			{return dLittleScriptParser::_NATIVE;}
"interface"			{return dLittleScriptParser::_INTERFACE;}

