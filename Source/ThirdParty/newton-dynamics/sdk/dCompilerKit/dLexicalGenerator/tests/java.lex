
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
//
// Newton Scrip Lex parcel
// based on a subset of Java language specification 1.0 
//
%}

%{
#include <stdafx.h>
#include <scriptTokens.h>

void CopyLiteral ()
{
	//strcpy (Newtonlval.m_literal, yytext);
	CopyLable (Newtonlval.m_literal, yytext);
}

%}

%e 1600
%n 800
%p 5000

Separator		[\(\)\{\}\[\]\;\,\.]
Delimiter1		[\=\>\<\!\~\?\:\+\-\*\/\&\|\^\%]
HexDigit		[0-9a-fA-F]
Digit			[0-9]
OctalDigit		[0-7]
TetraDigit		[0-3]
NonZeroDigit	[1-9]
Letter			[a-zA-Z_]
AnyButSlash		[^\/]
AnyButAstr		[^\*]
BLANK			[ ]
BLK				[\b]
TAB				[\t]
FF				[\f]
ESCCHR			[\\]
CR				[\r]
LF				[\n]
UniEsc          [\1b]

OctEscape1		[\\]{OctalDigit}
OctEscape2		[\\]{OctalDigit}{OctalDigit}
OctEscape3		[\\]{TetraDigit}{OctalDigit}{OctalDigit}
OctEscape		({OctEscape1}|{OctEscape2}|{OctEscape3})

Escape			[\\]([r]|[n]|[b]|[f]|[t]|[\\]|[\']|[\"])
ULetter         ({Letter}|{UniEsc})
Identifier 		{ULetter}({ULetter}|{Digit})*

Comment1        [\/][\*]({AnyButAstr}|[\*]{AnyButSlash})*[\*][\/]
Comment2        [\/][\/].*
Comment			({Comment1}|{Comment2})

Dimension		\[({CR}|{LF}|{FF}|{TAB}|{BLK}|{BLANK}|{Comment})*\]

IntSuffix		([l]|[L])
DecimalNum		{NonZeroDigit}{Digit}*{IntSuffix}?
OctalNum		[0]{OctalDigit}*{IntSuffix}?
HexNum			[0]([x]|[X]){HexDigit}{HexDigit}*{IntSuffix}?
IntegerLiteral	({DecimalNum}|{OctalNum}|{HexNum})

Sign			([\+]|[\-])
FlSuffix		([f]|[F]|[d]|[D])
SignedInt		{Sign}?{Digit}+
Expo			([e]|[E])
ExponentPart	{Expo}{SignedInt}?
Float1          {Digit}+[\.]{Digit}+?{ExponentPart}?{FlSuffix}?
Float2			[\.]{Digit}+{ExponentPart}?{FlSuffix}?
Float3			{Digit}+{ExponentPart}{FlSuffix}?
Float4			{Digit}+{FlSuffix}
FloatingPoint	({Float1}|{Float2}|{Float3}|{Float4})

AnyChrChr		[^\\']
AnyStrChr		[^\\"]
Character		[\']({Escape}|{OctEscape}|{AnyChrChr})[\']
String			[\"]({Escape}|{OctEscape}|{AnyStrChr})*[\"]
Numeric  		({IntegerLiteral}|{FloatingPoint})
Literal			({Numeric}|{Character}|{String})

%%
"true"			{return BOOLLIT;}
"false"			{return BOOLLIT;}

{Separator}	{	return yytext[0];}
{Delimiter1}	{return yytext[0];}
{Dimension}		{return OP_DIM;}

"=="			{return OP_EQ;}
"<="			{return OP_LE;}
">="			{return OP_GE;}
"!="			{return OP_NE;}
"\|\|"			{return OP_LOR;}
"&&"			{return OP_LAND;}
"\+\+"			{return OP_INC;}
"--"			{return OP_DEC;}
">>"			{return OP_SHR;}
"<<"			{return OP_SHL;}
">>>"			{return OP_SHRR;}
"\+="			{return ASS_ADD;}
"-="			{return ASS_SUB;}
"\*="			{return ASS_MUL;}
"/="			{return ASS_DIV;}
"&="			{return ASS_AND;}
"\|="			{return ASS_OR;}
"^="			{return ASS_XOR;}
"%="			{return ASS_MOD;}
"<<="			{return ASS_SHL;}
">>="			{return ASS_SHR;}
">>>="			{return ASS_SHRR;}

"abstract"		{return ABSTRACT;}
"do"            {return DO;}
"implements"    {return IMPLEMENTS;}
"package"		{return PACKAGE;}
"throw"			{return THROW;}
"boolean"		{return BOOLEAN;}
"double"		{return DOUBLE;}
"import"		{return IMPORT;}
"private"		{return PRIVATE;}
"throws"		{return THROWS;}
"break"			{return BREAK;}
"else"			{return ELSE;}
"inner"			{return INNER;}
"protected"		{return PROTECTED;}
"transient"		{return TRANSIENT;}
"byte"			{return BYTE;}
"extends"		{return EXTENDS;}
"instanceof"	{return INSTANCEOF;}
"public"		{return PUBLIC;}
"try"			{return TRY;}
"case"			{return CASE;}
"final"			{return FINAL;}
"int"			{return INT;}
"rest"			{return REST;}
"var"			{return VAR;}
"cast"			{return CAST;}
"finally"		{return FINALLY;}
"interface"		{return INTERFACE;}
"return"		{return RETURN;}
"void"			{return VOID;}
"catch"			{return CATCH;}
"float"			{return FLOAT;}
"long"			{return LONG;}
"short"			{return SHORT;}
"volatile"		{return VOLATILE;}
"char"			{return CHAR;}
"for"			{return FOR;}
"native"		{return NATIVE;}
"static"		{return STATIC;}
"while"			{return WHILE;}
"class"			{return CLASS;}
"future"		{return FUTURE;}
"new"			{return NEW;}
"super"			{return SUPER;}
"const"			{return CONST;}
"generic"		{return GENERIC;}
"null"			{return JNULL;}
"switch"		{return SWITCH;}
"continue"		{return CONTINUE;}
"goto"			{return GOTO;}
"operator"		{return OPERATOR;}
"synchronized"	{return SYNCHRONIZED;}
"default"		{return DEFAULT;}
"if"			{return IF;}
"outer"			{return OUTER;}
"this"			{return THIS;}

{Identifier}	{return IDENTIFIER;}

{DecimalNum}    {return LITERAL;}
{OctalNum}      {return LITERAL;}
{HexNum}        {return LITERAL;}

{Float1}        {return LITERAL;}
{Float2}        {return LITERAL;}
{Float3}        {return LITERAL;}
{Float4}        {return LITERAL;}

{Character}     {return LITERAL;}
{String}		{return LITERAL;}

{CR}   			{}
{LF}			{}
{FF}			{}
{TAB}			{}
{BLK}           {}
{BLANK}			{}
{Comment}		{}

