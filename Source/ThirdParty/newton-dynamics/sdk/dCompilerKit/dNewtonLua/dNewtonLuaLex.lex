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
#include "dNewtonLuaStdafx.h"
#include "dNewtonLuaParcer.h"

//
// Newton Tool embedded Lua script Language
// based of https://www.lua.org/manual/5.3/manual.html#9
//
%}

whiteSpace						[ \t\n\r]+
Comment1						--\[\[([^\]])*\]\]
Comment2						--.*
Comment							({Comment1}|{Comment2})
string1							\"[^\"]*\"
string2							\'[^\']*\'
string3							\[\[([^\]])*\]\]
string3							\[\=\[([^\]])*\]\=\]
string4							\[\=\=\[([^\]])*\]\=\=\]
string							({string1}|{string2|{string3}|{string4})

Integer1						[0-9]+
Integer2						[\0](\x|\X)([0-9]|[a-f]|[A-F])+
Integer							({Integer1}|{Integer2})
Float							{Integer1}[\.][0-9]+([eE]\-?{Integer1})?
Label							[A-Za-z_][A-Za-z0-9_]*

%%
{whiteSpace}					{}
{Comment}						{}

{string}						{return dNewtonLuaParcer::_STRING;}
{Integer}						{return dNewtonLuaParcer::_INTEGER;}
{Float}							{return dNewtonLuaParcer::_FLOAT;}
{Label}							{return dNewtonLuaParcer::_LABEL;}

"and"							{return dNewtonLuaParcer::_AND;}
"break"							{return dNewtonLuaParcer::_BREAK;}
"do"							{return dNewtonLuaParcer::_DO;}
"else"							{return dNewtonLuaParcer::_ELSE;}
"elseif"						{return dNewtonLuaParcer::_ELSEIF;}
"end"							{return dNewtonLuaParcer::_END;}
"false"							{return dNewtonLuaParcer::_FALSE;}
"for"							{return dNewtonLuaParcer::_FOR;}
"function"						{return dNewtonLuaParcer::_FUNCTION;}
"goto"							{return dNewtonLuaParcer::_GOTO;}
"if"							{return dNewtonLuaParcer::_IF;}
"in"							{return dNewtonLuaParcer::_IN;}
"local"							{return dNewtonLuaParcer::_LOCAL;}
"nil"							{return dNewtonLuaParcer::_NIL;}
"not"							{return dNewtonLuaParcer::_NOT;}
"or"							{return dNewtonLuaParcer::_OR;}
"repeat"						{return dNewtonLuaParcer::_REPEAT;}
"return"						{return dNewtonLuaParcer::_RETURN;}
"then"							{return dNewtonLuaParcer::_THEN;}
"true"							{return dNewtonLuaParcer::_TRUE;}
"until"							{return dNewtonLuaParcer::_UNTIL;}
"while"							{return dNewtonLuaParcer::_WHILE;}
"\<\<"							{return dNewtonLuaParcer::_LEFT_SHIFT;}
"\>\>"							{return dNewtonLuaParcer::_RIGHT_SHIFT;}
"\/\/"							{return dNewtonLuaParcer::_INTEGER_DIVIDE;}
"\=\="							{return dNewtonLuaParcer::_IDENTICAL;}
"\~\="							{return dNewtonLuaParcer::_DIFFERENT;}
"\<\="							{return dNewtonLuaParcer::_LEFT_EQUAL;}
"\>\="							{return dNewtonLuaParcer::_GREATHER_EQUAL;}
"\:\:"							{return dNewtonLuaParcer::_DOUBLE_COLUMN;}
"\.\."							{return dNewtonLuaParcer::_DOUBLE_DOT;}
"\.\.\."						{return dNewtonLuaParcer::_TRIPLE_DOT;}

"\+"                            {return '+';}
"\-"                            {return '-';}
"\*"                            {return '*';}
"\/"                            {return '/';}
"\%"                            {return '%';}
"\^"                            {return '^';}
"\&"                            {return '&';}
"\~"                            {return '~';}
"\#"                            {return '#';}
"\|"                            {return '|';}
"\<"							{return '<';}
"\>"							{return '>';}
"\="							{return '=';}
"\("							{return '(';}
"\)"							{return ')';}
"\{"							{return '{';}
"\}"							{return '}';}
"\["							{return '[';}
"\]"							{return ']';}
"\;"							{return ';';}
"\:"							{return ':';}
"\,"							{return ',';}
"\."							{return '.';}







