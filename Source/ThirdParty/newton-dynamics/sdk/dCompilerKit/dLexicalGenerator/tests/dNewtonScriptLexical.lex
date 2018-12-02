
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
#include "dScriptCompiler.h"
#include "dNewtonScriptParser.h"

//
// Newton Script lexer 
// loosely based on a subset of Java and C sharp
//
%}


WhiteSpace		[ \t\n\r]+

AnyButAstr		[^\*]
AnyButSlash		[^\/]
Comment1        [\/][\/].*
Comment3        [\/][\*]({AnyButAstr}|[\*]{AnyButSlash})*[\*][\/]
Comment			({Comment1}|{Comment2})


Indetifier		[a-zA-Z_][0-9a-zA-Z_]*
ImportFile		<{Indetifier}([\.]{Indetifier})*>


Integer			[\-\+]?[0-9]+
//Float			{Integer}[\.][0-9]+(e{Integer})?



%%


{WhiteSpace}	{/* skip is a white space*/}
{Comment}		{/* skip commnets */}
";"				{/* comma are no mandatory by tehy can be use for historical reasons */}

"="				{return '=';}
":"				{return ':';}
","				{return ',';}
"{"				{return '{';}
"}"				{return '}';}
"\("			{return '(';}
"\)"			{return ')';}

"bool"			{return dNewtonScriptParser::BOOLEAN;}
"byte"			{return dNewtonScriptParser::BYTE;}
"short"			{return dNewtonScriptParser::SHORT;}
"int"			{return dNewtonScriptParser::INT;}
"long"			{return dNewtonScriptParser::LONG;}
"unsigned"		{return dNewtonScriptParser::UNSIGNED;}
"float"			{return dNewtonScriptParser::FLOAT;}
"double"		{return dNewtonScriptParser::DOUBLE;}

"gui"			{return dNewtonScriptParser::GUI;}
"extends"		{return dNewtonScriptParser::EXTENDS;}
"private"		{return dNewtonScriptParser::PRIVATE;}
"class"			{return dNewtonScriptParser::CLASS;}
"import"		{return dNewtonScriptParser::IMPORT;}


{Integer}		{return dNewtonScriptParser::INTEGER_VALUE;}
{Indetifier}	{return dNewtonScriptParser::IDENTIFIER;}
{ImportFile}	{return dNewtonScriptParser::IMPORT_FILE_NAME;}









