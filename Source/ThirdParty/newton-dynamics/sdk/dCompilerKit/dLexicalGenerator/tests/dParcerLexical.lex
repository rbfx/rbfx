
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
#include <dParcerCompiler.h>
%}

				
WhiteSpace			[ \t\n\r]+

AnyButAstr			[^\*]
AnyButSlash			[^\/]
Comment1			[\/][\/].*
Comment2			[\/][\*]({AnyButAstr}|[\*]{AnyButSlash})*[\*][\/]
//Comment				({Comment1}|{Comment2})
Comment			([\/][\*])

AnyButPercent		[^\%]
AnyButCloseCurly	[^\}]
CodeBlock			%[\{]({AnyButPercent}|%{AnyButCloseCurly})*%[\}]
Literal				[a-zA-Z_][0-9a-zA-Z_]*


%%
{WhiteSpace}	{/* skip is a white space*/}
{Comment}		{/* skip commnets */}



{WhiteSpace}		{}
{Comment}			{}
"%%"				{ return dParcerCompiler::GRAMMAR_SEGMENT;}
"%start"			{ return dParcerCompiler::START;}
"%token"			{ return dParcerCompiler::TOKEN;}
"%union"			{ return dParcerCompiler::UNION;}
"%left"				{ return dParcerCompiler::LEFT;}
"%right"			{ return dParcerCompiler::RIGHT;}
{Literal}			{ return dParcerCompiler::LITERAL;}
{CodeBlock}			{ m_tokenString.replace(0, 2, ""); m_tokenString.replace(m_tokenString.size() - 2, 2, ""); return dParcerCompiler::CODE_BLOCK;}

[|]					{ return(dParcerCompiler::OR); }
[:]					{ return(dParcerCompiler::COLOM); }
[;]					{ return(dParcerCompiler::SIMICOLOM); }
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
[{]					{ ReadBalancedExpresion ('{', '}'); return dParcerCompiler::USER_ACTION;}

