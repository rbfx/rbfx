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
#include <dParserCompiler.h>
%}
				
WhiteSpace			[ \t\n\r]+
AnyButAstr			[^\*]
AnyButSlash			[^\/]
Comment1			[\/][\/].*
Comment2			[\/][\*]({AnyButAstr}|[\*]{AnyButSlash})*[\*][\/]
Comment				({Comment1}|{Comment2})

AnyButPercent		[^\%]
AnyButCloseCurly	[^\}]
CodeBlock			%[\{]({AnyButPercent}|%{AnyButCloseCurly})*%[\}]
Literal				[a-zA-Z_][0-9a-zA-Z_]*
Integer				[0-9]+

%%
{WhiteSpace}		{}
{Comment}			{}
"%%"				{ return dParserCompiler::GRAMMAR_SEGMENT;}
"%prec"				{ return dParserCompiler::PREC;}
"%start"			{ return dParserCompiler::START;}
"%token"			{ return dParserCompiler::TOKEN;}
"%union"			{ return dParserCompiler::UNION;}
"%left"				{ return dParserCompiler::LEFT;}
"%right"			{ return dParserCompiler::RIGHT;}
"%expect"			{ return dParserCompiler::EXPECT;}
{Literal}			{ return dParserCompiler::LITERAL;}
{Integer}			{ return dParserCompiler::INTEGER;}
{CodeBlock}			{ m_tokenString.Replace(0, 2, ""); m_tokenString.Replace(m_tokenString.Size() - 2, 2, ""); return dParserCompiler::CODE_BLOCK;}

[|]					{ return(dParserCompiler::OR); }
[:]					{ return(dParserCompiler::COLOM); }
[;]					{ return(dParserCompiler::SIMICOLOM); }
"';'"				{ m_tokenString = ";"; return(';'); }
"'{'"				{ m_tokenString = "{"; return('{'); }
"'}'"				{ m_tokenString = "}"; return('}'); }
"','"				{ m_tokenString = ","; return(','); }
"'='"				{ m_tokenString = "="; return('='); }
"'&'"				{ m_tokenString = "&"; return('&'); }
"'!'"				{ m_tokenString = "!"; return('!'); }
"'~'"				{ m_tokenString = "~"; return('~'); }
"'-'"				{ m_tokenString = "-"; return('-'); }
"'%'"				{ m_tokenString = "%"; return('%'); }
"'<'"				{ m_tokenString = "<"; return('<'); }
"'>'"				{ m_tokenString = ">"; return('>'); }
"'/'"				{ m_tokenString = "/"; return('/'); }
"'^'"				{ m_tokenString = "^"; return('^'); }
"'\:'"				{ m_tokenString = ":"; return(':'); }
"'\.'"				{ m_tokenString = "."; return('.'); }
"'\|'"				{ m_tokenString = "|"; return('|'); }
"'\?'"				{ m_tokenString = "?"; return('?'); }
"'\\'"				{ m_tokenString = "\\"; return('\\'); }
"'\('"				{ m_tokenString = "("; return('('); }
"'\)'"				{ m_tokenString = ")"; return(')'); }
"'\+'"				{ m_tokenString = "+"; return('+'); }
"'\*'"				{ m_tokenString = "*"; return('*'); }
"'\['"				{ m_tokenString = "["; return('['); }
"'\]'"				{ m_tokenString = "]"; return(']'); }
"'\#'"				{ m_tokenString = "#"; return('#'); }
'[A-Za-z]'			{ m_tokenString = m_tokenString.SubString (1, 1); return(m_tokenString[0]); }
[{]					{ ReadBalancedExpression ('{', '}'); return dParserCompiler::SEMANTIC_ACTION;}

