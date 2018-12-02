
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
#include <dAssemblerCompiler.h>
#include "dAssemblerParser.h"

//
// Newton virtual machine assembler grammar
// based loosely on the MIPS R3000 and the Intel 386 instructions sets 
//
%}


WhiteSpace		[ \t\n\r]+

AnyButAstr		[^\*]
AnyButSlash		[^\/]
Comment1        [\/][\/].*
Comment2        [;].*
Comment3        [\/][\*]({AnyButAstr}|[\*]{AnyButSlash})*[\*][\/]
Comment			({Comment1}|{Comment2}|{Comment3})


Integer			[\-\+]?[0-9]+
//Float			{Integer}[\.][0-9]+(e{Integer})?

Literal			[a-zA-Z_][0-9a-zA-Z_]*
ImportFile		<{Literal}([\.]{Literal})*>

LocalLabel		{Literal}[:]

Register		[rR][0-9]+

mov				[mM][oO][vV]
add				[aA][dD][dD]
sub				[sS][uU][bB]
addi			[aA][dD][dD][iI]

loadd			[lL][oO][aA][dD][dD]

ret				[rR][eE][tT]
call			[cC][aA][lL][lL]
callr			[cC][aA][lL][lL][rR]

pop				[pP][oO][pP]
push			[pP][uU][sS][hH]

beq				[bB][eE][qQ]	
bne				[bB][nN][eE]	
blt				[bB][lL][tT]	
ble				[bB][lL][eT]	
bgt				[bB][gG][tT]	
bge				[bB][gG][eE]



%%
{WhiteSpace}	{/* skip is a white space*/}
{Comment}		{/* skip commnets */}

{Integer}		{return dAssemblerParser::INTEGER;}
{Literal}		{return dAssemblerParser::LITERAL;}
{Register}		{return dAssemblerParser::REGISTER;}
{LocalLabel}		{return dAssemblerParser::LOCALLABEL;}
{ImportFile}	{return dAssemblerParser::IMPORT_FILENAME;}


","				{return ',';}
"="				{return '=';}

//"<"				{return '<';}
//">"				{return '>';}
//"{"				{return '{';}
//"}"				{return '}';}
//"\["			{return '[';}
//"\]"			{return ']';}
//"\."			{return '.';}

"int"			{return dAssemblerParser::INT;}
"import"		{return dAssemblerParser::IMPORT;}
"private"		{return dAssemblerParser::PRIVATE;}
"begin"			{return dAssemblerParser::BEGIN;}
"end"			{return dAssemblerParser::END;}

{loadd}			{return dAssemblerParser::LOADD;}
{mov}			{return dAssemblerParser::MOVE;}
{addi}			{return dAssemblerParser::ADDI;}
{add}			{return dAssemblerParser::ADD;}
{sub}			{return dAssemblerParser::SUB;}
{ret}			{return dAssemblerParser::RET;}
{call}			{return dAssemblerParser::CALL;}
{callr}			{return dAssemblerParser::CALLR;}
{pop}			{return dAssemblerParser::POP;}
{push}			{return dAssemblerParser::PUSH;}
{beq}			{return dAssemblerParser::BEQ;}
{bne}			{return dAssemblerParser::BNE;}
{blt}			{return dAssemblerParser::BLT;}
{ble}			{return dAssemblerParser::BLE;}
{bgt}			{return dAssemblerParser::BGT;}
{bge}			{return dAssemblerParser::BGE;}




