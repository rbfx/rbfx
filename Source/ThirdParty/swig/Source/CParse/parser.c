/* A Bison parser, made by GNU Bison 3.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 25 "CParse/parser.y" /* yacc.c:338  */

#define yylex yylex

#include "swig.h"
#include "cparse.h"
#include "preprocessor.h"
#include <ctype.h>

/* We do this for portability */
#undef alloca
#define alloca malloc

/* -----------------------------------------------------------------------------
 *                               Externals
 * ----------------------------------------------------------------------------- */

int  yyparse();

/* NEW Variables */

static Node    *top = 0;      /* Top of the generated parse tree */
static int      unnamed = 0;  /* Unnamed datatype counter */
static Hash    *classes = 0;        /* Hash table of classes */
static Hash    *classes_typedefs = 0; /* Hash table of typedef classes: typedef struct X {...} Y; */
static Symtab  *prev_symtab = 0;
static Node    *current_class = 0;
String  *ModuleName = 0;
static Node    *module_node = 0;
static String  *Classprefix = 0;  
static String  *Namespaceprefix = 0;
static int      inclass = 0;
static Node    *currentOuterClass = 0; /* for nested classes */
static const char *last_cpptype = 0;
static int      inherit_list = 0;
static Parm    *template_parameters = 0;
static int      parsing_template_declaration = 0;
static int      extendmode   = 0;
static int      compact_default_args = 0;
static int      template_reduce = 0;
static int      cparse_externc = 0;
int		ignore_nested_classes = 0;
int		kwargs_supported = 0;
/* -----------------------------------------------------------------------------
 *                            Doxygen Comment Globals
 * ----------------------------------------------------------------------------- */
static String *currentDeclComment = NULL; /* Comment of C/C++ declaration. */
static Node *previousNode = NULL; /* Pointer to the previous node (for post comments) */
static Node *currentNode = NULL; /* Pointer to the current node (for post comments) */

/* -----------------------------------------------------------------------------
 *                            Assist Functions
 * ----------------------------------------------------------------------------- */


 
/* Called by the parser (yyparse) when an error is found.*/
static void yyerror (const char *e) {
  (void)e;
}

static Node *new_node(const_String_or_char_ptr tag) {
  Node *n = Swig_cparse_new_node(tag);
  /* Remember the previous node in case it will need a post-comment */
  previousNode = currentNode;
  currentNode = n;
  return n;
}

/* Copies a node.  Does not copy tree links or symbol table data (except for
   sym:name) */

static Node *copy_node(Node *n) {
  Node *nn;
  Iterator k;
  nn = NewHash();
  Setfile(nn,Getfile(n));
  Setline(nn,Getline(n));
  for (k = First(n); k.key; k = Next(k)) {
    String *ci;
    String *key = k.key;
    char *ckey = Char(key);
    if ((strcmp(ckey,"nextSibling") == 0) ||
	(strcmp(ckey,"previousSibling") == 0) ||
	(strcmp(ckey,"parentNode") == 0) ||
	(strcmp(ckey,"lastChild") == 0)) {
      continue;
    }
    if (Strncmp(key,"csym:",5) == 0) continue;
    /* We do copy sym:name.  For templates */
    if ((strcmp(ckey,"sym:name") == 0) || 
	(strcmp(ckey,"sym:weak") == 0) ||
	(strcmp(ckey,"sym:typename") == 0)) {
      String *ci = Copy(k.item);
      Setattr(nn,key, ci);
      Delete(ci);
      continue;
    }
    if (strcmp(ckey,"sym:symtab") == 0) {
      Setattr(nn,"sym:needs_symtab", "1");
    }
    /* We don't copy any other symbol table attributes */
    if (strncmp(ckey,"sym:",4) == 0) {
      continue;
    }
    /* If children.  We copy them recursively using this function */
    if (strcmp(ckey,"firstChild") == 0) {
      /* Copy children */
      Node *cn = k.item;
      while (cn) {
	Node *copy = copy_node(cn);
	appendChild(nn,copy);
	Delete(copy);
	cn = nextSibling(cn);
      }
      continue;
    }
    /* We don't copy the symbol table.  But we drop an attribute 
       requires_symtab so that functions know it needs to be built */

    if (strcmp(ckey,"symtab") == 0) {
      /* Node defined a symbol table. */
      Setattr(nn,"requires_symtab","1");
      continue;
    }
    /* Can't copy nodes */
    if (strcmp(ckey,"node") == 0) {
      continue;
    }
    if ((strcmp(ckey,"parms") == 0) || (strcmp(ckey,"pattern") == 0) || (strcmp(ckey,"throws") == 0)
	|| (strcmp(ckey,"kwargs") == 0)) {
      ParmList *pl = CopyParmList(k.item);
      Setattr(nn,key,pl);
      Delete(pl);
      continue;
    }
    if (strcmp(ckey,"nested:outer") == 0) { /* don't copy outer classes links, they will be updated later */
      Setattr(nn, key, k.item);
      continue;
    }
    /* defaultargs will be patched back in later in update_defaultargs() */
    if (strcmp(ckey,"defaultargs") == 0) {
      Setattr(nn, "needs_defaultargs", "1");
      continue;
    }
    /* Looks okay.  Just copy the data using Copy */
    ci = Copy(k.item);
    Setattr(nn, key, ci);
    Delete(ci);
  }
  return nn;
}

static void set_comment(Node *n, String *comment) {
  String *name;
  Parm *p;
  if (!n || !comment)
    return;

  if (Getattr(n, "doxygen"))
    Append(Getattr(n, "doxygen"), comment);
  else {
    Setattr(n, "doxygen", comment);
    /* This is the first comment, populate it with @params, if any */
    p = Getattr(n, "parms");
    while (p) {
      if (Getattr(p, "doxygen"))
	Printv(comment, "\n@param ", Getattr(p, "name"), Getattr(p, "doxygen"), NIL);
      p=nextSibling(p);
    }
  }
  
  /* Append same comment to every generated overload */
  name = Getattr(n, "name");
  if (!name)
    return;
  n = nextSibling(n);
  while (n && Getattr(n, "name") && Strcmp(Getattr(n, "name"), name) == 0) {
    Setattr(n, "doxygen", comment);
    n = nextSibling(n);
  }
}

/* -----------------------------------------------------------------------------
 *                              Variables
 * ----------------------------------------------------------------------------- */

static char  *typemap_lang = 0;    /* Current language setting */

static int cplus_mode  = 0;

/* C++ modes */

#define  CPLUS_PUBLIC    1
#define  CPLUS_PRIVATE   2
#define  CPLUS_PROTECTED 3

/* include types */
static int   import_mode = 0;

void SWIG_typemap_lang(const char *tm_lang) {
  typemap_lang = Swig_copy_string(tm_lang);
}

void SWIG_cparse_set_compact_default_args(int defargs) {
  compact_default_args = defargs;
}

int SWIG_cparse_template_reduce(int treduce) {
  template_reduce = treduce;
  return treduce;  
}

/* -----------------------------------------------------------------------------
 *                           Assist functions
 * ----------------------------------------------------------------------------- */

static int promote_type(int t) {
  if (t <= T_UCHAR || t == T_CHAR || t == T_WCHAR) return T_INT;
  return t;
}

/* Perform type-promotion for binary operators */
static int promote(int t1, int t2) {
  t1 = promote_type(t1);
  t2 = promote_type(t2);
  return t1 > t2 ? t1 : t2;
}

static String *yyrename = 0;

/* Forward renaming operator */

static String *resolve_create_node_scope(String *cname, int is_class_definition);


Hash *Swig_cparse_features(void) {
  static Hash   *features_hash = 0;
  if (!features_hash) features_hash = NewHash();
  return features_hash;
}

/* Fully qualify any template parameters */
static String *feature_identifier_fix(String *s) {
  String *tp = SwigType_istemplate_templateprefix(s);
  if (tp) {
    String *ts, *ta, *tq;
    ts = SwigType_templatesuffix(s);
    ta = SwigType_templateargs(s);
    tq = Swig_symbol_type_qualify(ta,0);
    Append(tp,tq);
    Append(tp,ts);
    Delete(ts);
    Delete(ta);
    Delete(tq);
    return tp;
  } else {
    return NewString(s);
  }
}

static void set_access_mode(Node *n) {
  if (cplus_mode == CPLUS_PUBLIC)
    Setattr(n, "access", "public");
  else if (cplus_mode == CPLUS_PROTECTED)
    Setattr(n, "access", "protected");
  else
    Setattr(n, "access", "private");
}

static void restore_access_mode(Node *n) {
  String *mode = Getattr(n, "access");
  if (Strcmp(mode, "private") == 0)
    cplus_mode = CPLUS_PRIVATE;
  else if (Strcmp(mode, "protected") == 0)
    cplus_mode = CPLUS_PROTECTED;
  else
    cplus_mode = CPLUS_PUBLIC;
}

/* Generate the symbol table name for an object */
/* This is a bit of a mess. Need to clean up */
static String *add_oldname = 0;



static String *make_name(Node *n, String *name,SwigType *decl) {
  String *made_name = 0;
  int destructor = name && (*(Char(name)) == '~');

  if (yyrename) {
    String *s = NewString(yyrename);
    Delete(yyrename);
    yyrename = 0;
    if (destructor  && (*(Char(s)) != '~')) {
      Insert(s,0,"~");
    }
    return s;
  }

  if (!name) return 0;

  if (parsing_template_declaration)
    SetFlag(n, "parsing_template_declaration");
  made_name = Swig_name_make(n, Namespaceprefix, name, decl, add_oldname);
  Delattr(n, "parsing_template_declaration");

  return made_name;
}

/* Generate an unnamed identifier */
static String *make_unnamed() {
  unnamed++;
  return NewStringf("$unnamed%d$",unnamed);
}

/* Return if the node is a friend declaration */
static int is_friend(Node *n) {
  return Cmp(Getattr(n,"storage"),"friend") == 0;
}

static int is_operator(String *name) {
  return Strncmp(name,"operator ", 9) == 0;
}


/* Add declaration list to symbol table */
static int  add_only_one = 0;

static void add_symbols(Node *n) {
  String *decl;
  String *wrn = 0;

  if (inclass && n) {
    cparse_normalize_void(n);
  }
  while (n) {
    String *symname = 0;
    /* for friends, we need to pop the scope once */
    String *old_prefix = 0;
    Symtab *old_scope = 0;
    int isfriend = inclass && is_friend(n);
    int iscdecl = Cmp(nodeType(n),"cdecl") == 0;
    int only_csymbol = 0;
    
    if (inclass) {
      String *name = Getattr(n, "name");
      if (isfriend) {
	/* for friends, we need to add the scopename if needed */
	String *prefix = name ? Swig_scopename_prefix(name) : 0;
	old_prefix = Namespaceprefix;
	old_scope = Swig_symbol_popscope();
	Namespaceprefix = Swig_symbol_qualifiedscopename(0);
	if (!prefix) {
	  if (name && !is_operator(name) && Namespaceprefix) {
	    String *nname = NewStringf("%s::%s", Namespaceprefix, name);
	    Setattr(n,"name",nname);
	    Delete(nname);
	  }
	} else {
	  Symtab *st = Swig_symbol_getscope(prefix);
	  String *ns = st ? Getattr(st,"name") : prefix;
	  String *base  = Swig_scopename_last(name);
	  String *nname = NewStringf("%s::%s", ns, base);
	  Setattr(n,"name",nname);
	  Delete(nname);
	  Delete(base);
	  Delete(prefix);
	}
	Namespaceprefix = 0;
      } else {
	/* for member functions, we need to remove the redundant
	   class scope if provided, as in
	   
	   struct Foo {
	   int Foo::method(int a);
	   };
	   
	*/
	String *prefix = name ? Swig_scopename_prefix(name) : 0;
	if (prefix) {
	  if (Classprefix && (Equal(prefix,Classprefix))) {
	    String *base = Swig_scopename_last(name);
	    Setattr(n,"name",base);
	    Delete(base);
	  }
	  Delete(prefix);
	}
      }
    }

    if (!isfriend && (inclass || extendmode)) {
      Setattr(n,"ismember","1");
    }

    if (extendmode) {
      if (!Getattr(n, "template"))
        SetFlag(n,"isextendmember");
    }

    if (!isfriend && inclass) {
      if ((cplus_mode != CPLUS_PUBLIC)) {
	only_csymbol = 1;
	if (cplus_mode == CPLUS_PROTECTED) {
	  Setattr(n,"access", "protected");
	  only_csymbol = !Swig_need_protected(n);
	} else {
	  Setattr(n,"access", "private");
	  /* private are needed only when they are pure virtuals - why? */
	  if ((Cmp(Getattr(n,"storage"),"virtual") == 0) && (Cmp(Getattr(n,"value"),"0") == 0)) {
	    only_csymbol = 0;
	  }
	  if (Cmp(nodeType(n),"destructor") == 0) {
	    /* Needed for "unref" feature */
	    only_csymbol = 0;
	  }
	}
      } else {
	  Setattr(n,"access", "public");
      }
    }
    if (Getattr(n,"sym:name")) {
      n = nextSibling(n);
      continue;
    }
    decl = Getattr(n,"decl");
    if (!SwigType_isfunction(decl)) {
      String *name = Getattr(n,"name");
      String *makename = Getattr(n,"parser:makename");
      if (iscdecl) {	
	String *storage = Getattr(n, "storage");
	if (Cmp(storage,"typedef") == 0) {
	  Setattr(n,"kind","typedef");
	} else {
	  SwigType *type = Getattr(n,"type");
	  String *value = Getattr(n,"value");
	  Setattr(n,"kind","variable");
	  if (value && Len(value)) {
	    Setattr(n,"hasvalue","1");
	  }
	  if (type) {
	    SwigType *ty;
	    SwigType *tmp = 0;
	    if (decl) {
	      ty = tmp = Copy(type);
	      SwigType_push(ty,decl);
	    } else {
	      ty = type;
	    }
	    if (!SwigType_ismutable(ty) || (storage && Strstr(storage, "constexpr"))) {
	      SetFlag(n,"hasconsttype");
	      SetFlag(n,"feature:immutable");
	    }
	    if (tmp) Delete(tmp);
	  }
	  if (!type) {
	    Printf(stderr,"notype name %s\n", name);
	  }
	}
      }
      Swig_features_get(Swig_cparse_features(), Namespaceprefix, name, 0, n);
      if (makename) {
	symname = make_name(n, makename,0);
        Delattr(n,"parser:makename"); /* temporary information, don't leave it hanging around */
      } else {
        makename = name;
	symname = make_name(n, makename,0);
      }
      
      if (!symname) {
	symname = Copy(Getattr(n,"unnamed"));
      }
      if (symname) {
	if (parsing_template_declaration)
	  SetFlag(n, "parsing_template_declaration");
	wrn = Swig_name_warning(n, Namespaceprefix, symname,0);
	Delattr(n, "parsing_template_declaration");
      }
    } else {
      String *name = Getattr(n,"name");
      SwigType *fdecl = Copy(decl);
      SwigType *fun = SwigType_pop_function(fdecl);
      if (iscdecl) {	
	Setattr(n,"kind","function");
      }
      
      Swig_features_get(Swig_cparse_features(),Namespaceprefix,name,fun,n);

      symname = make_name(n, name,fun);
      if (parsing_template_declaration)
	SetFlag(n, "parsing_template_declaration");
      wrn = Swig_name_warning(n, Namespaceprefix,symname,fun);
      Delattr(n, "parsing_template_declaration");
      
      Delete(fdecl);
      Delete(fun);
      
    }
    if (!symname) {
      n = nextSibling(n);
      continue;
    }
    if (cparse_cplusplus) {
      String *value = Getattr(n, "value");
      if (value && Strcmp(value, "delete") == 0) {
	/* C++11 deleted definition / deleted function */
        SetFlag(n,"deleted");
        SetFlag(n,"feature:ignore");
      }
      if (SwigType_isrvalue_reference(Getattr(n, "refqualifier"))) {
	/* Ignore rvalue ref-qualifiers by default
	 * Use Getattr instead of GetFlag to handle explicit ignore and explicit not ignore */
	if (!(Getattr(n, "feature:ignore") || Strncmp(symname, "$ignore", 7) == 0)) {
	  SWIG_WARN_NODE_BEGIN(n);
	  Swig_warning(WARN_TYPE_RVALUE_REF_QUALIFIER_IGNORED, Getfile(n), Getline(n),
	      "Method with rvalue ref-qualifier %s ignored.\n", Swig_name_decl(n));
	  SWIG_WARN_NODE_END(n);
	  SetFlag(n, "feature:ignore");
	}
      }
    }
    if (only_csymbol || GetFlag(n, "feature:ignore") || Strncmp(symname, "$ignore", 7) == 0) {
      /* Only add to C symbol table and continue */
      Swig_symbol_add(0, n);
      if (!only_csymbol && !GetFlag(n, "feature:ignore")) {
	/* Print the warning attached to $ignore name, if any */
        char *c = Char(symname) + 7;
	if (strlen(c)) {
	  SWIG_WARN_NODE_BEGIN(n);
	  Swig_warning(0,Getfile(n), Getline(n), "%s\n",c+1);
	  SWIG_WARN_NODE_END(n);
	}
	/* If the symbol was ignored via "rename" and is visible, set also feature:ignore*/
	SetFlag(n, "feature:ignore");
      }
      if (!GetFlag(n, "feature:ignore") && Strcmp(symname,"$ignore") == 0) {
	/* Add feature:ignore if the symbol was explicitly ignored, regardless of visibility */
	SetFlag(n, "feature:ignore");
      }
    } else {
      Node *c;
      if ((wrn) && (Len(wrn))) {
	String *metaname = symname;
	if (!Getmeta(metaname,"already_warned")) {
	  SWIG_WARN_NODE_BEGIN(n);
	  Swig_warning(0,Getfile(n),Getline(n), "%s\n", wrn);
	  SWIG_WARN_NODE_END(n);
	  Setmeta(metaname,"already_warned","1");
	}
      }
      c = Swig_symbol_add(symname,n);

      if (c != n) {
        /* symbol conflict attempting to add in the new symbol */
        if (Getattr(n,"sym:weak")) {
          Setattr(n,"sym:name",symname);
        } else {
          String *e = NewStringEmpty();
          String *en = NewStringEmpty();
          String *ec = NewStringEmpty();
          int redefined = Swig_need_redefined_warn(n,c,inclass);
          if (redefined) {
            Printf(en,"Identifier '%s' redefined (ignored)",symname);
            Printf(ec,"previous definition of '%s'",symname);
          } else {
            Printf(en,"Redundant redeclaration of '%s'",symname);
            Printf(ec,"previous declaration of '%s'",symname);
          }
          if (Cmp(symname,Getattr(n,"name"))) {
            Printf(en," (Renamed from '%s')", SwigType_namestr(Getattr(n,"name")));
          }
          Printf(en,",");
          if (Cmp(symname,Getattr(c,"name"))) {
            Printf(ec," (Renamed from '%s')", SwigType_namestr(Getattr(c,"name")));
          }
          Printf(ec,".");
	  SWIG_WARN_NODE_BEGIN(n);
          if (redefined) {
            Swig_warning(WARN_PARSE_REDEFINED,Getfile(n),Getline(n),"%s\n",en);
            Swig_warning(WARN_PARSE_REDEFINED,Getfile(c),Getline(c),"%s\n",ec);
          } else if (!is_friend(n) && !is_friend(c)) {
            Swig_warning(WARN_PARSE_REDUNDANT,Getfile(n),Getline(n),"%s\n",en);
            Swig_warning(WARN_PARSE_REDUNDANT,Getfile(c),Getline(c),"%s\n",ec);
          }
	  SWIG_WARN_NODE_END(n);
          Printf(e,"%s:%d:%s\n%s:%d:%s\n",Getfile(n),Getline(n),en,
                 Getfile(c),Getline(c),ec);
          Setattr(n,"error",e);
	  Delete(e);
          Delete(en);
          Delete(ec);
        }
      }
    }
    /* restore the class scope if needed */
    if (isfriend) {
      Swig_symbol_setscope(old_scope);
      if (old_prefix) {
	Delete(Namespaceprefix);
	Namespaceprefix = old_prefix;
      }
    }
    Delete(symname);

    if (add_only_one) return;
    n = nextSibling(n);
  }
}


/* add symbols a parse tree node copy */

static void add_symbols_copy(Node *n) {
  String *name;
  int    emode = 0;
  while (n) {
    char *cnodeType = Char(nodeType(n));

    if (strcmp(cnodeType,"access") == 0) {
      String *kind = Getattr(n,"kind");
      if (Strcmp(kind,"public") == 0) {
	cplus_mode = CPLUS_PUBLIC;
      } else if (Strcmp(kind,"private") == 0) {
	cplus_mode = CPLUS_PRIVATE;
      } else if (Strcmp(kind,"protected") == 0) {
	cplus_mode = CPLUS_PROTECTED;
      }
      n = nextSibling(n);
      continue;
    }

    add_oldname = Getattr(n,"sym:name");
    if ((add_oldname) || (Getattr(n,"sym:needs_symtab"))) {
      int old_inclass = -1;
      Node *old_current_class = 0;
      if (add_oldname) {
	DohIncref(add_oldname);
	/*  Disable this, it prevents %rename to work with templates */
	/* If already renamed, we used that name  */
	/*
	if (Strcmp(add_oldname, Getattr(n,"name")) != 0) {
	  Delete(yyrename);
	  yyrename = Copy(add_oldname);
	}
	*/
      }
      Delattr(n,"sym:needs_symtab");
      Delattr(n,"sym:name");

      add_only_one = 1;
      add_symbols(n);

      if (Getattr(n,"partialargs")) {
	Swig_symbol_cadd(Getattr(n,"partialargs"),n);
      }
      add_only_one = 0;
      name = Getattr(n,"name");
      if (Getattr(n,"requires_symtab")) {
	Swig_symbol_newscope();
	Swig_symbol_setscopename(name);
	Delete(Namespaceprefix);
	Namespaceprefix = Swig_symbol_qualifiedscopename(0);
      }
      if (strcmp(cnodeType,"class") == 0) {
	old_inclass = inclass;
	inclass = 1;
	old_current_class = current_class;
	current_class = n;
	if (Strcmp(Getattr(n,"kind"),"class") == 0) {
	  cplus_mode = CPLUS_PRIVATE;
	} else {
	  cplus_mode = CPLUS_PUBLIC;
	}
      }
      if (strcmp(cnodeType,"extend") == 0) {
	emode = cplus_mode;
	cplus_mode = CPLUS_PUBLIC;
      }
      add_symbols_copy(firstChild(n));
      if (strcmp(cnodeType,"extend") == 0) {
	cplus_mode = emode;
      }
      if (Getattr(n,"requires_symtab")) {
	Setattr(n,"symtab", Swig_symbol_popscope());
	Delattr(n,"requires_symtab");
	Delete(Namespaceprefix);
	Namespaceprefix = Swig_symbol_qualifiedscopename(0);
      }
      if (add_oldname) {
	Delete(add_oldname);
	add_oldname = 0;
      }
      if (strcmp(cnodeType,"class") == 0) {
	inclass = old_inclass;
	current_class = old_current_class;
      }
    } else {
      if (strcmp(cnodeType,"extend") == 0) {
	emode = cplus_mode;
	cplus_mode = CPLUS_PUBLIC;
      }
      add_symbols_copy(firstChild(n));
      if (strcmp(cnodeType,"extend") == 0) {
	cplus_mode = emode;
      }
    }
    n = nextSibling(n);
  }
}

/* Add in the "defaultargs" attribute for functions in instantiated templates.
 * n should be any instantiated template (class or start of linked list of functions). */
static void update_defaultargs(Node *n) {
  if (n) {
    Node *firstdefaultargs = n;
    update_defaultargs(firstChild(n));
    n = nextSibling(n);
    /* recursively loop through nodes of all types, but all we really need are the overloaded functions */
    while (n) {
      update_defaultargs(firstChild(n));
      if (!Getattr(n, "defaultargs")) {
	if (Getattr(n, "needs_defaultargs")) {
	  Setattr(n, "defaultargs", firstdefaultargs);
	  Delattr(n, "needs_defaultargs");
	} else {
	  firstdefaultargs = n;
	}
      } else {
	/* Functions added in with %extend (for specialized template classes) will already have default args patched up */
	assert(Getattr(n, "defaultargs") == firstdefaultargs);
      }
      n = nextSibling(n);
    }
  }
}

/* Check a set of declarations to see if any are pure-abstract */

static List *pure_abstracts(Node *n) {
  List *abstracts = 0;
  while (n) {
    if (Cmp(nodeType(n),"cdecl") == 0) {
      String *decl = Getattr(n,"decl");
      if (SwigType_isfunction(decl)) {
	String *init = Getattr(n,"value");
	if (Cmp(init,"0") == 0) {
	  if (!abstracts) {
	    abstracts = NewList();
	  }
	  Append(abstracts,n);
	  SetFlag(n,"abstract");
	}
      }
    } else if (Cmp(nodeType(n),"destructor") == 0) {
      if (Cmp(Getattr(n,"value"),"0") == 0) {
	if (!abstracts) {
	  abstracts = NewList();
	}
	Append(abstracts,n);
	SetFlag(n,"abstract");
      }
    }
    n = nextSibling(n);
  }
  return abstracts;
}

/* Make a classname */

static String *make_class_name(String *name) {
  String *nname = 0;
  String *prefix;
  if (Namespaceprefix) {
    nname= NewStringf("%s::%s", Namespaceprefix, name);
  } else {
    nname = NewString(name);
  }
  prefix = SwigType_istemplate_templateprefix(nname);
  if (prefix) {
    String *args, *qargs;
    args   = SwigType_templateargs(nname);
    qargs  = Swig_symbol_type_qualify(args,0);
    Append(prefix,qargs);
    Delete(nname);
    Delete(args);
    Delete(qargs);
    nname = prefix;
  }
  return nname;
}

/* Use typedef name as class name */

static void add_typedef_name(Node *n, Node *declnode, String *oldName, Symtab *cscope, String *scpname) {
  String *class_rename = 0;
  SwigType *decl = Getattr(declnode, "decl");
  if (!decl || !Len(decl)) {
    String *cname;
    String *tdscopename;
    String *class_scope = Swig_symbol_qualifiedscopename(cscope);
    String *name = Getattr(declnode, "name");
    cname = Copy(name);
    Setattr(n, "tdname", cname);
    tdscopename = class_scope ? NewStringf("%s::%s", class_scope, name) : Copy(name);
    class_rename = Getattr(n, "class_rename");
    if (class_rename && (Strcmp(class_rename, oldName) == 0))
      Setattr(n, "class_rename", NewString(name));
    if (!classes_typedefs) classes_typedefs = NewHash();
    if (!Equal(scpname, tdscopename) && !Getattr(classes_typedefs, tdscopename)) {
      Setattr(classes_typedefs, tdscopename, n);
    }
    Setattr(n, "decl", decl);
    Delete(class_scope);
    Delete(cname);
    Delete(tdscopename);
  }
}

/* If the class name is qualified.  We need to create or lookup namespace entries */

static Symtab *set_scope_to_global() {
  Symtab *symtab = Swig_symbol_global_scope();
  Swig_symbol_setscope(symtab);
  return symtab;
}
 
/* Remove the block braces, { and }, if the 'noblock' attribute is set.
 * Node *kw can be either a Hash or Parmlist. */
static String *remove_block(Node *kw, const String *inputcode) {
  String *modified_code = 0;
  while (kw) {
   String *name = Getattr(kw,"name");
   if (name && (Cmp(name,"noblock") == 0)) {
     char *cstr = Char(inputcode);
     int len = Len(inputcode);
     if (len && cstr[0] == '{') {
       --len; ++cstr; 
       if (len && cstr[len - 1] == '}') { --len; }
       /* we now remove the extra spaces */
       while (len && isspace((int)cstr[0])) { --len; ++cstr; }
       while (len && isspace((int)cstr[len - 1])) { --len; }
       modified_code = NewStringWithSize(cstr, len);
       break;
     }
   }
   kw = nextSibling(kw);
  }
  return modified_code;
}

/*
#define RESOLVE_DEBUG 1
*/
static Node *nscope = 0;
static Node *nscope_inner = 0;

/* Remove the scope prefix from cname and return the base name without the prefix.
 * The scopes required for the symbol name are resolved and/or created, if required.
 * For example AA::BB::CC as input returns CC and creates the namespace AA then inner 
 * namespace BB in the current scope. */
static String *resolve_create_node_scope(String *cname, int is_class_definition) {
  Symtab *gscope = 0;
  Node *cname_node = 0;
  String *last = Swig_scopename_last(cname);
  nscope = 0;
  nscope_inner = 0;  

  if (Strncmp(cname,"::" ,2) != 0) {
    if (is_class_definition) {
      /* Only lookup symbols which are in scope via a using declaration but not via a using directive.
         For example find y via 'using x::y' but not y via a 'using namespace x'. */
      cname_node = Swig_symbol_clookup_no_inherit(cname, 0);
      if (!cname_node) {
	Node *full_lookup_node = Swig_symbol_clookup(cname, 0);
	if (full_lookup_node) {
	 /* This finds a symbol brought into scope via both a using directive and a using declaration. */
	  Node *last_node = Swig_symbol_clookup_no_inherit(last, 0);
	  if (last_node == full_lookup_node)
	    cname_node = last_node;
	}
      }
    } else {
      /* For %template, the template needs to be in scope via any means. */
      cname_node = Swig_symbol_clookup(cname, 0);
    }
  }
#if RESOLVE_DEBUG
  if (!cname_node)
    Printf(stdout, "symbol does not yet exist (%d): [%s]\n", is_class_definition, cname);
  else
    Printf(stdout, "symbol does exist (%d): [%s]\n", is_class_definition, cname);
#endif

  if (cname_node) {
    /* The symbol has been defined already or is in another scope.
       If it is a weak symbol, it needs replacing and if it was brought into the current scope,
       the scope needs adjusting appropriately for the new symbol.
       Similarly for defined templates. */
    Symtab *symtab = Getattr(cname_node, "sym:symtab");
    Node *sym_weak = Getattr(cname_node, "sym:weak");
    if ((symtab && sym_weak) || Equal(nodeType(cname_node), "template")) {
      /* Check if the scope is the current scope */
      String *current_scopename = Swig_symbol_qualifiedscopename(0);
      String *found_scopename = Swig_symbol_qualifiedscopename(symtab);
      if (!current_scopename)
	current_scopename = NewString("");
      if (!found_scopename)
	found_scopename = NewString("");

      {
	int fail = 1;
	List *current_scopes = Swig_scopename_tolist(current_scopename);
	List *found_scopes = Swig_scopename_tolist(found_scopename);
        Iterator cit = First(current_scopes);
	Iterator fit = First(found_scopes);
#if RESOLVE_DEBUG
Printf(stdout, "comparing current: [%s] found: [%s]\n", current_scopename, found_scopename);
#endif
	for (; fit.item && cit.item; fit = Next(fit), cit = Next(cit)) {
	  String *current = cit.item;
	  String *found = fit.item;
#if RESOLVE_DEBUG
	  Printf(stdout, "  looping %s %s\n", current, found);
#endif
	  if (Strcmp(current, found) != 0)
	    break;
	}

	if (!cit.item) {
	  String *subscope = NewString("");
	  for (; fit.item; fit = Next(fit)) {
	    if (Len(subscope) > 0)
	      Append(subscope, "::");
	    Append(subscope, fit.item);
	  }
	  if (Len(subscope) > 0)
	    cname = NewStringf("%s::%s", subscope, last);
	  else
	    cname = Copy(last);
#if RESOLVE_DEBUG
	  Printf(stdout, "subscope to create: [%s] cname: [%s]\n", subscope, cname);
#endif
	  fail = 0;
	  Delete(subscope);
	} else {
	  if (is_class_definition) {
	    if (!fit.item) {
	      /* It is valid to define a new class with the same name as one forward declared in a parent scope */
	      fail = 0;
	    } else if (Swig_scopename_check(cname)) {
	      /* Classes defined with scope qualifiers must have a matching forward declaration in matching scope */
	      fail = 1;
	    } else {
	      /* This may let through some invalid cases */
	      fail = 0;
	    }
#if RESOLVE_DEBUG
	    Printf(stdout, "scope for class definition, fail: %d\n", fail);
#endif
	  } else {
#if RESOLVE_DEBUG
	    Printf(stdout, "no matching base scope for template\n");
#endif
	    fail = 1;
	  }
	}

	Delete(found_scopes);
	Delete(current_scopes);

	if (fail) {
	  String *cname_resolved = NewStringf("%s::%s", found_scopename, last);
	  Swig_error(cparse_file, cparse_line, "'%s' resolves to '%s' and was incorrectly instantiated in scope '%s' instead of within scope '%s'.\n", cname, cname_resolved, current_scopename, found_scopename);
	  cname = Copy(last);
	  Delete(cname_resolved);
	}
      }

      Delete(current_scopename);
      Delete(found_scopename);
    }
  } else if (!is_class_definition) {
    /* A template instantiation requires a template to be found in scope... fail here too?
    Swig_error(cparse_file, cparse_line, "No template found to instantiate '%s' with %%template.\n", cname);
     */
  }

  if (Swig_scopename_check(cname)) {
    Node   *ns;
    String *prefix = Swig_scopename_prefix(cname);
    if (prefix && (Strncmp(prefix,"::",2) == 0)) {
/* I don't think we can use :: global scope to declare classes and hence neither %template. - consider reporting error instead - wsfulton. */
      /* Use the global scope */
      String *nprefix = NewString(Char(prefix)+2);
      Delete(prefix);
      prefix= nprefix;
      gscope = set_scope_to_global();
    }
    if (Len(prefix) == 0) {
      String *base = Copy(last);
      /* Use the global scope, but we need to add a 'global' namespace.  */
      if (!gscope) gscope = set_scope_to_global();
      /* note that this namespace is not the "unnamed" one,
	 and we don't use Setattr(nscope,"name", ""),
	 because the unnamed namespace is private */
      nscope = new_node("namespace");
      Setattr(nscope,"symtab", gscope);;
      nscope_inner = nscope;
      Delete(last);
      return base;
    }
    /* Try to locate the scope */
    ns = Swig_symbol_clookup(prefix,0);
    if (!ns) {
      Swig_error(cparse_file,cparse_line,"Undefined scope '%s'\n", prefix);
    } else {
      Symtab *nstab = Getattr(ns,"symtab");
      if (!nstab) {
	Swig_error(cparse_file,cparse_line, "'%s' is not defined as a valid scope.\n", prefix);
	ns = 0;
      } else {
	/* Check if the node scope is the current scope */
	String *tname = Swig_symbol_qualifiedscopename(0);
	String *nname = Swig_symbol_qualifiedscopename(nstab);
	if (tname && (Strcmp(tname,nname) == 0)) {
	  ns = 0;
	  cname = Copy(last);
	}
	Delete(tname);
	Delete(nname);
      }
      if (ns) {
	/* we will try to create a new node using the namespaces we
	   can find in the scope name */
	List *scopes = Swig_scopename_tolist(prefix);
	String *sname;
	Iterator si;

	for (si = First(scopes); si.item; si = Next(si)) {
	  Node *ns1,*ns2;
	  sname = si.item;
	  ns1 = Swig_symbol_clookup(sname,0);
	  assert(ns1);
	  if (Strcmp(nodeType(ns1),"namespace") == 0) {
	    if (Getattr(ns1,"alias")) {
	      ns1 = Getattr(ns1,"namespace");
	    }
	  } else {
	    /* now this last part is a class */
	    si = Next(si);
	    /*  or a nested class tree, which is unrolled here */
	    for (; si.item; si = Next(si)) {
	      if (si.item) {
		Printf(sname,"::%s",si.item);
	      }
	    }
	    /* we get the 'inner' class */
	    nscope_inner = Swig_symbol_clookup(sname,0);
	    /* set the scope to the inner class */
	    Swig_symbol_setscope(Getattr(nscope_inner,"symtab"));
	    /* save the last namespace prefix */
	    Delete(Namespaceprefix);
	    Namespaceprefix = Swig_symbol_qualifiedscopename(0);
	    /* and return the node name, including the inner class prefix */
	    break;
	  }
	  /* here we just populate the namespace tree as usual */
	  ns2 = new_node("namespace");
	  Setattr(ns2,"name",sname);
	  Setattr(ns2,"symtab", Getattr(ns1,"symtab"));
	  add_symbols(ns2);
	  Swig_symbol_setscope(Getattr(ns1,"symtab"));
	  Delete(Namespaceprefix);
	  Namespaceprefix = Swig_symbol_qualifiedscopename(0);
	  if (nscope_inner) {
	    if (Getattr(nscope_inner,"symtab") != Getattr(ns2,"symtab")) {
	      appendChild(nscope_inner,ns2);
	      Delete(ns2);
	    }
	  }
	  nscope_inner = ns2;
	  if (!nscope) nscope = ns2;
	}
	cname = Copy(last);
	Delete(scopes);
      }
    }
    Delete(prefix);
  }
  Delete(last);

  return cname;
}
 
/* look for simple typedef name in typedef list */
static String *try_to_find_a_name_for_unnamed_structure(const char *storage, Node *decls) {
  String *name = 0;
  Node *n = decls;
  if (storage && (strcmp(storage, "typedef") == 0)) {
    for (; n; n = nextSibling(n)) {
      if (!Len(Getattr(n, "decl"))) {
	name = Copy(Getattr(n, "name"));
	break;
      }
    }
  }
  return name;
}

/* traverse copied tree segment, and update outer class links*/
static void update_nested_classes(Node *n)
{
  Node *c = firstChild(n);
  while (c) {
    if (Getattr(c, "nested:outer"))
      Setattr(c, "nested:outer", n);
    update_nested_classes(c);
    c = nextSibling(c);
  }
}

/* -----------------------------------------------------------------------------
 * nested_forward_declaration()
 * 
 * Nested struct handling for C++ code if the nested classes are disabled.
 * Create the nested class/struct/union as a forward declaration.
 * ----------------------------------------------------------------------------- */

static Node *nested_forward_declaration(const char *storage, const char *kind, String *sname, String *name, Node *cpp_opt_declarators) {
  Node *nn = 0;

  if (sname) {
    /* Add forward declaration of the nested type */
    Node *n = new_node("classforward");
    Setattr(n, "kind", kind);
    Setattr(n, "name", sname);
    Setattr(n, "storage", storage);
    Setattr(n, "sym:weak", "1");
    add_symbols(n);
    nn = n;
  }

  /* Add any variable instances. Also add in any further typedefs of the nested type.
     Note that anonymous typedefs (eg typedef struct {...} a, b;) are treated as class forward declarations */
  if (cpp_opt_declarators) {
    int storage_typedef = (storage && (strcmp(storage, "typedef") == 0));
    int variable_of_anonymous_type = !sname && !storage_typedef;
    if (!variable_of_anonymous_type) {
      int anonymous_typedef = !sname && (storage && (strcmp(storage, "typedef") == 0));
      Node *n = cpp_opt_declarators;
      SwigType *type = name;
      while (n) {
	Setattr(n, "type", type);
	Setattr(n, "storage", storage);
	if (anonymous_typedef) {
	  Setattr(n, "nodeType", "classforward");
	  Setattr(n, "sym:weak", "1");
	}
	n = nextSibling(n);
      }
      add_symbols(cpp_opt_declarators);

      if (nn) {
	set_nextSibling(nn, cpp_opt_declarators);
      } else {
	nn = cpp_opt_declarators;
      }
    }
  }

  if (!currentOuterClass || !GetFlag(currentOuterClass, "nested")) {
    if (nn && Equal(nodeType(nn), "classforward")) {
      Node *n = nn;
      if (!GetFlag(n, "feature:ignore")) {
	SWIG_WARN_NODE_BEGIN(n);
	Swig_warning(WARN_PARSE_NAMED_NESTED_CLASS, cparse_file, cparse_line,"Nested %s not currently supported (%s ignored)\n", kind, sname ? sname : name);
	SWIG_WARN_NODE_END(n);
      }
    } else {
      Swig_warning(WARN_PARSE_UNNAMED_NESTED_CLASS, cparse_file, cparse_line, "Nested %s not currently supported (ignored).\n", kind);
    }
  }

  return nn;
}


Node *Swig_cparse(File *f) {
  scanner_file(f);
  top = 0;
  yyparse();
  return top;
}

static void single_new_feature(const char *featurename, String *val, Hash *featureattribs, char *declaratorid, SwigType *type, ParmList *declaratorparms, String *qualifier) {
  String *fname;
  String *name;
  String *fixname;
  SwigType *t = Copy(type);

  /* Printf(stdout, "single_new_feature: [%s] [%s] [%s] [%s] [%s] [%s]\n", featurename, val, declaratorid, t, ParmList_str_defaultargs(declaratorparms), qualifier); */

  /* Warn about deprecated features */
  if (strcmp(featurename, "nestedworkaround") == 0)
    Swig_warning(WARN_DEPRECATED_NESTED_WORKAROUND, cparse_file, cparse_line, "The 'nestedworkaround' feature is deprecated.\n");

  fname = NewStringf("feature:%s",featurename);
  if (declaratorid) {
    fixname = feature_identifier_fix(declaratorid);
  } else {
    fixname = NewStringEmpty();
  }
  if (Namespaceprefix) {
    name = NewStringf("%s::%s",Namespaceprefix, fixname);
  } else {
    name = fixname;
  }

  if (declaratorparms) Setmeta(val,"parms",declaratorparms);
  if (!Len(t)) t = 0;
  if (t) {
    if (qualifier) SwigType_push(t,qualifier);
    if (SwigType_isfunction(t)) {
      SwigType *decl = SwigType_pop_function(t);
      if (SwigType_ispointer(t)) {
	String *nname = NewStringf("*%s",name);
	Swig_feature_set(Swig_cparse_features(), nname, decl, fname, val, featureattribs);
	Delete(nname);
      } else {
	Swig_feature_set(Swig_cparse_features(), name, decl, fname, val, featureattribs);
      }
      Delete(decl);
    } else if (SwigType_ispointer(t)) {
      String *nname = NewStringf("*%s",name);
      Swig_feature_set(Swig_cparse_features(),nname,0,fname,val, featureattribs);
      Delete(nname);
    }
  } else {
    /* Global feature, that is, feature not associated with any particular symbol */
    Swig_feature_set(Swig_cparse_features(),name,0,fname,val, featureattribs);
  }
  Delete(fname);
  Delete(name);
}

/* Add a new feature to the Hash. Additional features are added if the feature has a parameter list (declaratorparms)
 * and one or more of the parameters have a default argument. An extra feature is added for each defaulted parameter,
 * simulating the equivalent overloaded method. */
static void new_feature(const char *featurename, String *val, Hash *featureattribs, char *declaratorid, SwigType *type, ParmList *declaratorparms, String *qualifier) {

  ParmList *declparms = declaratorparms;

  /* remove the { and } braces if the noblock attribute is set */
  String *newval = remove_block(featureattribs, val);
  val = newval ? newval : val;

  /* Add the feature */
  single_new_feature(featurename, val, featureattribs, declaratorid, type, declaratorparms, qualifier);

  /* Add extra features if there are default parameters in the parameter list */
  if (type) {
    while (declparms) {
      if (ParmList_has_defaultargs(declparms)) {

        /* Create a parameter list for the new feature by copying all
           but the last (defaulted) parameter */
        ParmList* newparms = CopyParmListMax(declparms, ParmList_len(declparms)-1);

        /* Create new declaration - with the last parameter removed */
        SwigType *newtype = Copy(type);
        Delete(SwigType_pop_function(newtype)); /* remove the old parameter list from newtype */
        SwigType_add_function(newtype,newparms);

        single_new_feature(featurename, Copy(val), featureattribs, declaratorid, newtype, newparms, qualifier);
        declparms = newparms;
      } else {
        declparms = 0;
      }
    }
  }
}

/* check if a function declaration is a plain C object */
static int is_cfunction(Node *n) {
  if (!cparse_cplusplus || cparse_externc)
    return 1;
  if (Swig_storage_isexternc(n)) {
    return 1;
  }
  return 0;
}

/* If the Node is a function with parameters, check to see if any of the parameters
 * have default arguments. If so create a new function for each defaulted argument. 
 * The additional functions form a linked list of nodes with the head being the original Node n. */
static void default_arguments(Node *n) {
  Node *function = n;

  if (function) {
    ParmList *varargs = Getattr(function,"feature:varargs");
    if (varargs) {
      /* Handles the %varargs directive by looking for "feature:varargs" and 
       * substituting ... with an alternative set of arguments.  */
      Parm     *p = Getattr(function,"parms");
      Parm     *pp = 0;
      while (p) {
	SwigType *t = Getattr(p,"type");
	if (Strcmp(t,"v(...)") == 0) {
	  if (pp) {
	    ParmList *cv = Copy(varargs);
	    set_nextSibling(pp,cv);
	    Delete(cv);
	  } else {
	    ParmList *cv =  Copy(varargs);
	    Setattr(function,"parms", cv);
	    Delete(cv);
	  }
	  break;
	}
	pp = p;
	p = nextSibling(p);
      }
    }

    /* Do not add in functions if kwargs is being used or if user wants old default argument wrapping
       (one wrapped method per function irrespective of number of default arguments) */
    if (compact_default_args 
	|| is_cfunction(function) 
	|| GetFlag(function,"feature:compactdefaultargs") 
	|| (GetFlag(function,"feature:kwargs") && kwargs_supported)) {
      ParmList *p = Getattr(function,"parms");
      if (p) 
        Setattr(p,"compactdefargs", "1"); /* mark parameters for special handling */
      function = 0; /* don't add in extra methods */
    }
  }

  while (function) {
    ParmList *parms = Getattr(function,"parms");
    if (ParmList_has_defaultargs(parms)) {

      /* Create a parameter list for the new function by copying all
         but the last (defaulted) parameter */
      ParmList* newparms = CopyParmListMax(parms,ParmList_len(parms)-1);

      /* Create new function and add to symbol table */
      {
	SwigType *ntype = Copy(nodeType(function));
	char *cntype = Char(ntype);
        Node *new_function = new_node(ntype);
        SwigType *decl = Copy(Getattr(function,"decl"));
        int constqualifier = SwigType_isconst(decl);
	String *ccode = Copy(Getattr(function,"code"));
	String *cstorage = Copy(Getattr(function,"storage"));
	String *cvalue = Copy(Getattr(function,"value"));
	SwigType *ctype = Copy(Getattr(function,"type"));
	String *cthrow = Copy(Getattr(function,"throw"));

        Delete(SwigType_pop_function(decl)); /* remove the old parameter list from decl */
        SwigType_add_function(decl,newparms);
        if (constqualifier)
          SwigType_add_qualifier(decl,"const");

        Setattr(new_function,"name", Getattr(function,"name"));
        Setattr(new_function,"code", ccode);
        Setattr(new_function,"decl", decl);
        Setattr(new_function,"parms", newparms);
        Setattr(new_function,"storage", cstorage);
        Setattr(new_function,"value", cvalue);
        Setattr(new_function,"type", ctype);
        Setattr(new_function,"throw", cthrow);

	Delete(ccode);
	Delete(cstorage);
	Delete(cvalue);
	Delete(ctype);
	Delete(cthrow);
	Delete(decl);

        {
          Node *throws = Getattr(function,"throws");
	  ParmList *pl = CopyParmList(throws);
          if (throws) Setattr(new_function,"throws",pl);
	  Delete(pl);
        }

        /* copy specific attributes for global (or in a namespace) template functions - these are not templated class methods */
        if (strcmp(cntype,"template") == 0) {
          Node *templatetype = Getattr(function,"templatetype");
          Node *symtypename = Getattr(function,"sym:typename");
          Parm *templateparms = Getattr(function,"templateparms");
          if (templatetype) {
	    Node *tmp = Copy(templatetype);
	    Setattr(new_function,"templatetype",tmp);
	    Delete(tmp);
	  }
          if (symtypename) {
	    Node *tmp = Copy(symtypename);
	    Setattr(new_function,"sym:typename",tmp);
	    Delete(tmp);
	  }
          if (templateparms) {
	    Parm *tmp = CopyParmList(templateparms);
	    Setattr(new_function,"templateparms",tmp);
	    Delete(tmp);
	  }
        } else if (strcmp(cntype,"constructor") == 0) {
          /* only copied for constructors as this is not a user defined feature - it is hard coded in the parser */
          if (GetFlag(function,"feature:new")) SetFlag(new_function,"feature:new");
        }

        add_symbols(new_function);
        /* mark added functions as ones with overloaded parameters and point to the parsed method */
        Setattr(new_function,"defaultargs", n);

        /* Point to the new function, extending the linked list */
        set_nextSibling(function, new_function);
	Delete(new_function);
        function = new_function;
	
	Delete(ntype);
      }
    } else {
      function = 0;
    }
  }
}

/* -----------------------------------------------------------------------------
 * mark_nodes_as_extend()
 *
 * Used by the %extend to mark subtypes with "feature:extend".
 * template instances declared within %extend are skipped
 * ----------------------------------------------------------------------------- */

static void mark_nodes_as_extend(Node *n) {
  for (; n; n = nextSibling(n)) {
    if (Getattr(n, "template") && Strcmp(nodeType(n), "class") == 0)
      continue;
    /* Fix me: extend is not a feature. Replace with isextendmember? */
    Setattr(n, "feature:extend", "1");
    mark_nodes_as_extend(firstChild(n));
  }
}

/* -----------------------------------------------------------------------------
 * add_qualifier_to_declarator()
 *
 * Normally the qualifier is pushed on to the front of the type.
 * Adding a qualifier to a pointer to member function is a special case.
 * For example       : typedef double (Cls::*pmf)(void) const;
 * The qualifier is  : q(const).
 * The declarator is : m(Cls).f(void).
 * We need           : m(Cls).q(const).f(void).
 * ----------------------------------------------------------------------------- */

static String *add_qualifier_to_declarator(SwigType *type, SwigType *qualifier) {
  int is_pointer_to_member_function = 0;
  String *decl = Copy(type);
  String *poppedtype = NewString("");
  assert(qualifier);

  while (decl) {
    if (SwigType_ismemberpointer(decl)) {
      String *memberptr = SwigType_pop(decl);
      if (SwigType_isfunction(decl)) {
	is_pointer_to_member_function = 1;
	SwigType_push(decl, qualifier);
	SwigType_push(decl, memberptr);
	Insert(decl, 0, poppedtype);
	Delete(memberptr);
	break;
      } else {
	Append(poppedtype, memberptr);
      }
      Delete(memberptr);
    } else {
      String *popped = SwigType_pop(decl);
      if (!popped)
	break;
      Append(poppedtype, popped);
      Delete(popped);
    }
  }

  if (!is_pointer_to_member_function) {
    Delete(decl);
    decl = Copy(type);
    SwigType_push(decl, qualifier);
  }

  Delete(poppedtype);
  return decl;
}


#line 1565 "CParse/parser.c" /* yacc.c:338  */
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_CPARSE_PARSER_H_INCLUDED
# define YY_YY_CPARSE_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ID = 258,
    HBLOCK = 259,
    POUND = 260,
    STRING = 261,
    WSTRING = 262,
    INCLUDE = 263,
    IMPORT = 264,
    INSERT = 265,
    CHARCONST = 266,
    WCHARCONST = 267,
    NUM_INT = 268,
    NUM_FLOAT = 269,
    NUM_UNSIGNED = 270,
    NUM_LONG = 271,
    NUM_ULONG = 272,
    NUM_LONGLONG = 273,
    NUM_ULONGLONG = 274,
    NUM_BOOL = 275,
    TYPEDEF = 276,
    TYPE_INT = 277,
    TYPE_UNSIGNED = 278,
    TYPE_SHORT = 279,
    TYPE_LONG = 280,
    TYPE_FLOAT = 281,
    TYPE_DOUBLE = 282,
    TYPE_CHAR = 283,
    TYPE_WCHAR = 284,
    TYPE_VOID = 285,
    TYPE_SIGNED = 286,
    TYPE_BOOL = 287,
    TYPE_COMPLEX = 288,
    TYPE_TYPEDEF = 289,
    TYPE_RAW = 290,
    TYPE_NON_ISO_INT8 = 291,
    TYPE_NON_ISO_INT16 = 292,
    TYPE_NON_ISO_INT32 = 293,
    TYPE_NON_ISO_INT64 = 294,
    LPAREN = 295,
    RPAREN = 296,
    COMMA = 297,
    SEMI = 298,
    EXTERN = 299,
    INIT = 300,
    LBRACE = 301,
    RBRACE = 302,
    PERIOD = 303,
    CONST_QUAL = 304,
    VOLATILE = 305,
    REGISTER = 306,
    STRUCT = 307,
    UNION = 308,
    EQUAL = 309,
    SIZEOF = 310,
    MODULE = 311,
    LBRACKET = 312,
    RBRACKET = 313,
    BEGINFILE = 314,
    ENDOFFILE = 315,
    ILLEGAL = 316,
    CONSTANT = 317,
    NAME = 318,
    RENAME = 319,
    NAMEWARN = 320,
    EXTEND = 321,
    PRAGMA = 322,
    FEATURE = 323,
    VARARGS = 324,
    ENUM = 325,
    CLASS = 326,
    TYPENAME = 327,
    PRIVATE = 328,
    PUBLIC = 329,
    PROTECTED = 330,
    COLON = 331,
    STATIC = 332,
    VIRTUAL = 333,
    FRIEND = 334,
    THROW = 335,
    CATCH = 336,
    EXPLICIT = 337,
    STATIC_ASSERT = 338,
    CONSTEXPR = 339,
    THREAD_LOCAL = 340,
    DECLTYPE = 341,
    AUTO = 342,
    NOEXCEPT = 343,
    OVERRIDE = 344,
    FINAL = 345,
    USING = 346,
    NAMESPACE = 347,
    NATIVE = 348,
    INLINE = 349,
    TYPEMAP = 350,
    EXCEPT = 351,
    ECHO = 352,
    APPLY = 353,
    CLEAR = 354,
    SWIGTEMPLATE = 355,
    FRAGMENT = 356,
    WARN = 357,
    LESSTHAN = 358,
    GREATERTHAN = 359,
    DELETE_KW = 360,
    DEFAULT = 361,
    LESSTHANOREQUALTO = 362,
    GREATERTHANOREQUALTO = 363,
    EQUALTO = 364,
    NOTEQUALTO = 365,
    ARROW = 366,
    QUESTIONMARK = 367,
    TYPES = 368,
    PARMS = 369,
    NONID = 370,
    DSTAR = 371,
    DCNOT = 372,
    TEMPLATE = 373,
    OPERATOR = 374,
    CONVERSIONOPERATOR = 375,
    PARSETYPE = 376,
    PARSEPARM = 377,
    PARSEPARMS = 378,
    DOXYGENSTRING = 379,
    DOXYGENPOSTSTRING = 380,
    CAST = 381,
    LOR = 382,
    LAND = 383,
    OR = 384,
    XOR = 385,
    AND = 386,
    LSHIFT = 387,
    RSHIFT = 388,
    PLUS = 389,
    MINUS = 390,
    STAR = 391,
    SLASH = 392,
    MODULO = 393,
    UMINUS = 394,
    NOT = 395,
    LNOT = 396,
    DCOLON = 397
  };
#endif
/* Tokens.  */
#define ID 258
#define HBLOCK 259
#define POUND 260
#define STRING 261
#define WSTRING 262
#define INCLUDE 263
#define IMPORT 264
#define INSERT 265
#define CHARCONST 266
#define WCHARCONST 267
#define NUM_INT 268
#define NUM_FLOAT 269
#define NUM_UNSIGNED 270
#define NUM_LONG 271
#define NUM_ULONG 272
#define NUM_LONGLONG 273
#define NUM_ULONGLONG 274
#define NUM_BOOL 275
#define TYPEDEF 276
#define TYPE_INT 277
#define TYPE_UNSIGNED 278
#define TYPE_SHORT 279
#define TYPE_LONG 280
#define TYPE_FLOAT 281
#define TYPE_DOUBLE 282
#define TYPE_CHAR 283
#define TYPE_WCHAR 284
#define TYPE_VOID 285
#define TYPE_SIGNED 286
#define TYPE_BOOL 287
#define TYPE_COMPLEX 288
#define TYPE_TYPEDEF 289
#define TYPE_RAW 290
#define TYPE_NON_ISO_INT8 291
#define TYPE_NON_ISO_INT16 292
#define TYPE_NON_ISO_INT32 293
#define TYPE_NON_ISO_INT64 294
#define LPAREN 295
#define RPAREN 296
#define COMMA 297
#define SEMI 298
#define EXTERN 299
#define INIT 300
#define LBRACE 301
#define RBRACE 302
#define PERIOD 303
#define CONST_QUAL 304
#define VOLATILE 305
#define REGISTER 306
#define STRUCT 307
#define UNION 308
#define EQUAL 309
#define SIZEOF 310
#define MODULE 311
#define LBRACKET 312
#define RBRACKET 313
#define BEGINFILE 314
#define ENDOFFILE 315
#define ILLEGAL 316
#define CONSTANT 317
#define NAME 318
#define RENAME 319
#define NAMEWARN 320
#define EXTEND 321
#define PRAGMA 322
#define FEATURE 323
#define VARARGS 324
#define ENUM 325
#define CLASS 326
#define TYPENAME 327
#define PRIVATE 328
#define PUBLIC 329
#define PROTECTED 330
#define COLON 331
#define STATIC 332
#define VIRTUAL 333
#define FRIEND 334
#define THROW 335
#define CATCH 336
#define EXPLICIT 337
#define STATIC_ASSERT 338
#define CONSTEXPR 339
#define THREAD_LOCAL 340
#define DECLTYPE 341
#define AUTO 342
#define NOEXCEPT 343
#define OVERRIDE 344
#define FINAL 345
#define USING 346
#define NAMESPACE 347
#define NATIVE 348
#define INLINE 349
#define TYPEMAP 350
#define EXCEPT 351
#define ECHO 352
#define APPLY 353
#define CLEAR 354
#define SWIGTEMPLATE 355
#define FRAGMENT 356
#define WARN 357
#define LESSTHAN 358
#define GREATERTHAN 359
#define DELETE_KW 360
#define DEFAULT 361
#define LESSTHANOREQUALTO 362
#define GREATERTHANOREQUALTO 363
#define EQUALTO 364
#define NOTEQUALTO 365
#define ARROW 366
#define QUESTIONMARK 367
#define TYPES 368
#define PARMS 369
#define NONID 370
#define DSTAR 371
#define DCNOT 372
#define TEMPLATE 373
#define OPERATOR 374
#define CONVERSIONOPERATOR 375
#define PARSETYPE 376
#define PARSEPARM 377
#define PARSEPARMS 378
#define DOXYGENSTRING 379
#define DOXYGENPOSTSTRING 380
#define CAST 381
#define LOR 382
#define LAND 383
#define OR 384
#define XOR 385
#define AND 386
#define LSHIFT 387
#define RSHIFT 388
#define PLUS 389
#define MINUS 390
#define STAR 391
#define SLASH 392
#define MODULO 393
#define UMINUS 394
#define NOT 395
#define LNOT 396
#define DCOLON 397

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 1521 "CParse/parser.y" /* yacc.c:353  */

  const char  *id;
  List  *bases;
  struct Define {
    String *val;
    String *rawval;
    int     type;
    String *qualifier;
    String *refqualifier;
    String *bitfield;
    Parm   *throws;
    String *throwf;
    String *nexcept;
  } dtype;
  struct {
    const char *type;
    String *filename;
    int   line;
  } loc;
  struct {
    char      *id;
    SwigType  *type;
    String    *defarg;
    ParmList  *parms;
    short      have_parms;
    ParmList  *throws;
    String    *throwf;
    String    *nexcept;
  } decl;
  Parm         *tparms;
  struct {
    String     *method;
    Hash       *kwargs;
  } tmap;
  struct {
    String     *type;
    String     *us;
  } ptype;
  SwigType     *type;
  String       *str;
  Parm         *p;
  ParmList     *pl;
  int           intvalue;
  Node         *node;

#line 1938 "CParse/parser.c" /* yacc.c:353  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_CPARSE_PARSER_H_INCLUDED  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  62
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   5677

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  143
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  182
/* YYNRULES -- Number of rules.  */
#define YYNRULES  615
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1198

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   397

#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,  1690,  1690,  1702,  1706,  1709,  1712,  1715,  1718,  1723,
    1732,  1736,  1743,  1748,  1749,  1750,  1751,  1752,  1762,  1778,
    1788,  1789,  1790,  1791,  1792,  1793,  1794,  1795,  1796,  1797,
    1798,  1799,  1800,  1801,  1802,  1803,  1804,  1805,  1806,  1807,
    1808,  1815,  1815,  1897,  1907,  1918,  1938,  1962,  1986,  1997,
    2006,  2025,  2031,  2037,  2042,  2049,  2056,  2060,  2073,  2082,
    2097,  2110,  2110,  2166,  2167,  2174,  2193,  2224,  2228,  2238,
    2243,  2261,  2304,  2310,  2323,  2329,  2355,  2361,  2368,  2369,
    2372,  2373,  2380,  2426,  2472,  2483,  2486,  2513,  2519,  2525,
    2531,  2539,  2545,  2551,  2557,  2565,  2566,  2567,  2570,  2575,
    2585,  2621,  2622,  2657,  2674,  2682,  2695,  2720,  2726,  2730,
    2733,  2744,  2749,  2762,  2774,  3071,  3081,  3088,  3089,  3093,
    3093,  3118,  3124,  3134,  3146,  3155,  3234,  3296,  3300,  3324,
    3328,  3339,  3344,  3345,  3346,  3350,  3351,  3352,  3363,  3368,
    3373,  3380,  3386,  3391,  3394,  3394,  3407,  3410,  3413,  3422,
    3425,  3432,  3454,  3483,  3581,  3633,  3634,  3635,  3636,  3637,
    3638,  3643,  3643,  3891,  3891,  4038,  4039,  4051,  4069,  4069,
    4330,  4336,  4342,  4345,  4348,  4351,  4354,  4357,  4362,  4398,
    4402,  4405,  4408,  4413,  4417,  4422,  4432,  4463,  4463,  4521,
    4521,  4543,  4570,  4587,  4592,  4587,  4600,  4601,  4602,  4602,
    4618,  4619,  4636,  4637,  4638,  4639,  4640,  4641,  4642,  4643,
    4644,  4645,  4646,  4647,  4648,  4649,  4650,  4651,  4653,  4656,
    4660,  4672,  4700,  4729,  4761,  4777,  4795,  4814,  4834,  4854,
    4862,  4869,  4876,  4884,  4892,  4895,  4899,  4902,  4903,  4904,
    4905,  4906,  4907,  4908,  4909,  4912,  4922,  4932,  4944,  4954,
    4964,  4977,  4980,  4983,  4984,  4988,  4990,  4998,  5010,  5011,
    5012,  5013,  5014,  5015,  5016,  5017,  5018,  5019,  5020,  5021,
    5022,  5023,  5024,  5025,  5026,  5027,  5028,  5029,  5036,  5047,
    5051,  5058,  5062,  5067,  5071,  5083,  5093,  5103,  5106,  5110,
    5116,  5129,  5133,  5136,  5140,  5144,  5172,  5180,  5192,  5207,
    5217,  5226,  5237,  5241,  5245,  5252,  5274,  5291,  5310,  5329,
    5336,  5344,  5353,  5362,  5366,  5375,  5386,  5397,  5409,  5419,
    5433,  5441,  5450,  5459,  5463,  5472,  5483,  5494,  5506,  5516,
    5526,  5537,  5550,  5557,  5565,  5581,  5589,  5600,  5611,  5622,
    5641,  5649,  5666,  5674,  5681,  5688,  5699,  5711,  5722,  5734,
    5745,  5756,  5776,  5797,  5803,  5809,  5816,  5823,  5832,  5841,
    5844,  5853,  5862,  5869,  5876,  5883,  5891,  5901,  5912,  5923,
    5934,  5941,  5948,  5951,  5968,  5986,  5996,  6003,  6009,  6014,
    6021,  6025,  6030,  6037,  6041,  6047,  6051,  6057,  6058,  6059,
    6065,  6071,  6075,  6076,  6080,  6087,  6090,  6091,  6095,  6096,
    6098,  6101,  6104,  6109,  6120,  6145,  6148,  6202,  6206,  6210,
    6214,  6218,  6222,  6226,  6230,  6234,  6238,  6242,  6246,  6250,
    6254,  6260,  6260,  6275,  6280,  6283,  6289,  6303,  6318,  6319,
    6323,  6324,  6328,  6329,  6330,  6334,  6338,  6344,  6349,  6353,
    6360,  6365,  6368,  6372,  6376,  6380,  6387,  6395,  6407,  6422,
    6423,  6443,  6447,  6457,  6463,  6466,  6469,  6473,  6478,  6483,
    6484,  6489,  6502,  6517,  6527,  6545,  6552,  6559,  6566,  6574,
    6582,  6586,  6590,  6596,  6597,  6598,  6599,  6600,  6601,  6602,
    6603,  6606,  6610,  6614,  6618,  6622,  6626,  6630,  6634,  6638,
    6642,  6646,  6650,  6654,  6658,  6672,  6676,  6680,  6686,  6690,
    6694,  6698,  6702,  6718,  6723,  6726,  6731,  6736,  6736,  6737,
    6740,  6757,  6766,  6766,  6784,  6784,  6802,  6803,  6804,  6807,
    6811,  6815,  6819,  6825,  6828,  6832,  6838,  6842,  6846,  6852,
    6855,  6860,  6861,  6864,  6867,  6870,  6873,  6878,  6881,  6886,
    6891,  6896,  6901,  6906,  6911,  6918,  6925,  6930,  6937,  6940,
    6949,  6959,  6969,  6978,  6987,  6994,  7005,  7006,  7009,  7010,
    7011,  7012,  7015,  7027,  7033,  7042,  7043,  7044,  7047,  7048,
    7049,  7052,  7053,  7056,  7061,  7065,  7068,  7071,  7074,  7077,
    7082,  7086,  7089,  7096,  7102,  7105,  7110,  7113,  7119,  7124,
    7128,  7131,  7134,  7137,  7142,  7146,  7149,  7152,  7158,  7161,
    7164,  7172,  7175,  7178,  7182,  7187,  7200,  7204,  7209,  7215,
    7219,  7224,  7228,  7235,  7238,  7243
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ID", "HBLOCK", "POUND", "STRING",
  "WSTRING", "INCLUDE", "IMPORT", "INSERT", "CHARCONST", "WCHARCONST",
  "NUM_INT", "NUM_FLOAT", "NUM_UNSIGNED", "NUM_LONG", "NUM_ULONG",
  "NUM_LONGLONG", "NUM_ULONGLONG", "NUM_BOOL", "TYPEDEF", "TYPE_INT",
  "TYPE_UNSIGNED", "TYPE_SHORT", "TYPE_LONG", "TYPE_FLOAT", "TYPE_DOUBLE",
  "TYPE_CHAR", "TYPE_WCHAR", "TYPE_VOID", "TYPE_SIGNED", "TYPE_BOOL",
  "TYPE_COMPLEX", "TYPE_TYPEDEF", "TYPE_RAW", "TYPE_NON_ISO_INT8",
  "TYPE_NON_ISO_INT16", "TYPE_NON_ISO_INT32", "TYPE_NON_ISO_INT64",
  "LPAREN", "RPAREN", "COMMA", "SEMI", "EXTERN", "INIT", "LBRACE",
  "RBRACE", "PERIOD", "CONST_QUAL", "VOLATILE", "REGISTER", "STRUCT",
  "UNION", "EQUAL", "SIZEOF", "MODULE", "LBRACKET", "RBRACKET",
  "BEGINFILE", "ENDOFFILE", "ILLEGAL", "CONSTANT", "NAME", "RENAME",
  "NAMEWARN", "EXTEND", "PRAGMA", "FEATURE", "VARARGS", "ENUM", "CLASS",
  "TYPENAME", "PRIVATE", "PUBLIC", "PROTECTED", "COLON", "STATIC",
  "VIRTUAL", "FRIEND", "THROW", "CATCH", "EXPLICIT", "STATIC_ASSERT",
  "CONSTEXPR", "THREAD_LOCAL", "DECLTYPE", "AUTO", "NOEXCEPT", "OVERRIDE",
  "FINAL", "USING", "NAMESPACE", "NATIVE", "INLINE", "TYPEMAP", "EXCEPT",
  "ECHO", "APPLY", "CLEAR", "SWIGTEMPLATE", "FRAGMENT", "WARN", "LESSTHAN",
  "GREATERTHAN", "DELETE_KW", "DEFAULT", "LESSTHANOREQUALTO",
  "GREATERTHANOREQUALTO", "EQUALTO", "NOTEQUALTO", "ARROW", "QUESTIONMARK",
  "TYPES", "PARMS", "NONID", "DSTAR", "DCNOT", "TEMPLATE", "OPERATOR",
  "CONVERSIONOPERATOR", "PARSETYPE", "PARSEPARM", "PARSEPARMS",
  "DOXYGENSTRING", "DOXYGENPOSTSTRING", "CAST", "LOR", "LAND", "OR", "XOR",
  "AND", "LSHIFT", "RSHIFT", "PLUS", "MINUS", "STAR", "SLASH", "MODULO",
  "UMINUS", "NOT", "LNOT", "DCOLON", "$accept", "program", "interface",
  "declaration", "swig_directive", "extend_directive", "$@1",
  "apply_directive", "clear_directive", "constant_directive",
  "echo_directive", "except_directive", "stringtype", "fname",
  "fragment_directive", "include_directive", "$@2", "includetype",
  "inline_directive", "insert_directive", "module_directive",
  "name_directive", "native_directive", "pragma_directive", "pragma_arg",
  "pragma_lang", "rename_directive", "rename_namewarn",
  "feature_directive", "stringbracesemi", "featattr", "varargs_directive",
  "varargs_parms", "typemap_directive", "typemap_type", "tm_list",
  "tm_tail", "typemap_parm", "types_directive", "template_directive",
  "warn_directive", "c_declaration", "$@3", "c_decl", "c_decl_tail",
  "initializer", "cpp_alternate_rettype", "cpp_lambda_decl",
  "lambda_introducer", "lambda_body", "lambda_tail", "$@4", "c_enum_key",
  "c_enum_inherit", "c_enum_forward_decl", "c_enum_decl",
  "c_constructor_decl", "cpp_declaration", "cpp_class_decl", "@5", "@6",
  "cpp_opt_declarators", "cpp_forward_class_decl", "cpp_template_decl",
  "$@7", "cpp_template_possible", "template_parms", "templateparameters",
  "templateparameter", "templateparameterstail", "cpp_using_decl",
  "cpp_namespace_decl", "@8", "$@9", "cpp_members", "$@10", "$@11", "$@12",
  "cpp_member_no_dox", "cpp_member", "cpp_constructor_decl",
  "cpp_destructor_decl", "cpp_conversion_operator", "cpp_catch_decl",
  "cpp_static_assert", "cpp_protection_decl", "cpp_swig_directive",
  "cpp_end", "cpp_vend", "anonymous_bitfield", "anon_bitfield_type",
  "extern_string", "storage_class", "parms", "rawparms", "ptail",
  "parm_no_dox", "parm", "valparms", "rawvalparms", "valptail", "valparm",
  "def_args", "parameter_declarator", "plain_declarator", "declarator",
  "notso_direct_declarator", "direct_declarator", "abstract_declarator",
  "direct_abstract_declarator", "pointer", "cv_ref_qualifier",
  "ref_qualifier", "type_qualifier", "type_qualifier_raw", "type",
  "rawtype", "type_right", "decltype", "primitive_type",
  "primitive_type_list", "type_specifier", "definetype", "$@13",
  "default_delete", "deleted_definition", "explicit_default", "ename",
  "optional_ignored_define", "optional_ignored_define_after_comma",
  "enumlist", "enumlist_tail", "enumlist_item", "edecl_with_dox", "edecl",
  "etype", "expr", "exprmem", "valexpr", "exprnum", "exprcompound",
  "ellipsis", "variadic", "inherit", "raw_inherit", "$@14", "base_list",
  "base_specifier", "@15", "@16", "access_specifier", "templcpptype",
  "cpptype", "classkey", "classkeyopt", "opt_virtual",
  "virt_specifier_seq", "virt_specifier_seq_opt",
  "exception_specification", "qualifiers_exception_specification",
  "cpp_const", "ctor_end", "ctor_initializer", "mem_initializer_list",
  "mem_initializer", "less_valparms_greater", "identifier", "idstring",
  "idstringopt", "idcolon", "idcolontail", "idtemplate",
  "idtemplatetemplate", "idcolonnt", "idcolontailnt", "string", "wstring",
  "stringbrace", "options", "kwargs", "stringnum", "empty", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397
};
# endif

#define YYPACT_NINF -1042

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-1042)))

#define YYTABLE_NINF -616

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     778,  4543,  4646,   250,   339,  3987, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042, -1042, -1042, -1042,    42, -1042, -1042, -1042,
   -1042, -1042,    34,   229,   238,   265, -1042, -1042,   204,   263,
     285,  5275,   105,   243,   352,  5558,   858,  1745,   858, -1042,
   -1042, -1042,  3650, -1042,   105,   285, -1042,   241, -1042,   359,
     371,  4956, -1042,   316, -1042, -1042, -1042,   416, -1042, -1042,
      59,   426,  5059,   436, -1042, -1042,   426,   447,   452,   465,
     412, -1042, -1042,   492,   453,   513,   408,    38,   199,   209,
     515,   201,   521,   592,   227,  5346,  5346,   525,   568,   420,
     576,   467, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042, -1042,   426, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042,  1848, -1042, -1042, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042,    79,  5417, -1042,   500, -1042, -1042,   534,
     570,   105,   553,   773,  2306, -1042, -1042, -1042, -1042, -1042,
     858, -1042,  3581,   593,   305,  2442,  3258,    50,   432,  1560,
     172,   105, -1042, -1042,   220,   291,   220,   297,  1924,   540,
   -1042, -1042, -1042, -1042, -1042,    51,   618, -1042, -1042, -1042,
     609, -1042,   619, -1042, -1042,   821, -1042, -1042,   773,   232,
     821,   821, -1042,   621,  1943, -1042,   146,  1181,    26,    51,
      51, -1042,   821,  4853, -1042, -1042,  4956, -1042, -1042, -1042,
   -1042, -1042, -1042,   105,   417, -1042,   165,   627,    51, -1042,
   -1042,   821,    51, -1042, -1042, -1042,   681,  4956,   663,  1507,
     668,   673,   821,   420,   681,  4956,  4956,   105,   420,   421,
     929,  1166,   821,   317,  2396,   602, -1042, -1042,  1943,   105,
    1987,   362, -1042,   672,   687,   722,    51, -1042, -1042,   241,
     642,   662, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042,  3258,   319,  3258,  3258,  3258,  3258,  3258,
    3258,  3258, -1042,   671, -1042,   736,   756,   342,  2549,    25,
      54, -1042, -1042,   681,   790, -1042, -1042,  3698,   254,   254,
     763,   768,   934,   694,   766, -1042, -1042, -1042,   760,  3258,
   -1042, -1042, -1042, -1042,  3762, -1042,  2549,   783,  3698,   780,
     105,   335,   297, -1042,   785,   335,   297, -1042,   700, -1042,
   -1042,  4956,  2578, -1042,  4956,  2714,   791,  2312,  2448,   335,
     297,   725,  1113, -1042, -1042,   241,   800,  4749, -1042, -1042,
   -1042, -1042,   805,   681,   105, -1042, -1042,   271,   807, -1042,
   -1042,    78,   220,   422,   575, -1042,   811, -1042, -1042, -1042,
   -1042,   105, -1042,   812,   801,   643,   814,   818, -1042,   819,
     822, -1042,  5488, -1042,   105, -1042,   823,   828, -1042,   831,
     835,  5346, -1042, -1042,   429, -1042, -1042, -1042,  5346, -1042,
   -1042, -1042,   838, -1042, -1042,   649,   295,   841,   781, -1042,
     842, -1042,    67, -1042, -1042,    62,   290,   290,   290,   326,
     771,   847,   414,   853,  4956,  1337,  2053,   774,  2532,  1300,
      73,   826,   286, -1042,  3815,  1300, -1042,   851, -1042,   184,
   -1042, -1042, -1042, -1042,   285, -1042,   773,   901,  2817,  5488,
     857,  3368,  2960, -1042, -1042, -1042, -1042, -1042, -1042,  2306,
   -1042, -1042, -1042,  3258,  3258,  3258,  3258,  3258,  3258,  3258,
    3258,  3258,  3258,  3258,  3258,  3258,  3258,  3258,  3258,  3258,
     914,   927, -1042,   434,   434,  1400,   815,   393, -1042,   443,
   -1042, -1042,   434,   434,   477,   824,  1434,   290,  3258,  2549,
   -1042,  4956,  1193,    10,   893, -1042,  4956,  2850,   896, -1042,
     904, -1042,  4490,   906, -1042,  4800,   902,   905,   335,   297,
     907,   335,   297,  2006,   908,   911,  2584,   335, -1042, -1042,
   -1042,  4956,   619,   306, -1042, -1042,   821,  1988, -1042,   917,
    4956,   919, -1042,   918, -1042,   508,   139,  2536,   913,  4956,
    1943,   920, -1042,  1507,  4100,   922, -1042,  1527,  5346,   485,
     932,   928,  4956,   673,   491,   924,   821,  4956,   239,   888,
    4956, -1042, -1042, -1042,  1434,  1540,  1737,    23, -1042,   940,
    2668,   943,    22,   897,   859, -1042, -1042,   705, -1042,   415,
   -1042, -1042, -1042,   877, -1042,   935,  5558,   555, -1042,   953,
     771,   220,   926, -1042, -1042,   952, -1042,   105, -1042,  3258,
    2986,  3122,  3394,    76,  1745,   954,   736,   900,   900,  1235,
    1235,  2277,  3226,  3368,  3097,  2825,  2960,   786,   786,   776,
     776, -1042, -1042, -1042, -1042, -1042,   824,   858, -1042, -1042,
   -1042,   434,   959,   964,  1507,   317,  4903,   965,   484,   824,
   -1042,   266,  1737,   971, -1042,  5435,  1737,   270, -1042,   270,
   -1042,  1737,   967,   968,   979,   980,  2720,   335,   297,   981,
     991,   993,   335,   619, -1042, -1042, -1042,   681,  4213, -1042,
    1000, -1042,   295,  1004, -1042,  1010, -1042, -1042, -1042, -1042,
     681, -1042, -1042, -1042,  1012, -1042,  1300,   681, -1042,  1003,
      68,   656,   139, -1042,  1300, -1042,  1015, -1042, -1042,  4326,
      61,  5488,   659, -1042, -1042,  4956, -1042,  1018, -1042,   925,
   -1042,   302,   958, -1042,  1029,  1026, -1042,   105,  1500,   842,
   -1042,  1507,  1300,   219,  1737, -1042,  4956,  3258, -1042, -1042,
   -1042, -1042, -1042,  4718, -1042,    49, -1042, -1042,  1013,  3252,
     370, -1042, -1042,  1034, -1042,   861, -1042,  2176, -1042,   220,
    2549,  3258,  3258,  3394,  3885,  3258,  1036,  1038,  1043,  1046,
   -1042,  3258, -1042, -1042,  1048,  1050, -1042, -1042, -1042,   543,
     335, -1042, -1042,   335, -1042, -1042,   335,  1737,  1737,  1039,
    1045,  1047,   335,  1737,  1049,  1052, -1042, -1042,   821,   821,
     270,  2176,  4956,   239,  1988,  1335,   821,  1053, -1042,  1300,
    1058, -1042, -1042,   681,  1943,    60, -1042,  5346, -1042,  1055,
     270,    72,    51,   121, -1042,  2306,   277, -1042,  1051,    59,
     542, -1042, -1042, -1042, -1042, -1042, -1042, -1042,  5130, -1042,
    4439,  1060, -1042,  1063,  2953, -1042, -1042, -1042,   638, -1042,
   -1042, -1042,  4956, -1042,   546, -1042,   147,  1061,  1065, -1042,
    4956,   575,  1056,  1033, -1042, -1042,  1943, -1042, -1042, -1042,
     926, -1042, -1042, -1042,   105, -1042, -1042, -1042,  1067,  1035,
    1042,  1057,   974,  3475,    51, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042, -1042,
   -1042, -1042, -1042, -1042, -1042, -1042,  1072,   996,  2176, -1042,
   -1042, -1042, -1042, -1042, -1042, -1042,  5202,  1075,  2176, -1042,
    2549,  2549,  2549,  3258,  3258, -1042,  5488,  2684, -1042, -1042,
   -1042,   335,   335,  1737,  1076,  1077,   335,  1737,  1737, -1042,
   -1042,   220,  1082,  1090, -1042,   681,  1094, -1042,  1300,  1669,
     239, -1042,  1089, -1042,  1095, -1042, -1042, -1042,   302, -1042,
   -1042,   302,  1064, -1042, -1042,  5488,  4956,  1943,  5488,  2020,
   -1042, -1042,   638, -1042, -1042,   220, -1042,  1098, -1042, -1042,
   -1042,    51,    51,  1013,  1017,  1086,  1772,    37, -1042,  1101,
   -1042,  1108,  1107,   575,   105,   577, -1042,  1300, -1042,  1106,
     926,  2176, -1042, -1042, -1042, -1042,    51, -1042,  1119,  1846,
   -1042, -1042,  1084,  1096,  1100,  1103,  1105,   233,  1118,  2549,
    2549,  1745,   335,  1737,  1737,   335,   335,  1128, -1042,  1134,
   -1042,  1139, -1042,  1300, -1042, -1042, -1042, -1042, -1042,  1140,
    1507,  1081,    47,  3815, -1042,   370,  1300,  1143, -1042,  1066,
   -1042, -1042,  3258, -1042,  1300,  1141,   147, -1042,    37, -1042,
     587, -1042,  1149,  1151,  1146,   418, -1042, -1042,   220,  1147,
   -1042, -1042, -1042,   105, -1042,  2176,  1158,  4956, -1042, -1042,
    1300,  3258, -1042,  1846,  1159,   335,   335, -1042, -1042, -1042,
    1156, -1042,  1164, -1042,  4956,  1172,  1178,    16,  1180, -1042,
      55, -1042, -1042, -1042,  2549,   220, -1042, -1042, -1042, -1042,
     105,  1171, -1042, -1042,   370,  1186,  1106,  1190,  4956,  1197,
     220,  3088, -1042, -1042, -1042, -1042,  1198,  4956,  4956,  4956,
    1200,  3252,  5488,   546,   370,  1194,  1201, -1042, -1042, -1042,
   -1042,  1204,  1300,   370, -1042,  1300,  1207,  1210,  1211,  4956,
   -1042,  1208, -1042, -1042,  1209, -1042,  2176,  1300, -1042,   394,
   -1042, -1042,   561,  1300,  1300,  1300,  1220,   546,  1216, -1042,
   -1042, -1042, -1042,   575, -1042, -1042,   575, -1042, -1042, -1042,
    1300, -1042, -1042,  1222,  1224, -1042, -1042, -1042
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
     615,     0,     0,     0,     0,     0,    12,     4,   565,   407,
     415,   408,   409,   412,   413,   410,   411,   397,   414,   396,
     416,   399,   417,   418,   419,   420,     0,   387,   388,   389,
     524,   525,   146,   519,   520,     0,   566,   567,     0,     0,
     577,     0,     0,   287,     0,     0,   385,   615,   392,   402,
     395,   404,   405,   523,     0,   584,   400,   575,     6,     0,
       0,   615,     1,    17,    67,    63,    64,     0,   263,    16,
     258,   615,     0,     0,    85,    86,   615,   615,     0,     0,
     262,   264,   265,     0,   266,     0,   267,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    10,    11,     9,    13,    20,    21,    22,    23,
      24,    25,    26,    27,   615,    28,    29,    30,    31,    32,
      33,    34,     0,    35,    36,    37,    38,    39,    40,    14,
     116,   121,   118,   117,    18,    15,   155,   156,   157,   158,
     159,   160,   124,   259,     0,   277,     0,   148,   147,     0,
       0,     0,     0,     0,   615,   578,   288,   398,   289,     3,
     391,   386,   615,     0,   421,     0,     0,   577,   363,   362,
     379,     0,   304,   284,   615,   313,   615,   359,   353,   340,
     301,   393,   406,   401,   585,     0,     0,   573,     5,     8,
       0,   278,   615,   280,    19,     0,   599,   275,     0,   257,
       0,     0,   606,     0,     0,   390,   584,     0,   615,     0,
       0,    81,     0,   615,   270,   274,   615,   268,   230,   271,
     269,   276,   273,     0,     0,   189,   584,     0,     0,    65,
      66,     0,     0,    54,    52,    49,    50,   615,     0,   615,
       0,   615,   615,     0,   115,   615,   615,     0,     0,     0,
       0,     0,     0,   313,     0,   340,   261,   260,     0,   615,
       0,   615,   286,     0,     0,     0,     0,   579,   586,   576,
       0,   565,   601,   461,   462,   473,   474,   475,   476,   477,
     478,   479,   480,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   295,     0,   290,   615,   450,   390,     0,   455,
     449,   454,   459,   456,   460,   292,   394,   615,   363,   362,
       0,     0,   353,   400,     0,   299,   426,   427,   297,     0,
     423,   424,   425,   370,     0,   449,   300,     0,   615,     0,
       0,   315,   361,   332,     0,   314,   360,   377,   378,   341,
     302,   615,     0,   303,   615,     0,     0,   356,   355,   310,
     354,   332,   364,   583,   582,   581,     0,     0,   279,   283,
     569,   568,     0,   570,     0,   598,   119,   609,     0,    71,
      48,     0,   615,   313,   421,    73,     0,   527,   528,   526,
     529,     0,   530,     0,    77,     0,     0,     0,   101,     0,
       0,   185,     0,   615,     0,   187,     0,     0,   106,     0,
       0,     0,   110,   306,   313,   307,   309,    44,     0,   107,
     109,   571,     0,   572,    57,     0,    56,     0,     0,   178,
     615,   182,   523,   180,   170,     0,     0,     0,     0,   568,
       0,     0,     0,     0,   615,     0,     0,   332,     0,   615,
     340,   615,   584,   429,   615,   615,   507,     0,   506,   401,
     509,   521,   522,   403,     0,   574,     0,     0,     0,     0,
       0,   471,   470,   499,   498,   472,   500,   501,   564,     0,
     291,   294,   502,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   600,   363,   362,   353,   400,     0,   340,     0,
     375,   372,   356,   355,     0,   340,   364,     0,     0,   422,
     371,   615,   353,   400,     0,   333,   615,     0,     0,   376,
       0,   349,     0,     0,   368,     0,     0,     0,   312,   358,
       0,   311,   357,   366,     0,     0,     0,   316,   365,   580,
       7,     0,   615,     0,   171,   615,     0,     0,   605,     0,
     615,     0,    72,     0,    80,     0,     0,     0,     0,     0,
       0,     0,   186,   615,     0,     0,   615,   615,     0,     0,
     111,     0,   615,   615,     0,     0,     0,     0,     0,   168,
       0,   179,   184,    61,     0,     0,     0,     0,    82,     0,
       0,     0,   540,   533,   534,   384,   383,   545,   382,   380,
     541,   546,   548,     0,   549,     0,     0,     0,   150,     0,
     400,   615,   615,   163,   167,     0,   587,     0,   451,   463,
       0,     0,   379,     0,   615,     0,   615,   496,   495,   493,
     494,     0,   492,   491,   487,   488,   486,   489,   490,   481,
     482,   483,   484,   485,   453,   452,     0,   364,   344,   343,
     342,   366,     0,     0,   365,   323,     0,     0,     0,   332,
     334,   364,     0,     0,   337,     0,     0,   351,   350,   373,
     369,     0,     0,     0,     0,     0,     0,   317,   367,     0,
       0,     0,   319,   615,   281,    69,    70,    68,     0,   610,
     611,   614,   613,   607,    46,     0,    45,    41,    79,    76,
      78,   604,    96,   603,     0,    91,   615,   602,    95,     0,
     613,     0,     0,   102,   615,   229,     0,   190,   191,     0,
     258,     0,     0,    53,    51,   615,    43,     0,   108,     0,
     592,   590,     0,    60,     0,     0,   113,     0,   615,   615,
     615,     0,   615,     0,     0,   351,   615,     0,   543,   536,
     535,   547,   381,     0,   141,     0,   149,   151,   615,   615,
       0,   131,   531,   508,   510,   512,   532,     0,   161,   615,
     464,     0,     0,   379,   378,     0,     0,     0,     0,     0,
     293,     0,   345,   347,     0,     0,   298,   352,   335,     0,
     325,   339,   338,   324,   305,   374,   320,     0,     0,     0,
       0,     0,   318,     0,     0,     0,   282,   120,     0,     0,
     351,     0,   615,     0,     0,     0,     0,     0,    93,   615,
       0,   122,   188,   257,     0,   584,   104,     0,   103,     0,
     351,     0,     0,     0,   588,   615,     0,    55,     0,   258,
       0,   172,   173,   176,   175,   169,   174,   177,     0,   183,
       0,     0,    84,     0,     0,   134,   133,   135,   615,   137,
     132,   136,   615,   142,     0,   430,   437,     0,   615,   431,
     615,   421,   546,   615,   154,   130,     0,   127,   129,   125,
     615,   517,   516,   518,     0,   514,   198,   217,     0,     0,
       0,     0,   264,   615,     0,   242,   243,   235,   244,   215,
     196,   240,   236,   234,   237,   238,   239,   241,   216,   212,
     213,   200,   207,   206,   210,   209,     0,   218,     0,   201,
     202,   205,   211,   203,   204,   214,     0,   277,     0,   285,
     467,   466,   465,     0,     0,   457,     0,   497,   346,   348,
     336,   322,   321,     0,     0,     0,   326,     0,     0,   612,
     608,   615,     0,     0,    87,   613,    98,    92,   615,     0,
       0,   100,     0,    74,     0,   112,   308,   593,   591,   597,
     596,   595,     0,    58,    59,     0,   615,     0,     0,     0,
      62,    83,   539,   544,   537,   615,   538,     0,   144,   143,
     140,     0,     0,   615,   441,   446,     0,   615,   435,   615,
     432,     0,     0,     0,     0,     0,   557,   615,   511,   615,
     615,     0,   193,   232,   231,   233,     0,   219,     0,     0,
     220,   192,   397,   396,   399,     0,   395,   400,     0,   469,
     468,   615,   327,     0,     0,   331,   330,     0,    42,     0,
      99,     0,    94,   615,    89,    75,   105,   589,   594,     0,
     615,     0,     0,   615,   542,     0,   615,     0,   442,   444,
     440,   443,     0,   152,   615,   430,     0,   438,   615,   436,
       0,   554,     0,   556,   558,     0,   550,   551,   615,     0,
     504,   513,   505,     0,   199,     0,     0,   615,   165,   164,
     615,     0,   208,     0,     0,   329,   328,    47,    97,    88,
       0,   114,     0,   168,   615,     0,     0,     0,     0,   126,
       0,   145,   445,   447,   448,   615,   439,   552,   553,   555,
       0,     0,   562,   563,     0,     0,   615,     0,   615,     0,
     615,     0,   162,   458,    90,   123,     0,   615,   615,   615,
       0,   615,     0,     0,     0,   559,     0,   128,   503,   515,
     194,     0,   615,     0,   251,   615,     0,     0,     0,   615,
     221,     0,   138,   153,     0,   560,     0,   615,   222,     0,
     166,   228,     0,   615,   615,   615,     0,     0,     0,   195,
     223,   245,   247,     0,   248,   250,   421,   226,   225,   224,
     615,   139,   561,     0,     0,   227,   246,   249
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1042, -1042,  -349, -1042, -1042, -1042, -1042,    11,    33,    24,
      41, -1042,   729, -1042,    45,    53, -1042, -1042, -1042,    65,
   -1042,    83, -1042,    88, -1042, -1042,    89, -1042,    93,  -563,
    -683,   102, -1042,   111, -1042,  -367,   699,   -73,   113,   118,
     128,   130, -1042,   539,  -996,  -946, -1042, -1042, -1042, -1041,
    -843, -1042,  -143, -1042, -1042, -1042, -1042, -1042,    12, -1042,
   -1042,   186,    13,    28, -1042, -1042,   304, -1042,   704,   549,
     132, -1042, -1042, -1042,  -763, -1042, -1042, -1042,   398, -1042,
     554, -1042,   557,   210, -1042, -1042, -1042, -1042,  -387, -1042,
   -1042, -1042,     7,   -57, -1042,  -493,  1256,    90,   463, -1042,
     674,   832,   -45,  -616,  -544,   386,  1157,    -5,  -156,  1121,
     216,  -612,   703,    56, -1042,   -59,    19,   -13,   550,  -723,
    1252, -1042,  -364, -1042,  -161, -1042, -1042, -1042,  -727,   308,
   -1042, -1042,  -934, -1042,  -200, -1042,  1249, -1042,  -131,  -512,
   -1042, -1042,   183,   866, -1042, -1042, -1042,   439, -1042, -1042,
   -1042,  -219,   -92, -1042, -1042,   310,  -578, -1042,  -573, -1042,
     706,   175, -1042, -1042,   203,   -29,   427,   -82, -1042,   966,
    -203,  -145,  1142, -1042,  -317,  1110, -1042,   603,    -4,  -206,
    -531,     0
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,     5,   104,   105,   106,   811,   895,   896,   897,
     898,   111,   414,   415,   899,   900,   740,   114,   115,   901,
     117,   902,   119,   903,   699,   210,   904,   122,   905,   705,
     558,   906,   387,   907,   397,   240,   409,   241,   908,   909,
     910,   911,   545,   130,   879,   760,   858,   131,   755,   864,
     990,  1057,    42,   607,   132,   133,   134,   135,   912,   928,
     767,  1089,   913,   914,   738,   845,   418,   419,   420,   581,
     915,   140,   566,   393,   916,  1085,  1166,  1011,   917,   918,
     919,   920,   921,   922,   142,   923,   924,  1168,  1171,   925,
    1025,   143,   926,   310,   191,   358,    43,   192,   293,   294,
     470,   295,   761,   173,   402,   174,   331,   253,   176,   177,
     254,   597,   598,    45,    46,   296,   205,    48,    49,    50,
      51,    52,   318,   319,   360,   321,   322,   441,  1066,   998,
     867,   999,   868,   993,   994,  1113,   298,   299,   325,   301,
     302,  1080,  1081,   447,   448,   612,   763,   764,   884,  1010,
     885,    53,    54,   380,   381,   765,   600,   985,   601,   602,
    1172,   874,  1005,  1073,  1074,   184,    55,   367,   412,    56,
     187,    57,   269,   732,   834,   303,   304,   708,   201,   368,
     693,   193
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
       6,   259,   172,   320,   190,   145,   311,   268,   778,   247,
     551,   155,   144,   204,   748,   736,   107,   136,   137,   716,
      47,    47,   238,   300,   751,   398,   711,   422,   817,   109,
     860,   866,   160,   138,   571,   691,   239,   239,   108,  1055,
     353,   268,   175,   431,   564,   691,   110,   180,   952,   684,
     112,   660,   261,   327,     8,   794,  1139,   795,   113,  1109,
      47,   270,   747,  1067,   660,   196,   455,   196,   365,  1143,
     116,   202,   208,   490,   365,     8,   202,   211,   377,   378,
      47,     8,   221,   405,  -433,   260,   147,  1104,   118,   862,
     146,    44,    59,   120,   121,   863,  -296,   379,   123,    72,
     256,   863,   161,   963,   181,   148,   364,   124,     8,  -181,
     248,   593,   594,   362,   202,   222,   125,   775,   126,   369,
    1177,   583,  -570,   127,     8,   376,   661,   605,  1147,   340,
     385,   343,  1124,   128,  1116,   129,   491,   139,   155,   744,
      36,    37,     8,   701,   197,   196,   197,  1140,  1163,   818,
       8,   311,   539,   154,   305,  1021,   388,  1170,  -296,   389,
     411,    36,    37,   154,   257,  1028,  1142,    36,    37,  1144,
     433,  -181,   311,   297,   180,  1105,   180,   198,  1106,   371,
     400,    47,   702,   170,  1153,   703,   872,   163,   417,   438,
     806,   967,   359,    38,    36,    37,   688,    40,   951,   373,
     374,   722,     8,  1026,   776,   229,   426,   777,   382,   427,
      36,    37,     8,   320,   170,   141,   306,   719,   966,   394,
      38,    27,    28,    29,    40,   365,   338,   614,    36,    37,
    -615,   235,    47,   196,   404,    47,    36,    37,   365,   406,
     970,   410,   413,   701,   292,   196,   423,   230,  1084,   154,
     954,    60,   172,   704,    38,   225,    47,     8,   167,   443,
     446,   450,   852,   178,    47,    47,  1060,   250,   154,     8,
     251,   991,   992,  1040,   164,   170,  1092,   149,   366,   171,
     984,   973,   702,   956,   520,   703,   150,   523,    36,    37,
      61,   223,   175,     8,   162,   471,   166,   180,    36,    37,
    1162,   365,   691,   497,   499,   151,   432,   504,   170,  -256,
     685,   165,   196,   546,    38,    27,    28,    29,    40,    27,
      28,    29,  1127,   974,    38,   547,    47,   549,    40,  -428,
     371,   341,  -428,   563,  1191,   573,   421,   344,   300,    62,
     689,   577,   239,    36,    37,     8,   152,    47,   342,   239,
     653,   315,   686,   704,   345,    36,    37,   434,   185,   459,
      47,   422,  -428,    47,   617,     8,   153,   460,   158,    38,
     734,   875,   180,   167,   342,   516,    47,   589,   312,    36,
      37,    38,   162,   186,  -615,    40,   337,   609,   154,   154,
     163,   850,   517,     6,   171,   159,   164,  1044,   595,   165,
     624,   596,   188,  1179,  1054,    38,   330,   405,   538,   167,
     316,   317,   876,   877,   189,  1094,   878,     8,   166,   832,
     582,   497,   499,   504,     8,   616,   196,   196,   653,   154,
     171,    36,    37,   434,   648,     8,   194,  1181,   446,   604,
    1182,   608,  -584,  -584,   833,   604,  -615,   542,  1183,   450,
     342,    36,    37,    47,   657,   178,   195,    38,  1122,   663,
     391,   167,   550,    47,  1123,   428,   200,  -615,  -584,   572,
     168,   392,   328,   169,   307,   709,   207,    38,   170,   342,
     329,    40,   171,   434,   649,   219,   342,   209,   297,   165,
     220,   165,   212,   695,     8,   599,   214,   215,   785,   206,
     342,   599,   655,    36,    37,   213,  1102,  1002,   252,   239,
      36,    37,   698,   178,   196,   727,   226,   434,   650,    30,
      31,    36,    37,   495,   516,   788,   316,   317,   723,    38,
      47,   724,   216,    40,   342,    47,    38,   217,    33,    34,
      40,   517,   359,   595,   512,     6,   596,    38,   262,   426,
     170,    40,   427,   218,   519,   228,     8,   170,   404,   292,
      47,   231,   654,   406,   145,   242,     6,   145,   872,    47,
     246,   144,   330,   410,   721,   107,   136,   137,    47,   172,
      36,    37,   263,   516,   940,   785,   988,   428,   109,   989,
     372,    47,   138,   756,    30,    31,    47,   108,   757,    47,
     517,   758,   949,   950,  1184,   110,   729,  1185,   243,   112,
     730,   180,   766,    33,    34,  1186,   245,   113,   264,   175,
    1076,     8,   361,  1077,   180,   403,   471,   361,   361,   116,
    1117,   683,   232,  1118,   361,   233,   383,   384,   234,   361,
     741,   314,    36,    37,   439,   976,   445,   118,   586,   713,
     356,  1047,   120,   121,  1048,   396,   352,   123,   361,   399,
     428,   357,   824,   701,   370,   196,   124,   735,   829,   361,
     421,   266,   267,   395,   623,   125,   429,   126,   774,   361,
     316,   317,   127,   359,   556,   557,   442,   365,   145,   853,
     575,   576,   128,   454,   129,   144,   139,   815,   816,   107,
     136,   137,   826,   538,   300,   703,   604,    36,    37,   401,
     320,   407,   109,   827,   604,   408,   138,   538,   438,   145,
     451,   108,   178,   599,   929,   599,   144,   593,   594,   110,
     107,   136,   137,   112,   709,   452,   266,   354,   145,   582,
       6,   113,   604,   109,    47,   848,   456,   138,   247,  1018,
     842,   843,   108,   116,   964,   953,   261,   422,   869,   604,
     110,   160,   599,   453,   112,    47,   844,   927,   239,   180,
     599,   118,   113,   457,   141,   468,   120,   121,   469,   178,
    1180,   123,   865,   259,   116,   591,  1187,  1188,  1189,   979,
     124,  1058,  1059,   592,   593,   594,   472,   492,   599,   125,
     178,   126,   118,  1195,   500,   987,   127,   120,   121,   501,
     506,   927,   123,  1001,   507,   599,   128,   508,   129,   604,
     139,   124,  1194,   511,     8,    30,    31,   196,   514,   774,
     125,    47,   126,   518,   261,   305,   170,   127,   337,   526,
     178,   533,  1072,   540,    33,    34,   543,   128,   548,   129,
     145,   139,   552,   554,   297,   555,   559,   144,   986,   560,
     561,   107,   136,   137,   567,   562,   599,   979,  1000,   568,
     178,  1086,   569,  1006,   109,   599,   570,  1031,   138,   574,
     766,    47,   578,   108,   580,   579,   599,   584,   585,    47,
     590,   110,   652,   145,   405,   112,   588,   613,   141,     1,
       2,     3,   606,   113,   618,   625,  1037,    27,    28,    29,
      36,    37,   487,   488,   489,   116,  1050,   644,   927,  1052,
     485,   486,   487,   488,   489,   292,   316,   317,   927,   141,
     645,   647,     8,   118,   881,   882,   883,     8,   120,   121,
     651,   662,   706,   123,   666,   667,   714,   669,   750,   403,
     671,   180,   124,   672,   712,   673,   679,   178,   604,   680,
     694,   125,   696,   126,   697,   718,   715,   733,   127,   432,
     652,   742,   725,   361,   162,   726,   423,   329,   128,   737,
     129,   745,   139,   746,   361,   180,   172,   749,   753,   337,
     519,   165,   754,   869,   759,    47,  1108,   869,   768,  1000,
     782,   731,   779,   361,   762,   783,   787,   604,   157,  1082,
     766,   927,   791,   179,   599,   797,   798,   865,    36,    37,
     183,  1065,  1193,    36,    37,   320,   175,   799,   800,   803,
    1129,   180,   483,   484,   485,   486,   487,   488,   489,   804,
     784,   805,   808,   604,    38,   404,   809,  1136,    40,    38,
     406,   810,   812,   167,   224,   227,   604,   814,   821,   830,
     141,   835,   502,   599,   604,   503,   421,   831,   869,   330,
     836,  1151,    47,   837,   171,    72,   880,   933,   180,   934,
    1156,  1157,  1158,  1161,   935,   927,   936,   943,   255,   938,
     604,   939,  1065,   944,   959,   945,   965,   947,   819,   599,
     948,   961,  1176,   981,   982,   975,    47,   997,   996,  1004,
    1003,  1013,   599,  1012,  1016,   180,     8,   265,  1014,  1019,
     599,  1020,  -197,    47,  1033,  1034,  1082,   784,   313,  1038,
     180,  1039,  1045,  1015,   333,   333,   816,   339,  1046,  1056,
    1062,   604,  1061,  1068,   351,   603,   599,    47,   825,  1070,
    1071,   611,   604,   432,  1079,   604,    47,    47,    47,  1087,
    -254,   534,    27,    28,    29,  1093,   927,   604,  1049,     8,
     255,  1097,  -253,   604,   604,   604,  -255,  1098,    47,  1091,
     199,  -252,  1099,  1101,     8,  1103,  1111,   196,  -434,   390,
     604,  1112,  1119,  1120,  1121,  1125,     8,   599,  1128,  1134,
    1133,   958,    36,    37,   236,   179,   432,  1135,   599,   244,
     962,   599,  1137,   424,   334,   430,   333,   333,  1138,  1146,
     437,  1141,   375,   599,   440,   157,   255,   449,    38,   599,
     599,   599,    40,   328,  1148,   361,   361,  1150,  1152,  1155,
    1159,   535,  1164,   361,   536,  1167,   599,   178,  1173,  1165,
     165,  1174,  1175,   330,   863,    36,    37,  1178,   968,   969,
     971,  1190,  1007,   179,  1192,  1196,   178,  1197,  1107,   428,
      36,    37,   728,   496,   498,   498,   690,   841,   505,  1132,
    1051,    38,    36,    37,   739,    40,   316,   317,   849,   332,
     336,  1017,   846,   995,   513,   847,   515,   156,   972,   350,
     780,   626,   752,   859,   182,   363,   330,  1069,    38,  1149,
     363,   363,    40,   333,   333,   615,  1160,   363,   333,  1008,
    1083,   502,   363,  1145,   503,   828,   335,     0,   355,     0,
     544,     0,     0,   330,     0,   349,     0,   430,     8,     0,
       8,   363,   473,   474,     0,  1043,     0,   553,     0,    27,
      28,    29,   363,   416,     0,     0,     0,     0,   425,   363,
     565,     0,   363,   439,     0,   445,     0,   483,   484,   485,
     486,   487,   488,   489,     0,   371,     0,   432,   957,     0,
     591,     0,  1064,   163,     0,   527,     0,     0,   592,   593,
     594,     0,   498,   498,   498,     0,     0,     0,   587,     0,
       0,   333,   333,     8,   333,  1090,     0,     0,   335,     0,
     610,   349,   813,     0,   324,   326,     0,     0,   995,   995,
     820,     0,     0,     0,    36,    37,    36,    37,   595,   332,
     336,   596,     0,   350,     0,     0,   403,     8,     0,     0,
     307,     0,     0,     0,     0,     0,     0,     0,   851,     0,
      38,     0,    38,     0,   167,     0,    40,   165,     0,     0,
       0,   646,     0,   250,     0,   873,   251,     0,   529,   532,
       0,   170,   179,   498,   162,   171,     0,   330,   659,  1090,
       0,     0,   163,    27,    28,    29,     0,     0,     0,    36,
      37,   165,     0,   995,     0,     0,     0,     0,     0,   333,
       0,     0,   333,     0,   528,   531,     0,     0,     0,   537,
       8,     0,     0,     0,     0,    38,     0,     0,     0,    40,
       0,    68,   255,    36,    37,   960,   255,     0,   502,   179,
       0,   503,   458,     0,   461,   462,   463,   464,   465,   466,
     467,     0,     0,     8,   839,     0,   196,   162,    68,    38,
     179,   255,   333,   167,     0,   163,   333,     0,     0,     0,
       0,     0,   168,     8,   165,   169,     0,     0,   509,     0,
     170,   720,     0,     0,   171,     0,     0,    80,    81,    82,
     371,     0,    84,   769,    86,    87,     0,     0,   163,     0,
     179,   522,   528,   531,   525,   537,    36,    37,     0,     0,
     328,     0,     0,     0,    80,    81,    82,     0,   334,    84,
       0,    86,    87,     0,   332,   336,   350,   165,   840,     0,
     179,     0,    38,   529,   532,     0,   167,   333,   333,    36,
      37,     0,   333,   350,     0,   168,     0,   333,   169,     0,
       0,     0,   333,   170,     0,     0,     0,   171,     0,    36,
      37,     0,     0,   687,   678,    38,   363,   692,     0,   167,
       0,     0,     0,     0,  1041,   700,   707,   710,   250,   658,
       0,   251,     8,     0,     0,    38,   170,     0,   255,    40,
     171,     0,     0,     0,     0,     0,   363,     0,   707,     0,
     677,     0,     0,   682,     0,   743,     0,     0,     0,     0,
     330,     0,     0,   838,     0,     0,     0,   179,     0,   371,
     333,     0,  1042,  1078,     0,     0,     0,   163,     0,   861,
       0,     0,   627,   628,   629,   630,   631,   632,   633,   634,
     635,   636,   637,   638,   639,   640,   641,   642,   643,     0,
       8,     0,     0,   658,     0,     0,     0,   677,     8,  1100,
       0,     0,     0,     0,     0,     0,     0,   656,    36,    37,
       0,     0,  1110,   333,   333,     0,   665,     0,     0,   333,
    1115,     0,   678,     0,     0,     8,     0,   432,     0,     0,
       0,   255,     0,     0,    38,   162,     0,     0,   167,     0,
     255,     0,     0,   163,     0,     0,  1130,   250,     0,   164,
     251,     0,   165,     0,     0,   170,     0,     0,     0,   171,
       0,     0,   371,     0,     0,  1063,     0,     0,   789,   790,
     163,   166,   707,   793,     0,     0,    36,    37,   796,     0,
     823,     0,   707,   802,    36,    37,     0,     0,     0,     0,
       0,     0,   255,     0,     0,     0,     0,   873,     0,     8,
    1009,     8,    38,     0,     0,     0,    40,     0,  1169,     0,
      38,    36,    37,     0,   167,     0,     0,     0,   770,   633,
     636,   641,     0,   168,     0,     0,   169,   330,     0,     0,
       0,   170,     0,     0,     0,   171,   371,    38,   249,  1088,
       0,   167,  1027,     0,   163,     0,   163,     0,     0,     0,
     250,   789,     0,   251,     0,     0,     0,     0,   170,   333,
       0,     0,   171,   333,   333,     0,     0,     0,   363,   363,
       0,     0,     0,   707,   955,   255,   363,     8,     0,     0,
       0,     0,     0,     0,     0,    36,    37,    36,    37,     0,
       0,     0,     0,   255,     0,   255,     8,     0,     0,   823,
       0,     0,     0,     0,   941,   942,     0,     0,     0,     0,
     946,    38,   255,    38,   328,   167,     0,   167,     0,     0,
    1075,     0,   346,     0,   250,     0,   250,   251,     0,   251,
       0,   165,   170,   371,   170,   255,   171,     0,   171,     0,
       8,   163,     0,     0,   196,     0,   854,   179,     0,   333,
     333,   275,   276,   277,   278,   279,   280,   281,   282,     8,
       0,     0,     0,    36,    37,     0,   179,     0,     0,   610,
     930,   931,   465,     8,   932,     0,     0,   444,     0,     0,
     937,     0,    36,    37,     0,   163,     0,     0,     0,    38,
       0,     0,     0,    40,     0,     0,   328,     0,     0,  1126,
       0,     0,   347,     0,   674,   348,     8,     0,    38,   255,
    1053,     0,   167,   165,   330,     0,     0,     0,   163,     0,
     707,   250,     0,     0,   251,     0,    36,    37,     0,   170,
       0,     0,     0,   171,     0,     0,  1075,     0,     0,     0,
       0,     0,     0,   432,     0,    36,    37,     0,     0,     0,
    1032,   530,    38,     0,  1035,  1036,   167,     0,     0,    36,
      37,     0,     0,     0,     0,   250,     0,     0,   251,     0,
       0,    38,     0,   170,     0,    40,     0,   171,     0,     0,
       0,     0,     0,     0,   675,    38,     0,   676,     0,   167,
       0,     0,    36,    37,     0,     0,   330,     0,   250,     0,
       0,   251,     0,     0,     0,     0,   170,     0,     0,     0,
     171,     0,     0,     0,     0,     0,     0,     0,    38,     0,
       0,     0,    40,     0,     0,     0,     0,   886,     0,  -615,
      64,     0,  1029,  1030,    65,    66,    67,     0,     0,     0,
    1095,  1096,     0,   330,     0,     0,     0,    68,  -615,  -615,
    -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,
       0,  -615,  -615,  -615,  -615,  -615,     0,     0,     0,   887,
      70,     0,     0,  -615,     0,  -615,  -615,  -615,  -615,  -615,
       0,     0,     0,     0,     0,     0,     0,     0,    72,    73,
      74,    75,   888,    77,    78,    79,  -615,  -615,  -615,   889,
     890,   891,     0,    80,   892,    82,     0,    83,    84,    85,
      86,    87,  -615,  -615,     0,  -615,  -615,    88,     0,     0,
       0,    92,     0,    94,    95,    96,    97,    98,    99,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   100,
       0,  -615,     0,     0,   101,  -615,  -615,     0,     0,     0,
     893,     0,     0,     0,     0,     0,     0,     0,     0,   271,
       0,  1114,   196,   272,     0,     8,   894,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
    1131,    21,    22,    23,    24,    25,   283,     0,     0,     0,
       0,     0,   328,   781,    26,    27,    28,    29,    30,    31,
     527,   284,     0,     0,     0,     0,     0,     0,     0,   165,
       0,     0,     0,     0,     0,     0,    32,    33,    34,     0,
       0,     0,     0,     0,   473,   474,   475,   476,     0,   477,
       0,     0,    35,     0,     0,    36,    37,     0,     0,     8,
       0,    36,    37,     0,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,     0,     0,     0,     0,
       0,    38,     0,     0,    39,    40,     0,    38,     0,     0,
      41,    40,     0,     0,   285,     0,   432,   286,     0,     0,
     287,   288,   289,     0,   346,   271,   290,   291,   196,   272,
       0,     8,   330,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,     0,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     0,    21,    22,    23,
      24,    25,   283,     0,     0,    36,    37,     0,   328,     0,
       0,    27,    28,    29,    30,    31,   530,   284,     0,     0,
     323,     0,     0,     0,     0,   165,     0,     0,     0,     0,
       0,    38,    32,    33,    34,    40,     0,     0,     0,     0,
       0,     0,     0,     0,   435,     0,     0,   436,    35,     0,
       0,    36,    37,     0,     0,     8,   330,    36,    37,     8,
       0,     0,   196,     0,     0,     0,     0,     0,     0,   275,
     276,   277,   278,   279,   280,   281,   282,    38,     0,     0,
       0,    40,     0,    38,     0,     0,     0,    40,     0,     0,
     285,     0,   432,   286,     0,     0,   287,   288,   289,     0,
     534,   271,   290,   291,   196,   272,     0,     8,   330,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,   283,     0,
       0,    36,    37,     0,   432,    36,    37,    27,    28,    29,
      30,    31,   681,   284,     0,     0,   521,     0,     0,     0,
       0,   316,   317,     0,     0,     0,     0,    38,    32,    33,
      34,    40,     0,     0,     0,     0,   473,   474,   475,   476,
     535,   477,     0,   536,    35,     0,     0,    36,    37,     0,
       0,     8,   330,    36,    37,     0,   478,   479,   480,   481,
     482,   483,   484,   485,   486,   487,   488,   489,     0,     0,
       0,     0,     0,    38,     0,     0,     0,    40,     0,    38,
       0,     0,     0,    40,     0,     0,   285,     0,   432,   286,
       0,     0,   287,   288,   289,     0,   674,   271,   290,   291,
     196,   272,     0,     8,   330,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,   283,     0,     0,    36,    37,     0,
     432,     0,     0,    27,    28,    29,    30,    31,   801,   284,
       0,     0,   524,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    38,    32,    33,    34,    40,     0,     0,
       0,   473,   474,   475,   476,     0,   675,     0,     0,   676,
      35,     0,     0,    36,    37,     0,     0,     0,   330,    36,
      37,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     487,   488,   489,     0,     0,     0,     0,     0,     0,    38,
       0,     0,     0,    40,     0,    38,     0,     0,     0,    40,
       0,     0,   285,     0,     0,   286,     0,     0,   287,   288,
     289,     0,     0,   271,   290,   291,   196,   272,   619,     0,
     330,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,     0,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,     0,    21,    22,    23,    24,    25,
     283,     0,     0,     0,     0,     0,     0,     0,     0,    27,
      28,    29,    30,    31,     0,   284,     0,     0,   664,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      32,    33,    34,     0,   473,   474,   475,   476,     0,   477,
       0,     0,   473,   474,   475,   476,    35,     0,     0,    36,
      37,     0,     0,     0,   478,   620,   480,   481,   621,   483,
     484,   485,   486,   622,   488,   489,   482,   483,   484,   485,
     486,   487,   488,   489,     0,    38,     0,     0,     0,    40,
       0,     0,     0,     0,     0,     0,     0,     0,   285,     0,
       0,   286,     0,     0,   287,   288,   289,     0,     0,   271,
     290,   291,   196,   272,   983,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
       0,    21,    22,    23,    24,    25,   283,   771,     0,     0,
       0,     0,     0,     0,     0,    27,    28,    29,    30,    31,
       0,   284,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,     0,
     473,   474,   475,   476,     0,   477,     0,   473,   474,   475,
     476,     0,    35,     0,     0,    36,    37,     0,     0,     0,
     478,   479,   480,   481,   482,   483,   484,   485,   486,   487,
     488,   489,   483,   484,   485,   486,   487,   488,   489,     0,
       0,    38,     0,     0,     0,    40,     0,     0,     0,     0,
       0,     0,     0,     0,   285,     0,     0,   286,     0,     0,
     287,   288,   289,     0,     0,   271,   290,   291,   196,   272,
       0,  1154,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,     0,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     0,    21,    22,    23,
      24,    25,   283,   772,     0,     0,     0,     0,     0,     0,
       0,    27,    28,    29,    30,    31,     0,   284,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,   473,   474,   475,   476,     0,
     477,     0,     0,     0,   473,   474,   475,   476,    35,     0,
       0,    36,    37,     0,     0,   478,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,   481,   482,   483,
     484,   485,   486,   487,   488,   489,     0,    38,     0,     0,
       0,    40,     0,     0,     0,     0,     0,     0,     0,     0,
     285,     0,     0,   286,     0,     0,   287,   288,   289,     0,
       0,   271,   290,   291,   196,   272,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,   870,    21,    22,    23,    24,    25,   283,     0,
       0,    27,    28,    29,     0,     0,   871,    27,    28,    29,
      30,    31,     0,   284,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,     0,   591,   473,   474,   475,   476,     0,     0,     0,
     592,   593,   594,     0,    35,     0,     0,    36,    37,     0,
       0,     0,     0,     0,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,     0,     0,     0,     0,     0,
       0,     0,     0,    38,     0,     0,     0,    40,     0,     0,
     595,     0,     0,   596,     0,     0,   285,     0,     0,   286,
       0,     0,   287,   288,   289,     0,     0,   271,   290,   291,
     196,   272,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,   283,     0,     0,     0,     0,     0,
       0,     0,     0,    27,    28,    29,    30,    31,     0,   284,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,     0,     0,     0,
       0,     0,     0,     0,     0,   473,   474,   475,   476,    64,
      35,     0,     0,    36,    37,    67,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    68,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,     0,     0,    38,
       0,     0,     0,    40,     0,     0,     0,     0,   887,    70,
       0,     0,     0,     0,     0,     0,     0,     0,   287,   288,
     773,     0,     0,     0,   290,   291,     0,    72,    73,    74,
      75,     0,    77,    78,    79,     0,     0,     0,   889,   890,
     891,     0,    80,   892,    82,     0,    83,    84,    85,    86,
      87,     0,     0,     0,     0,     0,    88,     0,     0,     0,
      92,     0,    94,    95,    96,    97,    98,    99,     0,     0,
       0,     0,     0,     0,     8,     0,     0,     0,   100,     0,
       0,     0,     0,   101,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,   894,    21,    22,    23,    24,
      25,   307,     0,     0,     0,     0,     0,     0,     0,    26,
      27,    28,    29,    30,    31,     0,     0,     0,   165,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    32,    33,    34,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    35,     0,     0,
      36,    37,     9,    10,    11,    12,    13,    14,    15,    16,
       0,    18,     0,    20,     0,     0,    22,    23,    24,    25,
       0,     0,     0,     0,     0,     0,    38,     0,     0,    39,
      40,     8,     0,     0,     0,    41,     0,     0,     0,   308,
       0,     0,   309,     0,     0,     0,     0,   170,     0,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,   307,     0,
       0,     0,     0,     0,     0,     0,    26,    27,    28,    29,
      30,    31,     0,     0,     0,   165,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    36,    37,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    38,     0,     0,    39,    40,     8,     0,
     510,     0,    41,     0,     0,     0,   493,     0,     0,   494,
       0,     0,     0,     0,   170,     0,     0,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,    24,    25,     0,     0,     0,     0,     0,
       0,     0,     0,    26,    27,    28,    29,    30,    31,   473,
     474,   475,   476,     0,   477,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    33,    34,     8,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,   488,
     489,    35,     0,     0,    36,    37,     0,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,    24,    25,     0,     0,     0,     0,     0,
      38,     0,     0,    39,    40,     0,     0,    30,    31,    41,
       0,     0,     0,   426,     0,     0,   427,     0,     0,     0,
       0,   170,     0,     0,     0,    32,    33,    34,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    35,     0,     0,    36,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    -2,    63,     0,
    -615,    64,     0,     0,     0,    65,    66,    67,     0,     0,
      38,     0,     0,     0,    40,     0,     0,     0,    68,  -615,
    -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,
    -615,   170,  -615,  -615,  -615,  -615,  -615,     0,     0,     0,
      69,    70,     0,     0,     0,     0,  -615,  -615,  -615,  -615,
    -615,     0,     0,    71,     0,     0,     0,     0,     0,    72,
      73,    74,    75,    76,    77,    78,    79,  -615,  -615,  -615,
       0,     0,     0,     0,    80,    81,    82,     0,    83,    84,
      85,    86,    87,  -615,  -615,     0,  -615,  -615,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     100,    63,  -615,  -615,    64,   101,  -615,     0,    65,    66,
      67,   102,   103,     0,     0,     0,     0,     0,     0,     0,
       0,    68,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,
    -615,  -615,  -615,  -615,     0,  -615,  -615,  -615,  -615,  -615,
       0,     0,     0,    69,    70,     0,     0,   717,     0,  -615,
    -615,  -615,  -615,  -615,     0,     0,    71,     0,     0,     0,
       0,     0,    72,    73,    74,    75,    76,    77,    78,    79,
    -615,  -615,  -615,     0,     0,     0,     0,    80,    81,    82,
       0,    83,    84,    85,    86,    87,  -615,  -615,     0,  -615,
    -615,    88,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   100,    63,  -615,  -615,    64,   101,  -615,
       0,    65,    66,    67,   102,   103,     0,     0,     0,     0,
       0,     0,     0,     0,    68,  -615,  -615,  -615,  -615,  -615,
    -615,  -615,  -615,  -615,  -615,  -615,  -615,     0,  -615,  -615,
    -615,  -615,  -615,     0,     0,     0,    69,    70,     0,     0,
     807,     0,  -615,  -615,  -615,  -615,  -615,     0,     0,    71,
       0,     0,     0,     0,     0,    72,    73,    74,    75,    76,
      77,    78,    79,  -615,  -615,  -615,     0,     0,     0,     0,
      80,    81,    82,     0,    83,    84,    85,    86,    87,  -615,
    -615,     0,  -615,  -615,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   100,    63,  -615,  -615,
      64,   101,  -615,     0,    65,    66,    67,   102,   103,     0,
       0,     0,     0,     0,     0,     0,     0,    68,  -615,  -615,
    -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,
       0,  -615,  -615,  -615,  -615,  -615,     0,     0,     0,    69,
      70,     0,     0,   822,     0,  -615,  -615,  -615,  -615,  -615,
       0,     0,    71,     0,     0,     0,     0,     0,    72,    73,
      74,    75,    76,    77,    78,    79,  -615,  -615,  -615,     0,
       0,     0,     0,    80,    81,    82,     0,    83,    84,    85,
      86,    87,  -615,  -615,     0,  -615,  -615,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   100,
      63,  -615,  -615,    64,   101,  -615,     0,    65,    66,    67,
     102,   103,     0,     0,     0,     0,     0,     0,     0,     0,
      68,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,  -615,
    -615,  -615,  -615,     0,  -615,  -615,  -615,  -615,  -615,     0,
       0,     0,    69,    70,     0,     0,     0,     0,  -615,  -615,
    -615,  -615,  -615,     0,     0,    71,     0,     0,     0,   980,
       0,    72,    73,    74,    75,    76,    77,    78,    79,  -615,
    -615,  -615,     0,     0,     0,     0,    80,    81,    82,     0,
      83,    84,    85,    86,    87,  -615,  -615,     0,  -615,  -615,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     7,     0,     8,     0,   668,     0,
       0,     0,   100,     0,  -615,     0,     0,   101,  -615,     0,
       0,     0,     0,   102,   103,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,     0,    21,    22,
      23,    24,    25,     0,     0,     0,     0,     0,     0,     0,
       0,    26,    27,    28,    29,    30,    31,   473,   474,   475,
     476,     0,   477,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    32,    33,    34,     0,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,    35,
       0,     0,    36,    37,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    58,     0,     8,
       0,     0,     0,     0,     0,     0,     0,     0,    38,     0,
       0,    39,    40,     0,     0,     0,     0,    41,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
       0,    21,    22,    23,    24,    25,     0,     0,     0,     0,
       0,     0,     0,     0,    26,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,     0,
       0,     8,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    35,     0,     0,    36,    37,     0,     0,     0,
       9,    10,    11,    12,    13,    14,    15,    16,   855,    18,
     856,    20,     8,   857,    22,    23,    24,    25,     0,     0,
       0,    38,     0,     0,    39,    40,     0,     0,     0,     0,
      41,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,     0,    21,    22,    23,    24,    25,     0,
       0,     0,     0,     0,     0,     0,     0,    26,    27,    28,
      29,    30,    31,     0,    35,     0,     0,    36,    37,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    32,
      33,    34,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    38,     0,    35,     0,    40,    36,    37,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     8,     0,   670,     0,
       0,     0,     0,     0,    38,     0,   386,    39,    40,     0,
       0,     0,     0,    41,   541,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,     0,    21,    22,
      23,    24,    25,     0,     0,     0,     0,     0,     0,     0,
       0,    26,    27,    28,    29,    30,    31,   473,   474,   475,
     476,     0,   477,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    32,    33,    34,     0,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,    35,
       0,     0,    36,    37,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     8,
       0,   786,     0,     0,     0,     0,     0,     0,    38,     0,
       0,    39,    40,     0,     0,     0,     0,    41,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
       0,    21,    22,    23,    24,    25,     0,     0,     0,     0,
       0,     0,     0,     0,    26,    27,    28,    29,    30,    31,
     473,   474,   475,   476,     0,   477,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,     0,
     478,   479,   480,   481,   482,   483,   484,   485,   486,   487,
     488,   489,    35,     0,     0,    36,    37,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     203,     0,     8,     0,     0,     0,     0,     0,     0,     0,
       0,    38,     0,     0,    39,    40,     0,     0,     0,     0,
      41,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,     0,    21,    22,    23,    24,    25,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    27,    28,
      29,    30,    31,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    32,
      33,    34,     0,     8,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,     0,     0,    36,    37,
       0,     0,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,     0,    21,    22,    23,    24,    25,
       0,     0,     0,     0,    38,     0,     0,     0,    40,    27,
      28,    29,    30,    31,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      32,    33,    34,     0,     0,     8,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    35,   977,     0,    36,
      37,     0,     0,     0,     9,    10,    11,    12,    13,    14,
      15,    16,  1022,    18,  1023,    20,     0,  1024,    22,    23,
      24,    25,     0,     0,     0,    38,     0,     0,     0,    40,
     978,    27,    28,    29,    30,    31,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,     0,     0,     0,     8,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    35,   258,
       0,    36,    37,     0,     0,     0,     0,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,    24,    25,     0,     0,    38,     0,     0,
       0,    40,   978,    26,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    33,    34,     0,     8,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    35,     0,     0,    36,    37,     0,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
       0,    21,    22,    23,    24,    25,   237,     0,     0,     0,
      38,     0,     0,    39,    40,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,     0,
       8,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    35,     0,     0,    36,    37,     0,     0,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     0,    21,    22,    23,    24,    25,     0,     0,     0,
       0,    38,     0,     0,     0,    40,    27,    28,    29,    30,
      31,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    32,    33,    34,
       0,     8,     0,   792,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    35,   258,     0,    36,    37,     0,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,     0,     0,
       0,     0,    38,     0,     0,     0,    40,    27,    28,    29,
      30,    31,   473,   474,   475,   476,     0,   477,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,     8,   478,   479,   480,   481,   482,   483,   484,   485,
     486,   487,   488,   489,    35,     0,     0,    36,    37,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,     0,     0,
       0,     0,     0,    38,     0,     0,     0,    40,     0,     0,
      30,    31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    36,    37,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    38,     0,     0,     0,    40
};

static const yytype_int16 yycheck[] =
{
       0,   144,    47,   164,    61,     5,   162,   152,   624,   101,
     374,    40,     5,    72,   592,   578,     5,     5,     5,   563,
       1,     2,    95,   154,   597,   231,   557,   246,   711,     5,
     753,   758,    45,     5,   401,   547,    95,    96,     5,   985,
     185,   186,    47,   249,   393,   557,     5,    47,   811,   542,
       5,    41,   144,     3,     3,   667,    40,   669,     5,  1055,
      41,   153,    40,   997,    41,     6,   269,     6,     6,  1110,
       5,    71,    76,    48,     6,     3,    76,    77,    52,    53,
      61,     3,    44,   239,    47,   144,    52,    40,     5,    40,
      48,     1,     2,     5,     5,    46,    42,    71,     5,    62,
      21,    46,    46,    43,    48,    71,   198,     5,     3,    42,
     114,    89,    90,   195,   114,    77,     5,    41,     5,   201,
    1161,    59,    54,     5,     3,   207,   116,    54,  1124,   174,
     212,   176,  1078,     5,  1068,     5,   111,     5,   167,   116,
      89,    90,     3,     4,    85,     6,    85,   131,  1144,   712,
       3,   307,   355,   103,   154,   918,   213,  1153,   104,   216,
     242,    89,    90,   103,    85,   928,   111,    89,    90,  1115,
     252,   104,   328,   154,   174,   128,   176,   118,   131,    40,
     237,   162,    43,   136,  1130,    46,   759,    48,   245,   116,
     683,   119,   192,   115,    89,    90,   545,   119,   810,   204,
      54,   568,     3,   926,   128,     4,   128,   131,   208,   131,
      89,    90,     3,   374,   136,     5,   160,   566,   830,    54,
     115,    49,    50,    51,   119,     6,   170,    43,    89,    90,
      46,     4,   213,     6,   239,   216,    89,    90,     6,   239,
     119,   241,   242,     4,   154,     6,   246,    46,  1011,   103,
     813,     1,   297,   114,   115,    46,   237,     3,   119,   259,
      76,   261,    43,    47,   245,   246,   993,   128,   103,     3,
     131,   124,   125,   956,    54,   136,    43,    48,    46,   140,
     858,     4,    43,   814,   341,    46,    48,   344,    89,    90,
      40,    92,   297,     3,    40,   295,    76,   297,    89,    90,
    1143,     6,   814,   308,   309,    40,    40,   312,   136,    76,
       4,    57,     6,    42,   115,    49,    50,    51,   119,    49,
      50,    51,  1085,    46,   115,    54,   307,   372,   119,    43,
      40,    40,    46,   392,  1177,   408,   246,    40,   469,     0,
     546,    46,   401,    89,    90,     3,   142,   328,    57,   408,
     506,    46,    46,   114,    57,    89,    90,    40,   117,    40,
     341,   580,    76,   344,   456,     3,   103,    48,   125,   115,
     576,     1,   372,   119,    57,    40,   357,   434,   162,    89,
      90,   115,    40,   142,    42,   119,   170,   444,   103,   103,
      48,   740,    57,   393,   140,    43,    54,   960,   128,    57,
     459,   131,    43,  1166,   982,   115,   140,   563,   352,   119,
     105,   106,    42,    43,    43,  1031,    46,     3,    76,   117,
     420,   426,   427,   428,     3,   454,     6,     6,   584,   103,
     140,    89,    90,    40,    41,     3,   120,    43,    76,   439,
      46,   441,   116,   117,   142,   445,   104,   357,    54,   449,
      57,    89,    90,   434,   511,   239,    40,   115,    40,   516,
      43,   119,    40,   444,    46,   249,    40,   125,   142,    40,
     128,    54,    40,   131,    40,   557,    40,   115,   136,    57,
      48,   119,   140,    40,    41,    77,    57,    40,   469,    57,
      82,    57,    40,   550,     3,   439,    84,    85,   654,    72,
      57,   445,   507,    89,    90,    40,  1050,   871,   122,   568,
      89,    90,     4,   297,     6,   572,    89,    40,    41,    52,
      53,    89,    90,   307,    40,    41,   105,   106,    43,   115,
     511,    46,    40,   119,    57,   516,   115,    84,    71,    72,
     119,    57,   542,   128,   328,   545,   131,   115,    48,   128,
     136,   119,   131,    40,   338,    40,     3,   136,   563,   469,
     541,    40,   506,   563,   564,    40,   566,   567,  1141,   550,
     103,   564,   140,   573,   567,   564,   564,   564,   559,   624,
      89,    90,    48,    40,    41,   741,    40,   371,   564,    43,
     204,   572,   564,   606,    52,    53,   577,   564,    43,   580,
      57,    46,   808,   809,    43,   564,   115,    46,    40,   564,
     119,   611,   612,    71,    72,    54,    40,   564,    48,   624,
      43,     3,   195,    46,   624,   239,   626,   200,   201,   564,
      43,   541,    40,    46,   207,    43,   209,   210,    46,   212,
     584,    48,    89,    90,   258,   103,   260,   564,   432,   559,
      41,   968,   564,   564,   971,   228,   116,   564,   231,   232,
     444,    42,   721,     4,    43,     6,   564,   577,   725,   242,
     580,   118,   119,    46,   458,   564,   249,   564,   622,   252,
     105,   106,   564,   683,    41,    42,   259,     6,   688,   746,
      41,    42,   564,   266,   564,   688,   564,    41,    42,   688,
     688,   688,    43,   647,   835,    46,   706,    89,    90,    46,
     871,    43,   688,    54,   714,    42,   688,   661,   116,   719,
      48,   688,   506,   667,   769,   669,   719,    89,    90,   688,
     719,   719,   719,   688,   816,    48,   118,   119,   738,   739,
     740,   688,   742,   719,   725,   738,   104,   719,   840,   894,
     738,   738,   719,   688,   827,   812,   848,   976,   758,   759,
     719,   774,   706,    41,   719,   746,   738,   767,   827,   769,
     714,   688,   719,   111,   564,   104,   688,   688,    42,   563,
    1167,   688,   758,   926,   719,    80,  1173,  1174,  1175,   848,
     688,   991,   992,    88,    89,    90,    40,     7,   742,   688,
     584,   688,   719,  1190,    41,   862,   688,   719,   719,    41,
     116,   811,   719,   870,    48,   759,   688,    57,   688,   819,
     688,   719,  1186,    40,     3,    52,    53,     6,    48,   773,
     719,   812,   719,    48,   926,   835,   136,   719,   622,    48,
     624,   116,  1003,    43,    71,    72,    41,   719,    41,   719,
     850,   719,    41,    41,   835,    54,    42,   850,   858,    41,
      41,   850,   850,   850,    41,    43,   810,   926,   868,    41,
     654,  1016,    41,   873,   850,   819,    41,   936,   850,    41,
     880,   862,    41,   850,    42,   104,   830,   116,    41,   870,
     116,   850,   506,   893,  1050,   850,    43,    46,   688,   121,
     122,   123,    76,   850,     3,    48,   951,    49,    50,    51,
      89,    90,   136,   137,   138,   850,   975,     3,   918,   978,
     134,   135,   136,   137,   138,   835,   105,   106,   928,   719,
       3,   116,     3,   850,    73,    74,    75,     3,   850,   850,
     116,    48,   556,   850,    48,    41,   560,    41,    89,   563,
      48,   951,   850,    48,    41,    48,    48,   741,   958,    48,
      43,   850,    43,   850,    46,    43,    46,    43,   850,    40,
     584,   585,    40,   546,    40,    47,   976,    48,   850,    91,
     850,    41,   850,    40,   557,   985,  1031,    90,   111,   773,
     774,    57,    57,   993,    41,   976,  1053,   997,    46,   999,
      41,   574,    48,   576,    78,    41,    41,  1007,    42,  1009,
    1010,  1011,    41,    47,   958,    48,    48,   993,    89,    90,
      54,   997,  1183,    89,    90,  1186,  1031,    48,    48,    48,
    1087,  1031,   132,   133,   134,   135,   136,   137,   138,    48,
     654,    48,    42,  1043,   115,  1050,    42,  1104,   119,   115,
    1050,    41,    40,   119,    88,    89,  1056,    54,    43,    41,
     850,   103,   128,  1007,  1064,   131,   976,   142,  1068,   140,
      41,  1128,  1053,    47,   140,    62,    42,    41,  1078,    41,
    1137,  1138,  1139,  1142,    41,  1085,    40,    48,   122,    41,
    1090,    41,  1068,    48,    41,    48,    41,    48,   712,  1043,
      48,    43,  1159,    43,    41,    54,  1087,    42,    47,    76,
      54,    76,  1056,    46,   140,  1115,     3,   151,    76,    47,
    1064,   125,    47,  1104,    48,    48,  1126,   741,   162,    47,
    1130,    41,    43,    76,   168,   169,    42,   171,    43,    41,
      54,  1141,   125,    42,   178,   439,  1090,  1128,   721,    41,
      43,   445,  1152,    40,    48,  1155,  1137,  1138,  1139,    40,
      76,    48,    49,    50,    51,    47,  1166,  1167,   104,     3,
     204,    43,    76,  1173,  1174,  1175,    76,    43,  1159,    76,
      70,    76,    43,    43,     3,   104,    43,     6,    47,   223,
    1190,   125,    43,    42,    48,    48,     3,  1141,    40,    43,
      41,   815,    89,    90,    94,   239,    40,    43,  1152,    99,
     824,  1155,    40,   247,    48,   249,   250,   251,    40,    48,
     254,    41,    41,  1167,   258,   259,   260,   261,   115,  1173,
    1174,  1175,   119,    40,    48,   808,   809,    47,    41,    41,
      40,   128,    48,   816,   131,    41,  1190,  1031,    41,    48,
      57,    41,    41,   140,    46,    89,    90,    48,   831,   832,
     833,    41,   876,   297,    48,    43,  1050,    43,  1052,  1053,
      89,    90,   573,   307,   308,   309,   547,   738,   312,  1093,
     976,   115,    89,    90,   580,   119,   105,   106,   739,   168,
     169,   893,   738,   866,   328,   738,   330,    41,   835,   178,
     626,   469,   599,   753,    52,   195,   140,   999,   115,  1126,
     200,   201,   119,   347,   348,   449,  1141,   207,   352,   880,
    1010,   128,   212,  1120,   131,   722,   169,    -1,   186,    -1,
     364,    -1,    -1,   140,    -1,   178,    -1,   371,     3,    -1,
       3,   231,   107,   108,    -1,   959,    -1,   381,    -1,    49,
      50,    51,   242,   243,    -1,    -1,    -1,    -1,   248,   249,
     394,    -1,   252,   977,    -1,   979,    -1,   132,   133,   134,
     135,   136,   137,   138,    -1,    40,    -1,    40,    43,    -1,
      80,    -1,   996,    48,    -1,    48,    -1,    -1,    88,    89,
      90,    -1,   426,   427,   428,    -1,    -1,    -1,   432,    -1,
      -1,   435,   436,     3,   438,  1019,    -1,    -1,   251,    -1,
     444,   254,   706,    -1,   165,   166,    -1,    -1,   991,   992,
     714,    -1,    -1,    -1,    89,    90,    89,    90,   128,   308,
     309,   131,    -1,   312,    -1,    -1,  1050,     3,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   742,    -1,
     115,    -1,   115,    -1,   119,    -1,   119,    57,    -1,    -1,
      -1,   495,    -1,   128,    -1,   759,   131,    -1,   347,   348,
      -1,   136,   506,   507,    40,   140,    -1,   140,   512,  1093,
      -1,    -1,    48,    49,    50,    51,    -1,    -1,    -1,    89,
      90,    57,    -1,  1066,    -1,    -1,    -1,    -1,    -1,   533,
      -1,    -1,   536,    -1,   347,   348,    -1,    -1,    -1,   352,
       3,    -1,    -1,    -1,    -1,   115,    -1,    -1,    -1,   119,
      -1,    21,   556,    89,    90,   819,   560,    -1,   128,   563,
      -1,   131,   283,    -1,   285,   286,   287,   288,   289,   290,
     291,    -1,    -1,     3,    44,    -1,     6,    40,    21,   115,
     584,   585,   586,   119,    -1,    48,   590,    -1,    -1,    -1,
      -1,    -1,   128,     3,    57,   131,    -1,    -1,   319,    -1,
     136,    44,    -1,    -1,   140,    -1,    -1,    77,    78,    79,
      40,    -1,    82,   617,    84,    85,    -1,    -1,    48,    -1,
     624,   342,   435,   436,   345,   438,    89,    90,    -1,    -1,
      40,    -1,    -1,    -1,    77,    78,    79,    -1,    48,    82,
      -1,    84,    85,    -1,   493,   494,   495,    57,   118,    -1,
     654,    -1,   115,   502,   503,    -1,   119,   661,   662,    89,
      90,    -1,   666,   512,    -1,   128,    -1,   671,   131,    -1,
      -1,    -1,   676,   136,    -1,    -1,    -1,   140,    -1,    89,
      90,    -1,    -1,   543,   533,   115,   546,   547,    -1,   119,
      -1,    -1,    -1,    -1,   958,   555,   556,   557,   128,   512,
      -1,   131,     3,    -1,    -1,   115,   136,    -1,   712,   119,
     140,    -1,    -1,    -1,    -1,    -1,   576,    -1,   578,    -1,
     533,    -1,    -1,   536,    -1,   585,    -1,    -1,    -1,    -1,
     140,    -1,    -1,   737,    -1,    -1,    -1,   741,    -1,    40,
     744,    -1,    43,  1007,    -1,    -1,    -1,    48,    -1,   753,
      -1,    -1,   473,   474,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,    -1,
       3,    -1,    -1,   586,    -1,    -1,    -1,   590,     3,  1043,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   508,    89,    90,
      -1,    -1,  1056,   797,   798,    -1,   517,    -1,    -1,   803,
    1064,    -1,   651,    -1,    -1,     3,    -1,    40,    -1,    -1,
      -1,   815,    -1,    -1,   115,    40,    -1,    -1,   119,    -1,
     824,    -1,    -1,    48,    -1,    -1,  1090,   128,    -1,    54,
     131,    -1,    57,    -1,    -1,   136,    -1,    -1,    -1,   140,
      -1,    -1,    40,    -1,    -1,    43,    -1,    -1,   661,   662,
      48,    76,   712,   666,    -1,    -1,    89,    90,   671,    -1,
     720,    -1,   722,   676,    89,    90,    -1,    -1,    -1,    -1,
      -1,    -1,   876,    -1,    -1,    -1,    -1,  1141,    -1,     3,
     884,     3,   115,    -1,    -1,    -1,   119,    -1,  1152,    -1,
     115,    89,    90,    -1,   119,    -1,    -1,    -1,   619,   620,
     621,   622,    -1,   128,    -1,    -1,   131,   140,    -1,    -1,
      -1,   136,    -1,    -1,    -1,   140,    40,   115,    40,    43,
      -1,   119,   926,    -1,    48,    -1,    48,    -1,    -1,    -1,
     128,   744,    -1,   131,    -1,    -1,    -1,    -1,   136,   943,
      -1,    -1,   140,   947,   948,    -1,    -1,    -1,   808,   809,
      -1,    -1,    -1,   813,   814,   959,   816,     3,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    90,    89,    90,    -1,
      -1,    -1,    -1,   977,    -1,   979,     3,    -1,    -1,   839,
      -1,    -1,    -1,    -1,   797,   798,    -1,    -1,    -1,    -1,
     803,   115,   996,   115,    40,   119,    -1,   119,    -1,    -1,
    1004,    -1,    48,    -1,   128,    -1,   128,   131,    -1,   131,
      -1,    57,   136,    40,   136,  1019,   140,    -1,   140,    -1,
       3,    48,    -1,    -1,     6,    -1,   747,  1031,    -1,  1033,
    1034,    13,    14,    15,    16,    17,    18,    19,    20,     3,
      -1,    -1,    -1,    89,    90,    -1,  1050,    -1,    -1,  1053,
     771,   772,   773,     3,   775,    -1,    -1,    40,    -1,    -1,
     781,    -1,    89,    90,    -1,    48,    -1,    -1,    -1,   115,
      -1,    -1,    -1,   119,    -1,    -1,    40,    -1,    -1,  1083,
      -1,    -1,   128,    -1,    48,   131,     3,    -1,   115,  1093,
      40,    -1,   119,    57,   140,    -1,    -1,    -1,    48,    -1,
     960,   128,    -1,    -1,   131,    -1,    89,    90,    -1,   136,
      -1,    -1,    -1,   140,    -1,    -1,  1120,    -1,    -1,    -1,
      -1,    -1,    -1,    40,    -1,    89,    90,    -1,    -1,    -1,
     943,    48,   115,    -1,   947,   948,   119,    -1,    -1,    89,
      90,    -1,    -1,    -1,    -1,   128,    -1,    -1,   131,    -1,
      -1,   115,    -1,   136,    -1,   119,    -1,   140,    -1,    -1,
      -1,    -1,    -1,    -1,   128,   115,    -1,   131,    -1,   119,
      -1,    -1,    89,    90,    -1,    -1,   140,    -1,   128,    -1,
      -1,   131,    -1,    -1,    -1,    -1,   136,    -1,    -1,    -1,
     140,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,
      -1,    -1,   119,    -1,    -1,    -1,    -1,     1,    -1,     3,
       4,    -1,   933,   934,     8,     9,    10,    -1,    -1,    -1,
    1033,  1034,    -1,   140,    -1,    -1,    -1,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    43,
      44,    -1,    -1,    47,    -1,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    -1,    77,    78,    79,    -1,    81,    82,    83,
      84,    85,    86,    87,    -1,    89,    90,    91,    -1,    -1,
      -1,    95,    -1,    97,    98,    99,   100,   101,   102,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,
      -1,   115,    -1,    -1,   118,   119,   120,    -1,    -1,    -1,
     124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
      -1,  1062,     6,     7,    -1,     3,   140,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    1091,    35,    36,    37,    38,    39,    40,    -1,    -1,    -1,
      -1,    -1,    40,    76,    48,    49,    50,    51,    52,    53,
      48,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    57,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,
      -1,    -1,    -1,    -1,   107,   108,   109,   110,    -1,   112,
      -1,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,     3,
      -1,    89,    90,    -1,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,    -1,    -1,    -1,    -1,
      -1,   115,    -1,    -1,   118,   119,    -1,   115,    -1,    -1,
     124,   119,    -1,    -1,   128,    -1,    40,   131,    -1,    -1,
     134,   135,   136,    -1,    48,     3,   140,   141,     6,     7,
      -1,     3,   140,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    40,    -1,    -1,    89,    90,    -1,    40,    -1,
      -1,    49,    50,    51,    52,    53,    48,    55,    -1,    -1,
      58,    -1,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,   115,    70,    71,    72,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   128,    -1,    -1,   131,    86,    -1,
      -1,    89,    90,    -1,    -1,     3,   140,    89,    90,     3,
      -1,    -1,     6,    -1,    -1,    -1,    -1,    -1,    -1,    13,
      14,    15,    16,    17,    18,    19,    20,   115,    -1,    -1,
      -1,   119,    -1,   115,    -1,    -1,    -1,   119,    -1,    -1,
     128,    -1,    40,   131,    -1,    -1,   134,   135,   136,    -1,
      48,     3,   140,   141,     6,     7,    -1,     3,   140,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    40,    -1,
      -1,    89,    90,    -1,    40,    89,    90,    49,    50,    51,
      52,    53,    48,    55,    -1,    -1,    58,    -1,    -1,    -1,
      -1,   105,   106,    -1,    -1,    -1,    -1,   115,    70,    71,
      72,   119,    -1,    -1,    -1,    -1,   107,   108,   109,   110,
     128,   112,    -1,   131,    86,    -1,    -1,    89,    90,    -1,
      -1,     3,   140,    89,    90,    -1,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    -1,   115,
      -1,    -1,    -1,   119,    -1,    -1,   128,    -1,    40,   131,
      -1,    -1,   134,   135,   136,    -1,    48,     3,   140,   141,
       6,     7,    -1,     3,   140,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    40,    -1,    -1,    89,    90,    -1,
      40,    -1,    -1,    49,    50,    51,    52,    53,    48,    55,
      -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    70,    71,    72,   119,    -1,    -1,
      -1,   107,   108,   109,   110,    -1,   128,    -1,    -1,   131,
      86,    -1,    -1,    89,    90,    -1,    -1,    -1,   140,    89,
      90,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,    -1,    -1,    -1,    -1,    -1,    -1,   115,
      -1,    -1,    -1,   119,    -1,   115,    -1,    -1,    -1,   119,
      -1,    -1,   128,    -1,    -1,   131,    -1,    -1,   134,   135,
     136,    -1,    -1,     3,   140,   141,     6,     7,    41,    -1,
     140,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    -1,    55,    -1,    -1,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    -1,   107,   108,   109,   110,    -1,   112,
      -1,    -1,   107,   108,   109,   110,    86,    -1,    -1,    89,
      90,    -1,    -1,    -1,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,   131,   132,   133,   134,
     135,   136,   137,   138,    -1,   115,    -1,    -1,    -1,   119,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   128,    -1,
      -1,   131,    -1,    -1,   134,   135,   136,    -1,    -1,     3,
     140,   141,     6,     7,    41,    -1,    -1,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    40,    41,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,
     107,   108,   109,   110,    -1,   112,    -1,   107,   108,   109,
     110,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,   132,   133,   134,   135,   136,   137,   138,    -1,
      -1,   115,    -1,    -1,    -1,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   128,    -1,    -1,   131,    -1,    -1,
     134,   135,   136,    -1,    -1,     3,   140,   141,     6,     7,
      -1,    43,    -1,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    40,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,   107,   108,   109,   110,    -1,
     112,    -1,    -1,    -1,   107,   108,   109,   110,    86,    -1,
      -1,    89,    90,    -1,    -1,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,   130,   131,   132,
     133,   134,   135,   136,   137,   138,    -1,   115,    -1,    -1,
      -1,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     128,    -1,    -1,   131,    -1,    -1,   134,   135,   136,    -1,
      -1,     3,   140,   141,     6,     7,    -1,    -1,    -1,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    40,    35,    36,    37,    38,    39,    40,    -1,
      -1,    49,    50,    51,    -1,    -1,    54,    49,    50,    51,
      52,    53,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    -1,    80,   107,   108,   109,   110,    -1,    -1,    -1,
      88,    89,    90,    -1,    86,    -1,    -1,    89,    90,    -1,
      -1,    -1,    -1,    -1,   128,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    -1,    -1,
     128,    -1,    -1,   131,    -1,    -1,   128,    -1,    -1,   131,
      -1,    -1,   134,   135,   136,    -1,    -1,     3,   140,   141,
       6,     7,    -1,    -1,    -1,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   107,   108,   109,   110,     4,
      86,    -1,    -1,    89,    90,    10,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,    -1,    -1,   115,
      -1,    -1,    -1,   119,    -1,    -1,    -1,    -1,    43,    44,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,   135,
     136,    -1,    -1,    -1,   140,   141,    -1,    62,    63,    64,
      65,    -1,    67,    68,    69,    -1,    -1,    -1,    73,    74,
      75,    -1,    77,    78,    79,    -1,    81,    82,    83,    84,
      85,    -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,
      95,    -1,    97,    98,    99,   100,   101,   102,    -1,    -1,
      -1,    -1,    -1,    -1,     3,    -1,    -1,    -1,   113,    -1,
      -1,    -1,    -1,   118,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,   140,    35,    36,    37,    38,
      39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,
      49,    50,    51,    52,    53,    -1,    -1,    -1,    57,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    71,    72,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,
      89,    90,    22,    23,    24,    25,    26,    27,    28,    29,
      -1,    31,    -1,    33,    -1,    -1,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,   118,
     119,     3,    -1,    -1,    -1,   124,    -1,    -1,    -1,   128,
      -1,    -1,   131,    -1,    -1,    -1,    -1,   136,    -1,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    40,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    50,    51,
      52,    53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    86,    -1,    -1,    89,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    -1,   118,   119,     3,    -1,
      58,    -1,   124,    -1,    -1,    -1,   128,    -1,    -1,   131,
      -1,    -1,    -1,    -1,   136,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    49,    50,    51,    52,    53,   107,
     108,   109,   110,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,     3,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,    86,    -1,    -1,    89,    90,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,
     115,    -1,    -1,   118,   119,    -1,    -1,    52,    53,   124,
      -1,    -1,    -1,   128,    -1,    -1,   131,    -1,    -1,    -1,
      -1,   136,    -1,    -1,    -1,    70,    71,    72,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     0,     1,    -1,
       3,     4,    -1,    -1,    -1,     8,     9,    10,    -1,    -1,
     115,    -1,    -1,    -1,   119,    -1,    -1,    -1,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,   136,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      43,    44,    -1,    -1,    -1,    -1,    49,    50,    51,    52,
      53,    -1,    -1,    56,    -1,    -1,    -1,    -1,    -1,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    -1,    77,    78,    79,    -1,    81,    82,
      83,    84,    85,    86,    87,    -1,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     113,     1,   115,     3,     4,   118,   119,    -1,     8,     9,
      10,   124,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    43,    44,    -1,    -1,    47,    -1,    49,
      50,    51,    52,    53,    -1,    -1,    56,    -1,    -1,    -1,
      -1,    -1,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    -1,    -1,    -1,    -1,    77,    78,    79,
      -1,    81,    82,    83,    84,    85,    86,    87,    -1,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   113,     1,   115,     3,     4,   118,   119,
      -1,     8,     9,    10,   124,   125,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    44,    -1,    -1,
      47,    -1,    49,    50,    51,    52,    53,    -1,    -1,    56,
      -1,    -1,    -1,    -1,    -1,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,
      77,    78,    79,    -1,    81,    82,    83,    84,    85,    86,
      87,    -1,    89,    90,    91,    92,    93,    94,    95,    96,
      97,    98,    99,   100,   101,   102,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,     1,   115,     3,
       4,   118,   119,    -1,     8,     9,    10,   124,   125,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    43,
      44,    -1,    -1,    47,    -1,    49,    50,    51,    52,    53,
      -1,    -1,    56,    -1,    -1,    -1,    -1,    -1,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    -1,
      -1,    -1,    -1,    77,    78,    79,    -1,    81,    82,    83,
      84,    85,    86,    87,    -1,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,
       1,   115,     3,     4,   118,   119,    -1,     8,     9,    10,
     124,   125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    43,    44,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    -1,    -1,    56,    -1,    -1,    -1,    60,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    -1,    -1,    -1,    -1,    77,    78,    79,    -1,
      81,    82,    83,    84,    85,    86,    87,    -1,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,    -1,    -1,     1,    -1,     3,    -1,    58,    -1,
      -1,    -1,   113,    -1,   115,    -1,    -1,   118,   119,    -1,
      -1,    -1,    -1,   124,   125,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    48,    49,    50,    51,    52,    53,   107,   108,   109,
     110,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    -1,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,    86,
      -1,    -1,    89,    90,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,   124,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    48,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,
      -1,     3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,     3,    35,    36,    37,    38,    39,    -1,    -1,
      -1,   115,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
     124,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    50,
      51,    52,    53,    -1,    86,    -1,    -1,    89,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    86,    -1,   119,    89,    90,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,    58,    -1,
      -1,    -1,    -1,    -1,   115,    -1,    13,   118,   119,    -1,
      -1,    -1,    -1,   124,   125,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    48,    49,    50,    51,    52,    53,   107,   108,   109,
     110,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    70,    71,    72,    -1,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,    86,
      -1,    -1,    89,    90,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
      -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,
      -1,   118,   119,    -1,    -1,    -1,    -1,   124,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    48,    49,    50,    51,    52,    53,
     107,   108,   109,   110,    -1,   112,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,
     127,   128,   129,   130,   131,   132,   133,   134,   135,   136,
     137,   138,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
       1,    -1,     3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   115,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,
     124,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,
      51,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    -1,     3,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,    89,    90,
      -1,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    49,
      50,    51,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    -1,    -1,     3,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    86,    87,    -1,    89,
      90,    -1,    -1,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    -1,    -1,    -1,   115,    -1,    -1,    -1,   119,
     120,    49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    -1,    -1,    -1,     3,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    87,
      -1,    89,    90,    -1,    -1,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    -1,    -1,   115,    -1,    -1,
      -1,   119,   120,    48,    49,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,     3,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    86,    -1,    -1,    89,    90,    -1,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    40,    -1,    -1,    -1,
     115,    -1,    -1,   118,   119,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,
       3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      -1,   115,    -1,    -1,    -1,   119,    49,    50,    51,    52,
      53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      -1,     3,    -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    86,    87,    -1,    89,    90,    -1,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,   115,    -1,    -1,    -1,   119,    49,    50,    51,
      52,    53,   107,   108,   109,   110,    -1,   112,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,     3,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,    86,    -1,    -1,    89,    90,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    -1,    -1,
      52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    86,    -1,    -1,    89,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,   121,   122,   123,   144,   145,   324,     1,     3,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    35,    36,    37,    38,    39,    48,    49,    50,    51,
      52,    53,    70,    71,    72,    86,    89,    90,   115,   118,
     119,   124,   195,   239,   240,   256,   257,   259,   260,   261,
     262,   263,   264,   294,   295,   309,   312,   314,     1,   240,
       1,    40,     0,     1,     4,     8,     9,    10,    21,    43,
      44,    56,    62,    63,    64,    65,    66,    67,    68,    69,
      77,    78,    79,    81,    82,    83,    84,    85,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     113,   118,   124,   125,   146,   147,   148,   150,   151,   152,
     153,   154,   157,   158,   160,   161,   162,   163,   164,   165,
     166,   169,   170,   171,   174,   176,   181,   182,   183,   184,
     186,   190,   197,   198,   199,   200,   201,   205,   206,   213,
     214,   226,   227,   234,   235,   324,    48,    52,    71,    48,
      48,    40,   142,   103,   103,   308,   239,   312,   125,    43,
     260,   256,    40,    48,    54,    57,    76,   119,   128,   131,
     136,   140,   245,   246,   248,   250,   251,   252,   253,   312,
     324,   256,   263,   312,   308,   117,   142,   313,    43,    43,
     236,   237,   240,   324,   120,    40,     6,    85,   118,   318,
      40,   321,   324,     1,   258,   259,   309,    40,   321,    40,
     168,   324,    40,    40,    84,    85,    40,    84,    40,    77,
      82,    44,    77,    92,   312,    46,   309,   312,    40,     4,
      46,    40,    40,    43,    46,     4,   318,    40,   180,   258,
     178,   180,    40,    40,   318,    40,   103,   295,   321,    40,
     128,   131,   248,   250,   253,   312,    21,    85,    87,   195,
     258,   295,    48,    48,    48,   312,   118,   119,   314,   315,
     295,     3,     7,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    40,    55,   128,   131,   134,   135,   136,
     140,   141,   240,   241,   242,   244,   258,   259,   279,   280,
     281,   282,   283,   318,   319,   324,   256,    40,   128,   131,
     236,   251,   253,   312,    48,    46,   105,   106,   265,   266,
     267,   268,   269,    58,   279,   281,   279,     3,    40,    48,
     140,   249,   252,   312,    48,   249,   252,   253,   256,   312,
     245,    40,    57,   245,    40,    57,    48,   128,   131,   249,
     252,   312,   116,   314,   119,   315,    41,    42,   238,   324,
     267,   309,   310,   318,   295,     6,    46,   310,   322,   310,
      43,    40,   248,   250,    54,    41,   310,    52,    53,    71,
     296,   297,   324,   309,   309,   310,    13,   175,   236,   236,
     312,    43,    54,   216,    54,    46,   309,   177,   322,   309,
     236,    46,   247,   248,   250,   251,   324,    43,    42,   179,
     324,   310,   311,   324,   155,   156,   318,   236,   209,   210,
     211,   240,   294,   324,   312,   318,   128,   131,   253,   309,
     312,   322,    40,   310,    40,   128,   131,   312,   116,   248,
     312,   270,   309,   324,    40,   248,    76,   286,   287,   312,
     324,    48,    48,    41,   309,   313,   104,   111,   279,    40,
      48,   279,   279,   279,   279,   279,   279,   279,   104,    42,
     243,   324,    40,   107,   108,   109,   110,   112,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
      48,   111,     7,   128,   131,   253,   312,   250,   312,   250,
      41,    41,   128,   131,   250,   312,   116,    48,    57,   279,
      58,    40,   253,   312,    48,   312,    40,    57,    48,   253,
     236,    58,   279,   236,    58,   279,    48,    48,   249,   252,
      48,   249,   252,   116,    48,   128,   131,   249,   256,   313,
      43,   125,   240,    41,   312,   185,    42,    54,    41,   245,
      40,   265,    41,   312,    41,    54,    41,    42,   173,    42,
      41,    41,    43,   258,   145,   312,   215,    41,    41,    41,
      41,   178,    40,   180,    41,    41,    42,    46,    41,   104,
      42,   212,   324,    59,   116,    41,   253,   312,    43,   236,
     116,    80,    88,    89,    90,   128,   131,   254,   255,   256,
     299,   301,   302,   303,   324,    54,    76,   196,   324,   236,
     312,   303,   288,    46,    43,   286,   308,   295,     3,    41,
     128,   131,   136,   253,   258,    48,   244,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,     3,     3,   312,   116,    41,    41,
      41,   116,   248,   251,   256,   250,   279,   236,   249,   312,
      41,   116,    48,   236,    58,   279,    48,    41,    58,    41,
      58,    48,    48,    48,    48,   128,   131,   249,   252,    48,
      48,    48,   249,   240,   238,     4,    46,   318,   145,   322,
     155,   282,   318,   323,    43,   236,    43,    46,     4,   167,
     318,     4,    43,    46,   114,   172,   248,   318,   320,   310,
     318,   323,    41,   240,   248,    46,   247,    47,    43,   145,
      44,   235,   178,    43,    46,    40,    47,   236,   179,   115,
     119,   309,   316,    43,   322,   240,   172,    91,   207,   211,
     159,   256,   248,   318,   116,    41,    40,    40,   299,    90,
      89,   301,   255,   111,    57,   191,   260,    43,    46,    41,
     188,   245,    78,   289,   290,   298,   324,   203,    46,   312,
     279,    41,    41,   136,   256,    41,   128,   131,   246,    48,
     243,    76,    41,    41,   248,   251,    58,    41,    41,   249,
     249,    41,    58,   249,   254,   254,   249,    48,    48,    48,
      48,    48,   249,    48,    48,    48,   238,    47,    42,    42,
      41,   149,    40,   303,    54,    41,    42,   173,   172,   248,
     303,    43,    47,   318,   258,   309,    43,    54,   320,   236,
      41,   142,   117,   142,   317,   103,    41,    47,   312,    44,
     118,   186,   201,   205,   206,   208,   223,   225,   235,   212,
     145,   303,    43,   236,   279,    30,    32,    35,   189,   261,
     262,   312,    40,    46,   192,   152,   271,   273,   275,   324,
      40,    54,   301,   303,   304,     1,    42,    43,    46,   187,
      42,    73,    74,    75,   291,   293,     1,    43,    66,    73,
      74,    75,    78,   124,   140,   150,   151,   152,   153,   157,
     158,   162,   164,   166,   169,   171,   174,   176,   181,   182,
     183,   184,   201,   205,   206,   213,   217,   221,   222,   223,
     224,   225,   226,   228,   229,   232,   235,   324,   202,   245,
     279,   279,   279,    41,    41,    41,    40,   279,    41,    41,
      41,   249,   249,    48,    48,    48,   249,    48,    48,   322,
     322,   254,   217,   236,   172,   318,   323,    43,   248,    41,
     303,    43,   248,    43,   180,    41,   254,   119,   309,   309,
     119,   309,   241,     4,    46,    54,   103,    87,   120,   258,
      60,    43,    41,    41,   299,   300,   324,   236,    40,    43,
     193,   124,   125,   276,   277,   309,    47,    42,   272,   274,
     324,   236,   265,    54,    76,   305,   324,   248,   290,   312,
     292,   220,    46,    76,    76,    76,   140,   221,   314,    47,
     125,   217,    30,    32,    35,   233,   262,   312,   217,   279,
     279,   258,   249,    48,    48,   249,   249,   245,    47,    41,
     173,   303,    43,   248,   172,    43,    43,   317,   317,   104,
     258,   209,   258,    40,   299,   188,    41,   194,   277,   277,
     271,   125,    54,    43,   248,   152,   271,   275,    42,   272,
      41,    43,   267,   306,   307,   312,    43,    46,   303,    48,
     284,   285,   324,   298,   217,   218,   314,    40,    43,   204,
     248,    76,    43,    47,   246,   249,   249,    43,    43,    43,
     303,    43,   247,   104,    40,   128,   131,   253,   236,   187,
     303,    43,   125,   278,   279,   303,   275,    43,    46,    43,
      42,    48,    40,    46,   188,    48,   312,   217,    40,   236,
     303,   279,   204,    41,    43,    43,   236,    40,    40,    40,
     131,    41,   111,   192,   188,   307,    48,   187,    48,   285,
      47,   236,    41,   188,    43,    41,   236,   236,   236,    40,
     304,   258,   193,   187,    48,    48,   219,    41,   230,   303,
     187,   231,   303,    41,    41,    41,   236,   192,    48,   217,
     231,    43,    46,    54,    43,    46,    54,   231,   231,   231,
      41,   193,    48,   267,   265,   231,    43,    43
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   143,   144,   144,   144,   144,   144,   144,   144,   145,
     145,   145,   145,   146,   146,   146,   146,   146,   146,   146,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   147,   147,   147,   147,   147,   147,   147,   147,   147,
     147,   149,   148,   150,   151,   152,   152,   152,   152,   153,
     153,   154,   154,   154,   154,   155,   156,   156,   157,   157,
     157,   159,   158,   160,   160,   161,   161,   162,   162,   162,
     162,   163,   164,   164,   165,   165,   166,   166,   167,   167,
     168,   168,   169,   169,   169,   170,   170,   171,   171,   171,
     171,   171,   171,   171,   171,   172,   172,   172,   173,   173,
     174,   175,   175,   176,   176,   176,   177,   178,   179,   179,
     180,   180,   180,   181,   182,   183,   184,   184,   184,   185,
     184,   184,   184,   184,   184,   186,   186,   187,   187,   187,
     187,   188,   189,   189,   189,   189,   189,   189,   190,   190,
     190,   191,   192,   193,   194,   193,   195,   195,   195,   196,
     196,   197,   198,   198,   199,   200,   200,   200,   200,   200,
     200,   202,   201,   203,   201,   204,   204,   205,   207,   206,
     206,   206,   208,   208,   208,   208,   208,   208,   209,   210,
     210,   211,   211,   212,   212,   213,   213,   215,   214,   216,
     214,   214,   217,   218,   219,   217,   217,   217,   220,   217,
     221,   221,   221,   221,   221,   221,   221,   221,   221,   221,
     221,   221,   221,   221,   221,   221,   221,   221,   222,   222,
     222,   223,   224,   224,   225,   225,   225,   225,   225,   226,
     227,   228,   228,   228,   229,   229,   229,   229,   229,   229,
     229,   229,   229,   229,   229,   230,   230,   230,   231,   231,
     231,   232,   233,   233,   233,   233,   233,   234,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   235,   235,
     235,   235,   235,   235,   235,   235,   235,   235,   236,   237,
     237,   238,   238,   238,   239,   239,   239,   240,   240,   240,
     241,   242,   242,   243,   243,   244,   244,   245,   245,   245,
     245,   245,   246,   246,   246,   246,   247,   247,   247,   247,
     248,   248,   248,   248,   248,   248,   248,   248,   248,   248,
     248,   248,   248,   248,   248,   248,   248,   248,   248,   248,
     248,   248,   249,   249,   249,   249,   249,   249,   249,   249,
     250,   250,   250,   250,   250,   250,   250,   250,   250,   250,
     250,   250,   250,   251,   251,   251,   251,   251,   251,   251,
     251,   251,   251,   251,   251,   251,   251,   251,   252,   252,
     252,   252,   252,   252,   252,   252,   253,   253,   253,   253,
     254,   254,   254,   255,   255,   256,   256,   257,   257,   257,
     258,   259,   259,   259,   259,   260,   260,   260,   260,   260,
     260,   260,   260,   261,   262,   263,   263,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   266,   265,   265,   267,   267,   268,   269,   270,   270,
     271,   271,   272,   272,   272,   273,   273,   273,   274,   274,
     275,   276,   276,   276,   276,   276,   277,   277,   278,   279,
     279,   280,   280,   280,   281,   281,   281,   281,   281,   281,
     281,   281,   281,   281,   281,   281,   281,   281,   281,   281,
     281,   281,   281,   282,   282,   282,   282,   282,   282,   282,
     282,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   283,   283,   283,   283,   283,   283,   283,
     283,   283,   283,   284,   285,   285,   286,   288,   287,   287,
     289,   289,   291,   290,   292,   290,   293,   293,   293,   294,
     294,   294,   294,   295,   295,   295,   296,   296,   296,   297,
     297,   298,   298,   299,   299,   299,   299,   300,   300,   301,
     301,   301,   301,   301,   301,   302,   302,   302,   303,   303,
     304,   304,   304,   304,   304,   304,   305,   305,   306,   306,
     306,   306,   307,   307,   308,   309,   309,   309,   310,   310,
     310,   311,   311,   312,   312,   312,   312,   312,   312,   312,
     313,   313,   313,   313,   314,   314,   315,   315,   316,   316,
     316,   316,   316,   316,   317,   317,   317,   317,   318,   318,
     319,   319,   320,   320,   320,   321,   321,   322,   322,   322,
     322,   322,   322,   323,   323,   324
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     3,     2,     3,     2,     5,     3,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     8,     5,     3,     5,     5,     9,     3,     2,
       2,     5,     2,     5,     2,     4,     1,     1,     7,     7,
       5,     0,     7,     1,     1,     2,     2,     1,     5,     5,
       5,     3,     4,     3,     7,     8,     5,     3,     1,     1,
       3,     1,     4,     7,     6,     1,     1,     7,     9,     8,
      10,     5,     7,     6,     8,     1,     1,     5,     4,     5,
       7,     1,     3,     6,     6,     8,     1,     2,     3,     1,
       2,     3,     6,     5,     9,     2,     1,     1,     1,     0,
       6,     1,     6,    10,     1,     6,     9,     1,     5,     1,
       1,     1,     1,     1,     1,     1,     1,     1,    11,    13,
       7,     1,     1,     1,     0,     3,     1,     2,     2,     2,
       1,     5,     8,    11,     6,     1,     1,     1,     1,     1,
       1,     0,     9,     0,     8,     1,     4,     4,     0,     6,
       3,     4,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     3,     1,     3,     4,     0,     6,     0,
       5,     5,     2,     0,     0,     7,     1,     1,     0,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     6,     6,     7,     8,     8,     8,     9,     7,     5,
       2,     2,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     4,     2,     2,     4,
       2,     5,     1,     1,     1,     1,     1,     2,     1,     1,
       2,     2,     1,     1,     1,     1,     1,     1,     2,     2,
       2,     2,     1,     2,     2,     2,     2,     1,     1,     2,
       1,     3,     4,     1,     2,     7,     3,     1,     2,     2,
       1,     2,     1,     3,     1,     1,     1,     2,     5,     2,
       2,     1,     2,     2,     1,     5,     1,     1,     5,     1,
       2,     3,     3,     1,     2,     2,     3,     4,     5,     4,
       5,     6,     6,     4,     5,     5,     6,     7,     8,     8,
       7,     7,     1,     2,     3,     4,     5,     3,     4,     4,
       1,     2,     4,     4,     4,     5,     6,     5,     6,     3,
       4,     4,     5,     1,     2,     2,     2,     3,     3,     1,
       2,     2,     1,     1,     2,     3,     3,     4,     3,     4,
       2,     3,     3,     4,     5,     3,     3,     2,     2,     1,
       1,     2,     1,     1,     1,     1,     2,     1,     1,     1,
       1,     2,     1,     2,     3,     1,     1,     1,     2,     1,
       1,     2,     1,     4,     1,     1,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     2,     3,     1,     2,     3,
       3,     1,     2,     2,     2,     3,     1,     3,     1,     1,
       1,     3,     3,     3,     1,     1,     1,     5,     8,     1,
       1,     1,     1,     3,     4,     5,     5,     5,     6,     6,
       2,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     5,     2,     2,
       2,     2,     2,     3,     1,     1,     1,     0,     3,     1,
       1,     3,     0,     4,     0,     6,     1,     1,     1,     1,
       1,     4,     4,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     1,     1,     4,
       1,     1,     5,     2,     4,     1,     1,     2,     1,     1,
       3,     3,     4,     4,     3,     4,     2,     1,     1,     3,
       4,     6,     2,     2,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     4,     1,     3,     1,     2,     3,
       3,     2,     2,     2,     1,     2,     1,     3,     2,     4,
       1,     3,     1,     3,     3,     2,     2,     2,     2,     1,
       2,     1,     1,     1,     1,     3,     1,     3,     5,     1,
       3,     3,     5,     1,     1,     0
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 1690 "CParse/parser.y" /* yacc.c:1645  */
    {
                   if (!classes) classes = NewHash();
		   Setattr((yyvsp[0].node),"classes",classes); 
		   Setattr((yyvsp[0].node),"name",ModuleName);
		   
		   if ((!module_node) && ModuleName) {
		     module_node = new_node("module");
		     Setattr(module_node,"name",ModuleName);
		   }
		   Setattr((yyvsp[0].node),"module",module_node);
	           top = (yyvsp[0].node);
               }
#line 4826 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 3:
#line 1702 "CParse/parser.y" /* yacc.c:1645  */
    {
                 top = Copy(Getattr((yyvsp[-1].p),"type"));
		 Delete((yyvsp[-1].p));
               }
#line 4835 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 4:
#line 1706 "CParse/parser.y" /* yacc.c:1645  */
    {
                 top = 0;
               }
#line 4843 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 5:
#line 1709 "CParse/parser.y" /* yacc.c:1645  */
    {
                 top = (yyvsp[-1].p);
               }
#line 4851 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 6:
#line 1712 "CParse/parser.y" /* yacc.c:1645  */
    {
                 top = 0;
               }
#line 4859 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 7:
#line 1715 "CParse/parser.y" /* yacc.c:1645  */
    {
                 top = (yyvsp[-2].pl);
               }
#line 4867 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 8:
#line 1718 "CParse/parser.y" /* yacc.c:1645  */
    {
                 top = 0;
               }
#line 4875 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 9:
#line 1723 "CParse/parser.y" /* yacc.c:1645  */
    {  
                   /* add declaration to end of linked list (the declaration isn't always a single declaration, sometimes it is a linked list itself) */
                   if (currentDeclComment != NULL) {
		     set_comment((yyvsp[0].node), currentDeclComment);
		     currentDeclComment = NULL;
                   }                                      
                   appendChild((yyvsp[-1].node),(yyvsp[0].node));
                   (yyval.node) = (yyvsp[-1].node);
               }
#line 4889 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 10:
#line 1732 "CParse/parser.y" /* yacc.c:1645  */
    {
                   currentDeclComment = (yyvsp[0].str); 
                   (yyval.node) = (yyvsp[-1].node);
               }
#line 4898 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 11:
#line 1736 "CParse/parser.y" /* yacc.c:1645  */
    {
                   Node *node = lastChild((yyvsp[-1].node));
                   if (node) {
                     set_comment(node, (yyvsp[0].str));
                   }
                   (yyval.node) = (yyvsp[-1].node);
               }
#line 4910 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 12:
#line 1743 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = new_node("top");
               }
#line 4918 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 13:
#line 1748 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4924 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 14:
#line 1749 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4930 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 15:
#line 1750 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4936 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 16:
#line 1751 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 4942 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 17:
#line 1752 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.node) = 0;
		  if (cparse_unknown_directive) {
		      Swig_error(cparse_file, cparse_line, "Unknown directive '%s'.\n", cparse_unknown_directive);
		  } else {
		      Swig_error(cparse_file, cparse_line, "Syntax error in input(1).\n");
		  }
		  exit(1);
               }
#line 4956 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 18:
#line 1762 "CParse/parser.y" /* yacc.c:1645  */
    { 
                  if ((yyval.node)) {
   		      add_symbols((yyval.node));
                  }
                  (yyval.node) = (yyvsp[0].node); 
	       }
#line 4967 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 19:
#line 1778 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.node) = 0;
                  skip_decl();
               }
#line 4976 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 20:
#line 1788 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4982 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 21:
#line 1789 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4988 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 22:
#line 1790 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 4994 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 23:
#line 1791 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5000 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 24:
#line 1792 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5006 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 25:
#line 1793 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5012 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 26:
#line 1794 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5018 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 27:
#line 1795 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5024 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 28:
#line 1796 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5030 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 29:
#line 1797 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5036 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 30:
#line 1798 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5042 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 31:
#line 1799 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5048 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 32:
#line 1800 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5054 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 33:
#line 1801 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5060 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 34:
#line 1802 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5066 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 35:
#line 1803 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5072 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 36:
#line 1804 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5078 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 37:
#line 1805 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5084 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 38:
#line 1806 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5090 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 39:
#line 1807 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5096 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 40:
#line 1808 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 5102 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 41:
#line 1815 "CParse/parser.y" /* yacc.c:1645  */
    {
               Node *cls;
	       String *clsname;
	       extendmode = 1;
	       cplus_mode = CPLUS_PUBLIC;
	       if (!classes) classes = NewHash();
	       if (!classes_typedefs) classes_typedefs = NewHash();
	       clsname = make_class_name((yyvsp[-1].str));
	       cls = Getattr(classes,clsname);
	       if (!cls) {
	         cls = Getattr(classes_typedefs, clsname);
		 if (!cls) {
		   /* No previous definition. Create a new scope */
		   Node *am = Getattr(Swig_extend_hash(),clsname);
		   if (!am) {
		     Swig_symbol_newscope();
		     Swig_symbol_setscopename((yyvsp[-1].str));
		     prev_symtab = 0;
		   } else {
		     prev_symtab = Swig_symbol_setscope(Getattr(am,"symtab"));
		   }
		   current_class = 0;
		 } else {
		   /* Previous typedef class definition.  Use its symbol table.
		      Deprecated, just the real name should be used. 
		      Note that %extend before the class typedef never worked, only %extend after the class typdef. */
		   prev_symtab = Swig_symbol_setscope(Getattr(cls, "symtab"));
		   current_class = cls;
		   SWIG_WARN_NODE_BEGIN(cls);
		   Swig_warning(WARN_PARSE_EXTEND_NAME, cparse_file, cparse_line, "Deprecated %%extend name used - the %s name '%s' should be used instead of the typedef name '%s'.\n", Getattr(cls, "kind"), SwigType_namestr(Getattr(cls, "name")), (yyvsp[-1].str));
		   SWIG_WARN_NODE_END(cls);
		 }
	       } else {
		 /* Previous class definition.  Use its symbol table */
		 prev_symtab = Swig_symbol_setscope(Getattr(cls,"symtab"));
		 current_class = cls;
	       }
	       Classprefix = NewString((yyvsp[-1].str));
	       Namespaceprefix= Swig_symbol_qualifiedscopename(0);
	       Delete(clsname);
	     }
#line 5148 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 42:
#line 1855 "CParse/parser.y" /* yacc.c:1645  */
    {
               String *clsname;
	       extendmode = 0;
               (yyval.node) = new_node("extend");
	       Setattr((yyval.node),"symtab",Swig_symbol_popscope());
	       if (prev_symtab) {
		 Swig_symbol_setscope(prev_symtab);
	       }
	       Namespaceprefix = Swig_symbol_qualifiedscopename(0);
               clsname = make_class_name((yyvsp[-4].str));
	       Setattr((yyval.node),"name",clsname);

	       mark_nodes_as_extend((yyvsp[-1].node));
	       if (current_class) {
		 /* We add the extension to the previously defined class */
		 appendChild((yyval.node), (yyvsp[-1].node));
		 appendChild(current_class,(yyval.node));
	       } else {
		 /* We store the extensions in the extensions hash */
		 Node *am = Getattr(Swig_extend_hash(),clsname);
		 if (am) {
		   /* Append the members to the previous extend methods */
		   appendChild(am, (yyvsp[-1].node));
		 } else {
		   appendChild((yyval.node), (yyvsp[-1].node));
		   Setattr(Swig_extend_hash(),clsname,(yyval.node));
		 }
	       }
	       current_class = 0;
	       Delete(Classprefix);
	       Delete(clsname);
	       Classprefix = 0;
	       prev_symtab = 0;
	       (yyval.node) = 0;

	     }
#line 5189 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 43:
#line 1897 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.node) = new_node("apply");
                    Setattr((yyval.node),"pattern",Getattr((yyvsp[-3].p),"pattern"));
		    appendChild((yyval.node),(yyvsp[-1].p));
               }
#line 5199 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 44:
#line 1907 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = new_node("clear");
		 appendChild((yyval.node),(yyvsp[-1].p));
               }
#line 5208 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 45:
#line 1918 "CParse/parser.y" /* yacc.c:1645  */
    {
		   if (((yyvsp[-1].dtype).type != T_ERROR) && ((yyvsp[-1].dtype).type != T_SYMBOL)) {
		     SwigType *type = NewSwigType((yyvsp[-1].dtype).type);
		     (yyval.node) = new_node("constant");
		     Setattr((yyval.node),"name",(yyvsp[-3].id));
		     Setattr((yyval.node),"type",type);
		     Setattr((yyval.node),"value",(yyvsp[-1].dtype).val);
		     if ((yyvsp[-1].dtype).rawval) Setattr((yyval.node),"rawval", (yyvsp[-1].dtype).rawval);
		     Setattr((yyval.node),"storage","%constant");
		     SetFlag((yyval.node),"feature:immutable");
		     add_symbols((yyval.node));
		     Delete(type);
		   } else {
		     if ((yyvsp[-1].dtype).type == T_ERROR) {
		       Swig_warning(WARN_PARSE_UNSUPPORTED_VALUE,cparse_file,cparse_line,"Unsupported constant value (ignored)\n");
		     }
		     (yyval.node) = 0;
		   }

	       }
#line 5233 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 46:
#line 1938 "CParse/parser.y" /* yacc.c:1645  */
    {
		 if (((yyvsp[-1].dtype).type != T_ERROR) && ((yyvsp[-1].dtype).type != T_SYMBOL)) {
		   SwigType_push((yyvsp[-3].type),(yyvsp[-2].decl).type);
		   /* Sneaky callback function trick */
		   if (SwigType_isfunction((yyvsp[-3].type))) {
		     SwigType_add_pointer((yyvsp[-3].type));
		   }
		   (yyval.node) = new_node("constant");
		   Setattr((yyval.node),"name",(yyvsp[-2].decl).id);
		   Setattr((yyval.node),"type",(yyvsp[-3].type));
		   Setattr((yyval.node),"value",(yyvsp[-1].dtype).val);
		   if ((yyvsp[-1].dtype).rawval) Setattr((yyval.node),"rawval", (yyvsp[-1].dtype).rawval);
		   Setattr((yyval.node),"storage","%constant");
		   SetFlag((yyval.node),"feature:immutable");
		   add_symbols((yyval.node));
		 } else {
		   if ((yyvsp[-1].dtype).type == T_ERROR) {
		     Swig_warning(WARN_PARSE_UNSUPPORTED_VALUE,cparse_file,cparse_line, "Unsupported constant value\n");
		   }
		   (yyval.node) = 0;
		 }
               }
#line 5260 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 47:
#line 1962 "CParse/parser.y" /* yacc.c:1645  */
    {
		 if (((yyvsp[-1].dtype).type != T_ERROR) && ((yyvsp[-1].dtype).type != T_SYMBOL)) {
		   SwigType_add_function((yyvsp[-7].type), (yyvsp[-4].pl));
		   SwigType_push((yyvsp[-7].type), (yyvsp[-2].dtype).qualifier);
		   SwigType_push((yyvsp[-7].type), (yyvsp[-6].decl).type);
		   /* Sneaky callback function trick */
		   if (SwigType_isfunction((yyvsp[-7].type))) {
		     SwigType_add_pointer((yyvsp[-7].type));
		   }
		   (yyval.node) = new_node("constant");
		   Setattr((yyval.node), "name", (yyvsp[-6].decl).id);
		   Setattr((yyval.node), "type", (yyvsp[-7].type));
		   Setattr((yyval.node), "value", (yyvsp[-1].dtype).val);
		   if ((yyvsp[-1].dtype).rawval) Setattr((yyval.node), "rawval", (yyvsp[-1].dtype).rawval);
		   Setattr((yyval.node), "storage", "%constant");
		   SetFlag((yyval.node), "feature:immutable");
		   add_symbols((yyval.node));
		 } else {
		   if ((yyvsp[-1].dtype).type == T_ERROR) {
		     Swig_warning(WARN_PARSE_UNSUPPORTED_VALUE,cparse_file,cparse_line, "Unsupported constant value\n");
		   }
		   (yyval.node) = 0;
		 }
	       }
#line 5289 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 48:
#line 1986 "CParse/parser.y" /* yacc.c:1645  */
    {
		 Swig_warning(WARN_PARSE_BAD_VALUE,cparse_file,cparse_line,"Bad constant value (ignored).\n");
		 (yyval.node) = 0;
	       }
#line 5298 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 49:
#line 1997 "CParse/parser.y" /* yacc.c:1645  */
    {
		 char temp[64];
		 Replace((yyvsp[0].str),"$file",cparse_file, DOH_REPLACE_ANY);
		 sprintf(temp,"%d", cparse_line);
		 Replace((yyvsp[0].str),"$line",temp,DOH_REPLACE_ANY);
		 Printf(stderr,"%s\n", (yyvsp[0].str));
		 Delete((yyvsp[0].str));
                 (yyval.node) = 0;
	       }
#line 5312 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 50:
#line 2006 "CParse/parser.y" /* yacc.c:1645  */
    {
		 char temp[64];
		 String *s = (yyvsp[0].str);
		 Replace(s,"$file",cparse_file, DOH_REPLACE_ANY);
		 sprintf(temp,"%d", cparse_line);
		 Replace(s,"$line",temp,DOH_REPLACE_ANY);
		 Printf(stderr,"%s\n", s);
		 Delete(s);
                 (yyval.node) = 0;
               }
#line 5327 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 51:
#line 2025 "CParse/parser.y" /* yacc.c:1645  */
    {
                    skip_balanced('{','}');
		    (yyval.node) = 0;
		    Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
	       }
#line 5337 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 52:
#line 2031 "CParse/parser.y" /* yacc.c:1645  */
    {
                    skip_balanced('{','}');
		    (yyval.node) = 0;
		    Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
               }
#line 5347 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 53:
#line 2037 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = 0;
		 Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
               }
#line 5356 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 54:
#line 2042 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = 0;
		 Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
	       }
#line 5365 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 55:
#line 2049 "CParse/parser.y" /* yacc.c:1645  */
    {		 
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"value",(yyvsp[-3].str));
		 Setattr((yyval.node),"type",Getattr((yyvsp[-1].p),"type"));
               }
#line 5375 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 56:
#line 2056 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"value",(yyvsp[0].str));
              }
#line 5384 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 57:
#line 2060 "CParse/parser.y" /* yacc.c:1645  */
    {
                (yyval.node) = (yyvsp[0].node);
              }
#line 5392 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 58:
#line 2073 "CParse/parser.y" /* yacc.c:1645  */
    {
                   Hash *p = (yyvsp[-2].node);
		   (yyval.node) = new_node("fragment");
		   Setattr((yyval.node),"value",Getattr((yyvsp[-4].node),"value"));
		   Setattr((yyval.node),"type",Getattr((yyvsp[-4].node),"type"));
		   Setattr((yyval.node),"section",Getattr(p,"name"));
		   Setattr((yyval.node),"kwargs",nextSibling(p));
		   Setattr((yyval.node),"code",(yyvsp[0].str));
                 }
#line 5406 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 59:
#line 2082 "CParse/parser.y" /* yacc.c:1645  */
    {
		   Hash *p = (yyvsp[-2].node);
		   String *code;
                   skip_balanced('{','}');
		   (yyval.node) = new_node("fragment");
		   Setattr((yyval.node),"value",Getattr((yyvsp[-4].node),"value"));
		   Setattr((yyval.node),"type",Getattr((yyvsp[-4].node),"type"));
		   Setattr((yyval.node),"section",Getattr(p,"name"));
		   Setattr((yyval.node),"kwargs",nextSibling(p));
		   Delitem(scanner_ccode,0);
		   Delitem(scanner_ccode,DOH_END);
		   code = Copy(scanner_ccode);
		   Setattr((yyval.node),"code",code);
		   Delete(code);
                 }
#line 5426 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 60:
#line 2097 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = new_node("fragment");
		   Setattr((yyval.node),"value",Getattr((yyvsp[-2].node),"value"));
		   Setattr((yyval.node),"type",Getattr((yyvsp[-2].node),"type"));
		   Setattr((yyval.node),"emitonly","1");
		 }
#line 5437 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 61:
#line 2110 "CParse/parser.y" /* yacc.c:1645  */
    {
                     (yyvsp[-3].loc).filename = Copy(cparse_file);
		     (yyvsp[-3].loc).line = cparse_line;
		     scanner_set_location((yyvsp[-1].str),1);
                     if ((yyvsp[-2].node)) { 
		       String *maininput = Getattr((yyvsp[-2].node), "maininput");
		       if (maininput)
		         scanner_set_main_input_file(NewString(maininput));
		     }
               }
#line 5452 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 62:
#line 2119 "CParse/parser.y" /* yacc.c:1645  */
    {
                     String *mname = 0;
                     (yyval.node) = (yyvsp[-1].node);
		     scanner_set_location((yyvsp[-6].loc).filename,(yyvsp[-6].loc).line+1);
		     if (strcmp((yyvsp[-6].loc).type,"include") == 0) set_nodeType((yyval.node),"include");
		     if (strcmp((yyvsp[-6].loc).type,"import") == 0) {
		       mname = (yyvsp[-5].node) ? Getattr((yyvsp[-5].node),"module") : 0;
		       set_nodeType((yyval.node),"import");
		       if (import_mode) --import_mode;
		     }
		     
		     Setattr((yyval.node),"name",(yyvsp[-4].str));
		     /* Search for the module (if any) */
		     {
			 Node *n = firstChild((yyval.node));
			 while (n) {
			     if (Strcmp(nodeType(n),"module") == 0) {
			         if (mname) {
				   Setattr(n,"name", mname);
				   mname = 0;
				 }
				 Setattr((yyval.node),"module",Getattr(n,"name"));
				 break;
			     }
			     n = nextSibling(n);
			 }
			 if (mname) {
			   /* There is no module node in the import
			      node, ie, you imported a .h file
			      directly.  We are forced then to create
			      a new import node with a module node.
			   */			      
			   Node *nint = new_node("import");
			   Node *mnode = new_node("module");
			   Setattr(mnode,"name", mname);
                           Setattr(mnode,"options",(yyvsp[-5].node));
			   appendChild(nint,mnode);
			   Delete(mnode);
			   appendChild(nint,firstChild((yyval.node)));
			   (yyval.node) = nint;
			   Setattr((yyval.node),"module",mname);
			 }
		     }
		     Setattr((yyval.node),"options",(yyvsp[-5].node));
               }
#line 5502 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 63:
#line 2166 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.loc).type = "include"; }
#line 5508 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 64:
#line 2167 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.loc).type = "import"; ++import_mode;}
#line 5514 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 65:
#line 2174 "CParse/parser.y" /* yacc.c:1645  */
    {
                 String *cpps;
		 if (Namespaceprefix) {
		   Swig_error(cparse_file, cparse_start_line, "%%inline directive inside a namespace is disallowed.\n");
		   (yyval.node) = 0;
		 } else {
		   (yyval.node) = new_node("insert");
		   Setattr((yyval.node),"code",(yyvsp[0].str));
		   /* Need to run through the preprocessor */
		   Seek((yyvsp[0].str),0,SEEK_SET);
		   Setline((yyvsp[0].str),cparse_start_line);
		   Setfile((yyvsp[0].str),cparse_file);
		   cpps = Preprocessor_parse((yyvsp[0].str));
		   start_inline(Char(cpps), cparse_start_line);
		   Delete((yyvsp[0].str));
		   Delete(cpps);
		 }
		 
	       }
#line 5538 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 66:
#line 2193 "CParse/parser.y" /* yacc.c:1645  */
    {
                 String *cpps;
		 int start_line = cparse_line;
		 skip_balanced('{','}');
		 if (Namespaceprefix) {
		   Swig_error(cparse_file, cparse_start_line, "%%inline directive inside a namespace is disallowed.\n");
		   
		   (yyval.node) = 0;
		 } else {
		   String *code;
                   (yyval.node) = new_node("insert");
		   Delitem(scanner_ccode,0);
		   Delitem(scanner_ccode,DOH_END);
		   code = Copy(scanner_ccode);
		   Setattr((yyval.node),"code", code);
		   Delete(code);		   
		   cpps=Copy(scanner_ccode);
		   start_inline(Char(cpps), start_line);
		   Delete(cpps);
		 }
               }
#line 5564 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 67:
#line 2224 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"code",(yyvsp[0].str));
	       }
#line 5573 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 68:
#line 2228 "CParse/parser.y" /* yacc.c:1645  */
    {
		 String *code = NewStringEmpty();
		 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"section",(yyvsp[-2].id));
		 Setattr((yyval.node),"code",code);
		 if (Swig_insert_file((yyvsp[0].str),code) < 0) {
		   Swig_error(cparse_file, cparse_line, "Couldn't find '%s'.\n", (yyvsp[0].str));
		   (yyval.node) = 0;
		 } 
               }
#line 5588 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 69:
#line 2238 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"section",(yyvsp[-2].id));
		 Setattr((yyval.node),"code",(yyvsp[0].str));
               }
#line 5598 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 70:
#line 2243 "CParse/parser.y" /* yacc.c:1645  */
    {
		 String *code;
                 skip_balanced('{','}');
		 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"section",(yyvsp[-2].id));
		 Delitem(scanner_ccode,0);
		 Delitem(scanner_ccode,DOH_END);
		 code = Copy(scanner_ccode);
		 Setattr((yyval.node),"code", code);
		 Delete(code);
	       }
#line 5614 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 71:
#line 2261 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = new_node("module");
		 if ((yyvsp[-1].node)) {
		   Setattr((yyval.node),"options",(yyvsp[-1].node));
		   if (Getattr((yyvsp[-1].node),"directors")) {
		     Wrapper_director_mode_set(1);
		     if (!cparse_cplusplus) {
		       Swig_error(cparse_file, cparse_line, "Directors are not supported for C code and require the -c++ option\n");
		     }
		   } 
		   if (Getattr((yyvsp[-1].node),"dirprot")) {
		     Wrapper_director_protected_mode_set(1);
		   } 
		   if (Getattr((yyvsp[-1].node),"allprotected")) {
		     Wrapper_all_protected_mode_set(1);
		   } 
		   if (Getattr((yyvsp[-1].node),"templatereduce")) {
		     template_reduce = 1;
		   }
		   if (Getattr((yyvsp[-1].node),"notemplatereduce")) {
		     template_reduce = 0;
		   }
		 }
		 if (!ModuleName) ModuleName = NewString((yyvsp[0].id));
		 if (!import_mode) {
		   /* first module included, we apply global
		      ModuleName, which can be modify by -module */
		   String *mname = Copy(ModuleName);
		   Setattr((yyval.node),"name",mname);
		   Delete(mname);
		 } else { 
		   /* import mode, we just pass the idstring */
		   Setattr((yyval.node),"name",(yyvsp[0].id));   
		 }		 
		 if (!module_node) module_node = (yyval.node);
	       }
#line 5655 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 72:
#line 2304 "CParse/parser.y" /* yacc.c:1645  */
    {
                 Swig_warning(WARN_DEPRECATED_NAME,cparse_file,cparse_line, "%%name is deprecated.  Use %%rename instead.\n");
		 Delete(yyrename);
                 yyrename = NewString((yyvsp[-1].id));
		 (yyval.node) = 0;
               }
#line 5666 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 73:
#line 2310 "CParse/parser.y" /* yacc.c:1645  */
    {
		 Swig_warning(WARN_DEPRECATED_NAME,cparse_file,cparse_line, "%%name is deprecated.  Use %%rename instead.\n");
		 (yyval.node) = 0;
		 Swig_error(cparse_file,cparse_line,"Missing argument to %%name directive.\n");
	       }
#line 5676 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 74:
#line 2323 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = new_node("native");
		 Setattr((yyval.node),"name",(yyvsp[-4].id));
		 Setattr((yyval.node),"wrap:name",(yyvsp[-1].id));
	         add_symbols((yyval.node));
	       }
#line 5687 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 75:
#line 2329 "CParse/parser.y" /* yacc.c:1645  */
    {
		 if (!SwigType_isfunction((yyvsp[-1].decl).type)) {
		   Swig_error(cparse_file,cparse_line,"%%native declaration '%s' is not a function.\n", (yyvsp[-1].decl).id);
		   (yyval.node) = 0;
		 } else {
		     Delete(SwigType_pop_function((yyvsp[-1].decl).type));
		     /* Need check for function here */
		     SwigType_push((yyvsp[-2].type),(yyvsp[-1].decl).type);
		     (yyval.node) = new_node("native");
	             Setattr((yyval.node),"name",(yyvsp[-5].id));
		     Setattr((yyval.node),"wrap:name",(yyvsp[-1].decl).id);
		     Setattr((yyval.node),"type",(yyvsp[-2].type));
		     Setattr((yyval.node),"parms",(yyvsp[-1].decl).parms);
		     Setattr((yyval.node),"decl",(yyvsp[-1].decl).type);
		 }
	         add_symbols((yyval.node));
	       }
#line 5709 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 76:
#line 2355 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = new_node("pragma");
		 Setattr((yyval.node),"lang",(yyvsp[-3].id));
		 Setattr((yyval.node),"name",(yyvsp[-2].id));
		 Setattr((yyval.node),"value",(yyvsp[0].str));
	       }
#line 5720 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 77:
#line 2361 "CParse/parser.y" /* yacc.c:1645  */
    {
		(yyval.node) = new_node("pragma");
		Setattr((yyval.node),"lang",(yyvsp[-1].id));
		Setattr((yyval.node),"name",(yyvsp[0].id));
	      }
#line 5730 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 78:
#line 2368 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = (yyvsp[0].str); }
#line 5736 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 79:
#line 2369 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = (yyvsp[0].str); }
#line 5742 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 80:
#line 2372 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (yyvsp[-1].id); }
#line 5748 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 81:
#line 2373 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (char *) "swig"; }
#line 5754 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 82:
#line 2380 "CParse/parser.y" /* yacc.c:1645  */
    {
                SwigType *t = (yyvsp[-2].decl).type;
		Hash *kws = NewHash();
		String *fixname;
		fixname = feature_identifier_fix((yyvsp[-2].decl).id);
		Setattr(kws,"name",(yyvsp[-1].id));
		if (!Len(t)) t = 0;
		/* Special declarator check */
		if (t) {
		  if (SwigType_isfunction(t)) {
		    SwigType *decl = SwigType_pop_function(t);
		    if (SwigType_ispointer(t)) {
		      String *nname = NewStringf("*%s",fixname);
		      if ((yyvsp[-3].intvalue)) {
			Swig_name_rename_add(Namespaceprefix, nname,decl,kws,(yyvsp[-2].decl).parms);
		      } else {
			Swig_name_namewarn_add(Namespaceprefix,nname,decl,kws);
		      }
		      Delete(nname);
		    } else {
		      if ((yyvsp[-3].intvalue)) {
			Swig_name_rename_add(Namespaceprefix,(fixname),decl,kws,(yyvsp[-2].decl).parms);
		      } else {
			Swig_name_namewarn_add(Namespaceprefix,(fixname),decl,kws);
		      }
		    }
		    Delete(decl);
		  } else if (SwigType_ispointer(t)) {
		    String *nname = NewStringf("*%s",fixname);
		    if ((yyvsp[-3].intvalue)) {
		      Swig_name_rename_add(Namespaceprefix,(nname),0,kws,(yyvsp[-2].decl).parms);
		    } else {
		      Swig_name_namewarn_add(Namespaceprefix,(nname),0,kws);
		    }
		    Delete(nname);
		  }
		} else {
		  if ((yyvsp[-3].intvalue)) {
		    Swig_name_rename_add(Namespaceprefix,(fixname),0,kws,(yyvsp[-2].decl).parms);
		  } else {
		    Swig_name_namewarn_add(Namespaceprefix,(fixname),0,kws);
		  }
		}
                (yyval.node) = 0;
		scanner_clear_rename();
              }
#line 5805 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 83:
#line 2426 "CParse/parser.y" /* yacc.c:1645  */
    {
		String *fixname;
		Hash *kws = (yyvsp[-4].node);
		SwigType *t = (yyvsp[-2].decl).type;
		fixname = feature_identifier_fix((yyvsp[-2].decl).id);
		if (!Len(t)) t = 0;
		/* Special declarator check */
		if (t) {
		  if ((yyvsp[-1].dtype).qualifier) SwigType_push(t,(yyvsp[-1].dtype).qualifier);
		  if (SwigType_isfunction(t)) {
		    SwigType *decl = SwigType_pop_function(t);
		    if (SwigType_ispointer(t)) {
		      String *nname = NewStringf("*%s",fixname);
		      if ((yyvsp[-6].intvalue)) {
			Swig_name_rename_add(Namespaceprefix, nname,decl,kws,(yyvsp[-2].decl).parms);
		      } else {
			Swig_name_namewarn_add(Namespaceprefix,nname,decl,kws);
		      }
		      Delete(nname);
		    } else {
		      if ((yyvsp[-6].intvalue)) {
			Swig_name_rename_add(Namespaceprefix,(fixname),decl,kws,(yyvsp[-2].decl).parms);
		      } else {
			Swig_name_namewarn_add(Namespaceprefix,(fixname),decl,kws);
		      }
		    }
		    Delete(decl);
		  } else if (SwigType_ispointer(t)) {
		    String *nname = NewStringf("*%s",fixname);
		    if ((yyvsp[-6].intvalue)) {
		      Swig_name_rename_add(Namespaceprefix,(nname),0,kws,(yyvsp[-2].decl).parms);
		    } else {
		      Swig_name_namewarn_add(Namespaceprefix,(nname),0,kws);
		    }
		    Delete(nname);
		  }
		} else {
		  if ((yyvsp[-6].intvalue)) {
		    Swig_name_rename_add(Namespaceprefix,(fixname),0,kws,(yyvsp[-2].decl).parms);
		  } else {
		    Swig_name_namewarn_add(Namespaceprefix,(fixname),0,kws);
		  }
		}
                (yyval.node) = 0;
		scanner_clear_rename();
              }
#line 5856 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 84:
#line 2472 "CParse/parser.y" /* yacc.c:1645  */
    {
		if ((yyvsp[-5].intvalue)) {
		  Swig_name_rename_add(Namespaceprefix,(yyvsp[-1].str),0,(yyvsp[-3].node),0);
		} else {
		  Swig_name_namewarn_add(Namespaceprefix,(yyvsp[-1].str),0,(yyvsp[-3].node));
		}
		(yyval.node) = 0;
		scanner_clear_rename();
              }
#line 5870 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 85:
#line 2483 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.intvalue) = 1;
                }
#line 5878 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 86:
#line 2486 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.intvalue) = 0;
                }
#line 5886 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 87:
#line 2513 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-4].id), val, 0, (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5897 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 88:
#line 2519 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = Len((yyvsp[-4].str)) ? (yyvsp[-4].str) : 0;
                    new_feature((yyvsp[-6].id), val, 0, (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5908 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 89:
#line 2525 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-5].id), val, (yyvsp[-4].node), (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5919 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 90:
#line 2531 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = Len((yyvsp[-5].str)) ? (yyvsp[-5].str) : 0;
                    new_feature((yyvsp[-7].id), val, (yyvsp[-4].node), (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5930 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 91:
#line 2539 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-2].id), val, 0, 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5941 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 92:
#line 2545 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = Len((yyvsp[-2].str)) ? (yyvsp[-2].str) : 0;
                    new_feature((yyvsp[-4].id), val, 0, 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5952 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 93:
#line 2551 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-3].id), val, (yyvsp[-2].node), 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5963 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 94:
#line 2557 "CParse/parser.y" /* yacc.c:1645  */
    {
                    String *val = Len((yyvsp[-3].str)) ? (yyvsp[-3].str) : 0;
                    new_feature((yyvsp[-5].id), val, (yyvsp[-2].node), 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5974 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 95:
#line 2565 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = (yyvsp[0].str); }
#line 5980 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 96:
#line 2566 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = 0; }
#line 5986 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 97:
#line 2567 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = (yyvsp[-2].pl); }
#line 5992 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 98:
#line 2570 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = NewHash();
		  Setattr((yyval.node),"name",(yyvsp[-2].id));
		  Setattr((yyval.node),"value",(yyvsp[0].str));
                }
#line 6002 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 99:
#line 2575 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = NewHash();
		  Setattr((yyval.node),"name",(yyvsp[-3].id));
		  Setattr((yyval.node),"value",(yyvsp[-1].str));
                  set_nextSibling((yyval.node),(yyvsp[0].node));
                }
#line 6013 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 100:
#line 2585 "CParse/parser.y" /* yacc.c:1645  */
    {
                 Parm *val;
		 String *name;
		 SwigType *t;
		 if (Namespaceprefix) name = NewStringf("%s::%s", Namespaceprefix, (yyvsp[-2].decl).id);
		 else name = NewString((yyvsp[-2].decl).id);
		 val = (yyvsp[-4].pl);
		 if ((yyvsp[-2].decl).parms) {
		   Setmeta(val,"parms",(yyvsp[-2].decl).parms);
		 }
		 t = (yyvsp[-2].decl).type;
		 if (!Len(t)) t = 0;
		 if (t) {
		   if ((yyvsp[-1].dtype).qualifier) SwigType_push(t,(yyvsp[-1].dtype).qualifier);
		   if (SwigType_isfunction(t)) {
		     SwigType *decl = SwigType_pop_function(t);
		     if (SwigType_ispointer(t)) {
		       String *nname = NewStringf("*%s",name);
		       Swig_feature_set(Swig_cparse_features(), nname, decl, "feature:varargs", val, 0);
		       Delete(nname);
		     } else {
		       Swig_feature_set(Swig_cparse_features(), name, decl, "feature:varargs", val, 0);
		     }
		     Delete(decl);
		   } else if (SwigType_ispointer(t)) {
		     String *nname = NewStringf("*%s",name);
		     Swig_feature_set(Swig_cparse_features(),nname,0,"feature:varargs",val, 0);
		     Delete(nname);
		   }
		 } else {
		   Swig_feature_set(Swig_cparse_features(),name,0,"feature:varargs",val, 0);
		 }
		 Delete(name);
		 (yyval.node) = 0;
              }
#line 6053 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 101:
#line 2621 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.pl) = (yyvsp[0].pl); }
#line 6059 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 102:
#line 2622 "CParse/parser.y" /* yacc.c:1645  */
    { 
		  int i;
		  int n;
		  Parm *p;
		  n = atoi(Char((yyvsp[-2].dtype).val));
		  if (n <= 0) {
		    Swig_error(cparse_file, cparse_line,"Argument count in %%varargs must be positive.\n");
		    (yyval.pl) = 0;
		  } else {
		    String *name = Getattr((yyvsp[0].p), "name");
		    (yyval.pl) = Copy((yyvsp[0].p));
		    if (name)
		      Setattr((yyval.pl), "name", NewStringf("%s%d", name, n));
		    for (i = 1; i < n; i++) {
		      p = Copy((yyvsp[0].p));
		      name = Getattr(p, "name");
		      if (name)
		        Setattr(p, "name", NewStringf("%s%d", name, n-i));
		      set_nextSibling(p,(yyval.pl));
		      Delete((yyval.pl));
		      (yyval.pl) = p;
		    }
		  }
                }
#line 6088 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 103:
#line 2657 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = 0;
		   if ((yyvsp[-3].tmap).method) {
		     String *code = 0;
		     (yyval.node) = new_node("typemap");
		     Setattr((yyval.node),"method",(yyvsp[-3].tmap).method);
		     if ((yyvsp[-3].tmap).kwargs) {
		       ParmList *kw = (yyvsp[-3].tmap).kwargs;
                       code = remove_block(kw, (yyvsp[0].str));
		       Setattr((yyval.node),"kwargs", (yyvsp[-3].tmap).kwargs);
		     }
		     code = code ? code : NewString((yyvsp[0].str));
		     Setattr((yyval.node),"code", code);
		     Delete(code);
		     appendChild((yyval.node),(yyvsp[-1].p));
		   }
	       }
#line 6110 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 104:
#line 2674 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = 0;
		 if ((yyvsp[-3].tmap).method) {
		   (yyval.node) = new_node("typemap");
		   Setattr((yyval.node),"method",(yyvsp[-3].tmap).method);
		   appendChild((yyval.node),(yyvsp[-1].p));
		 }
	       }
#line 6123 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 105:
#line 2682 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = 0;
		   if ((yyvsp[-5].tmap).method) {
		     (yyval.node) = new_node("typemapcopy");
		     Setattr((yyval.node),"method",(yyvsp[-5].tmap).method);
		     Setattr((yyval.node),"pattern", Getattr((yyvsp[-1].p),"pattern"));
		     appendChild((yyval.node),(yyvsp[-3].p));
		   }
	       }
#line 6137 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 106:
#line 2695 "CParse/parser.y" /* yacc.c:1645  */
    {
		 Hash *p;
		 String *name;
		 p = nextSibling((yyvsp[0].node));
		 if (p && (!Getattr(p,"value"))) {
 		   /* this is the deprecated two argument typemap form */
 		   Swig_warning(WARN_DEPRECATED_TYPEMAP_LANG,cparse_file, cparse_line,
				"Specifying the language name in %%typemap is deprecated - use #ifdef SWIG<LANG> instead.\n");
		   /* two argument typemap form */
		   name = Getattr((yyvsp[0].node),"name");
		   if (!name || (Strcmp(name,typemap_lang))) {
		     (yyval.tmap).method = 0;
		     (yyval.tmap).kwargs = 0;
		   } else {
		     (yyval.tmap).method = Getattr(p,"name");
		     (yyval.tmap).kwargs = nextSibling(p);
		   }
		 } else {
		   /* one-argument typemap-form */
		   (yyval.tmap).method = Getattr((yyvsp[0].node),"name");
		   (yyval.tmap).kwargs = p;
		 }
                }
#line 6165 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 107:
#line 2720 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.p) = (yyvsp[-1].p);
		 set_nextSibling((yyval.p),(yyvsp[0].p));
		}
#line 6174 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 108:
#line 2726 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.p) = (yyvsp[-1].p);
		 set_nextSibling((yyval.p),(yyvsp[0].p));
                }
#line 6183 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 109:
#line 2730 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.p) = 0;}
#line 6189 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 110:
#line 2733 "CParse/parser.y" /* yacc.c:1645  */
    {
                  Parm *parm;
		  SwigType_push((yyvsp[-1].type),(yyvsp[0].decl).type);
		  (yyval.p) = new_node("typemapitem");
		  parm = NewParmWithoutFileLineInfo((yyvsp[-1].type),(yyvsp[0].decl).id);
		  Setattr((yyval.p),"pattern",parm);
		  Setattr((yyval.p),"parms", (yyvsp[0].decl).parms);
		  Delete(parm);
		  /*		  $$ = NewParmWithoutFileLineInfo($1,$2.id);
				  Setattr($$,"parms",$2.parms); */
                }
#line 6205 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 111:
#line 2744 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.p) = new_node("typemapitem");
		  Setattr((yyval.p),"pattern",(yyvsp[-1].pl));
		  /*		  Setattr($$,"multitype",$2); */
               }
#line 6215 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 112:
#line 2749 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.p) = new_node("typemapitem");
		 Setattr((yyval.p),"pattern", (yyvsp[-4].pl));
		 /*                 Setattr($$,"multitype",$2); */
		 Setattr((yyval.p),"parms",(yyvsp[-1].pl));
               }
#line 6226 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 113:
#line 2762 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = new_node("types");
		   Setattr((yyval.node),"parms",(yyvsp[-2].pl));
                   if ((yyvsp[0].str))
		     Setattr((yyval.node),"convcode",NewString((yyvsp[0].str)));
               }
#line 6237 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 114:
#line 2774 "CParse/parser.y" /* yacc.c:1645  */
    {
                  Parm *p, *tp;
		  Node *n;
		  Node *outer_class = currentOuterClass;
		  Symtab *tscope = 0;
		  int     specialized = 0;
		  int     variadic = 0;

		  (yyval.node) = 0;

		  tscope = Swig_symbol_current();          /* Get the current scope */

		  /* If the class name is qualified, we need to create or lookup namespace entries */
		  (yyvsp[-4].str) = resolve_create_node_scope((yyvsp[-4].str), 0);

		  if (nscope_inner && Strcmp(nodeType(nscope_inner), "class") == 0) {
		    outer_class	= nscope_inner;
		  }

		  /*
		    We use the new namespace entry 'nscope' only to
		    emit the template node. The template parameters are
		    resolved in the current 'tscope'.

		    This is closer to the C++ (typedef) behavior.
		  */
		  n = Swig_cparse_template_locate((yyvsp[-4].str),(yyvsp[-2].p),tscope);

		  /* Patch the argument types to respect namespaces */
		  p = (yyvsp[-2].p);
		  while (p) {
		    SwigType *value = Getattr(p,"value");
		    if (!value) {
		      SwigType *ty = Getattr(p,"type");
		      if (ty) {
			SwigType *rty = 0;
			int reduce = template_reduce;
			if (reduce || !SwigType_ispointer(ty)) {
			  rty = Swig_symbol_typedef_reduce(ty,tscope);
			  if (!reduce) reduce = SwigType_ispointer(rty);
			}
			ty = reduce ? Swig_symbol_type_qualify(rty,tscope) : Swig_symbol_type_qualify(ty,tscope);
			Setattr(p,"type",ty);
			Delete(ty);
			Delete(rty);
		      }
		    } else {
		      value = Swig_symbol_type_qualify(value,tscope);
		      Setattr(p,"value",value);
		      Delete(value);
		    }

		    p = nextSibling(p);
		  }

		  /* Look for the template */
		  {
                    Node *nn = n;
                    Node *linklistend = 0;
                    Node *linkliststart = 0;
                    while (nn) {
                      Node *templnode = 0;
                      if (Strcmp(nodeType(nn),"template") == 0) {
                        int nnisclass = (Strcmp(Getattr(nn,"templatetype"),"class") == 0); /* if not a templated class it is a templated function */
                        Parm *tparms = Getattr(nn,"templateparms");
                        if (!tparms) {
                          specialized = 1;
                        } else if (Getattr(tparms,"variadic") && strncmp(Char(Getattr(tparms,"variadic")), "1", 1)==0) {
                          variadic = 1;
                        }
                        if (nnisclass && !variadic && !specialized && (ParmList_len((yyvsp[-2].p)) > ParmList_len(tparms))) {
                          Swig_error(cparse_file, cparse_line, "Too many template parameters. Maximum of %d.\n", ParmList_len(tparms));
                        } else if (nnisclass && !specialized && ((ParmList_len((yyvsp[-2].p)) < (ParmList_numrequired(tparms) - (variadic?1:0))))) { /* Variadic parameter is optional */
                          Swig_error(cparse_file, cparse_line, "Not enough template parameters specified. %d required.\n", (ParmList_numrequired(tparms)-(variadic?1:0)) );
                        } else if (!nnisclass && ((ParmList_len((yyvsp[-2].p)) != ParmList_len(tparms)))) {
                          /* must be an overloaded templated method - ignore it as it is overloaded with a different number of template parameters */
                          nn = Getattr(nn,"sym:nextSibling"); /* repeat for overloaded templated functions */
                          continue;
                        } else {
			  String *tname = Copy((yyvsp[-4].str));
                          int def_supplied = 0;
                          /* Expand the template */
			  Node *templ = Swig_symbol_clookup((yyvsp[-4].str),0);
			  Parm *targs = templ ? Getattr(templ,"templateparms") : 0;

                          ParmList *temparms;
                          if (specialized) temparms = CopyParmList((yyvsp[-2].p));
                          else temparms = CopyParmList(tparms);

                          /* Create typedef's and arguments */
                          p = (yyvsp[-2].p);
                          tp = temparms;
                          if (!p && ParmList_len(p) != ParmList_len(temparms)) {
                            /* we have no template parameters supplied in %template for a template that has default args*/
                            p = tp;
                            def_supplied = 1;
                          }

                          while (p) {
                            String *value = Getattr(p,"value");
                            if (def_supplied) {
                              Setattr(p,"default","1");
                            }
                            if (value) {
                              Setattr(tp,"value",value);
                            } else {
                              SwigType *ty = Getattr(p,"type");
                              if (ty) {
                                Setattr(tp,"type",ty);
                              }
                              Delattr(tp,"value");
                            }
			    /* fix default arg values */
			    if (targs) {
			      Parm *pi = temparms;
			      Parm *ti = targs;
			      String *tv = Getattr(tp,"value");
			      if (!tv) tv = Getattr(tp,"type");
			      while(pi != tp && ti && pi) {
				String *name = Getattr(ti,"name");
				String *value = Getattr(pi,"value");
				if (!value) value = Getattr(pi,"type");
				Replaceid(tv, name, value);
				pi = nextSibling(pi);
				ti = nextSibling(ti);
			      }
			    }
                            p = nextSibling(p);
                            tp = nextSibling(tp);
                            if (!p && tp) {
                              p = tp;
                              def_supplied = 1;
                            } else if (p && !tp) { /* Variadic template - tp < p */
			      SWIG_WARN_NODE_BEGIN(nn);
                              Swig_warning(WARN_CPP11_VARIADIC_TEMPLATE,cparse_file, cparse_line,"Only the first variadic template argument is currently supported.\n");
			      SWIG_WARN_NODE_END(nn);
                              break;
                            }
                          }

                          templnode = copy_node(nn);
			  update_nested_classes(templnode); /* update classes nested within template */
                          /* We need to set the node name based on name used to instantiate */
                          Setattr(templnode,"name",tname);
			  Delete(tname);
                          if (!specialized) {
                            Delattr(templnode,"sym:typename");
                          } else {
                            Setattr(templnode,"sym:typename","1");
                          }
			  /* for now, nested %template is allowed only in the same scope as the template declaration */
                          if ((yyvsp[-6].id) && !(nnisclass && ((outer_class && (outer_class != Getattr(nn, "nested:outer")))
			    ||(extendmode && current_class && (current_class != Getattr(nn, "nested:outer")))))) {
			    /*
			       Comment this out for 1.3.28. We need to
			       re-enable it later but first we need to
			       move %ignore from using %rename to use
			       %feature(ignore).

			       String *symname = Swig_name_make(templnode,0,$3,0,0);
			    */
			    String *symname = NewString((yyvsp[-6].id));
                            Swig_cparse_template_expand(templnode,symname,temparms,tscope);
                            Setattr(templnode,"sym:name",symname);
                          } else {
                            static int cnt = 0;
                            String *nname = NewStringf("__dummy_%d__", cnt++);
                            Swig_cparse_template_expand(templnode,nname,temparms,tscope);
                            Setattr(templnode,"sym:name",nname);
			    Delete(nname);
                            Setattr(templnode,"feature:onlychildren", "typemap,typemapitem,typemapcopy,typedef,types,fragment");
			    if ((yyvsp[-6].id)) {
			      Swig_warning(WARN_PARSE_NESTED_TEMPLATE, cparse_file, cparse_line, "Named nested template instantiations not supported. Processing as if no name was given to %%template().\n");
			    }
                          }
                          Delattr(templnode,"templatetype");
                          Setattr(templnode,"template",nn);
                          Setfile(templnode,cparse_file);
                          Setline(templnode,cparse_line);
                          Delete(temparms);
			  if (outer_class && nnisclass) {
			    SetFlag(templnode, "nested");
			    Setattr(templnode, "nested:outer", outer_class);
			  }
                          add_symbols_copy(templnode);

                          if (Strcmp(nodeType(templnode),"class") == 0) {

                            /* Identify pure abstract methods */
                            Setattr(templnode,"abstracts", pure_abstracts(firstChild(templnode)));

                            /* Set up inheritance in symbol table */
                            {
                              Symtab  *csyms;
                              List *baselist = Getattr(templnode,"baselist");
                              csyms = Swig_symbol_current();
                              Swig_symbol_setscope(Getattr(templnode,"symtab"));
                              if (baselist) {
                                List *bases = Swig_make_inherit_list(Getattr(templnode,"name"),baselist, Namespaceprefix);
                                if (bases) {
                                  Iterator s;
                                  for (s = First(bases); s.item; s = Next(s)) {
                                    Symtab *st = Getattr(s.item,"symtab");
                                    if (st) {
				      Setfile(st,Getfile(s.item));
				      Setline(st,Getline(s.item));
                                      Swig_symbol_inherit(st);
                                    }
                                  }
				  Delete(bases);
                                }
                              }
                              Swig_symbol_setscope(csyms);
                            }

                            /* Merge in %extend methods for this class.
			       This only merges methods within %extend for a template specialized class such as
			         template<typename T> class K {}; %extend K<int> { ... }
			       The copy_node() call above has already added in the generic %extend methods such as
			         template<typename T> class K {}; %extend K { ... } */

			    /* !!! This may be broken.  We may have to add the
			       %extend methods at the beginning of the class */
                            {
                              String *stmp = 0;
                              String *clsname;
                              Node *am;
                              if (Namespaceprefix) {
                                clsname = stmp = NewStringf("%s::%s", Namespaceprefix, Getattr(templnode,"name"));
                              } else {
                                clsname = Getattr(templnode,"name");
                              }
                              am = Getattr(Swig_extend_hash(),clsname);
                              if (am) {
                                Symtab *st = Swig_symbol_current();
                                Swig_symbol_setscope(Getattr(templnode,"symtab"));
                                /*			    Printf(stdout,"%s: %s %p %p\n", Getattr(templnode,"name"), clsname, Swig_symbol_current(), Getattr(templnode,"symtab")); */
                                Swig_extend_merge(templnode,am);
                                Swig_symbol_setscope(st);
				Swig_extend_append_previous(templnode,am);
                                Delattr(Swig_extend_hash(),clsname);
                              }
			      if (stmp) Delete(stmp);
                            }

                            /* Add to classes hash */
			    if (!classes)
			      classes = NewHash();

			    if (Namespaceprefix) {
			      String *temp = NewStringf("%s::%s", Namespaceprefix, Getattr(templnode,"name"));
			      Setattr(classes,temp,templnode);
			      Delete(temp);
			    } else {
			      String *qs = Swig_symbol_qualifiedscopename(templnode);
			      Setattr(classes, qs,templnode);
			      Delete(qs);
			    }
                          }
                        }

                        /* all the overloaded templated functions are added into a linked list */
                        if (!linkliststart)
                          linkliststart = templnode;
                        if (nscope_inner) {
                          /* non-global namespace */
                          if (templnode) {
                            appendChild(nscope_inner,templnode);
			    Delete(templnode);
                            if (nscope) (yyval.node) = nscope;
                          }
                        } else {
                          /* global namespace */
                          if (!linklistend) {
                            (yyval.node) = templnode;
                          } else {
                            set_nextSibling(linklistend,templnode);
			    Delete(templnode);
                          }
                          linklistend = templnode;
                        }
                      }
                      nn = Getattr(nn,"sym:nextSibling"); /* repeat for overloaded templated functions. If a templated class there will never be a sibling. */
                    }
                    update_defaultargs(linkliststart);
		  }
	          Swig_symbol_setscope(tscope);
		  Delete(Namespaceprefix);
		  Namespaceprefix = Swig_symbol_qualifiedscopename(0);
                }
#line 6532 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 115:
#line 3071 "CParse/parser.y" /* yacc.c:1645  */
    {
		  Swig_warning(0,cparse_file, cparse_line,"%s\n", (yyvsp[0].str));
		  (yyval.node) = 0;
               }
#line 6541 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 116:
#line 3081 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.node) = (yyvsp[0].node); 
                    if ((yyval.node)) {
   		      add_symbols((yyval.node));
                      default_arguments((yyval.node));
   	            }
                }
#line 6553 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 117:
#line 3088 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6559 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 118:
#line 3089 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 6565 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 119:
#line 3093 "CParse/parser.y" /* yacc.c:1645  */
    {
		  if (Strcmp((yyvsp[-1].str),"C") == 0) {
		    cparse_externc = 1;
		  }
		}
#line 6575 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 120:
#line 3097 "CParse/parser.y" /* yacc.c:1645  */
    {
		  cparse_externc = 0;
		  if (Strcmp((yyvsp[-4].str),"C") == 0) {
		    Node *n = firstChild((yyvsp[-1].node));
		    (yyval.node) = new_node("extern");
		    Setattr((yyval.node),"name",(yyvsp[-4].str));
		    appendChild((yyval.node),n);
		    while (n) {
		      SwigType *decl = Getattr(n,"decl");
		      if (SwigType_isfunction(decl) && !Equal(Getattr(n, "storage"), "typedef")) {
			Setattr(n,"storage","externc");
		      }
		      n = nextSibling(n);
		    }
		  } else {
		     Swig_warning(WARN_PARSE_UNDEFINED_EXTERN,cparse_file, cparse_line,"Unrecognized extern type \"%s\".\n", (yyvsp[-4].str));
		    (yyval.node) = new_node("extern");
		    Setattr((yyval.node),"name",(yyvsp[-4].str));
		    appendChild((yyval.node),firstChild((yyvsp[-1].node)));
		  }
                }
#line 6601 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 121:
#line 3118 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[0].node);
		  SWIG_WARN_NODE_BEGIN((yyval.node));
		  Swig_warning(WARN_CPP11_LAMBDA, cparse_file, cparse_line, "Lambda expressions and closures are not fully supported yet.\n");
		  SWIG_WARN_NODE_END((yyval.node));
		}
#line 6612 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 122:
#line 3124 "CParse/parser.y" /* yacc.c:1645  */
    {
		  /* Convert using statement to a typedef statement */
		  (yyval.node) = new_node("cdecl");
		  Setattr((yyval.node),"type",(yyvsp[-2].type));
		  Setattr((yyval.node),"storage","typedef");
		  Setattr((yyval.node),"name",(yyvsp[-4].str));
		  Setattr((yyval.node),"decl",(yyvsp[-1].decl).type);
		  SetFlag((yyval.node),"typealias");
		  add_symbols((yyval.node));
		}
#line 6627 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 123:
#line 3134 "CParse/parser.y" /* yacc.c:1645  */
    {
		  /* Convert alias template to a "template" typedef statement */
		  (yyval.node) = new_node("template");
		  Setattr((yyval.node),"type",(yyvsp[-2].type));
		  Setattr((yyval.node),"storage","typedef");
		  Setattr((yyval.node),"name",(yyvsp[-4].str));
		  Setattr((yyval.node),"decl",(yyvsp[-1].decl).type);
		  Setattr((yyval.node),"templateparms",(yyvsp[-7].tparms));
		  Setattr((yyval.node),"templatetype","cdecl");
		  SetFlag((yyval.node),"aliastemplate");
		  add_symbols((yyval.node));
		}
#line 6644 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 124:
#line 3146 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = (yyvsp[0].node);
                }
#line 6652 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 125:
#line 3155 "CParse/parser.y" /* yacc.c:1645  */
    {
	      String *decl = (yyvsp[-3].decl).type;
              (yyval.node) = new_node("cdecl");
	      if ((yyvsp[-2].dtype).qualifier)
	        decl = add_qualifier_to_declarator((yyvsp[-3].decl).type, (yyvsp[-2].dtype).qualifier);
	      Setattr((yyval.node),"refqualifier",(yyvsp[-2].dtype).refqualifier);
	      Setattr((yyval.node),"type",(yyvsp[-4].type));
	      Setattr((yyval.node),"storage",(yyvsp[-5].id));
	      Setattr((yyval.node),"name",(yyvsp[-3].decl).id);
	      Setattr((yyval.node),"decl",decl);
	      Setattr((yyval.node),"parms",(yyvsp[-3].decl).parms);
	      Setattr((yyval.node),"value",(yyvsp[-1].dtype).val);
	      Setattr((yyval.node),"throws",(yyvsp[-2].dtype).throws);
	      Setattr((yyval.node),"throw",(yyvsp[-2].dtype).throwf);
	      Setattr((yyval.node),"noexcept",(yyvsp[-2].dtype).nexcept);
	      if ((yyvsp[-1].dtype).val && (yyvsp[-1].dtype).type) {
		/* store initializer type as it might be different to the declared type */
		SwigType *valuetype = NewSwigType((yyvsp[-1].dtype).type);
		if (Len(valuetype) > 0)
		  Setattr((yyval.node),"valuetype",valuetype);
		else
		  Delete(valuetype);
	      }
	      if (!(yyvsp[0].node)) {
		if (Len(scanner_ccode)) {
		  String *code = Copy(scanner_ccode);
		  Setattr((yyval.node),"code",code);
		  Delete(code);
		}
	      } else {
		Node *n = (yyvsp[0].node);
		/* Inherit attributes */
		while (n) {
		  String *type = Copy((yyvsp[-4].type));
		  Setattr(n,"type",type);
		  Setattr(n,"storage",(yyvsp[-5].id));
		  n = nextSibling(n);
		  Delete(type);
		}
	      }
	      if ((yyvsp[-1].dtype).bitfield) {
		Setattr((yyval.node),"bitfield", (yyvsp[-1].dtype).bitfield);
	      }

	      if ((yyvsp[-3].decl).id) {
		/* Look for "::" declarations (ignored) */
		if (Strstr((yyvsp[-3].decl).id, "::")) {
		  /* This is a special case. If the scope name of the declaration exactly
		     matches that of the declaration, then we will allow it. Otherwise, delete. */
		  String *p = Swig_scopename_prefix((yyvsp[-3].decl).id);
		  if (p) {
		    if ((Namespaceprefix && Strcmp(p, Namespaceprefix) == 0) ||
			(Classprefix && Strcmp(p, Classprefix) == 0)) {
		      String *lstr = Swig_scopename_last((yyvsp[-3].decl).id);
		      Setattr((yyval.node), "name", lstr);
		      Delete(lstr);
		      set_nextSibling((yyval.node), (yyvsp[0].node));
		    } else {
		      Delete((yyval.node));
		      (yyval.node) = (yyvsp[0].node);
		    }
		    Delete(p);
		  } else {
		    Delete((yyval.node));
		    (yyval.node) = (yyvsp[0].node);
		  }
		} else {
		  set_nextSibling((yyval.node), (yyvsp[0].node));
		}
	      } else {
		Swig_error(cparse_file, cparse_line, "Missing symbol name for global declaration\n");
		(yyval.node) = 0;
	      }

	      if ((yyvsp[-2].dtype).qualifier && (yyvsp[-5].id) && Strstr((yyvsp[-5].id), "static"))
		Swig_error(cparse_file, cparse_line, "Static function %s cannot have a qualifier.\n", Swig_name_decl((yyval.node)));
           }
#line 6734 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 126:
#line 3234 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.node) = new_node("cdecl");
	      if ((yyvsp[-5].dtype).qualifier) SwigType_push((yyvsp[-6].decl).type, (yyvsp[-5].dtype).qualifier);
	      Setattr((yyval.node),"refqualifier",(yyvsp[-5].dtype).refqualifier);
	      Setattr((yyval.node),"type",(yyvsp[-3].node));
	      Setattr((yyval.node),"storage",(yyvsp[-8].id));
	      Setattr((yyval.node),"name",(yyvsp[-6].decl).id);
	      Setattr((yyval.node),"decl",(yyvsp[-6].decl).type);
	      Setattr((yyval.node),"parms",(yyvsp[-6].decl).parms);
	      Setattr((yyval.node),"value",(yyvsp[-5].dtype).val);
	      Setattr((yyval.node),"throws",(yyvsp[-5].dtype).throws);
	      Setattr((yyval.node),"throw",(yyvsp[-5].dtype).throwf);
	      Setattr((yyval.node),"noexcept",(yyvsp[-5].dtype).nexcept);
	      if (!(yyvsp[0].node)) {
		if (Len(scanner_ccode)) {
		  String *code = Copy(scanner_ccode);
		  Setattr((yyval.node),"code",code);
		  Delete(code);
		}
	      } else {
		Node *n = (yyvsp[0].node);
		while (n) {
		  String *type = Copy((yyvsp[-3].node));
		  Setattr(n,"type",type);
		  Setattr(n,"storage",(yyvsp[-8].id));
		  n = nextSibling(n);
		  Delete(type);
		}
	      }
	      if ((yyvsp[-5].dtype).bitfield) {
		Setattr((yyval.node),"bitfield", (yyvsp[-5].dtype).bitfield);
	      }

	      if (Strstr((yyvsp[-6].decl).id,"::")) {
                String *p = Swig_scopename_prefix((yyvsp[-6].decl).id);
		if (p) {
		  if ((Namespaceprefix && Strcmp(p, Namespaceprefix) == 0) ||
		      (Classprefix && Strcmp(p, Classprefix) == 0)) {
		    String *lstr = Swig_scopename_last((yyvsp[-6].decl).id);
		    Setattr((yyval.node),"name",lstr);
		    Delete(lstr);
		    set_nextSibling((yyval.node), (yyvsp[0].node));
		  } else {
		    Delete((yyval.node));
		    (yyval.node) = (yyvsp[0].node);
		  }
		  Delete(p);
		} else {
		  Delete((yyval.node));
		  (yyval.node) = (yyvsp[0].node);
		}
	      } else {
		set_nextSibling((yyval.node), (yyvsp[0].node));
	      }

	      if ((yyvsp[-5].dtype).qualifier && (yyvsp[-8].id) && Strstr((yyvsp[-8].id), "static"))
		Swig_error(cparse_file, cparse_line, "Static function %s cannot have a qualifier.\n", Swig_name_decl((yyval.node)));
           }
#line 6797 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 127:
#line 3296 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.node) = 0;
                   Clear(scanner_ccode); 
               }
#line 6806 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 128:
#line 3300 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = new_node("cdecl");
		 if ((yyvsp[-2].dtype).qualifier) SwigType_push((yyvsp[-3].decl).type,(yyvsp[-2].dtype).qualifier);
		 Setattr((yyval.node),"refqualifier",(yyvsp[-2].dtype).refqualifier);
		 Setattr((yyval.node),"name",(yyvsp[-3].decl).id);
		 Setattr((yyval.node),"decl",(yyvsp[-3].decl).type);
		 Setattr((yyval.node),"parms",(yyvsp[-3].decl).parms);
		 Setattr((yyval.node),"value",(yyvsp[-1].dtype).val);
		 Setattr((yyval.node),"throws",(yyvsp[-2].dtype).throws);
		 Setattr((yyval.node),"throw",(yyvsp[-2].dtype).throwf);
		 Setattr((yyval.node),"noexcept",(yyvsp[-2].dtype).nexcept);
		 if ((yyvsp[-1].dtype).bitfield) {
		   Setattr((yyval.node),"bitfield", (yyvsp[-1].dtype).bitfield);
		 }
		 if (!(yyvsp[0].node)) {
		   if (Len(scanner_ccode)) {
		     String *code = Copy(scanner_ccode);
		     Setattr((yyval.node),"code",code);
		     Delete(code);
		   }
		 } else {
		   set_nextSibling((yyval.node), (yyvsp[0].node));
		 }
	       }
#line 6835 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 129:
#line 3324 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   skip_balanced('{','}');
                   (yyval.node) = 0;
               }
#line 6844 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 130:
#line 3328 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = 0;
		   if (yychar == RPAREN) {
		       Swig_error(cparse_file, cparse_line, "Unexpected ')'.\n");
		   } else {
		       Swig_error(cparse_file, cparse_line, "Syntax error - possibly a missing semicolon.\n");
		   }
		   exit(1);
               }
#line 6858 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 131:
#line 3339 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.dtype) = (yyvsp[0].dtype);
              }
#line 6866 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 132:
#line 3344 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].type); }
#line 6872 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 133:
#line 3345 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].type); }
#line 6878 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 134:
#line 3346 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].type); }
#line 6884 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 135:
#line 3350 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].type); }
#line 6890 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 136:
#line 3351 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].str); }
#line 6896 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 137:
#line 3352 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].type); }
#line 6902 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 138:
#line 3363 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = new_node("lambda");
		  Setattr((yyval.node),"name",(yyvsp[-8].str));
		  add_symbols((yyval.node));
	        }
#line 6912 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 139:
#line 3368 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = new_node("lambda");
		  Setattr((yyval.node),"name",(yyvsp[-10].str));
		  add_symbols((yyval.node));
		}
#line 6922 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 140:
#line 3373 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = new_node("lambda");
		  Setattr((yyval.node),"name",(yyvsp[-4].str));
		  add_symbols((yyval.node));
		}
#line 6932 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 141:
#line 3380 "CParse/parser.y" /* yacc.c:1645  */
    {
		  skip_balanced('[',']');
		  (yyval.node) = 0;
	        }
#line 6941 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 142:
#line 3386 "CParse/parser.y" /* yacc.c:1645  */
    {
		  skip_balanced('{','}');
		  (yyval.node) = 0;
		}
#line 6950 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 143:
#line 3391 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.pl) = 0;
		}
#line 6958 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 144:
#line 3394 "CParse/parser.y" /* yacc.c:1645  */
    {
		  skip_balanced('(',')');
		}
#line 6966 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 145:
#line 3396 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.pl) = 0;
		}
#line 6974 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 146:
#line 3407 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = (char *)"enum";
	      }
#line 6982 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 147:
#line 3410 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = (char *)"enum class";
	      }
#line 6990 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 148:
#line 3413 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.node) = (char *)"enum struct";
	      }
#line 6998 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 149:
#line 3422 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = (yyvsp[0].type);
              }
#line 7006 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 150:
#line 3425 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 7012 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 151:
#line 3432 "CParse/parser.y" /* yacc.c:1645  */
    {
		   SwigType *ty = 0;
		   int scopedenum = (yyvsp[-2].id) && !Equal((yyvsp[-3].node), "enum");
		   (yyval.node) = new_node("enumforward");
		   ty = NewStringf("enum %s", (yyvsp[-2].id));
		   Setattr((yyval.node),"enumkey",(yyvsp[-3].node));
		   if (scopedenum)
		     SetFlag((yyval.node), "scopedenum");
		   Setattr((yyval.node),"name",(yyvsp[-2].id));
		   Setattr((yyval.node),"inherit",(yyvsp[-1].node));
		   Setattr((yyval.node),"type",ty);
		   Setattr((yyval.node),"sym:weak", "1");
		   add_symbols((yyval.node));
	      }
#line 7031 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 152:
#line 3454 "CParse/parser.y" /* yacc.c:1645  */
    {
		  SwigType *ty = 0;
		  int scopedenum = (yyvsp[-5].id) && !Equal((yyvsp[-6].node), "enum");
                  (yyval.node) = new_node("enum");
		  ty = NewStringf("enum %s", (yyvsp[-5].id));
		  Setattr((yyval.node),"enumkey",(yyvsp[-6].node));
		  if (scopedenum)
		    SetFlag((yyval.node), "scopedenum");
		  Setattr((yyval.node),"name",(yyvsp[-5].id));
		  Setattr((yyval.node),"inherit",(yyvsp[-4].node));
		  Setattr((yyval.node),"type",ty);
		  appendChild((yyval.node),(yyvsp[-2].node));
		  add_symbols((yyval.node));      /* Add to tag space */

		  if (scopedenum) {
		    Swig_symbol_newscope();
		    Swig_symbol_setscopename((yyvsp[-5].id));
		    Delete(Namespaceprefix);
		    Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		  }

		  add_symbols((yyvsp[-2].node));      /* Add enum values to appropriate enum or enum class scope */

		  if (scopedenum) {
		    Setattr((yyval.node),"symtab", Swig_symbol_popscope());
		    Delete(Namespaceprefix);
		    Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		  }
               }
#line 7065 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 153:
#line 3483 "CParse/parser.y" /* yacc.c:1645  */
    {
		 Node *n;
		 SwigType *ty = 0;
		 String   *unnamed = 0;
		 int       unnamedinstance = 0;
		 int scopedenum = (yyvsp[-8].id) && !Equal((yyvsp[-9].node), "enum");

		 (yyval.node) = new_node("enum");
		 Setattr((yyval.node),"enumkey",(yyvsp[-9].node));
		 if (scopedenum)
		   SetFlag((yyval.node), "scopedenum");
		 Setattr((yyval.node),"inherit",(yyvsp[-7].node));
		 if ((yyvsp[-8].id)) {
		   Setattr((yyval.node),"name",(yyvsp[-8].id));
		   ty = NewStringf("enum %s", (yyvsp[-8].id));
		 } else if ((yyvsp[-3].decl).id) {
		   unnamed = make_unnamed();
		   ty = NewStringf("enum %s", unnamed);
		   Setattr((yyval.node),"unnamed",unnamed);
                   /* name is not set for unnamed enum instances, e.g. enum { foo } Instance; */
		   if ((yyvsp[-10].id) && Cmp((yyvsp[-10].id),"typedef") == 0) {
		     Setattr((yyval.node),"name",(yyvsp[-3].decl).id);
                   } else {
                     unnamedinstance = 1;
                   }
		   Setattr((yyval.node),"storage",(yyvsp[-10].id));
		 }
		 if ((yyvsp[-3].decl).id && Cmp((yyvsp[-10].id),"typedef") == 0) {
		   Setattr((yyval.node),"tdname",(yyvsp[-3].decl).id);
                   Setattr((yyval.node),"allows_typedef","1");
                 }
		 appendChild((yyval.node),(yyvsp[-5].node));
		 n = new_node("cdecl");
		 Setattr(n,"type",ty);
		 Setattr(n,"name",(yyvsp[-3].decl).id);
		 Setattr(n,"storage",(yyvsp[-10].id));
		 Setattr(n,"decl",(yyvsp[-3].decl).type);
		 Setattr(n,"parms",(yyvsp[-3].decl).parms);
		 Setattr(n,"unnamed",unnamed);

                 if (unnamedinstance) {
		   SwigType *cty = NewString("enum ");
		   Setattr((yyval.node),"type",cty);
		   SetFlag((yyval.node),"unnamedinstance");
		   SetFlag(n,"unnamedinstance");
		   Delete(cty);
                 }
		 if ((yyvsp[0].node)) {
		   Node *p = (yyvsp[0].node);
		   set_nextSibling(n,p);
		   while (p) {
		     SwigType *cty = Copy(ty);
		     Setattr(p,"type",cty);
		     Setattr(p,"unnamed",unnamed);
		     Setattr(p,"storage",(yyvsp[-10].id));
		     Delete(cty);
		     p = nextSibling(p);
		   }
		 } else {
		   if (Len(scanner_ccode)) {
		     String *code = Copy(scanner_ccode);
		     Setattr(n,"code",code);
		     Delete(code);
		   }
		 }

                 /* Ensure that typedef enum ABC {foo} XYZ; uses XYZ for sym:name, like structs.
                  * Note that class_rename/yyrename are bit of a mess so used this simple approach to change the name. */
                 if ((yyvsp[-3].decl).id && (yyvsp[-8].id) && Cmp((yyvsp[-10].id),"typedef") == 0) {
		   String *name = NewString((yyvsp[-3].decl).id);
                   Setattr((yyval.node), "parser:makename", name);
		   Delete(name);
                 }

		 add_symbols((yyval.node));       /* Add enum to tag space */
		 set_nextSibling((yyval.node),n);
		 Delete(n);

		 if (scopedenum) {
		   Swig_symbol_newscope();
		   Swig_symbol_setscopename((yyvsp[-8].id));
		   Delete(Namespaceprefix);
		   Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		 }

		 add_symbols((yyvsp[-5].node));      /* Add enum values to appropriate enum or enum class scope */

		 if (scopedenum) {
		   Setattr((yyval.node),"symtab", Swig_symbol_popscope());
		   Delete(Namespaceprefix);
		   Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		 }

	         add_symbols(n);
		 Delete(unnamed);
	       }
#line 7166 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 154:
#line 3581 "CParse/parser.y" /* yacc.c:1645  */
    {
                   /* This is a sick hack.  If the ctor_end has parameters,
                      and the parms parameter only has 1 parameter, this
                      could be a declaration of the form:

                         type (id)(parms)

			 Otherwise it's an error. */
                    int err = 0;
                    (yyval.node) = 0;

		    if ((ParmList_len((yyvsp[-2].pl)) == 1) && (!Swig_scopename_check((yyvsp[-4].type)))) {
		      SwigType *ty = Getattr((yyvsp[-2].pl),"type");
		      String *name = Getattr((yyvsp[-2].pl),"name");
		      err = 1;
		      if (!name) {
			(yyval.node) = new_node("cdecl");
			Setattr((yyval.node),"type",(yyvsp[-4].type));
			Setattr((yyval.node),"storage",(yyvsp[-5].id));
			Setattr((yyval.node),"name",ty);

			if ((yyvsp[0].decl).have_parms) {
			  SwigType *decl = NewStringEmpty();
			  SwigType_add_function(decl,(yyvsp[0].decl).parms);
			  Setattr((yyval.node),"decl",decl);
			  Setattr((yyval.node),"parms",(yyvsp[0].decl).parms);
			  if (Len(scanner_ccode)) {
			    String *code = Copy(scanner_ccode);
			    Setattr((yyval.node),"code",code);
			    Delete(code);
			  }
			}
			if ((yyvsp[0].decl).defarg) {
			  Setattr((yyval.node),"value",(yyvsp[0].decl).defarg);
			}
			Setattr((yyval.node),"throws",(yyvsp[0].decl).throws);
			Setattr((yyval.node),"throw",(yyvsp[0].decl).throwf);
			Setattr((yyval.node),"noexcept",(yyvsp[0].decl).nexcept);
			err = 0;
		      }
		    }
		    if (err) {
		      Swig_error(cparse_file,cparse_line,"Syntax error in input(2).\n");
		      exit(1);
		    }
                }
#line 7217 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 155:
#line 3633 "CParse/parser.y" /* yacc.c:1645  */
    {  (yyval.node) = (yyvsp[0].node); }
#line 7223 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 156:
#line 3634 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7229 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 157:
#line 3635 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7235 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 158:
#line 3636 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7241 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 159:
#line 3637 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 7247 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 160:
#line 3638 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 7253 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 161:
#line 3643 "CParse/parser.y" /* yacc.c:1645  */
    {
                   String *prefix;
                   List *bases = 0;
		   Node *scope = 0;
		   String *code;
		   (yyval.node) = new_node("class");
		   Setline((yyval.node),cparse_start_line);
		   Setattr((yyval.node),"kind",(yyvsp[-3].id));
		   if ((yyvsp[-1].bases)) {
		     Setattr((yyval.node),"baselist", Getattr((yyvsp[-1].bases),"public"));
		     Setattr((yyval.node),"protectedbaselist", Getattr((yyvsp[-1].bases),"protected"));
		     Setattr((yyval.node),"privatebaselist", Getattr((yyvsp[-1].bases),"private"));
		   }
		   Setattr((yyval.node),"allows_typedef","1");

		   /* preserve the current scope */
		   Setattr((yyval.node),"prev_symtab",Swig_symbol_current());
		  
		   /* If the class name is qualified.  We need to create or lookup namespace/scope entries */
		   scope = resolve_create_node_scope((yyvsp[-2].str), 1);
		   /* save nscope_inner to the class - it may be overwritten in nested classes*/
		   Setattr((yyval.node), "nested:innerscope", nscope_inner);
		   Setattr((yyval.node), "nested:nscope", nscope);
		   Setfile(scope,cparse_file);
		   Setline(scope,cparse_line);
		   (yyvsp[-2].str) = scope;
		   Setattr((yyval.node),"name",(yyvsp[-2].str));

		   if (currentOuterClass) {
		     SetFlag((yyval.node), "nested");
		     Setattr((yyval.node), "nested:outer", currentOuterClass);
		     set_access_mode((yyval.node));
		   }
		   Swig_features_get(Swig_cparse_features(), Namespaceprefix, Getattr((yyval.node), "name"), 0, (yyval.node));
		   /* save yyrename to the class attribute, to be used later in add_symbols()*/
		   Setattr((yyval.node), "class_rename", make_name((yyval.node), (yyvsp[-2].str), 0));
		   Setattr((yyval.node), "Classprefix", (yyvsp[-2].str));
		   Classprefix = NewString((yyvsp[-2].str));
		   /* Deal with inheritance  */
		   if ((yyvsp[-1].bases))
		     bases = Swig_make_inherit_list((yyvsp[-2].str),Getattr((yyvsp[-1].bases),"public"),Namespaceprefix);
		   prefix = SwigType_istemplate_templateprefix((yyvsp[-2].str));
		   if (prefix) {
		     String *fbase, *tbase;
		     if (Namespaceprefix) {
		       fbase = NewStringf("%s::%s", Namespaceprefix,(yyvsp[-2].str));
		       tbase = NewStringf("%s::%s", Namespaceprefix, prefix);
		     } else {
		       fbase = Copy((yyvsp[-2].str));
		       tbase = Copy(prefix);
		     }
		     Swig_name_inherit(tbase,fbase);
		     Delete(fbase);
		     Delete(tbase);
		   }
                   if (strcmp((yyvsp[-3].id),"class") == 0) {
		     cplus_mode = CPLUS_PRIVATE;
		   } else {
		     cplus_mode = CPLUS_PUBLIC;
		   }
		   if (!cparse_cplusplus) {
		     set_scope_to_global();
		   }
		   Swig_symbol_newscope();
		   Swig_symbol_setscopename((yyvsp[-2].str));
		   Swig_inherit_base_symbols(bases);
		   Delete(Namespaceprefix);
		   Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		   cparse_start_line = cparse_line;

		   /* If there are active template parameters, we need to make sure they are
                      placed in the class symbol table so we can catch shadows */

		   if (template_parameters) {
		     Parm *tp = template_parameters;
		     while(tp) {
		       String *tpname = Copy(Getattr(tp,"name"));
		       Node *tn = new_node("templateparm");
		       Setattr(tn,"name",tpname);
		       Swig_symbol_cadd(tpname,tn);
		       tp = nextSibling(tp);
		       Delete(tpname);
		     }
		   }
		   Delete(prefix);
		   inclass = 1;
		   currentOuterClass = (yyval.node);
		   if (cparse_cplusplusout) {
		     /* save the structure declaration to declare it in global scope for C++ to see */
		     code = get_raw_text_balanced('{', '}');
		     Setattr((yyval.node), "code", code);
		     Delete(code);
		   }
               }
#line 7352 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 162:
#line 3736 "CParse/parser.y" /* yacc.c:1645  */
    {
		   Node *p;
		   SwigType *ty;
		   Symtab *cscope;
		   Node *am = 0;
		   String *scpname = 0;
		   (void) (yyvsp[-3].node);
		   (yyval.node) = currentOuterClass;
		   currentOuterClass = Getattr((yyval.node), "nested:outer");
		   nscope_inner = Getattr((yyval.node), "nested:innerscope");
		   nscope = Getattr((yyval.node), "nested:nscope");
		   Delattr((yyval.node), "nested:innerscope");
		   Delattr((yyval.node), "nested:nscope");
		   if (nscope_inner && Strcmp(nodeType(nscope_inner), "class") == 0) { /* actual parent class for this class */
		     Node* forward_declaration = Swig_symbol_clookup_no_inherit(Getattr((yyval.node),"name"), Getattr(nscope_inner, "symtab"));
		     if (forward_declaration) {
		       Setattr((yyval.node), "access", Getattr(forward_declaration, "access"));
		     }
		     Setattr((yyval.node), "nested:outer", nscope_inner);
		     SetFlag((yyval.node), "nested");
                   }
		   if (!currentOuterClass)
		     inclass = 0;
		   cscope = Getattr((yyval.node), "prev_symtab");
		   Delattr((yyval.node), "prev_symtab");
		   
		   /* Check for pure-abstract class */
		   Setattr((yyval.node),"abstracts", pure_abstracts((yyvsp[-2].node)));
		   
		   /* This bit of code merges in a previously defined %extend directive (if any) */
		   {
		     String *clsname = Swig_symbol_qualifiedscopename(0);
		     am = Getattr(Swig_extend_hash(), clsname);
		     if (am) {
		       Swig_extend_merge((yyval.node), am);
		       Delattr(Swig_extend_hash(), clsname);
		     }
		     Delete(clsname);
		   }
		   if (!classes) classes = NewHash();
		   scpname = Swig_symbol_qualifiedscopename(0);
		   Setattr(classes, scpname, (yyval.node));

		   appendChild((yyval.node), (yyvsp[-2].node));
		   
		   if (am) 
		     Swig_extend_append_previous((yyval.node), am);

		   p = (yyvsp[0].node);
		   if (p && !nscope_inner) {
		     if (!cparse_cplusplus && currentOuterClass)
		       appendChild(currentOuterClass, p);
		     else
		      appendSibling((yyval.node), p);
		   }
		   
		   if (nscope_inner) {
		     ty = NewString(scpname); /* if the class is declared out of scope, let the declarator use fully qualified type*/
		   } else if (cparse_cplusplus && !cparse_externc) {
		     ty = NewString((yyvsp[-6].str));
		   } else {
		     ty = NewStringf("%s %s", (yyvsp[-7].id), (yyvsp[-6].str));
		   }
		   while (p) {
		     Setattr(p, "storage", (yyvsp[-8].id));
		     Setattr(p, "type" ,ty);
		     if (!cparse_cplusplus && currentOuterClass && (!Getattr(currentOuterClass, "name"))) {
		       SetFlag(p, "hasconsttype");
		       SetFlag(p, "feature:immutable");
		     }
		     p = nextSibling(p);
		   }
		   if ((yyvsp[0].node) && Cmp((yyvsp[-8].id),"typedef") == 0)
		     add_typedef_name((yyval.node), (yyvsp[0].node), (yyvsp[-6].str), cscope, scpname);
		   Delete(scpname);

		   if (cplus_mode != CPLUS_PUBLIC) {
		   /* we 'open' the class at the end, to allow %template
		      to add new members */
		     Node *pa = new_node("access");
		     Setattr(pa, "kind", "public");
		     cplus_mode = CPLUS_PUBLIC;
		     appendChild((yyval.node), pa);
		     Delete(pa);
		   }
		   if (currentOuterClass)
		     restore_access_mode((yyval.node));
		   Setattr((yyval.node), "symtab", Swig_symbol_popscope());
		   Classprefix = Getattr((yyval.node), "Classprefix");
		   Delattr((yyval.node), "Classprefix");
		   Delete(Namespaceprefix);
		   Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		   if (cplus_mode == CPLUS_PRIVATE) {
		     (yyval.node) = 0; /* skip private nested classes */
		   } else if (cparse_cplusplus && currentOuterClass && ignore_nested_classes && !GetFlag((yyval.node), "feature:flatnested")) {
		     (yyval.node) = nested_forward_declaration((yyvsp[-8].id), (yyvsp[-7].id), (yyvsp[-6].str), Copy((yyvsp[-6].str)), (yyvsp[0].node));
		   } else if (nscope_inner) {
		     /* this is tricky */
		     /* we add the declaration in the original namespace */
		     if (Strcmp(nodeType(nscope_inner), "class") == 0 && cparse_cplusplus && ignore_nested_classes && !GetFlag((yyval.node), "feature:flatnested"))
		       (yyval.node) = nested_forward_declaration((yyvsp[-8].id), (yyvsp[-7].id), (yyvsp[-6].str), Copy((yyvsp[-6].str)), (yyvsp[0].node));
		     appendChild(nscope_inner, (yyval.node));
		     Swig_symbol_setscope(Getattr(nscope_inner, "symtab"));
		     Delete(Namespaceprefix);
		     Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		     yyrename = Copy(Getattr((yyval.node), "class_rename"));
		     add_symbols((yyval.node));
		     Delattr((yyval.node), "class_rename");
		     /* but the variable definition in the current scope */
		     Swig_symbol_setscope(cscope);
		     Delete(Namespaceprefix);
		     Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		     add_symbols((yyvsp[0].node));
		     if (nscope) {
		       (yyval.node) = nscope; /* here we return recreated namespace tower instead of the class itself */
		       if ((yyvsp[0].node)) {
			 appendSibling((yyval.node), (yyvsp[0].node));
		       }
		     } else if (!SwigType_istemplate(ty) && template_parameters == 0) { /* for template we need the class itself */
		       (yyval.node) = (yyvsp[0].node);
		     }
		   } else {
		     Delete(yyrename);
		     yyrename = 0;
		     if (!cparse_cplusplus && currentOuterClass) { /* nested C structs go into global scope*/
		       Node *outer = currentOuterClass;
		       while (Getattr(outer, "nested:outer"))
			 outer = Getattr(outer, "nested:outer");
		       appendSibling(outer, (yyval.node));
		       add_symbols((yyvsp[0].node));
		       set_scope_to_global();
		       Delete(Namespaceprefix);
		       Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		       yyrename = Copy(Getattr((yyval.node), "class_rename"));
		       add_symbols((yyval.node));
		       if (!cparse_cplusplusout)
			 Delattr((yyval.node), "nested:outer");
		       Delattr((yyval.node), "class_rename");
		       (yyval.node) = 0;
		     } else {
		       yyrename = Copy(Getattr((yyval.node), "class_rename"));
		       add_symbols((yyval.node));
		       add_symbols((yyvsp[0].node));
		       Delattr((yyval.node), "class_rename");
		     }
		   }
		   Delete(ty);
		   Swig_symbol_setscope(cscope);
		   Delete(Namespaceprefix);
		   Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		   Classprefix = currentOuterClass ? Getattr(currentOuterClass, "Classprefix") : 0;
	       }
#line 7509 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 163:
#line 3891 "CParse/parser.y" /* yacc.c:1645  */
    {
	       String *unnamed;
	       String *code;
	       unnamed = make_unnamed();
	       (yyval.node) = new_node("class");
	       Setline((yyval.node),cparse_start_line);
	       Setattr((yyval.node),"kind",(yyvsp[-2].id));
	       if ((yyvsp[-1].bases)) {
		 Setattr((yyval.node),"baselist", Getattr((yyvsp[-1].bases),"public"));
		 Setattr((yyval.node),"protectedbaselist", Getattr((yyvsp[-1].bases),"protected"));
		 Setattr((yyval.node),"privatebaselist", Getattr((yyvsp[-1].bases),"private"));
	       }
	       Setattr((yyval.node),"storage",(yyvsp[-3].id));
	       Setattr((yyval.node),"unnamed",unnamed);
	       Setattr((yyval.node),"allows_typedef","1");
	       if (currentOuterClass) {
		 SetFlag((yyval.node), "nested");
		 Setattr((yyval.node), "nested:outer", currentOuterClass);
		 set_access_mode((yyval.node));
	       }
	       Swig_features_get(Swig_cparse_features(), Namespaceprefix, 0, 0, (yyval.node));
	       /* save yyrename to the class attribute, to be used later in add_symbols()*/
	       Setattr((yyval.node), "class_rename", make_name((yyval.node),0,0));
	       if (strcmp((yyvsp[-2].id),"class") == 0) {
		 cplus_mode = CPLUS_PRIVATE;
	       } else {
		 cplus_mode = CPLUS_PUBLIC;
	       }
	       Swig_symbol_newscope();
	       cparse_start_line = cparse_line;
	       currentOuterClass = (yyval.node);
	       inclass = 1;
	       Classprefix = 0;
	       Delete(Namespaceprefix);
	       Namespaceprefix = Swig_symbol_qualifiedscopename(0);
	       /* save the structure declaration to make a typedef for it later*/
	       code = get_raw_text_balanced('{', '}');
	       Setattr((yyval.node), "code", code);
	       Delete(code);
	     }
#line 7554 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 164:
#line 3930 "CParse/parser.y" /* yacc.c:1645  */
    {
	       String *unnamed;
               List *bases = 0;
	       String *name = 0;
	       Node *n;
	       Classprefix = 0;
	       (void)(yyvsp[-3].node);
	       (yyval.node) = currentOuterClass;
	       currentOuterClass = Getattr((yyval.node), "nested:outer");
	       if (!currentOuterClass)
		 inclass = 0;
	       else
		 restore_access_mode((yyval.node));
	       unnamed = Getattr((yyval.node),"unnamed");
               /* Check for pure-abstract class */
	       Setattr((yyval.node),"abstracts", pure_abstracts((yyvsp[-2].node)));
	       n = (yyvsp[0].node);
	       if (cparse_cplusplus && currentOuterClass && ignore_nested_classes && !GetFlag((yyval.node), "feature:flatnested")) {
		 String *name = n ? Copy(Getattr(n, "name")) : 0;
		 (yyval.node) = nested_forward_declaration((yyvsp[-7].id), (yyvsp[-6].id), 0, name, n);
		 Swig_symbol_popscope();
	         Delete(Namespaceprefix);
		 Namespaceprefix = Swig_symbol_qualifiedscopename(0);
	       } else if (n) {
	         appendSibling((yyval.node),n);
		 /* If a proper typedef name was given, we'll use it to set the scope name */
		 name = try_to_find_a_name_for_unnamed_structure((yyvsp[-7].id), n);
		 if (name) {
		   String *scpname = 0;
		   SwigType *ty;
		   Setattr((yyval.node),"tdname",name);
		   Setattr((yyval.node),"name",name);
		   Swig_symbol_setscopename(name);
		   if ((yyvsp[-5].bases))
		     bases = Swig_make_inherit_list(name,Getattr((yyvsp[-5].bases),"public"),Namespaceprefix);
		   Swig_inherit_base_symbols(bases);

		     /* If a proper name was given, we use that as the typedef, not unnamed */
		   Clear(unnamed);
		   Append(unnamed, name);
		   if (cparse_cplusplus && !cparse_externc) {
		     ty = NewString(name);
		   } else {
		     ty = NewStringf("%s %s", (yyvsp[-6].id),name);
		   }
		   while (n) {
		     Setattr(n,"storage",(yyvsp[-7].id));
		     Setattr(n, "type", ty);
		     if (!cparse_cplusplus && currentOuterClass && (!Getattr(currentOuterClass, "name"))) {
		       SetFlag(n,"hasconsttype");
		       SetFlag(n,"feature:immutable");
		     }
		     n = nextSibling(n);
		   }
		   n = (yyvsp[0].node);

		   /* Check for previous extensions */
		   {
		     String *clsname = Swig_symbol_qualifiedscopename(0);
		     Node *am = Getattr(Swig_extend_hash(),clsname);
		     if (am) {
		       /* Merge the extension into the symbol table */
		       Swig_extend_merge((yyval.node),am);
		       Swig_extend_append_previous((yyval.node),am);
		       Delattr(Swig_extend_hash(),clsname);
		     }
		     Delete(clsname);
		   }
		   if (!classes) classes = NewHash();
		   scpname = Swig_symbol_qualifiedscopename(0);
		   Setattr(classes,scpname,(yyval.node));
		   Delete(scpname);
		 } else { /* no suitable name was found for a struct */
		   Setattr((yyval.node), "nested:unnamed", Getattr(n, "name")); /* save the name of the first declarator for later use in name generation*/
		   while (n) { /* attach unnamed struct to the declarators, so that they would receive proper type later*/
		     Setattr(n, "nested:unnamedtype", (yyval.node));
		     Setattr(n, "storage", (yyvsp[-7].id));
		     n = nextSibling(n);
		   }
		   n = (yyvsp[0].node);
		   Swig_symbol_setscopename("<unnamed>");
		 }
		 appendChild((yyval.node),(yyvsp[-2].node));
		 /* Pop the scope */
		 Setattr((yyval.node),"symtab",Swig_symbol_popscope());
		 if (name) {
		   Delete(yyrename);
		   yyrename = Copy(Getattr((yyval.node), "class_rename"));
		   Delete(Namespaceprefix);
		   Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		   add_symbols((yyval.node));
		   add_symbols(n);
		   Delattr((yyval.node), "class_rename");
		 }else if (cparse_cplusplus)
		   (yyval.node) = 0; /* ignore unnamed structs for C++ */
	         Delete(unnamed);
	       } else { /* unnamed struct w/o declarator*/
		 Swig_symbol_popscope();
	         Delete(Namespaceprefix);
		 Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		 add_symbols((yyvsp[-2].node));
		 Delete((yyval.node));
		 (yyval.node) = (yyvsp[-2].node); /* pass member list to outer class/namespace (instead of self)*/
	       }
	       Classprefix = currentOuterClass ? Getattr(currentOuterClass, "Classprefix") : 0;
              }
#line 7665 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 165:
#line 4038 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 7671 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 166:
#line 4039 "CParse/parser.y" /* yacc.c:1645  */
    {
                        (yyval.node) = new_node("cdecl");
                        Setattr((yyval.node),"name",(yyvsp[-3].decl).id);
                        Setattr((yyval.node),"decl",(yyvsp[-3].decl).type);
                        Setattr((yyval.node),"parms",(yyvsp[-3].decl).parms);
			set_nextSibling((yyval.node), (yyvsp[0].node));
                    }
#line 7683 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 167:
#line 4051 "CParse/parser.y" /* yacc.c:1645  */
    {
              if ((yyvsp[-3].id) && (Strcmp((yyvsp[-3].id),"friend") == 0)) {
		/* Ignore */
                (yyval.node) = 0; 
	      } else {
		(yyval.node) = new_node("classforward");
		Setattr((yyval.node),"kind",(yyvsp[-2].id));
		Setattr((yyval.node),"name",(yyvsp[-1].str));
		Setattr((yyval.node),"sym:weak", "1");
		add_symbols((yyval.node));
	      }
             }
#line 7700 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 168:
#line 4069 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    if (currentOuterClass)
		      Setattr(currentOuterClass, "template_parameters", template_parameters);
		    template_parameters = (yyvsp[-1].tparms); 
		    parsing_template_declaration = 1;
		  }
#line 7711 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 169:
#line 4074 "CParse/parser.y" /* yacc.c:1645  */
    {
			String *tname = 0;
			int     error = 0;

			/* check if we get a namespace node with a class declaration, and retrieve the class */
			Symtab *cscope = Swig_symbol_current();
			Symtab *sti = 0;
			Node *ntop = (yyvsp[0].node);
			Node *ni = ntop;
			SwigType *ntype = ni ? nodeType(ni) : 0;
			while (ni && Strcmp(ntype,"namespace") == 0) {
			  sti = Getattr(ni,"symtab");
			  ni = firstChild(ni);
			  ntype = nodeType(ni);
			}
			if (sti) {
			  Swig_symbol_setscope(sti);
			  Delete(Namespaceprefix);
			  Namespaceprefix = Swig_symbol_qualifiedscopename(0);
			  (yyvsp[0].node) = ni;
			}

			(yyval.node) = (yyvsp[0].node);
			if ((yyval.node)) tname = Getattr((yyval.node),"name");
			
			/* Check if the class is a template specialization */
			if (((yyval.node)) && (Strchr(tname,'<')) && (!is_operator(tname))) {
			  /* If a specialization.  Check if defined. */
			  Node *tempn = 0;
			  {
			    String *tbase = SwigType_templateprefix(tname);
			    tempn = Swig_symbol_clookup_local(tbase,0);
			    if (!tempn || (Strcmp(nodeType(tempn),"template") != 0)) {
			      SWIG_WARN_NODE_BEGIN(tempn);
			      Swig_warning(WARN_PARSE_TEMPLATE_SP_UNDEF, Getfile((yyval.node)),Getline((yyval.node)),"Specialization of non-template '%s'.\n", tbase);
			      SWIG_WARN_NODE_END(tempn);
			      tempn = 0;
			      error = 1;
			    }
			    Delete(tbase);
			  }
			  Setattr((yyval.node),"specialization","1");
			  Setattr((yyval.node),"templatetype",nodeType((yyval.node)));
			  set_nodeType((yyval.node),"template");
			  /* Template partial specialization */
			  if (tempn && ((yyvsp[-3].tparms)) && ((yyvsp[0].node))) {
			    List   *tlist;
			    String *targs = SwigType_templateargs(tname);
			    tlist = SwigType_parmlist(targs);
			    /*			  Printf(stdout,"targs = '%s' %s\n", targs, tlist); */
			    if (!Getattr((yyval.node),"sym:weak")) {
			      Setattr((yyval.node),"sym:typename","1");
			    }
			    
			    if (Len(tlist) != ParmList_len(Getattr(tempn,"templateparms"))) {
			      Swig_error(Getfile((yyval.node)),Getline((yyval.node)),"Inconsistent argument count in template partial specialization. %d %d\n", Len(tlist), ParmList_len(Getattr(tempn,"templateparms")));
			      
			    } else {

			    /* This code builds the argument list for the partial template
			       specialization.  This is a little hairy, but the idea is as
			       follows:

			       $3 contains a list of arguments supplied for the template.
			       For example template<class T>.

			       tlist is a list of the specialization arguments--which may be
			       different.  For example class<int,T>.

			       tp is a copy of the arguments in the original template definition.
       
			       The patching algorithm walks through the list of supplied
			       arguments ($3), finds the position in the specialization arguments
			       (tlist), and then patches the name in the argument list of the
			       original template.
			    */

			    {
			      String *pn;
			      Parm *p, *p1;
			      int i, nargs;
			      Parm *tp = CopyParmList(Getattr(tempn,"templateparms"));
			      nargs = Len(tlist);
			      p = (yyvsp[-3].tparms);
			      while (p) {
				for (i = 0; i < nargs; i++){
				  pn = Getattr(p,"name");
				  if (Strcmp(pn,SwigType_base(Getitem(tlist,i))) == 0) {
				    int j;
				    Parm *p1 = tp;
				    for (j = 0; j < i; j++) {
				      p1 = nextSibling(p1);
				    }
				    Setattr(p1,"name",pn);
				    Setattr(p1,"partialarg","1");
				  }
				}
				p = nextSibling(p);
			      }
			      p1 = tp;
			      i = 0;
			      while (p1) {
				if (!Getattr(p1,"partialarg")) {
				  Delattr(p1,"name");
				  Setattr(p1,"type", Getitem(tlist,i));
				} 
				i++;
				p1 = nextSibling(p1);
			      }
			      Setattr((yyval.node),"templateparms",tp);
			      Delete(tp);
			    }
  #if 0
			    /* Patch the parameter list */
			    if (tempn) {
			      Parm *p,*p1;
			      ParmList *tp = CopyParmList(Getattr(tempn,"templateparms"));
			      p = (yyvsp[-3].tparms);
			      p1 = tp;
			      while (p && p1) {
				String *pn = Getattr(p,"name");
				Printf(stdout,"pn = '%s'\n", pn);
				if (pn) Setattr(p1,"name",pn);
				else Delattr(p1,"name");
				pn = Getattr(p,"type");
				if (pn) Setattr(p1,"type",pn);
				p = nextSibling(p);
				p1 = nextSibling(p1);
			      }
			      Setattr((yyval.node),"templateparms",tp);
			      Delete(tp);
			    } else {
			      Setattr((yyval.node),"templateparms",(yyvsp[-3].tparms));
			    }
  #endif
			    Delattr((yyval.node),"specialization");
			    Setattr((yyval.node),"partialspecialization","1");
			    /* Create a specialized name for matching */
			    {
			      Parm *p = (yyvsp[-3].tparms);
			      String *fname = NewString(Getattr((yyval.node),"name"));
			      String *ffname = 0;
			      ParmList *partialparms = 0;

			      char   tmp[32];
			      int    i, ilen;
			      while (p) {
				String *n = Getattr(p,"name");
				if (!n) {
				  p = nextSibling(p);
				  continue;
				}
				ilen = Len(tlist);
				for (i = 0; i < ilen; i++) {
				  if (Strstr(Getitem(tlist,i),n)) {
				    sprintf(tmp,"$%d",i+1);
				    Replaceid(fname,n,tmp);
				  }
				}
				p = nextSibling(p);
			      }
			      /* Patch argument names with typedef */
			      {
				Iterator tt;
				Parm *parm_current = 0;
				List *tparms = SwigType_parmlist(fname);
				ffname = SwigType_templateprefix(fname);
				Append(ffname,"<(");
				for (tt = First(tparms); tt.item; ) {
				  SwigType *rtt = Swig_symbol_typedef_reduce(tt.item,0);
				  SwigType *ttr = Swig_symbol_type_qualify(rtt,0);

				  Parm *newp = NewParmWithoutFileLineInfo(ttr, 0);
				  if (partialparms)
				    set_nextSibling(parm_current, newp);
				  else
				    partialparms = newp;
				  parm_current = newp;

				  Append(ffname,ttr);
				  tt = Next(tt);
				  if (tt.item) Putc(',',ffname);
				  Delete(rtt);
				  Delete(ttr);
				}
				Delete(tparms);
				Append(ffname,")>");
			      }
			      {
				Node *new_partial = NewHash();
				String *partials = Getattr(tempn,"partials");
				if (!partials) {
				  partials = NewList();
				  Setattr(tempn,"partials",partials);
				  Delete(partials);
				}
				/*			      Printf(stdout,"partial: fname = '%s', '%s'\n", fname, Swig_symbol_typedef_reduce(fname,0)); */
				Setattr(new_partial, "partialparms", partialparms);
				Setattr(new_partial, "templcsymname", ffname);
				Append(partials, new_partial);
			      }
			      Setattr((yyval.node),"partialargs",ffname);
			      Swig_symbol_cadd(ffname,(yyval.node));
			    }
			    }
			    Delete(tlist);
			    Delete(targs);
			  } else {
			    /* An explicit template specialization */
			    /* add default args from primary (unspecialized) template */
			    String *ty = Swig_symbol_template_deftype(tname,0);
			    String *fname = Swig_symbol_type_qualify(ty,0);
			    Swig_symbol_cadd(fname,(yyval.node));
			    Delete(ty);
			    Delete(fname);
			  }
			}  else if ((yyval.node)) {
			  Setattr((yyval.node),"templatetype",nodeType((yyvsp[0].node)));
			  set_nodeType((yyval.node),"template");
			  Setattr((yyval.node),"templateparms", (yyvsp[-3].tparms));
			  if (!Getattr((yyval.node),"sym:weak")) {
			    Setattr((yyval.node),"sym:typename","1");
			  }
			  add_symbols((yyval.node));
			  default_arguments((yyval.node));
			  /* We also place a fully parameterized version in the symbol table */
			  {
			    Parm *p;
			    String *fname = NewStringf("%s<(", Getattr((yyval.node),"name"));
			    p = (yyvsp[-3].tparms);
			    while (p) {
			      String *n = Getattr(p,"name");
			      if (!n) n = Getattr(p,"type");
			      Append(fname,n);
			      p = nextSibling(p);
			      if (p) Putc(',',fname);
			    }
			    Append(fname,")>");
			    Swig_symbol_cadd(fname,(yyval.node));
			  }
			}
			(yyval.node) = ntop;
			Swig_symbol_setscope(cscope);
			Delete(Namespaceprefix);
			Namespaceprefix = Swig_symbol_qualifiedscopename(0);
			if (error || (nscope_inner && Strcmp(nodeType(nscope_inner), "class") == 0)) {
			  (yyval.node) = 0;
			}
			if (currentOuterClass)
			  template_parameters = Getattr(currentOuterClass, "template_parameters");
			else
			  template_parameters = 0;
			parsing_template_declaration = 0;
                }
#line 7970 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 170:
#line 4330 "CParse/parser.y" /* yacc.c:1645  */
    {
		  Swig_warning(WARN_PARSE_EXPLICIT_TEMPLATE, cparse_file, cparse_line, "Explicit template instantiation ignored.\n");
                  (yyval.node) = 0; 
		}
#line 7979 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 171:
#line 4336 "CParse/parser.y" /* yacc.c:1645  */
    {
		  Swig_warning(WARN_PARSE_EXPLICIT_TEMPLATE, cparse_file, cparse_line, "Explicit template instantiation ignored.\n");
                  (yyval.node) = 0; 
                }
#line 7988 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 172:
#line 4342 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[0].node);
                }
#line 7996 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 173:
#line 4345 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = (yyvsp[0].node);
                }
#line 8004 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 174:
#line 4348 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = (yyvsp[0].node);
                }
#line 8012 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 175:
#line 4351 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = 0;
                }
#line 8020 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 176:
#line 4354 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.node) = (yyvsp[0].node);
                }
#line 8028 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 177:
#line 4357 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.node) = (yyvsp[0].node);
                }
#line 8036 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 178:
#line 4362 "CParse/parser.y" /* yacc.c:1645  */
    {
		   /* Rip out the parameter names */
		  Parm *p = (yyvsp[0].pl);
		  (yyval.tparms) = (yyvsp[0].pl);

		  while (p) {
		    String *name = Getattr(p,"name");
		    if (!name) {
		      /* Hmmm. Maybe it's a 'class T' parameter */
		      char *type = Char(Getattr(p,"type"));
		      /* Template template parameter */
		      if (strncmp(type,"template<class> ",16) == 0) {
			type += 16;
		      }
		      if ((strncmp(type,"class ",6) == 0) || (strncmp(type,"typename ", 9) == 0)) {
			char *t = strchr(type,' ');
			Setattr(p,"name", t+1);
		      } else 
                      /* Variadic template args */
		      if ((strncmp(type,"class... ",9) == 0) || (strncmp(type,"typename... ", 12) == 0)) {
			char *t = strchr(type,' ');
			Setattr(p,"name", t+1);
			Setattr(p,"variadic", "1");
		      } else {
			/*
			 Swig_error(cparse_file, cparse_line, "Missing template parameter name\n");
			 $$.rparms = 0;
			 $$.parms = 0;
			 break; */
		      }
		    }
		    p = nextSibling(p);
		  }
                 }
#line 8075 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 179:
#line 4398 "CParse/parser.y" /* yacc.c:1645  */
    {
                      set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
                      (yyval.pl) = (yyvsp[-1].p);
                   }
#line 8084 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 180:
#line 4402 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.pl) = 0; }
#line 8090 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 181:
#line 4405 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.p) = NewParmWithoutFileLineInfo(NewString((yyvsp[0].id)), 0);
                  }
#line 8098 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 182:
#line 4408 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.p) = (yyvsp[0].p);
                  }
#line 8106 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 183:
#line 4413 "CParse/parser.y" /* yacc.c:1645  */
    {
                         set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
                         (yyval.pl) = (yyvsp[-1].p);
                       }
#line 8115 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 184:
#line 4417 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.pl) = 0; }
#line 8121 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 185:
#line 4422 "CParse/parser.y" /* yacc.c:1645  */
    {
                  String *uname = Swig_symbol_type_qualify((yyvsp[-1].str),0);
		  String *name = Swig_scopename_last((yyvsp[-1].str));
                  (yyval.node) = new_node("using");
		  Setattr((yyval.node),"uname",uname);
		  Setattr((yyval.node),"name", name);
		  Delete(uname);
		  Delete(name);
		  add_symbols((yyval.node));
             }
#line 8136 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 186:
#line 4432 "CParse/parser.y" /* yacc.c:1645  */
    {
	       Node *n = Swig_symbol_clookup((yyvsp[-1].str),0);
	       if (!n) {
		 Swig_error(cparse_file, cparse_line, "Nothing known about namespace '%s'\n", (yyvsp[-1].str));
		 (yyval.node) = 0;
	       } else {

		 while (Strcmp(nodeType(n),"using") == 0) {
		   n = Getattr(n,"node");
		 }
		 if (n) {
		   if (Strcmp(nodeType(n),"namespace") == 0) {
		     Symtab *current = Swig_symbol_current();
		     Symtab *symtab = Getattr(n,"symtab");
		     (yyval.node) = new_node("using");
		     Setattr((yyval.node),"node",n);
		     Setattr((yyval.node),"namespace", (yyvsp[-1].str));
		     if (current != symtab) {
		       Swig_symbol_inherit(symtab);
		     }
		   } else {
		     Swig_error(cparse_file, cparse_line, "'%s' is not a namespace.\n", (yyvsp[-1].str));
		     (yyval.node) = 0;
		   }
		 } else {
		   (yyval.node) = 0;
		 }
	       }
             }
#line 8170 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 187:
#line 4463 "CParse/parser.y" /* yacc.c:1645  */
    { 
                Hash *h;
		Node *parent_ns = 0;
		List *scopes = Swig_scopename_tolist((yyvsp[-1].str));
		int ilen = Len(scopes);
		int i;

/*
Printf(stdout, "==== Namespace %s creation...\n", $2);
*/
		(yyval.node) = 0;
		for (i = 0; i < ilen; i++) {
		  Node *ns = new_node("namespace");
		  Symtab *current_symtab = Swig_symbol_current();
		  String *scopename = Getitem(scopes, i);
		  Setattr(ns, "name", scopename);
		  (yyval.node) = ns;
		  if (parent_ns)
		    appendChild(parent_ns, ns);
		  parent_ns = ns;
		  h = Swig_symbol_clookup(scopename, 0);
		  if (h && (current_symtab == Getattr(h, "sym:symtab")) && (Strcmp(nodeType(h), "namespace") == 0)) {
/*
Printf(stdout, "  Scope %s [found C++17 style]\n", scopename);
*/
		    if (Getattr(h, "alias")) {
		      h = Getattr(h, "namespace");
		      Swig_warning(WARN_PARSE_NAMESPACE_ALIAS, cparse_file, cparse_line, "Namespace alias '%s' not allowed here. Assuming '%s'\n",
				   scopename, Getattr(h, "name"));
		      scopename = Getattr(h, "name");
		    }
		    Swig_symbol_setscope(Getattr(h, "symtab"));
		  } else {
/*
Printf(stdout, "  Scope %s [creating single scope C++17 style]\n", scopename);
*/
		    h = Swig_symbol_newscope();
		    Swig_symbol_setscopename(scopename);
		  }
		  Delete(Namespaceprefix);
		  Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		}
		Delete(scopes);
             }
#line 8219 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 188:
#line 4506 "CParse/parser.y" /* yacc.c:1645  */
    {
		Node *n = (yyvsp[-2].node);
		Node *top_ns = 0;
		do {
		  Setattr(n, "symtab", Swig_symbol_popscope());
		  Delete(Namespaceprefix);
		  Namespaceprefix = Swig_symbol_qualifiedscopename(0);
		  add_symbols(n);
		  top_ns = n;
		  n = parentNode(n);
		} while(n);
		appendChild((yyvsp[-2].node), firstChild((yyvsp[-1].node)));
		Delete((yyvsp[-1].node));
		(yyval.node) = top_ns;
             }
#line 8239 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 189:
#line 4521 "CParse/parser.y" /* yacc.c:1645  */
    {
	       Hash *h;
	       (yyvsp[-1].node) = Swig_symbol_current();
	       h = Swig_symbol_clookup("    ",0);
	       if (h && (Strcmp(nodeType(h),"namespace") == 0)) {
		 Swig_symbol_setscope(Getattr(h,"symtab"));
	       } else {
		 Swig_symbol_newscope();
		 /* we don't use "__unnamed__", but a long 'empty' name */
		 Swig_symbol_setscopename("    ");
	       }
	       Namespaceprefix = 0;
             }
#line 8257 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 190:
#line 4533 "CParse/parser.y" /* yacc.c:1645  */
    {
	       (yyval.node) = (yyvsp[-1].node);
	       set_nodeType((yyval.node),"namespace");
	       Setattr((yyval.node),"unnamed","1");
	       Setattr((yyval.node),"symtab", Swig_symbol_popscope());
	       Swig_symbol_setscope((yyvsp[-4].node));
	       Delete(Namespaceprefix);
	       Namespaceprefix = Swig_symbol_qualifiedscopename(0);
	       add_symbols((yyval.node));
             }
#line 8272 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 191:
#line 4543 "CParse/parser.y" /* yacc.c:1645  */
    {
	       /* Namespace alias */
	       Node *n;
	       (yyval.node) = new_node("namespace");
	       Setattr((yyval.node),"name",(yyvsp[-3].id));
	       Setattr((yyval.node),"alias",(yyvsp[-1].str));
	       n = Swig_symbol_clookup((yyvsp[-1].str),0);
	       if (!n) {
		 Swig_error(cparse_file, cparse_line, "Unknown namespace '%s'\n", (yyvsp[-1].str));
		 (yyval.node) = 0;
	       } else {
		 if (Strcmp(nodeType(n),"namespace") != 0) {
		   Swig_error(cparse_file, cparse_line, "'%s' is not a namespace\n",(yyvsp[-1].str));
		   (yyval.node) = 0;
		 } else {
		   while (Getattr(n,"alias")) {
		     n = Getattr(n,"namespace");
		   }
		   Setattr((yyval.node),"namespace",n);
		   add_symbols((yyval.node));
		   /* Set up a scope alias */
		   Swig_symbol_alias((yyvsp[-3].id),Getattr(n,"symtab"));
		 }
	       }
             }
#line 8302 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 192:
#line 4570 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.node) = (yyvsp[-1].node);
                   /* Insert cpp_member (including any siblings) to the front of the cpp_members linked list */
		   if ((yyval.node)) {
		     Node *p = (yyval.node);
		     Node *pp =0;
		     while (p) {
		       pp = p;
		       p = nextSibling(p);
		     }
		     set_nextSibling(pp,(yyvsp[0].node));
		     if ((yyvsp[0].node))
		       set_previousSibling((yyvsp[0].node), pp);
		   } else {
		     (yyval.node) = (yyvsp[0].node);
		   }
             }
#line 8324 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 193:
#line 4587 "CParse/parser.y" /* yacc.c:1645  */
    { 
	       extendmode = 1;
	       if (cplus_mode != CPLUS_PUBLIC) {
		 Swig_error(cparse_file,cparse_line,"%%extend can only be used in a public section\n");
	       }
             }
#line 8335 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 194:
#line 4592 "CParse/parser.y" /* yacc.c:1645  */
    {
	       extendmode = 0;
	     }
#line 8343 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 195:
#line 4594 "CParse/parser.y" /* yacc.c:1645  */
    {
	       (yyval.node) = new_node("extend");
	       mark_nodes_as_extend((yyvsp[-3].node));
	       appendChild((yyval.node),(yyvsp[-3].node));
	       set_nextSibling((yyval.node),(yyvsp[0].node));
	     }
#line 8354 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 196:
#line 4600 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8360 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 197:
#line 4601 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0;}
#line 8366 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 198:
#line 4602 "CParse/parser.y" /* yacc.c:1645  */
    {
	       int start_line = cparse_line;
	       skip_decl();
	       Swig_error(cparse_file,start_line,"Syntax error in input(3).\n");
	       exit(1);
	       }
#line 8377 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 199:
#line 4607 "CParse/parser.y" /* yacc.c:1645  */
    { 
		 (yyval.node) = (yyvsp[0].node);
   	     }
#line 8385 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 200:
#line 4618 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8391 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 201:
#line 4619 "CParse/parser.y" /* yacc.c:1645  */
    { 
                 (yyval.node) = (yyvsp[0].node); 
		 if (extendmode && current_class) {
		   String *symname;
		   symname= make_name((yyval.node),Getattr((yyval.node),"name"), Getattr((yyval.node),"decl"));
		   if (Strcmp(symname,Getattr((yyval.node),"name")) == 0) {
		     /* No renaming operation.  Set name to class name */
		     Delete(yyrename);
		     yyrename = NewString(Getattr(current_class,"sym:name"));
		   } else {
		     Delete(yyrename);
		     yyrename = symname;
		   }
		 }
		 add_symbols((yyval.node));
                 default_arguments((yyval.node));
             }
#line 8413 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 202:
#line 4636 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8419 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 203:
#line 4637 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8425 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 204:
#line 4638 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8431 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 205:
#line 4639 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8437 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 206:
#line 4640 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8443 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 207:
#line 4641 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8449 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 208:
#line 4642 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 8455 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 209:
#line 4643 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8461 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 210:
#line 4644 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8467 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 211:
#line 4645 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 8473 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 212:
#line 4646 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8479 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 213:
#line 4647 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8485 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 214:
#line 4648 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 8491 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 215:
#line 4649 "CParse/parser.y" /* yacc.c:1645  */
    {(yyval.node) = (yyvsp[0].node); }
#line 8497 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 216:
#line 4650 "CParse/parser.y" /* yacc.c:1645  */
    {(yyval.node) = (yyvsp[0].node); }
#line 8503 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 217:
#line 4651 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 8509 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 218:
#line 4653 "CParse/parser.y" /* yacc.c:1645  */
    {
		(yyval.node) = (yyvsp[0].node);
	     }
#line 8517 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 219:
#line 4656 "CParse/parser.y" /* yacc.c:1645  */
    {
	         (yyval.node) = (yyvsp[0].node);
		 set_comment((yyvsp[0].node), (yyvsp[-1].str));
	     }
#line 8526 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 220:
#line 4660 "CParse/parser.y" /* yacc.c:1645  */
    {
	         (yyval.node) = (yyvsp[-1].node);
		 set_comment((yyvsp[-1].node), (yyvsp[0].str));
	     }
#line 8535 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 221:
#line 4672 "CParse/parser.y" /* yacc.c:1645  */
    {
              if (inclass || extendmode) {
		SwigType *decl = NewStringEmpty();
		(yyval.node) = new_node("constructor");
		Setattr((yyval.node),"storage",(yyvsp[-5].id));
		Setattr((yyval.node),"name",(yyvsp[-4].type));
		Setattr((yyval.node),"parms",(yyvsp[-2].pl));
		SwigType_add_function(decl,(yyvsp[-2].pl));
		Setattr((yyval.node),"decl",decl);
		Setattr((yyval.node),"throws",(yyvsp[0].decl).throws);
		Setattr((yyval.node),"throw",(yyvsp[0].decl).throwf);
		Setattr((yyval.node),"noexcept",(yyvsp[0].decl).nexcept);
		if (Len(scanner_ccode)) {
		  String *code = Copy(scanner_ccode);
		  Setattr((yyval.node),"code",code);
		  Delete(code);
		}
		SetFlag((yyval.node),"feature:new");
		if ((yyvsp[0].decl).defarg)
		  Setattr((yyval.node),"value",(yyvsp[0].decl).defarg);
	      } else {
		(yyval.node) = 0;
              }
              }
#line 8564 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 222:
#line 4700 "CParse/parser.y" /* yacc.c:1645  */
    {
               String *name = NewStringf("%s",(yyvsp[-4].str));
	       if (*(Char(name)) != '~') Insert(name,0,"~");
               (yyval.node) = new_node("destructor");
	       Setattr((yyval.node),"name",name);
	       Delete(name);
	       if (Len(scanner_ccode)) {
		 String *code = Copy(scanner_ccode);
		 Setattr((yyval.node),"code",code);
		 Delete(code);
	       }
	       {
		 String *decl = NewStringEmpty();
		 SwigType_add_function(decl,(yyvsp[-2].pl));
		 Setattr((yyval.node),"decl",decl);
		 Delete(decl);
	       }
	       Setattr((yyval.node),"throws",(yyvsp[0].dtype).throws);
	       Setattr((yyval.node),"throw",(yyvsp[0].dtype).throwf);
	       Setattr((yyval.node),"noexcept",(yyvsp[0].dtype).nexcept);
	       if ((yyvsp[0].dtype).val)
	         Setattr((yyval.node),"value",(yyvsp[0].dtype).val);
	       if ((yyvsp[0].dtype).qualifier)
		 Swig_error(cparse_file, cparse_line, "Destructor %s %s cannot have a qualifier.\n", Swig_name_decl((yyval.node)), SwigType_str((yyvsp[0].dtype).qualifier, 0));
	       add_symbols((yyval.node));
	      }
#line 8595 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 223:
#line 4729 "CParse/parser.y" /* yacc.c:1645  */
    {
		String *name;
		(yyval.node) = new_node("destructor");
		Setattr((yyval.node),"storage","virtual");
	        name = NewStringf("%s",(yyvsp[-4].str));
		if (*(Char(name)) != '~') Insert(name,0,"~");
		Setattr((yyval.node),"name",name);
		Delete(name);
		Setattr((yyval.node),"throws",(yyvsp[0].dtype).throws);
		Setattr((yyval.node),"throw",(yyvsp[0].dtype).throwf);
		Setattr((yyval.node),"noexcept",(yyvsp[0].dtype).nexcept);
		if ((yyvsp[0].dtype).val)
		  Setattr((yyval.node),"value",(yyvsp[0].dtype).val);
		if (Len(scanner_ccode)) {
		  String *code = Copy(scanner_ccode);
		  Setattr((yyval.node),"code",code);
		  Delete(code);
		}
		{
		  String *decl = NewStringEmpty();
		  SwigType_add_function(decl,(yyvsp[-2].pl));
		  Setattr((yyval.node),"decl",decl);
		  Delete(decl);
		}
		if ((yyvsp[0].dtype).qualifier)
		  Swig_error(cparse_file, cparse_line, "Destructor %s %s cannot have a qualifier.\n", Swig_name_decl((yyval.node)), SwigType_str((yyvsp[0].dtype).qualifier, 0));
		add_symbols((yyval.node));
	      }
#line 8628 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 224:
#line 4761 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = new_node("cdecl");
                 Setattr((yyval.node),"type",(yyvsp[-5].type));
		 Setattr((yyval.node),"name",(yyvsp[-6].str));
		 Setattr((yyval.node),"storage",(yyvsp[-7].id));

		 SwigType_add_function((yyvsp[-4].type),(yyvsp[-2].pl));
		 if ((yyvsp[0].dtype).qualifier) {
		   SwigType_push((yyvsp[-4].type),(yyvsp[0].dtype).qualifier);
		 }
		 Setattr((yyval.node),"refqualifier",(yyvsp[0].dtype).refqualifier);
		 Setattr((yyval.node),"decl",(yyvsp[-4].type));
		 Setattr((yyval.node),"parms",(yyvsp[-2].pl));
		 Setattr((yyval.node),"conversion_operator","1");
		 add_symbols((yyval.node));
              }
#line 8649 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 225:
#line 4777 "CParse/parser.y" /* yacc.c:1645  */
    {
		 SwigType *decl;
                 (yyval.node) = new_node("cdecl");
                 Setattr((yyval.node),"type",(yyvsp[-5].type));
		 Setattr((yyval.node),"name",(yyvsp[-6].str));
		 Setattr((yyval.node),"storage",(yyvsp[-7].id));
		 decl = NewStringEmpty();
		 SwigType_add_reference(decl);
		 SwigType_add_function(decl,(yyvsp[-2].pl));
		 if ((yyvsp[0].dtype).qualifier) {
		   SwigType_push(decl,(yyvsp[0].dtype).qualifier);
		 }
		 Setattr((yyval.node),"refqualifier",(yyvsp[0].dtype).refqualifier);
		 Setattr((yyval.node),"decl",decl);
		 Setattr((yyval.node),"parms",(yyvsp[-2].pl));
		 Setattr((yyval.node),"conversion_operator","1");
		 add_symbols((yyval.node));
	       }
#line 8672 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 226:
#line 4795 "CParse/parser.y" /* yacc.c:1645  */
    {
		 SwigType *decl;
                 (yyval.node) = new_node("cdecl");
                 Setattr((yyval.node),"type",(yyvsp[-5].type));
		 Setattr((yyval.node),"name",(yyvsp[-6].str));
		 Setattr((yyval.node),"storage",(yyvsp[-7].id));
		 decl = NewStringEmpty();
		 SwigType_add_rvalue_reference(decl);
		 SwigType_add_function(decl,(yyvsp[-2].pl));
		 if ((yyvsp[0].dtype).qualifier) {
		   SwigType_push(decl,(yyvsp[0].dtype).qualifier);
		 }
		 Setattr((yyval.node),"refqualifier",(yyvsp[0].dtype).refqualifier);
		 Setattr((yyval.node),"decl",decl);
		 Setattr((yyval.node),"parms",(yyvsp[-2].pl));
		 Setattr((yyval.node),"conversion_operator","1");
		 add_symbols((yyval.node));
	       }
#line 8695 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 227:
#line 4814 "CParse/parser.y" /* yacc.c:1645  */
    {
		 SwigType *decl;
                 (yyval.node) = new_node("cdecl");
                 Setattr((yyval.node),"type",(yyvsp[-6].type));
		 Setattr((yyval.node),"name",(yyvsp[-7].str));
		 Setattr((yyval.node),"storage",(yyvsp[-8].id));
		 decl = NewStringEmpty();
		 SwigType_add_pointer(decl);
		 SwigType_add_reference(decl);
		 SwigType_add_function(decl,(yyvsp[-2].pl));
		 if ((yyvsp[0].dtype).qualifier) {
		   SwigType_push(decl,(yyvsp[0].dtype).qualifier);
		 }
		 Setattr((yyval.node),"refqualifier",(yyvsp[0].dtype).refqualifier);
		 Setattr((yyval.node),"decl",decl);
		 Setattr((yyval.node),"parms",(yyvsp[-2].pl));
		 Setattr((yyval.node),"conversion_operator","1");
		 add_symbols((yyval.node));
	       }
#line 8719 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 228:
#line 4834 "CParse/parser.y" /* yacc.c:1645  */
    {
		String *t = NewStringEmpty();
		(yyval.node) = new_node("cdecl");
		Setattr((yyval.node),"type",(yyvsp[-4].type));
		Setattr((yyval.node),"name",(yyvsp[-5].str));
		 Setattr((yyval.node),"storage",(yyvsp[-6].id));
		SwigType_add_function(t,(yyvsp[-2].pl));
		if ((yyvsp[0].dtype).qualifier) {
		  SwigType_push(t,(yyvsp[0].dtype).qualifier);
		}
		 Setattr((yyval.node),"refqualifier",(yyvsp[0].dtype).refqualifier);
		Setattr((yyval.node),"decl",t);
		Setattr((yyval.node),"parms",(yyvsp[-2].pl));
		Setattr((yyval.node),"conversion_operator","1");
		add_symbols((yyval.node));
              }
#line 8740 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 229:
#line 4854 "CParse/parser.y" /* yacc.c:1645  */
    {
                 skip_balanced('{','}');
                 (yyval.node) = 0;
               }
#line 8749 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 230:
#line 4862 "CParse/parser.y" /* yacc.c:1645  */
    {
                skip_balanced('(',')');
                (yyval.node) = 0;
              }
#line 8758 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 231:
#line 4869 "CParse/parser.y" /* yacc.c:1645  */
    { 
                (yyval.node) = new_node("access");
		Setattr((yyval.node),"kind","public");
                cplus_mode = CPLUS_PUBLIC;
              }
#line 8768 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 232:
#line 4876 "CParse/parser.y" /* yacc.c:1645  */
    { 
                (yyval.node) = new_node("access");
                Setattr((yyval.node),"kind","private");
		cplus_mode = CPLUS_PRIVATE;
	      }
#line 8778 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 233:
#line 4884 "CParse/parser.y" /* yacc.c:1645  */
    { 
		(yyval.node) = new_node("access");
		Setattr((yyval.node),"kind","protected");
		cplus_mode = CPLUS_PROTECTED;
	      }
#line 8788 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 234:
#line 4892 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8794 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 235:
#line 4895 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8800 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 236:
#line 4899 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8806 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 237:
#line 4902 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8812 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 238:
#line 4903 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8818 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 239:
#line 4904 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8824 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 240:
#line 4905 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8830 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 241:
#line 4906 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8836 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 242:
#line 4907 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8842 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 243:
#line 4908 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8848 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 244:
#line 4909 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = (yyvsp[0].node); }
#line 8854 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 245:
#line 4912 "CParse/parser.y" /* yacc.c:1645  */
    {
	            Clear(scanner_ccode);
		    (yyval.dtype).val = 0;
		    (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
		    (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = (yyvsp[-1].dtype).throws;
		    (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf;
		    (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
               }
#line 8869 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 246:
#line 4922 "CParse/parser.y" /* yacc.c:1645  */
    {
	            Clear(scanner_ccode);
		    (yyval.dtype).val = (yyvsp[-1].dtype).val;
		    (yyval.dtype).qualifier = (yyvsp[-3].dtype).qualifier;
		    (yyval.dtype).refqualifier = (yyvsp[-3].dtype).refqualifier;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = (yyvsp[-3].dtype).throws;
		    (yyval.dtype).throwf = (yyvsp[-3].dtype).throwf;
		    (yyval.dtype).nexcept = (yyvsp[-3].dtype).nexcept;
               }
#line 8884 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 247:
#line 4932 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    skip_balanced('{','}'); 
		    (yyval.dtype).val = 0;
		    (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
		    (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = (yyvsp[-1].dtype).throws;
		    (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf;
		    (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
	       }
#line 8899 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 248:
#line 4944 "CParse/parser.y" /* yacc.c:1645  */
    { 
                     Clear(scanner_ccode);
                     (yyval.dtype).val = 0;
                     (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
                     (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
                     (yyval.dtype).bitfield = 0;
                     (yyval.dtype).throws = (yyvsp[-1].dtype).throws;
                     (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf;
                     (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
                }
#line 8914 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 249:
#line 4954 "CParse/parser.y" /* yacc.c:1645  */
    { 
                     Clear(scanner_ccode);
                     (yyval.dtype).val = (yyvsp[-1].dtype).val;
                     (yyval.dtype).qualifier = (yyvsp[-3].dtype).qualifier;
                     (yyval.dtype).refqualifier = (yyvsp[-3].dtype).refqualifier;
                     (yyval.dtype).bitfield = 0;
                     (yyval.dtype).throws = (yyvsp[-3].dtype).throws; 
                     (yyval.dtype).throwf = (yyvsp[-3].dtype).throwf; 
                     (yyval.dtype).nexcept = (yyvsp[-3].dtype).nexcept; 
               }
#line 8929 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 250:
#line 4964 "CParse/parser.y" /* yacc.c:1645  */
    { 
                     skip_balanced('{','}');
                     (yyval.dtype).val = 0;
                     (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
                     (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
                     (yyval.dtype).bitfield = 0;
                     (yyval.dtype).throws = (yyvsp[-1].dtype).throws; 
                     (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf; 
                     (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept; 
               }
#line 8944 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 251:
#line 4977 "CParse/parser.y" /* yacc.c:1645  */
    { }
#line 8950 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 252:
#line 4980 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type);
                  /* Printf(stdout,"primitive = '%s'\n", $$);*/
                }
#line 8958 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 253:
#line 4983 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 8964 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 254:
#line 4984 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 8970 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 255:
#line 4988 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 8976 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 256:
#line 4990 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.type) = (yyvsp[0].str);
               }
#line 8984 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 257:
#line 4998 "CParse/parser.y" /* yacc.c:1645  */
    {
                   if (Strcmp((yyvsp[0].str),"C") == 0) {
		     (yyval.id) = "externc";
                   } else if (Strcmp((yyvsp[0].str),"C++") == 0) {
		     (yyval.id) = "extern";
		   } else {
		     Swig_warning(WARN_PARSE_UNDEFINED_EXTERN,cparse_file, cparse_line,"Unrecognized extern type \"%s\".\n", (yyvsp[0].str));
		     (yyval.id) = 0;
		   }
               }
#line 8999 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 258:
#line 5010 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "extern"; }
#line 9005 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 259:
#line 5011 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (yyvsp[0].id); }
#line 9011 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 260:
#line 5012 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "thread_local"; }
#line 9017 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 261:
#line 5013 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "typedef"; }
#line 9023 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 262:
#line 5014 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "static"; }
#line 9029 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 263:
#line 5015 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "typedef"; }
#line 9035 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 264:
#line 5016 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "virtual"; }
#line 9041 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 265:
#line 5017 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "friend"; }
#line 9047 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 266:
#line 5018 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "explicit"; }
#line 9053 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 267:
#line 5019 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "constexpr"; }
#line 9059 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 268:
#line 5020 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "explicit constexpr"; }
#line 9065 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 269:
#line 5021 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "explicit constexpr"; }
#line 9071 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 270:
#line 5022 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "static constexpr"; }
#line 9077 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 271:
#line 5023 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "static constexpr"; }
#line 9083 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 272:
#line 5024 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "thread_local"; }
#line 9089 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 273:
#line 5025 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "static thread_local"; }
#line 9095 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 274:
#line 5026 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "static thread_local"; }
#line 9101 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 275:
#line 5027 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "extern thread_local"; }
#line 9107 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 276:
#line 5028 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "extern thread_local"; }
#line 9113 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 277:
#line 5029 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = 0; }
#line 9119 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 278:
#line 5036 "CParse/parser.y" /* yacc.c:1645  */
    {
                 Parm *p;
		 (yyval.pl) = (yyvsp[0].pl);
		 p = (yyvsp[0].pl);
                 while (p) {
		   Replace(Getattr(p,"type"),"typename ", "", DOH_REPLACE_ANY);
		   p = nextSibling(p);
                 }
               }
#line 9133 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 279:
#line 5047 "CParse/parser.y" /* yacc.c:1645  */
    {
                  set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
                  (yyval.pl) = (yyvsp[-1].p);
		}
#line 9142 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 280:
#line 5051 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.pl) = 0;
		  previousNode = currentNode;
		  currentNode=0;
	       }
#line 9152 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 281:
#line 5058 "CParse/parser.y" /* yacc.c:1645  */
    {
                 set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
		 (yyval.pl) = (yyvsp[-1].p);
                }
#line 9161 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 282:
#line 5062 "CParse/parser.y" /* yacc.c:1645  */
    {
		 set_comment(previousNode, (yyvsp[-2].str));
                 set_nextSibling((yyvsp[-1].p), (yyvsp[0].pl));
		 (yyval.pl) = (yyvsp[-1].p);
               }
#line 9171 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 283:
#line 5067 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.pl) = 0; }
#line 9177 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 284:
#line 5071 "CParse/parser.y" /* yacc.c:1645  */
    {
                   SwigType_push((yyvsp[-1].type),(yyvsp[0].decl).type);
		   (yyval.p) = NewParmWithoutFileLineInfo((yyvsp[-1].type),(yyvsp[0].decl).id);
		   previousNode = currentNode;
		   currentNode = (yyval.p);
		   Setfile((yyval.p),cparse_file);
		   Setline((yyval.p),cparse_line);
		   if ((yyvsp[0].decl).defarg) {
		     Setattr((yyval.p),"value",(yyvsp[0].decl).defarg);
		   }
		}
#line 9193 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 285:
#line 5083 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.p) = NewParmWithoutFileLineInfo(NewStringf("template<class> %s %s", (yyvsp[-2].id),(yyvsp[-1].str)), 0);
		  previousNode = currentNode;
		  currentNode = (yyval.p);
		  Setfile((yyval.p),cparse_file);
		  Setline((yyval.p),cparse_line);
                  if ((yyvsp[0].dtype).val) {
                    Setattr((yyval.p),"value",(yyvsp[0].dtype).val);
                  }
                }
#line 9208 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 286:
#line 5093 "CParse/parser.y" /* yacc.c:1645  */
    {
		  SwigType *t = NewString("v(...)");
		  (yyval.p) = NewParmWithoutFileLineInfo(t, 0);
		  previousNode = currentNode;
		  currentNode = (yyval.p);
		  Setfile((yyval.p),cparse_file);
		  Setline((yyval.p),cparse_line);
		}
#line 9221 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 287:
#line 5103 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.p) = (yyvsp[0].p);
		}
#line 9229 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 288:
#line 5106 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.p) = (yyvsp[0].p);
		  set_comment((yyvsp[0].p), (yyvsp[-1].str));
		}
#line 9238 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 289:
#line 5110 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.p) = (yyvsp[-1].p);
		  set_comment((yyvsp[-1].p), (yyvsp[0].str));
		}
#line 9247 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 290:
#line 5116 "CParse/parser.y" /* yacc.c:1645  */
    {
                 Parm *p;
		 (yyval.p) = (yyvsp[0].p);
		 p = (yyvsp[0].p);
                 while (p) {
		   if (Getattr(p,"type")) {
		     Replace(Getattr(p,"type"),"typename ", "", DOH_REPLACE_ANY);
		   }
		   p = nextSibling(p);
                 }
               }
#line 9263 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 291:
#line 5129 "CParse/parser.y" /* yacc.c:1645  */
    {
                  set_nextSibling((yyvsp[-1].p),(yyvsp[0].p));
                  (yyval.p) = (yyvsp[-1].p);
		}
#line 9272 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 292:
#line 5133 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.p) = 0; }
#line 9278 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 293:
#line 5136 "CParse/parser.y" /* yacc.c:1645  */
    {
                 set_nextSibling((yyvsp[-1].p),(yyvsp[0].p));
		 (yyval.p) = (yyvsp[-1].p);
                }
#line 9287 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 294:
#line 5140 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.p) = 0; }
#line 9293 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 295:
#line 5144 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.p) = (yyvsp[0].p);
		  {
		    /* We need to make a possible adjustment for integer parameters. */
		    SwigType *type;
		    Node     *n = 0;

		    while (!n) {
		      type = Getattr((yyvsp[0].p),"type");
		      n = Swig_symbol_clookup(type,0);     /* See if we can find a node that matches the typename */
		      if ((n) && (Strcmp(nodeType(n),"cdecl") == 0)) {
			SwigType *decl = Getattr(n,"decl");
			if (!SwigType_isfunction(decl)) {
			  String *value = Getattr(n,"value");
			  if (value) {
			    String *v = Copy(value);
			    Setattr((yyvsp[0].p),"type",v);
			    Delete(v);
			    n = 0;
			  }
			}
		      } else {
			break;
		      }
		    }
		  }

               }
#line 9326 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 296:
#line 5172 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.p) = NewParmWithoutFileLineInfo(0,0);
                  Setfile((yyval.p),cparse_file);
		  Setline((yyval.p),cparse_line);
		  Setattr((yyval.p),"value",(yyvsp[0].dtype).val);
               }
#line 9337 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 297:
#line 5180 "CParse/parser.y" /* yacc.c:1645  */
    { 
                  (yyval.dtype) = (yyvsp[0].dtype); 
		  if ((yyvsp[0].dtype).type == T_ERROR) {
		    Swig_warning(WARN_PARSE_BAD_DEFAULT,cparse_file, cparse_line, "Can't set default argument (ignored)\n");
		    (yyval.dtype).val = 0;
		    (yyval.dtype).rawval = 0;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = 0;
		    (yyval.dtype).throwf = 0;
		    (yyval.dtype).nexcept = 0;
		  }
               }
#line 9354 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 298:
#line 5192 "CParse/parser.y" /* yacc.c:1645  */
    { 
		  (yyval.dtype) = (yyvsp[-3].dtype);
		  if ((yyvsp[-3].dtype).type == T_ERROR) {
		    Swig_warning(WARN_PARSE_BAD_DEFAULT,cparse_file, cparse_line, "Can't set default argument (ignored)\n");
		    (yyval.dtype) = (yyvsp[-3].dtype);
		    (yyval.dtype).val = 0;
		    (yyval.dtype).rawval = 0;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = 0;
		    (yyval.dtype).throwf = 0;
		    (yyval.dtype).nexcept = 0;
		  } else {
		    (yyval.dtype).val = NewStringf("%s[%s]",(yyvsp[-3].dtype).val,(yyvsp[-1].dtype).val); 
		  }		  
               }
#line 9374 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 299:
#line 5207 "CParse/parser.y" /* yacc.c:1645  */
    {
		 skip_balanced('{','}');
		 (yyval.dtype).val = NewString(scanner_ccode);
		 (yyval.dtype).rawval = 0;
                 (yyval.dtype).type = T_INT;
		 (yyval.dtype).bitfield = 0;
		 (yyval.dtype).throws = 0;
		 (yyval.dtype).throwf = 0;
		 (yyval.dtype).nexcept = 0;
	       }
#line 9389 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 300:
#line 5217 "CParse/parser.y" /* yacc.c:1645  */
    { 
		 (yyval.dtype).val = 0;
		 (yyval.dtype).rawval = 0;
		 (yyval.dtype).type = 0;
		 (yyval.dtype).bitfield = (yyvsp[0].dtype).val;
		 (yyval.dtype).throws = 0;
		 (yyval.dtype).throwf = 0;
		 (yyval.dtype).nexcept = 0;
	       }
#line 9403 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 301:
#line 5226 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype).val = 0;
                 (yyval.dtype).rawval = 0;
                 (yyval.dtype).type = T_INT;
		 (yyval.dtype).bitfield = 0;
		 (yyval.dtype).throws = 0;
		 (yyval.dtype).throwf = 0;
		 (yyval.dtype).nexcept = 0;
               }
#line 9417 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 302:
#line 5237 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.decl) = (yyvsp[-1].decl);
		 (yyval.decl).defarg = (yyvsp[0].dtype).rawval ? (yyvsp[0].dtype).rawval : (yyvsp[0].dtype).val;
            }
#line 9426 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 303:
#line 5241 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[-1].decl);
	      (yyval.decl).defarg = (yyvsp[0].dtype).rawval ? (yyvsp[0].dtype).rawval : (yyvsp[0].dtype).val;
            }
#line 9435 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 304:
#line 5245 "CParse/parser.y" /* yacc.c:1645  */
    {
   	      (yyval.decl).type = 0;
              (yyval.decl).id = 0;
	      (yyval.decl).defarg = (yyvsp[0].dtype).rawval ? (yyvsp[0].dtype).rawval : (yyvsp[0].dtype).val;
            }
#line 9445 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 305:
#line 5252 "CParse/parser.y" /* yacc.c:1645  */
    {
	      SwigType *t;
	      (yyval.decl) = (yyvsp[-4].decl);
	      t = NewStringEmpty();
	      SwigType_add_function(t,(yyvsp[-2].pl));
	      if ((yyvsp[0].dtype).qualifier)
	        SwigType_push(t, (yyvsp[0].dtype).qualifier);
	      if (!(yyval.decl).have_parms) {
		(yyval.decl).parms = (yyvsp[-2].pl);
		(yyval.decl).have_parms = 1;
	      }
	      if (!(yyval.decl).type) {
		(yyval.decl).type = t;
	      } else {
		SwigType_push(t, (yyval.decl).type);
		Delete((yyval.decl).type);
		(yyval.decl).type = t;
	      }
	      (yyval.decl).defarg = 0;
	    }
#line 9470 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 306:
#line 5274 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.decl) = (yyvsp[0].decl);
		 if (SwigType_isfunction((yyvsp[0].decl).type)) {
		   Delete(SwigType_pop_function((yyvsp[0].decl).type));
		 } else if (SwigType_isarray((yyvsp[0].decl).type)) {
		   SwigType *ta = SwigType_pop_arrays((yyvsp[0].decl).type);
		   if (SwigType_isfunction((yyvsp[0].decl).type)) {
		     Delete(SwigType_pop_function((yyvsp[0].decl).type));
		   } else {
		     (yyval.decl).parms = 0;
		   }
		   SwigType_push((yyvsp[0].decl).type,ta);
		   Delete(ta);
		 } else {
		   (yyval.decl).parms = 0;
		 }
            }
#line 9492 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 307:
#line 5291 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      if (SwigType_isfunction((yyvsp[0].decl).type)) {
		Delete(SwigType_pop_function((yyvsp[0].decl).type));
	      } else if (SwigType_isarray((yyvsp[0].decl).type)) {
		SwigType *ta = SwigType_pop_arrays((yyvsp[0].decl).type);
		if (SwigType_isfunction((yyvsp[0].decl).type)) {
		  Delete(SwigType_pop_function((yyvsp[0].decl).type));
		} else {
		  (yyval.decl).parms = 0;
		}
		SwigType_push((yyvsp[0].decl).type,ta);
		Delete(ta);
	      } else {
		(yyval.decl).parms = 0;
	      }
            }
#line 9514 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 308:
#line 5310 "CParse/parser.y" /* yacc.c:1645  */
    {
	      SwigType *t;
	      (yyval.decl) = (yyvsp[-4].decl);
	      t = NewStringEmpty();
	      SwigType_add_function(t, (yyvsp[-2].pl));
	      if ((yyvsp[0].dtype).qualifier)
	        SwigType_push(t, (yyvsp[0].dtype).qualifier);
	      if (!(yyval.decl).have_parms) {
		(yyval.decl).parms = (yyvsp[-2].pl);
		(yyval.decl).have_parms = 1;
	      }
	      if (!(yyval.decl).type) {
		(yyval.decl).type = t;
	      } else {
		SwigType_push(t, (yyval.decl).type);
		Delete((yyval.decl).type);
		(yyval.decl).type = t;
	      }
	    }
#line 9538 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 309:
#line 5329 "CParse/parser.y" /* yacc.c:1645  */
    {
   	      (yyval.decl).type = 0;
              (yyval.decl).id = 0;
	      (yyval.decl).parms = 0;
	      }
#line 9548 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 310:
#line 5336 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      if ((yyval.decl).type) {
		SwigType_push((yyvsp[-1].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-1].type);
           }
#line 9561 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 311:
#line 5344 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_reference((yyvsp[-2].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-2].type);
           }
#line 9575 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 312:
#line 5353 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_rvalue_reference((yyvsp[-2].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-2].type);
           }
#line 9589 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 313:
#line 5362 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      if (!(yyval.decl).type) (yyval.decl).type = NewStringEmpty();
           }
#line 9598 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 314:
#line 5366 "CParse/parser.y" /* yacc.c:1645  */
    {
	     (yyval.decl) = (yyvsp[0].decl);
	     (yyval.decl).type = NewStringEmpty();
	     SwigType_add_reference((yyval.decl).type);
	     if ((yyvsp[0].decl).type) {
	       SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
	       Delete((yyvsp[0].decl).type);
	     }
           }
#line 9612 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 315:
#line 5375 "CParse/parser.y" /* yacc.c:1645  */
    {
	     /* Introduced in C++11, move operator && */
             /* Adds one S/R conflict */
	     (yyval.decl) = (yyvsp[0].decl);
	     (yyval.decl).type = NewStringEmpty();
	     SwigType_add_rvalue_reference((yyval.decl).type);
	     if ((yyvsp[0].decl).type) {
	       SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
	       Delete((yyvsp[0].decl).type);
	     }
           }
#line 9628 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 316:
#line 5386 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();

	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-2].str));
	     if ((yyval.decl).type) {
	       SwigType_push(t,(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = t;
	     }
#line 9644 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 317:
#line 5397 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-2].str));
	     SwigType_push((yyvsp[-3].type),t);
	     if ((yyval.decl).type) {
	       SwigType_push((yyvsp[-3].type),(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = (yyvsp[-3].type);
	     Delete(t);
	   }
#line 9661 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 318:
#line 5409 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer((yyvsp[-4].type),(yyvsp[-3].str));
	     SwigType_add_reference((yyvsp[-4].type));
	     if ((yyval.decl).type) {
	       SwigType_push((yyvsp[-4].type),(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = (yyvsp[-4].type);
	   }
#line 9676 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 319:
#line 5419 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-3].str));
	     SwigType_add_reference(t);
	     if ((yyval.decl).type) {
	       SwigType_push(t,(yyval.decl).type);
	       Delete((yyval.decl).type);
	     } 
	     (yyval.decl).type = t;
	   }
#line 9692 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 320:
#line 5433 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      if ((yyval.decl).type) {
		SwigType_push((yyvsp[-4].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-4].type);
           }
#line 9705 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 321:
#line 5441 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_reference((yyvsp[-5].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-5].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-5].type);
           }
#line 9719 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 322:
#line 5450 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_rvalue_reference((yyvsp[-5].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-5].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-5].type);
           }
#line 9733 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 323:
#line 5459 "CParse/parser.y" /* yacc.c:1645  */
    {
              (yyval.decl) = (yyvsp[0].decl);
	      if (!(yyval.decl).type) (yyval.decl).type = NewStringEmpty();
           }
#line 9742 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 324:
#line 5463 "CParse/parser.y" /* yacc.c:1645  */
    {
	     (yyval.decl) = (yyvsp[0].decl);
	     (yyval.decl).type = NewStringEmpty();
	     SwigType_add_reference((yyval.decl).type);
	     if ((yyvsp[0].decl).type) {
	       SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
	       Delete((yyvsp[0].decl).type);
	     }
           }
#line 9756 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 325:
#line 5472 "CParse/parser.y" /* yacc.c:1645  */
    {
	     /* Introduced in C++11, move operator && */
             /* Adds one S/R conflict */
	     (yyval.decl) = (yyvsp[0].decl);
	     (yyval.decl).type = NewStringEmpty();
	     SwigType_add_rvalue_reference((yyval.decl).type);
	     if ((yyvsp[0].decl).type) {
	       SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
	       Delete((yyvsp[0].decl).type);
	     }
           }
#line 9772 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 326:
#line 5483 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();

	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-5].str));
	     if ((yyval.decl).type) {
	       SwigType_push(t,(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = t;
	     }
#line 9788 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 327:
#line 5494 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-5].str));
	     SwigType_push((yyvsp[-6].type),t);
	     if ((yyval.decl).type) {
	       SwigType_push((yyvsp[-6].type),(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = (yyvsp[-6].type);
	     Delete(t);
	   }
#line 9805 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 328:
#line 5506 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer((yyvsp[-7].type),(yyvsp[-6].str));
	     SwigType_add_reference((yyvsp[-7].type));
	     if ((yyval.decl).type) {
	       SwigType_push((yyvsp[-7].type),(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = (yyvsp[-7].type);
	   }
#line 9820 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 329:
#line 5516 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer((yyvsp[-7].type),(yyvsp[-6].str));
	     SwigType_add_rvalue_reference((yyvsp[-7].type));
	     if ((yyval.decl).type) {
	       SwigType_push((yyvsp[-7].type),(yyval.decl).type);
	       Delete((yyval.decl).type);
	     }
	     (yyval.decl).type = (yyvsp[-7].type);
	   }
#line 9835 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 330:
#line 5526 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-6].str));
	     SwigType_add_reference(t);
	     if ((yyval.decl).type) {
	       SwigType_push(t,(yyval.decl).type);
	       Delete((yyval.decl).type);
	     } 
	     (yyval.decl).type = t;
	   }
#line 9851 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 331:
#line 5537 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     SwigType *t = NewStringEmpty();
	     (yyval.decl) = (yyvsp[0].decl);
	     SwigType_add_memberpointer(t,(yyvsp[-6].str));
	     SwigType_add_rvalue_reference(t);
	     if ((yyval.decl).type) {
	       SwigType_push(t,(yyval.decl).type);
	       Delete((yyval.decl).type);
	     } 
	     (yyval.decl).type = t;
	   }
#line 9867 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 332:
#line 5550 "CParse/parser.y" /* yacc.c:1645  */
    {
                /* Note: This is non-standard C.  Template declarator is allowed to follow an identifier */
                 (yyval.decl).id = Char((yyvsp[0].str));
		 (yyval.decl).type = 0;
		 (yyval.decl).parms = 0;
		 (yyval.decl).have_parms = 0;
                  }
#line 9879 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 333:
#line 5557 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.decl).id = Char(NewStringf("~%s",(yyvsp[0].str)));
                  (yyval.decl).type = 0;
                  (yyval.decl).parms = 0;
                  (yyval.decl).have_parms = 0;
                  }
#line 9890 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 334:
#line 5565 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.decl).id = Char((yyvsp[-1].str));
                  (yyval.decl).type = 0;
                  (yyval.decl).parms = 0;
                  (yyval.decl).have_parms = 0;
                  }
#line 9901 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 335:
#line 5581 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[-1].decl);
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 9914 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 336:
#line 5589 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-1].decl);
		    t = NewStringEmpty();
		    SwigType_add_memberpointer(t,(yyvsp[-3].str));
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
		    }
#line 9930 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 337:
#line 5600 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-2].decl);
		    t = NewStringEmpty();
		    SwigType_add_array(t,"");
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
                  }
#line 9946 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 338:
#line 5611 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-3].decl);
		    t = NewStringEmpty();
		    SwigType_add_array(t,(yyvsp[-1].dtype).val);
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
                  }
#line 9962 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 339:
#line 5622 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
                    (yyval.decl) = (yyvsp[-3].decl);
		    t = NewStringEmpty();
		    SwigType_add_function(t,(yyvsp[-1].pl));
		    if (!(yyval.decl).have_parms) {
		      (yyval.decl).parms = (yyvsp[-1].pl);
		      (yyval.decl).have_parms = 1;
		    }
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = t;
		    } else {
		      SwigType_push(t, (yyval.decl).type);
		      Delete((yyval.decl).type);
		      (yyval.decl).type = t;
		    }
		  }
#line 9984 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 340:
#line 5641 "CParse/parser.y" /* yacc.c:1645  */
    {
                /* Note: This is non-standard C.  Template declarator is allowed to follow an identifier */
                 (yyval.decl).id = Char((yyvsp[0].str));
		 (yyval.decl).type = 0;
		 (yyval.decl).parms = 0;
		 (yyval.decl).have_parms = 0;
                  }
#line 9996 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 341:
#line 5649 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.decl).id = Char(NewStringf("~%s",(yyvsp[0].str)));
                  (yyval.decl).type = 0;
                  (yyval.decl).parms = 0;
                  (yyval.decl).have_parms = 0;
                  }
#line 10007 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 342:
#line 5666 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[-1].decl);
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 10020 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 343:
#line 5674 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl) = (yyvsp[-1].decl);
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = NewStringEmpty();
		    }
		    SwigType_add_reference((yyval.decl).type);
                  }
#line 10032 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 344:
#line 5681 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl) = (yyvsp[-1].decl);
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = NewStringEmpty();
		    }
		    SwigType_add_rvalue_reference((yyval.decl).type);
                  }
#line 10044 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 345:
#line 5688 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-1].decl);
		    t = NewStringEmpty();
		    SwigType_add_memberpointer(t,(yyvsp[-3].str));
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
		  }
#line 10060 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 346:
#line 5699 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-1].decl);
		    t = NewStringEmpty();
		    SwigType_add_memberpointer(t, (yyvsp[-4].str));
		    SwigType_push(t, (yyvsp[-2].str));
		    if ((yyval.decl).type) {
		      SwigType_push(t, (yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
		  }
#line 10077 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 347:
#line 5711 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-1].decl);
		    t = NewStringEmpty();
		    SwigType_add_memberpointer(t, (yyvsp[-3].str));
		    if ((yyval.decl).type) {
		      SwigType_push(t, (yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
		  }
#line 10093 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 348:
#line 5722 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-1].decl);
		    t = NewStringEmpty();
		    SwigType_add_memberpointer(t, (yyvsp[-4].str));
		    SwigType_push(t, (yyvsp[-2].str));
		    if ((yyval.decl).type) {
		      SwigType_push(t, (yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
		  }
#line 10110 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 349:
#line 5734 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-2].decl);
		    t = NewStringEmpty();
		    SwigType_add_array(t,"");
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
                  }
#line 10126 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 350:
#line 5745 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-3].decl);
		    t = NewStringEmpty();
		    SwigType_add_array(t,(yyvsp[-1].dtype).val);
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
                  }
#line 10142 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 351:
#line 5756 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
                    (yyval.decl) = (yyvsp[-3].decl);
		    t = NewStringEmpty();
		    SwigType_add_function(t,(yyvsp[-1].pl));
		    if (!(yyval.decl).have_parms) {
		      (yyval.decl).parms = (yyvsp[-1].pl);
		      (yyval.decl).have_parms = 1;
		    }
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = t;
		    } else {
		      SwigType_push(t, (yyval.decl).type);
		      Delete((yyval.decl).type);
		      (yyval.decl).type = t;
		    }
		  }
#line 10164 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 352:
#line 5776 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
                    Append((yyvsp[-4].str), " "); /* intervening space is mandatory */
                    Append((yyvsp[-4].str), Char((yyvsp[-3].id)));
		    (yyval.decl).id = Char((yyvsp[-4].str));
		    t = NewStringEmpty();
		    SwigType_add_function(t,(yyvsp[-1].pl));
		    if (!(yyval.decl).have_parms) {
		      (yyval.decl).parms = (yyvsp[-1].pl);
		      (yyval.decl).have_parms = 1;
		    }
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = t;
		    } else {
		      SwigType_push(t, (yyval.decl).type);
		      Delete((yyval.decl).type);
		      (yyval.decl).type = t;
		    }
		  }
#line 10188 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 353:
#line 5797 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl).type = (yyvsp[0].type);
                    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
                  }
#line 10199 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 354:
#line 5803 "CParse/parser.y" /* yacc.c:1645  */
    { 
                     (yyval.decl) = (yyvsp[0].decl);
                     SwigType_push((yyvsp[-1].type),(yyvsp[0].decl).type);
		     (yyval.decl).type = (yyvsp[-1].type);
		     Delete((yyvsp[0].decl).type);
                  }
#line 10210 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 355:
#line 5809 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl).type = (yyvsp[-1].type);
		    SwigType_add_reference((yyval.decl).type);
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		  }
#line 10222 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 356:
#line 5816 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl).type = (yyvsp[-1].type);
		    SwigType_add_rvalue_reference((yyval.decl).type);
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		  }
#line 10234 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 357:
#line 5823 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[0].decl);
		    SwigType_add_reference((yyvsp[-2].type));
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 10248 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 358:
#line 5832 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[0].decl);
		    SwigType_add_rvalue_reference((yyvsp[-2].type));
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 10262 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 359:
#line 5841 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[0].decl);
                  }
#line 10270 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 360:
#line 5844 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[0].decl);
		    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_reference((yyval.decl).type);
		    if ((yyvsp[0].decl).type) {
		      SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
		      Delete((yyvsp[0].decl).type);
		    }
                  }
#line 10284 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 361:
#line 5853 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl) = (yyvsp[0].decl);
		    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_rvalue_reference((yyval.decl).type);
		    if ((yyvsp[0].decl).type) {
		      SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
		      Delete((yyvsp[0].decl).type);
		    }
                  }
#line 10298 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 362:
#line 5862 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl).id = 0;
                    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
                    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_reference((yyval.decl).type);
                  }
#line 10310 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 363:
#line 5869 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl).id = 0;
                    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
                    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_rvalue_reference((yyval.decl).type);
                  }
#line 10322 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 364:
#line 5876 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    (yyval.decl).type = NewStringEmpty();
                    SwigType_add_memberpointer((yyval.decl).type,(yyvsp[-1].str));
                    (yyval.decl).id = 0;
                    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
      	          }
#line 10334 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 365:
#line 5883 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_memberpointer((yyval.decl).type, (yyvsp[-2].str));
		    SwigType_push((yyval.decl).type, (yyvsp[0].str));
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		  }
#line 10347 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 366:
#line 5891 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t = NewStringEmpty();
                    (yyval.decl).type = (yyvsp[-2].type);
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		    SwigType_add_memberpointer(t,(yyvsp[-1].str));
		    SwigType_push((yyval.decl).type,t);
		    Delete(t);
                  }
#line 10362 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 367:
#line 5901 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    (yyval.decl) = (yyvsp[0].decl);
		    SwigType_add_memberpointer((yyvsp[-3].type),(yyvsp[-2].str));
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-3].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-3].type);
                  }
#line 10376 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 368:
#line 5912 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-2].decl);
		    t = NewStringEmpty();
		    SwigType_add_array(t,"");
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
                  }
#line 10392 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 369:
#line 5923 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    SwigType *t;
		    (yyval.decl) = (yyvsp[-3].decl);
		    t = NewStringEmpty();
		    SwigType_add_array(t,(yyvsp[-1].dtype).val);
		    if ((yyval.decl).type) {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = t;
                  }
#line 10408 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 370:
#line 5934 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    (yyval.decl).type = NewStringEmpty();
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		    SwigType_add_array((yyval.decl).type,"");
                  }
#line 10420 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 371:
#line 5941 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    (yyval.decl).type = NewStringEmpty();
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		    SwigType_add_array((yyval.decl).type,(yyvsp[-1].dtype).val);
		  }
#line 10432 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 372:
#line 5948 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl) = (yyvsp[-1].decl);
		  }
#line 10440 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 373:
#line 5951 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
                    (yyval.decl) = (yyvsp[-3].decl);
		    t = NewStringEmpty();
                    SwigType_add_function(t,(yyvsp[-1].pl));
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = t;
		    } else {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		      (yyval.decl).type = t;
		    }
		    if (!(yyval.decl).have_parms) {
		      (yyval.decl).parms = (yyvsp[-1].pl);
		      (yyval.decl).have_parms = 1;
		    }
		  }
#line 10462 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 374:
#line 5968 "CParse/parser.y" /* yacc.c:1645  */
    {
		    SwigType *t;
                    (yyval.decl) = (yyvsp[-4].decl);
		    t = NewStringEmpty();
                    SwigType_add_function(t,(yyvsp[-2].pl));
		    SwigType_push(t, (yyvsp[0].dtype).qualifier);
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = t;
		    } else {
		      SwigType_push(t,(yyval.decl).type);
		      Delete((yyval.decl).type);
		      (yyval.decl).type = t;
		    }
		    if (!(yyval.decl).have_parms) {
		      (yyval.decl).parms = (yyvsp[-2].pl);
		      (yyval.decl).have_parms = 1;
		    }
		  }
#line 10485 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 375:
#line 5986 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl).type = NewStringEmpty();
                    SwigType_add_function((yyval.decl).type,(yyvsp[-1].pl));
		    (yyval.decl).parms = (yyvsp[-1].pl);
		    (yyval.decl).have_parms = 1;
		    (yyval.decl).id = 0;
                  }
#line 10497 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 376:
#line 5996 "CParse/parser.y" /* yacc.c:1645  */
    { 
             (yyval.type) = NewStringEmpty();
             SwigType_add_pointer((yyval.type));
	     SwigType_push((yyval.type),(yyvsp[-1].str));
	     SwigType_push((yyval.type),(yyvsp[0].type));
	     Delete((yyvsp[0].type));
           }
#line 10509 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 377:
#line 6003 "CParse/parser.y" /* yacc.c:1645  */
    {
	     (yyval.type) = NewStringEmpty();
	     SwigType_add_pointer((yyval.type));
	     SwigType_push((yyval.type),(yyvsp[0].type));
	     Delete((yyvsp[0].type));
	   }
#line 10520 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 378:
#line 6009 "CParse/parser.y" /* yacc.c:1645  */
    { 
	     (yyval.type) = NewStringEmpty();
	     SwigType_add_pointer((yyval.type));
	     SwigType_push((yyval.type),(yyvsp[0].str));
           }
#line 10530 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 379:
#line 6014 "CParse/parser.y" /* yacc.c:1645  */
    {
	     (yyval.type) = NewStringEmpty();
	     SwigType_add_pointer((yyval.type));
           }
#line 10539 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 380:
#line 6021 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).qualifier = (yyvsp[0].str);
		  (yyval.dtype).refqualifier = 0;
	       }
#line 10548 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 381:
#line 6025 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).qualifier = (yyvsp[-1].str);
		  (yyval.dtype).refqualifier = (yyvsp[0].str);
		  SwigType_push((yyval.dtype).qualifier, (yyvsp[0].str));
	       }
#line 10558 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 382:
#line 6030 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).qualifier = NewStringEmpty();
		  (yyval.dtype).refqualifier = (yyvsp[0].str);
		  SwigType_push((yyval.dtype).qualifier, (yyvsp[0].str));
	       }
#line 10568 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 383:
#line 6037 "CParse/parser.y" /* yacc.c:1645  */
    {
	          (yyval.str) = NewStringEmpty();
	          SwigType_add_reference((yyval.str));
	       }
#line 10577 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 384:
#line 6041 "CParse/parser.y" /* yacc.c:1645  */
    {
	          (yyval.str) = NewStringEmpty();
	          SwigType_add_rvalue_reference((yyval.str));
	       }
#line 10586 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 385:
#line 6047 "CParse/parser.y" /* yacc.c:1645  */
    {
	          (yyval.str) = NewStringEmpty();
	          if ((yyvsp[0].id)) SwigType_add_qualifier((yyval.str),(yyvsp[0].id));
               }
#line 10595 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 386:
#line 6051 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.str) = (yyvsp[0].str);
	          if ((yyvsp[-1].id)) SwigType_add_qualifier((yyval.str),(yyvsp[-1].id));
               }
#line 10604 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 387:
#line 6057 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "const"; }
#line 10610 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 388:
#line 6058 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = "volatile"; }
#line 10616 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 389:
#line 6059 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = 0; }
#line 10622 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 390:
#line 6065 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.type) = (yyvsp[0].type);
                   Replace((yyval.type),"typename ","", DOH_REPLACE_ANY);
                }
#line 10631 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 391:
#line 6071 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.type) = (yyvsp[0].type);
	           SwigType_push((yyval.type),(yyvsp[-1].str));
               }
#line 10640 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 392:
#line 6075 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 10646 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 393:
#line 6076 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.type) = (yyvsp[-1].type);
	          SwigType_push((yyval.type),(yyvsp[0].str));
	       }
#line 10655 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 394:
#line 6080 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.type) = (yyvsp[-1].type);
	          SwigType_push((yyval.type),(yyvsp[0].str));
	          SwigType_push((yyval.type),(yyvsp[-2].str));
	       }
#line 10665 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 395:
#line 6087 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type);
                  /* Printf(stdout,"primitive = '%s'\n", $$);*/
               }
#line 10673 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 396:
#line 6090 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 10679 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 397:
#line 6091 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 10685 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 398:
#line 6095 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = NewStringf("enum %s", (yyvsp[0].str)); }
#line 10691 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 399:
#line 6096 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.type) = (yyvsp[0].type); }
#line 10697 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 400:
#line 6098 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.type) = (yyvsp[0].str);
               }
#line 10705 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 401:
#line 6101 "CParse/parser.y" /* yacc.c:1645  */
    { 
		 (yyval.type) = NewStringf("%s %s", (yyvsp[-1].id), (yyvsp[0].str));
               }
#line 10713 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 402:
#line 6104 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.type) = (yyvsp[0].type);
               }
#line 10721 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 403:
#line 6109 "CParse/parser.y" /* yacc.c:1645  */
    {
                 Node *n = Swig_symbol_clookup((yyvsp[-1].str),0);
                 if (!n) {
		   Swig_error(cparse_file, cparse_line, "Identifier %s not defined.\n", (yyvsp[-1].str));
                   (yyval.type) = (yyvsp[-1].str);
                 } else {
                   (yyval.type) = Getattr(n, "type");
                 }
               }
#line 10735 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 404:
#line 6120 "CParse/parser.y" /* yacc.c:1645  */
    {
		 if (!(yyvsp[0].ptype).type) (yyvsp[0].ptype).type = NewString("int");
		 if ((yyvsp[0].ptype).us) {
		   (yyval.type) = NewStringf("%s %s", (yyvsp[0].ptype).us, (yyvsp[0].ptype).type);
		   Delete((yyvsp[0].ptype).us);
                   Delete((yyvsp[0].ptype).type);
		 } else {
                   (yyval.type) = (yyvsp[0].ptype).type;
		 }
		 if (Cmp((yyval.type),"signed int") == 0) {
		   Delete((yyval.type));
		   (yyval.type) = NewString("int");
                 } else if (Cmp((yyval.type),"signed long") == 0) {
		   Delete((yyval.type));
                   (yyval.type) = NewString("long");
                 } else if (Cmp((yyval.type),"signed short") == 0) {
		   Delete((yyval.type));
		   (yyval.type) = NewString("short");
		 } else if (Cmp((yyval.type),"signed long long") == 0) {
		   Delete((yyval.type));
		   (yyval.type) = NewString("long long");
		 }
               }
#line 10763 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 405:
#line 6145 "CParse/parser.y" /* yacc.c:1645  */
    { 
                 (yyval.ptype) = (yyvsp[0].ptype);
               }
#line 10771 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 406:
#line 6148 "CParse/parser.y" /* yacc.c:1645  */
    {
                    if ((yyvsp[-1].ptype).us && (yyvsp[0].ptype).us) {
		      Swig_error(cparse_file, cparse_line, "Extra %s specifier.\n", (yyvsp[0].ptype).us);
		    }
                    (yyval.ptype) = (yyvsp[0].ptype);
                    if ((yyvsp[-1].ptype).us) (yyval.ptype).us = (yyvsp[-1].ptype).us;
		    if ((yyvsp[-1].ptype).type) {
		      if (!(yyvsp[0].ptype).type) (yyval.ptype).type = (yyvsp[-1].ptype).type;
		      else {
			int err = 0;
			if ((Cmp((yyvsp[-1].ptype).type,"long") == 0)) {
			  if ((Cmp((yyvsp[0].ptype).type,"long") == 0) || (Strncmp((yyvsp[0].ptype).type,"double",6) == 0)) {
			    (yyval.ptype).type = NewStringf("long %s", (yyvsp[0].ptype).type);
			  } else if (Cmp((yyvsp[0].ptype).type,"int") == 0) {
			    (yyval.ptype).type = (yyvsp[-1].ptype).type;
			  } else {
			    err = 1;
			  }
			} else if ((Cmp((yyvsp[-1].ptype).type,"short")) == 0) {
			  if (Cmp((yyvsp[0].ptype).type,"int") == 0) {
			    (yyval.ptype).type = (yyvsp[-1].ptype).type;
			  } else {
			    err = 1;
			  }
			} else if (Cmp((yyvsp[-1].ptype).type,"int") == 0) {
			  (yyval.ptype).type = (yyvsp[0].ptype).type;
			} else if (Cmp((yyvsp[-1].ptype).type,"double") == 0) {
			  if (Cmp((yyvsp[0].ptype).type,"long") == 0) {
			    (yyval.ptype).type = NewString("long double");
			  } else if (Cmp((yyvsp[0].ptype).type,"complex") == 0) {
			    (yyval.ptype).type = NewString("double complex");
			  } else {
			    err = 1;
			  }
			} else if (Cmp((yyvsp[-1].ptype).type,"float") == 0) {
			  if (Cmp((yyvsp[0].ptype).type,"complex") == 0) {
			    (yyval.ptype).type = NewString("float complex");
			  } else {
			    err = 1;
			  }
			} else if (Cmp((yyvsp[-1].ptype).type,"complex") == 0) {
			  (yyval.ptype).type = NewStringf("%s complex", (yyvsp[0].ptype).type);
			} else {
			  err = 1;
			}
			if (err) {
			  Swig_error(cparse_file, cparse_line, "Extra %s specifier.\n", (yyvsp[-1].ptype).type);
			}
		      }
		    }
               }
#line 10827 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 407:
#line 6202 "CParse/parser.y" /* yacc.c:1645  */
    { 
		    (yyval.ptype).type = NewString("int");
                    (yyval.ptype).us = 0;
               }
#line 10836 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 408:
#line 6206 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("short");
                    (yyval.ptype).us = 0;
                }
#line 10845 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 409:
#line 6210 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("long");
                    (yyval.ptype).us = 0;
                }
#line 10854 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 410:
#line 6214 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("char");
                    (yyval.ptype).us = 0;
                }
#line 10863 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 411:
#line 6218 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("wchar_t");
                    (yyval.ptype).us = 0;
                }
#line 10872 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 412:
#line 6222 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("float");
                    (yyval.ptype).us = 0;
                }
#line 10881 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 413:
#line 6226 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("double");
                    (yyval.ptype).us = 0;
                }
#line 10890 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 414:
#line 6230 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).us = NewString("signed");
                    (yyval.ptype).type = 0;
                }
#line 10899 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 415:
#line 6234 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).us = NewString("unsigned");
                    (yyval.ptype).type = 0;
                }
#line 10908 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 416:
#line 6238 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("complex");
                    (yyval.ptype).us = 0;
                }
#line 10917 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 417:
#line 6242 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("__int8");
                    (yyval.ptype).us = 0;
                }
#line 10926 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 418:
#line 6246 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("__int16");
                    (yyval.ptype).us = 0;
                }
#line 10935 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 419:
#line 6250 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("__int32");
                    (yyval.ptype).us = 0;
                }
#line 10944 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 420:
#line 6254 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.ptype).type = NewString("__int64");
                    (yyval.ptype).us = 0;
                }
#line 10953 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 421:
#line 6260 "CParse/parser.y" /* yacc.c:1645  */
    { /* scanner_check_typedef(); */ }
#line 10959 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 422:
#line 6260 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.dtype) = (yyvsp[0].dtype);
		   if ((yyval.dtype).type == T_STRING) {
		     (yyval.dtype).rawval = NewStringf("\"%(escape)s\"",(yyval.dtype).val);
		   } else if ((yyval.dtype).type != T_CHAR && (yyval.dtype).type != T_WSTRING && (yyval.dtype).type != T_WCHAR) {
		     (yyval.dtype).rawval = NewStringf("%s", (yyval.dtype).val);
		   }
		   (yyval.dtype).qualifier = 0;
		   (yyval.dtype).refqualifier = 0;
		   (yyval.dtype).bitfield = 0;
		   (yyval.dtype).throws = 0;
		   (yyval.dtype).throwf = 0;
		   (yyval.dtype).nexcept = 0;
		   scanner_ignore_typedef();
                }
#line 10979 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 423:
#line 6275 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype) = (yyvsp[0].dtype);
		}
#line 10987 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 424:
#line 6280 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype) = (yyvsp[0].dtype);
		}
#line 10995 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 425:
#line 6283 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype) = (yyvsp[0].dtype);
		}
#line 11003 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 426:
#line 6289 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).val = NewString("delete");
		  (yyval.dtype).rawval = 0;
		  (yyval.dtype).type = T_STRING;
		  (yyval.dtype).qualifier = 0;
		  (yyval.dtype).refqualifier = 0;
		  (yyval.dtype).bitfield = 0;
		  (yyval.dtype).throws = 0;
		  (yyval.dtype).throwf = 0;
		  (yyval.dtype).nexcept = 0;
		}
#line 11019 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 427:
#line 6303 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).val = NewString("default");
		  (yyval.dtype).rawval = 0;
		  (yyval.dtype).type = T_STRING;
		  (yyval.dtype).qualifier = 0;
		  (yyval.dtype).refqualifier = 0;
		  (yyval.dtype).bitfield = 0;
		  (yyval.dtype).throws = 0;
		  (yyval.dtype).throwf = 0;
		  (yyval.dtype).nexcept = 0;
		}
#line 11035 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 428:
#line 6318 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (yyvsp[0].id); }
#line 11041 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 429:
#line 6319 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (char *) 0;}
#line 11047 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 435:
#line 6334 "CParse/parser.y" /* yacc.c:1645  */
    {
		  Setattr((yyvsp[-1].node),"_last",(yyvsp[-1].node));
		  (yyval.node) = (yyvsp[-1].node);
		}
#line 11056 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 436:
#line 6338 "CParse/parser.y" /* yacc.c:1645  */
    {
		  set_nextSibling((yyvsp[-2].node), (yyvsp[-1].node));
		  Setattr((yyvsp[-2].node),"_last",Getattr((yyvsp[-1].node),"_last"));
		  Setattr((yyvsp[-1].node),"_last",NULL);
		  (yyval.node) = (yyvsp[-2].node);
		}
#line 11067 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 437:
#line 6344 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = 0;
		}
#line 11075 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 438:
#line 6349 "CParse/parser.y" /* yacc.c:1645  */
    {
		  Setattr((yyvsp[0].node),"_last",(yyvsp[0].node));
		  (yyval.node) = (yyvsp[0].node);
		}
#line 11084 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 439:
#line 6353 "CParse/parser.y" /* yacc.c:1645  */
    {
		  set_nextSibling(Getattr((yyvsp[-2].node),"_last"), (yyvsp[0].node));
		  Setattr((yyvsp[-2].node),"_last",(yyvsp[0].node));
		  (yyval.node) = (yyvsp[-2].node);
		}
#line 11094 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 440:
#line 6360 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[-1].node);
		}
#line 11102 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 441:
#line 6365 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[0].node);
		}
#line 11110 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 442:
#line 6368 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[0].node);
		  set_comment((yyvsp[0].node), (yyvsp[-1].str));
		}
#line 11119 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 443:
#line 6372 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[-1].node);
		  set_comment((yyvsp[-1].node), (yyvsp[0].str));
		}
#line 11128 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 444:
#line 6376 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[0].node);
		  set_comment(previousNode, (yyvsp[-1].str));
		}
#line 11137 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 445:
#line 6380 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.node) = (yyvsp[-1].node);
		  set_comment(previousNode, (yyvsp[-2].str));
		  set_comment((yyvsp[-1].node), (yyvsp[0].str));
		}
#line 11147 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 446:
#line 6387 "CParse/parser.y" /* yacc.c:1645  */
    {
		   SwigType *type = NewSwigType(T_INT);
		   (yyval.node) = new_node("enumitem");
		   Setattr((yyval.node),"name",(yyvsp[0].id));
		   Setattr((yyval.node),"type",type);
		   SetFlag((yyval.node),"feature:immutable");
		   Delete(type);
		 }
#line 11160 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 447:
#line 6395 "CParse/parser.y" /* yacc.c:1645  */
    {
		   SwigType *type = NewSwigType((yyvsp[0].dtype).type == T_BOOL ? T_BOOL : ((yyvsp[0].dtype).type == T_CHAR ? T_CHAR : T_INT));
		   (yyval.node) = new_node("enumitem");
		   Setattr((yyval.node),"name",(yyvsp[-2].id));
		   Setattr((yyval.node),"type",type);
		   SetFlag((yyval.node),"feature:immutable");
		   Setattr((yyval.node),"enumvalue", (yyvsp[0].dtype).val);
		   Setattr((yyval.node),"value",(yyvsp[-2].id));
		   Delete(type);
                 }
#line 11175 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 448:
#line 6407 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.dtype) = (yyvsp[0].dtype);
		   if (((yyval.dtype).type != T_INT) && ((yyval.dtype).type != T_UINT) &&
		       ((yyval.dtype).type != T_LONG) && ((yyval.dtype).type != T_ULONG) &&
		       ((yyval.dtype).type != T_LONGLONG) && ((yyval.dtype).type != T_ULONGLONG) &&
		       ((yyval.dtype).type != T_SHORT) && ((yyval.dtype).type != T_USHORT) &&
		       ((yyval.dtype).type != T_SCHAR) && ((yyval.dtype).type != T_UCHAR) &&
		       ((yyval.dtype).type != T_CHAR) && ((yyval.dtype).type != T_BOOL)) {
		     Swig_error(cparse_file,cparse_line,"Type error. Expecting an integral type\n");
		   }
                }
#line 11191 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 449:
#line 6422 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11197 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 450:
#line 6423 "CParse/parser.y" /* yacc.c:1645  */
    {
		 Node *n;
		 (yyval.dtype).val = (yyvsp[0].type);
		 (yyval.dtype).type = T_INT;
		 /* Check if value is in scope */
		 n = Swig_symbol_clookup((yyvsp[0].type),0);
		 if (n) {
                   /* A band-aid for enum values used in expressions. */
                   if (Strcmp(nodeType(n),"enumitem") == 0) {
                     String *q = Swig_symbol_qualified(n);
                     if (q) {
                       (yyval.dtype).val = NewStringf("%s::%s", q, Getattr(n,"name"));
                       Delete(q);
                     }
                   }
		 }
               }
#line 11219 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 451:
#line 6443 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s->%s", (yyvsp[-2].id), (yyvsp[0].id));
		 (yyval.dtype).type = 0;
	       }
#line 11228 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 452:
#line 6447 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype) = (yyvsp[-2].dtype);
		 Printf((yyval.dtype).val, "->%s", (yyvsp[0].id));
	       }
#line 11237 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 453:
#line 6457 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype) = (yyvsp[-2].dtype);
		 Printf((yyval.dtype).val, ".%s", (yyvsp[0].id));
	       }
#line 11246 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 454:
#line 6463 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype) = (yyvsp[0].dtype);
               }
#line 11254 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 455:
#line 6466 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype) = (yyvsp[0].dtype);
               }
#line 11262 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 456:
#line 6469 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype).val = (yyvsp[0].str);
                    (yyval.dtype).type = T_STRING;
               }
#line 11271 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 457:
#line 6473 "CParse/parser.y" /* yacc.c:1645  */
    {
		  SwigType_push((yyvsp[-2].type),(yyvsp[-1].decl).type);
		  (yyval.dtype).val = NewStringf("sizeof(%s)",SwigType_str((yyvsp[-2].type),0));
		  (yyval.dtype).type = T_ULONG;
               }
#line 11281 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 458:
#line 6478 "CParse/parser.y" /* yacc.c:1645  */
    {
		  SwigType_push((yyvsp[-2].type),(yyvsp[-1].decl).type);
		  (yyval.dtype).val = NewStringf("sizeof...(%s)",SwigType_str((yyvsp[-2].type),0));
		  (yyval.dtype).type = T_ULONG;
               }
#line 11291 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 459:
#line 6483 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11297 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 460:
#line 6484 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype).val = (yyvsp[0].str);
		    (yyval.dtype).rawval = NewStringf("L\"%s\"", (yyval.dtype).val);
                    (yyval.dtype).type = T_WSTRING;
	       }
#line 11307 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 461:
#line 6489 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).val = NewString((yyvsp[0].str));
		  if (Len((yyval.dtype).val)) {
		    (yyval.dtype).rawval = NewStringf("'%(escape)s'", (yyval.dtype).val);
		  } else {
		    (yyval.dtype).rawval = NewString("'\\0'");
		  }
		  (yyval.dtype).type = T_CHAR;
		  (yyval.dtype).bitfield = 0;
		  (yyval.dtype).throws = 0;
		  (yyval.dtype).throwf = 0;
		  (yyval.dtype).nexcept = 0;
	       }
#line 11325 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 462:
#line 6502 "CParse/parser.y" /* yacc.c:1645  */
    {
		  (yyval.dtype).val = NewString((yyvsp[0].str));
		  if (Len((yyval.dtype).val)) {
		    (yyval.dtype).rawval = NewStringf("L\'%s\'", (yyval.dtype).val);
		  } else {
		    (yyval.dtype).rawval = NewString("L'\\0'");
		  }
		  (yyval.dtype).type = T_WCHAR;
		  (yyval.dtype).bitfield = 0;
		  (yyval.dtype).throws = 0;
		  (yyval.dtype).throwf = 0;
		  (yyval.dtype).nexcept = 0;
	       }
#line 11343 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 463:
#line 6517 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype).val = NewStringf("(%s)",(yyvsp[-1].dtype).val);
		    if ((yyvsp[-1].dtype).rawval) {
		      (yyval.dtype).rawval = NewStringf("(%s)",(yyvsp[-1].dtype).rawval);
		    }
		    (yyval.dtype).type = (yyvsp[-1].dtype).type;
	       }
#line 11355 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 464:
#line 6527 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   switch ((yyvsp[-2].dtype).type) {
		     case T_FLOAT:
		     case T_DOUBLE:
		     case T_LONGDOUBLE:
		     case T_FLTCPLX:
		     case T_DBLCPLX:
		       (yyval.dtype).val = NewStringf("(%s)%s", (yyvsp[-2].dtype).val, (yyvsp[0].dtype).val); /* SwigType_str and decimal points don't mix! */
		       break;
		     default:
		       (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-2].dtype).val,0), (yyvsp[0].dtype).val);
		       break;
		   }
		 }
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type, (yyvsp[0].dtype).type);
 	       }
#line 11378 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 465:
#line 6545 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_push((yyvsp[-3].dtype).val,(yyvsp[-2].type));
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-3].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11390 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 466:
#line 6552 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_add_reference((yyvsp[-3].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-3].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11402 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 467:
#line 6559 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_add_rvalue_reference((yyvsp[-3].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-3].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11414 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 468:
#line 6566 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_push((yyvsp[-4].dtype).val,(yyvsp[-3].type));
		   SwigType_add_reference((yyvsp[-4].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-4].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11427 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 469:
#line 6574 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_push((yyvsp[-4].dtype).val,(yyvsp[-3].type));
		   SwigType_add_rvalue_reference((yyvsp[-4].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-4].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11440 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 470:
#line 6582 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype) = (yyvsp[0].dtype);
                 (yyval.dtype).val = NewStringf("&%s",(yyvsp[0].dtype).val);
	       }
#line 11449 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 471:
#line 6586 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype) = (yyvsp[0].dtype);
                 (yyval.dtype).val = NewStringf("&&%s",(yyvsp[0].dtype).val);
	       }
#line 11458 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 472:
#line 6590 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype) = (yyvsp[0].dtype);
                 (yyval.dtype).val = NewStringf("*%s",(yyvsp[0].dtype).val);
	       }
#line 11467 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 473:
#line 6596 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11473 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 474:
#line 6597 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11479 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 475:
#line 6598 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11485 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 476:
#line 6599 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11491 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 477:
#line 6600 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11497 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 478:
#line 6601 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11503 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 479:
#line 6602 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11509 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 480:
#line 6603 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11515 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 481:
#line 6606 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s+%s", COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11524 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 482:
#line 6610 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s-%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11533 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 483:
#line 6614 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s*%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11542 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 484:
#line 6618 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s/%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11551 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 485:
#line 6622 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s%%%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11560 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 486:
#line 6626 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s&%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11569 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 487:
#line 6630 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s|%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11578 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 488:
#line 6634 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s^%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11587 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 489:
#line 6638 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s << %s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote_type((yyvsp[-2].dtype).type);
	       }
#line 11596 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 490:
#line 6642 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s >> %s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote_type((yyvsp[-2].dtype).type);
	       }
#line 11605 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 491:
#line 6646 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s&&%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11614 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 492:
#line 6650 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s||%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11623 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 493:
#line 6654 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s==%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11632 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 494:
#line 6658 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s!=%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11641 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 495:
#line 6672 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s >= %s", COMPOUND_EXPR_VAL((yyvsp[-2].dtype)), COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11650 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 496:
#line 6676 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s <= %s", COMPOUND_EXPR_VAL((yyvsp[-2].dtype)), COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11659 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 497:
#line 6680 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("%s?%s:%s", COMPOUND_EXPR_VAL((yyvsp[-4].dtype)), COMPOUND_EXPR_VAL((yyvsp[-2].dtype)), COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 /* This may not be exactly right, but is probably good enough
		  * for the purposes of parsing constant expressions. */
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type, (yyvsp[0].dtype).type);
	       }
#line 11670 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 498:
#line 6686 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("-%s",(yyvsp[0].dtype).val);
		 (yyval.dtype).type = (yyvsp[0].dtype).type;
	       }
#line 11679 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 499:
#line 6690 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype).val = NewStringf("+%s",(yyvsp[0].dtype).val);
		 (yyval.dtype).type = (yyvsp[0].dtype).type;
	       }
#line 11688 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 500:
#line 6694 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.dtype).val = NewStringf("~%s",(yyvsp[0].dtype).val);
		 (yyval.dtype).type = (yyvsp[0].dtype).type;
	       }
#line 11697 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 501:
#line 6698 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.dtype).val = NewStringf("!%s",COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = T_INT;
	       }
#line 11706 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 502:
#line 6702 "CParse/parser.y" /* yacc.c:1645  */
    {
		 String *qty;
                 skip_balanced('(',')');
		 qty = Swig_symbol_type_qualify((yyvsp[-1].type),0);
		 if (SwigType_istemplate(qty)) {
		   String *nstr = SwigType_namestr(qty);
		   Delete(qty);
		   qty = nstr;
		 }
		 (yyval.dtype).val = NewStringf("%s%s",qty,scanner_ccode);
		 Clear(scanner_ccode);
		 (yyval.dtype).type = T_INT;
		 Delete(qty);
               }
#line 11725 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 503:
#line 6718 "CParse/parser.y" /* yacc.c:1645  */
    {
	        (yyval.str) = NewString("...");
	      }
#line 11733 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 504:
#line 6723 "CParse/parser.y" /* yacc.c:1645  */
    {
	        (yyval.str) = (yyvsp[0].str);
	      }
#line 11741 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 505:
#line 6726 "CParse/parser.y" /* yacc.c:1645  */
    {
	        (yyval.str) = 0;
	      }
#line 11749 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 506:
#line 6731 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.bases) = (yyvsp[0].bases);
               }
#line 11757 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 507:
#line 6736 "CParse/parser.y" /* yacc.c:1645  */
    { inherit_list = 1; }
#line 11763 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 508:
#line 6736 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.bases) = (yyvsp[0].bases); inherit_list = 0; }
#line 11769 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 509:
#line 6737 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.bases) = 0; }
#line 11775 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 510:
#line 6740 "CParse/parser.y" /* yacc.c:1645  */
    {
		   Hash *list = NewHash();
		   Node *base = (yyvsp[0].node);
		   Node *name = Getattr(base,"name");
		   List *lpublic = NewList();
		   List *lprotected = NewList();
		   List *lprivate = NewList();
		   Setattr(list,"public",lpublic);
		   Setattr(list,"protected",lprotected);
		   Setattr(list,"private",lprivate);
		   Delete(lpublic);
		   Delete(lprotected);
		   Delete(lprivate);
		   Append(Getattr(list,Getattr(base,"access")),name);
	           (yyval.bases) = list;
               }
#line 11796 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 511:
#line 6757 "CParse/parser.y" /* yacc.c:1645  */
    {
		   Hash *list = (yyvsp[-2].bases);
		   Node *base = (yyvsp[0].node);
		   Node *name = Getattr(base,"name");
		   Append(Getattr(list,Getattr(base,"access")),name);
                   (yyval.bases) = list;
               }
#line 11808 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 512:
#line 6766 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.intvalue) = cparse_line;
	       }
#line 11816 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 513:
#line 6768 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = NewHash();
		 Setfile((yyval.node),cparse_file);
		 Setline((yyval.node),(yyvsp[-2].intvalue));
		 Setattr((yyval.node),"name",(yyvsp[-1].str));
		 Setfile((yyvsp[-1].str),cparse_file);
		 Setline((yyvsp[-1].str),(yyvsp[-2].intvalue));
                 if (last_cpptype && (Strcmp(last_cpptype,"struct") != 0)) {
		   Setattr((yyval.node),"access","private");
		   Swig_warning(WARN_PARSE_NO_ACCESS, Getfile((yyval.node)), Getline((yyval.node)), "No access specifier given for base class '%s' (ignored).\n", SwigType_namestr((yyvsp[-1].str)));
                 } else {
		   Setattr((yyval.node),"access","public");
		 }
		 if ((yyvsp[0].str))
		   SetFlag((yyval.node), "variadic");
               }
#line 11837 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 514:
#line 6784 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.intvalue) = cparse_line;
	       }
#line 11845 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 515:
#line 6786 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = NewHash();
		 Setfile((yyval.node),cparse_file);
		 Setline((yyval.node),(yyvsp[-3].intvalue));
		 Setattr((yyval.node),"name",(yyvsp[-1].str));
		 Setfile((yyvsp[-1].str),cparse_file);
		 Setline((yyvsp[-1].str),(yyvsp[-3].intvalue));
		 Setattr((yyval.node),"access",(yyvsp[-4].id));
	         if (Strcmp((yyvsp[-4].id),"public") != 0) {
		   Swig_warning(WARN_PARSE_PRIVATE_INHERIT, Getfile((yyval.node)), Getline((yyval.node)), "%s inheritance from base '%s' (ignored).\n", (yyvsp[-4].id), SwigType_namestr((yyvsp[-1].str)));
		 }
		 if ((yyvsp[0].str))
		   SetFlag((yyval.node), "variadic");
               }
#line 11864 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 516:
#line 6802 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (char*)"public"; }
#line 11870 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 517:
#line 6803 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (char*)"private"; }
#line 11876 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 518:
#line 6804 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (char*)"protected"; }
#line 11882 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 519:
#line 6807 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.id) = (char*)"class"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11891 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 520:
#line 6811 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.id) = (char *)"typename"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11900 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 521:
#line 6815 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.id) = (char *)"class..."; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11909 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 522:
#line 6819 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.id) = (char *)"typename..."; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11918 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 523:
#line 6825 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.id) = (yyvsp[0].id);
               }
#line 11926 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 524:
#line 6828 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.id) = (char*)"struct"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11935 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 525:
#line 6832 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.id) = (char*)"union"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11944 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 526:
#line 6838 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.id) = (char*)"class";
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11953 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 527:
#line 6842 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.id) = (char*)"struct";
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11962 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 528:
#line 6846 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.id) = (char*)"union";
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11971 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 529:
#line 6852 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.id) = (yyvsp[0].id);
               }
#line 11979 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 530:
#line 6855 "CParse/parser.y" /* yacc.c:1645  */
    {
		   (yyval.id) = 0;
               }
#line 11987 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 533:
#line 6864 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = 0;
	       }
#line 11995 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 534:
#line 6867 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = 0;
	       }
#line 12003 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 535:
#line 6870 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = 0;
	       }
#line 12011 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 536:
#line 6873 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = 0;
	       }
#line 12019 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 537:
#line 6878 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = 0;
               }
#line 12027 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 538:
#line 6881 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = 0;
               }
#line 12035 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 539:
#line 6886 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = (yyvsp[-1].pl);
                    (yyval.dtype).throwf = NewString("1");
                    (yyval.dtype).nexcept = 0;
	       }
#line 12045 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 540:
#line 6891 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = NewString("true");
	       }
#line 12055 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 541:
#line 6896 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = 0;
	       }
#line 12065 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 542:
#line 6901 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = (yyvsp[-2].pl);
                    (yyval.dtype).throwf = NewString("1");
                    (yyval.dtype).nexcept = 0;
	       }
#line 12075 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 543:
#line 6906 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = NewString("true");
	       }
#line 12085 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 544:
#line 6911 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = (yyvsp[-1].dtype).val;
	       }
#line 12095 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 545:
#line 6918 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).qualifier = (yyvsp[0].dtype).qualifier;
                    (yyval.dtype).refqualifier = (yyvsp[0].dtype).refqualifier;
               }
#line 12107 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 546:
#line 6925 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype) = (yyvsp[0].dtype);
                    (yyval.dtype).qualifier = 0;
                    (yyval.dtype).refqualifier = 0;
               }
#line 12117 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 547:
#line 6930 "CParse/parser.y" /* yacc.c:1645  */
    {
		    (yyval.dtype) = (yyvsp[0].dtype);
                    (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
                    (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
               }
#line 12127 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 548:
#line 6937 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.dtype) = (yyvsp[0].dtype);
               }
#line 12135 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 549:
#line 6940 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).qualifier = 0;
                    (yyval.dtype).refqualifier = 0;
               }
#line 12147 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 550:
#line 6949 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    Clear(scanner_ccode); 
                    (yyval.decl).have_parms = 0; 
                    (yyval.decl).defarg = 0; 
		    (yyval.decl).throws = (yyvsp[-2].dtype).throws;
		    (yyval.decl).throwf = (yyvsp[-2].dtype).throwf;
		    (yyval.decl).nexcept = (yyvsp[-2].dtype).nexcept;
                    if ((yyvsp[-2].dtype).qualifier)
                      Swig_error(cparse_file, cparse_line, "Constructor cannot have a qualifier.\n");
               }
#line 12162 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 551:
#line 6959 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    skip_balanced('{','}'); 
                    (yyval.decl).have_parms = 0; 
                    (yyval.decl).defarg = 0; 
                    (yyval.decl).throws = (yyvsp[-2].dtype).throws;
                    (yyval.decl).throwf = (yyvsp[-2].dtype).throwf;
                    (yyval.decl).nexcept = (yyvsp[-2].dtype).nexcept;
                    if ((yyvsp[-2].dtype).qualifier)
                      Swig_error(cparse_file, cparse_line, "Constructor cannot have a qualifier.\n");
               }
#line 12177 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 552:
#line 6969 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    Clear(scanner_ccode); 
                    (yyval.decl).parms = (yyvsp[-2].pl); 
                    (yyval.decl).have_parms = 1; 
                    (yyval.decl).defarg = 0; 
		    (yyval.decl).throws = 0;
		    (yyval.decl).throwf = 0;
		    (yyval.decl).nexcept = 0;
               }
#line 12191 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 553:
#line 6978 "CParse/parser.y" /* yacc.c:1645  */
    {
                    skip_balanced('{','}'); 
                    (yyval.decl).parms = (yyvsp[-2].pl); 
                    (yyval.decl).have_parms = 1; 
                    (yyval.decl).defarg = 0; 
                    (yyval.decl).throws = 0;
                    (yyval.decl).throwf = 0;
                    (yyval.decl).nexcept = 0;
               }
#line 12205 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 554:
#line 6987 "CParse/parser.y" /* yacc.c:1645  */
    { 
                    (yyval.decl).have_parms = 0; 
                    (yyval.decl).defarg = (yyvsp[-1].dtype).val; 
                    (yyval.decl).throws = 0;
                    (yyval.decl).throwf = 0;
                    (yyval.decl).nexcept = 0;
               }
#line 12217 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 555:
#line 6994 "CParse/parser.y" /* yacc.c:1645  */
    {
                    (yyval.decl).have_parms = 0;
                    (yyval.decl).defarg = (yyvsp[-1].dtype).val;
                    (yyval.decl).throws = (yyvsp[-3].dtype).throws;
                    (yyval.decl).throwf = (yyvsp[-3].dtype).throwf;
                    (yyval.decl).nexcept = (yyvsp[-3].dtype).nexcept;
                    if ((yyvsp[-3].dtype).qualifier)
                      Swig_error(cparse_file, cparse_line, "Constructor cannot have a qualifier.\n");
               }
#line 12231 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 562:
#line 7015 "CParse/parser.y" /* yacc.c:1645  */
    {
		  skip_balanced('(',')');
		  Clear(scanner_ccode);
		}
#line 12240 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 563:
#line 7027 "CParse/parser.y" /* yacc.c:1645  */
    {
		  skip_balanced('{','}');
		  Clear(scanner_ccode);
		}
#line 12249 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 564:
#line 7033 "CParse/parser.y" /* yacc.c:1645  */
    {
                     String *s = NewStringEmpty();
                     SwigType_add_template(s,(yyvsp[-1].p));
                     (yyval.id) = Char(s);
		     scanner_last_id(1);
                }
#line 12260 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 565:
#line 7042 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (yyvsp[0].id); }
#line 12266 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 566:
#line 7043 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = Swig_copy_string("override"); }
#line 12272 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 567:
#line 7044 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = Swig_copy_string("final"); }
#line 12278 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 568:
#line 7047 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (yyvsp[0].id); }
#line 12284 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 569:
#line 7048 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = Char((yyvsp[0].dtype).val); }
#line 12290 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 570:
#line 7049 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = Char((yyvsp[0].str)); }
#line 12296 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 571:
#line 7052 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = (yyvsp[0].id); }
#line 12302 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 572:
#line 7053 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.id) = 0; }
#line 12308 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 573:
#line 7056 "CParse/parser.y" /* yacc.c:1645  */
    { 
                  (yyval.str) = 0;
		  if (!(yyval.str)) (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str),(yyvsp[0].str));
      	          Delete((yyvsp[0].str));
               }
#line 12318 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 574:
#line 7061 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].str),(yyvsp[0].str));
                 Delete((yyvsp[0].str));
               }
#line 12327 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 575:
#line 7065 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewString((yyvsp[0].str));
   	       }
#line 12335 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 576:
#line 7068 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12343 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 577:
#line 7071 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.str) = NewStringf("%s", (yyvsp[0].str));
	       }
#line 12351 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 578:
#line 7074 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str), (yyvsp[0].id));
	       }
#line 12359 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 579:
#line 7077 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12367 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 580:
#line 7082 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].str),(yyvsp[0].str));
		   Delete((yyvsp[0].str));
               }
#line 12376 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 581:
#line 7086 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12384 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 582:
#line 7089 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12392 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 583:
#line 7096 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewStringf("::~%s",(yyvsp[0].str));
               }
#line 12400 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 584:
#line 7102 "CParse/parser.y" /* yacc.c:1645  */
    {
		(yyval.str) = NewStringf("%s", (yyvsp[0].id));
	      }
#line 12408 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 585:
#line 7105 "CParse/parser.y" /* yacc.c:1645  */
    {
		(yyval.str) = NewStringf("%s%s", (yyvsp[-1].id), (yyvsp[0].id));
	      }
#line 12416 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 586:
#line 7110 "CParse/parser.y" /* yacc.c:1645  */
    {
		(yyval.str) = (yyvsp[0].str);
	      }
#line 12424 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 587:
#line 7113 "CParse/parser.y" /* yacc.c:1645  */
    {
		(yyval.str) = NewStringf("%s%s", (yyvsp[-1].id), (yyvsp[0].id));
	      }
#line 12432 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 588:
#line 7119 "CParse/parser.y" /* yacc.c:1645  */
    {
                  (yyval.str) = 0;
		  if (!(yyval.str)) (yyval.str) = NewStringf("%s%s", (yyvsp[-1].id),(yyvsp[0].str));
      	          Delete((yyvsp[0].str));
               }
#line 12442 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 589:
#line 7124 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].id),(yyvsp[0].str));
                 Delete((yyvsp[0].str));
               }
#line 12451 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 590:
#line 7128 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewString((yyvsp[0].id));
   	       }
#line 12459 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 591:
#line 7131 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewStringf("::%s",(yyvsp[0].id));
               }
#line 12467 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 592:
#line 7134 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.str) = NewString((yyvsp[0].str));
	       }
#line 12475 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 593:
#line 7137 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12483 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 594:
#line 7142 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].id),(yyvsp[0].str));
		   Delete((yyvsp[0].str));
               }
#line 12492 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 595:
#line 7146 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].id));
               }
#line 12500 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 596:
#line 7149 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12508 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 597:
#line 7152 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = NewStringf("::~%s",(yyvsp[0].id));
               }
#line 12516 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 598:
#line 7158 "CParse/parser.y" /* yacc.c:1645  */
    { 
                   (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str), (yyvsp[0].id));
               }
#line 12524 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 599:
#line 7161 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = NewString((yyvsp[0].id));}
#line 12530 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 600:
#line 7164 "CParse/parser.y" /* yacc.c:1645  */
    {
                   (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str), (yyvsp[0].id));
               }
#line 12538 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 601:
#line 7172 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.str) = NewString((yyvsp[0].id));}
#line 12544 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 602:
#line 7175 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = (yyvsp[0].str);
               }
#line 12552 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 603:
#line 7178 "CParse/parser.y" /* yacc.c:1645  */
    {
                  skip_balanced('{','}');
		  (yyval.str) = NewString(scanner_ccode);
               }
#line 12561 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 604:
#line 7182 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = (yyvsp[0].str);
              }
#line 12569 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 605:
#line 7187 "CParse/parser.y" /* yacc.c:1645  */
    {
                  Hash *n;
                  (yyval.node) = NewHash();
                  n = (yyvsp[-1].node);
                  while(n) {
                     String *name, *value;
                     name = Getattr(n,"name");
                     value = Getattr(n,"value");
		     if (!value) value = (String *) "1";
                     Setattr((yyval.node),name, value);
		     n = nextSibling(n);
		  }
               }
#line 12587 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 606:
#line 7200 "CParse/parser.y" /* yacc.c:1645  */
    { (yyval.node) = 0; }
#line 12593 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 607:
#line 7204 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = NewHash();
		 Setattr((yyval.node),"name",(yyvsp[-2].id));
		 Setattr((yyval.node),"value",(yyvsp[0].str));
               }
#line 12603 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 608:
#line 7209 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.node) = NewHash();
		 Setattr((yyval.node),"name",(yyvsp[-4].id));
		 Setattr((yyval.node),"value",(yyvsp[-2].str));
		 set_nextSibling((yyval.node),(yyvsp[0].node));
               }
#line 12614 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 609:
#line 7215 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"name",(yyvsp[0].id));
	       }
#line 12623 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 610:
#line 7219 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"name",(yyvsp[-2].id));
                 set_nextSibling((yyval.node),(yyvsp[0].node));
               }
#line 12633 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 611:
#line 7224 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = (yyvsp[0].node);
		 Setattr((yyval.node),"name",(yyvsp[-2].id));
               }
#line 12642 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 612:
#line 7228 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.node) = (yyvsp[-2].node);
		 Setattr((yyval.node),"name",(yyvsp[-4].id));
		 set_nextSibling((yyval.node),(yyvsp[0].node));
               }
#line 12652 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 613:
#line 7235 "CParse/parser.y" /* yacc.c:1645  */
    {
		 (yyval.str) = (yyvsp[0].str);
               }
#line 12660 "CParse/parser.c" /* yacc.c:1645  */
    break;

  case 614:
#line 7238 "CParse/parser.y" /* yacc.c:1645  */
    {
                 (yyval.str) = Char((yyvsp[0].dtype).val);
               }
#line 12668 "CParse/parser.c" /* yacc.c:1645  */
    break;


#line 12672 "CParse/parser.c" /* yacc.c:1645  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 7245 "CParse/parser.y" /* yacc.c:1903  */


SwigType *Swig_cparse_type(String *s) {
   String *ns;
   ns = NewStringf("%s;",s);
   Seek(ns,0,SEEK_SET);
   scanner_file(ns);
   top = 0;
   scanner_next_token(PARSETYPE);
   yyparse();
   /*   Printf(stdout,"typeparse: '%s' ---> '%s'\n", s, top); */
   return top;
}


Parm *Swig_cparse_parm(String *s) {
   String *ns;
   ns = NewStringf("%s;",s);
   Seek(ns,0,SEEK_SET);
   scanner_file(ns);
   top = 0;
   scanner_next_token(PARSEPARM);
   yyparse();
   /*   Printf(stdout,"typeparse: '%s' ---> '%s'\n", s, top); */
   Delete(ns);
   return top;
}


ParmList *Swig_cparse_parms(String *s, Node *file_line_node) {
   String *ns;
   char *cs = Char(s);
   if (cs && cs[0] != '(') {
     ns = NewStringf("(%s);",s);
   } else {
     ns = NewStringf("%s;",s);
   }
   Setfile(ns, Getfile(file_line_node));
   Setline(ns, Getline(file_line_node));
   Seek(ns,0,SEEK_SET);
   scanner_file(ns);
   top = 0;
   scanner_next_token(PARSEPARMS);
   yyparse();
   /*   Printf(stdout,"typeparse: '%s' ---> '%s'\n", s, top); */
   return top;
}

