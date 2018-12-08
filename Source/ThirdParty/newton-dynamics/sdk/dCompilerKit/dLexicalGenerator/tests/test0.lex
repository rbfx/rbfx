
D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][\+\-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*
ANY			[0-9a-zA-Z_.]


%%
"int"				{ CopyLiteral (); return INT;}
"char"				{ CopyLiteral (); return CHAR;}
"void"				{ CopyLiteral (); return VOID;}
"short"				{ CopyLiteral (); return SHORT;}
"unsigned"          { CopyLiteral (); return UNSIGNED;}
"const"				{ CopyLiteral (); return CONST;}
"float"				{ CopyLable (Newtonlval.m_literal, "float"); return FLOAT;}
"dFloat"			{ CopyLable (Newtonlval.m_literal, "float"); return FLOAT;}
"double"			{ return DOUBLE;}
"struct"			{ return STRUCT;}
"union"				{ return UNION;}
"typedef"			{ return TYPEDEF;}

"#ifdef"		    { return IFDEF;}
"#ifndef"		    { return IFNDEF;}
"#else"			    { return ELSE;}
"#endif"			{ return ENDIF;}
"#define"			{ return DEFINE;}
"#include"			{ return INCLUDE;}





{L}({L}|{D})*		{ CopyLiteral (); return(LITERAL_IDENTIFIER); }
{D}({D})*			{ CopyLiteral (); return(NUMERIC_CONSTANT); }
[\"]({ANY})*[\"]	{ CopyLiteral (); return(QUOTED_CONSTANT); }


"\;"			{ return(';'); }
"\{"			{ return('{'); }
"\}"			{ return('}'); }
"\("			{ return('('); }
"\)"			{ return(')'); }
"\,"			{ return(','); }
"\:"			{ return(':'); }
"\="			{ return('='); }

"\["			{ return('['); }
"\]"			{ return(']'); }
"\."			{ return('.'); }
"\&"			{ return('&'); }
"\!"			{ return('!'); }
"\~"			{ return('~'); }
"\-"			{ return('-'); }
"\+"			{ return('+'); }
"\*"			{ return('*'); }
"\/"			{ return('/'); }
"\%"			{ return('%'); }
"\<"			{ return('<'); }
"\>"			{ return('>'); }
"\^"			{ return('^'); }
"\|"			{ return('|'); }
"\?"			{ return('?'); }


[ \t\v\n\f]*	{}
.				{ /* ignore bad characters */ }
"//".*			{ /* ignore bad characters */ }	

%%

int yywrap()
{
//	yy_delete_buffer (yy_current_buffer);
	return(1);
}

void dgScript_cleanup()
{
	yy_delete_buffer (yy_current_buffer);
}





