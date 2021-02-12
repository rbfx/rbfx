/* A Bison parser, made by GNU Bison 3.7.3.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.7.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 25 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"

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
    /* same for abstracts, which contains pointers to the source node children, and so will need to be patch too */
    if (strcmp(ckey,"abstracts") == 0) {
      SetFlag(nn, "needs_abstracts");
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

/* Recompute the "abstracts" attribute for the classes in instantiated templates, similarly to update_defaultargs() above. */
static void update_abstracts(Node *n) {
  for (; n; n = nextSibling(n)) {
    Node* const child = firstChild(n);
    if (!child)
      continue;

    update_abstracts(child);

    if (Getattr(n, "needs_abstracts")) {
      Setattr(n, "abstracts", pure_abstracts(child));
      Delattr(n, "needs_abstracts");
    }
  }
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


#line 1588 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
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

#include "parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ID = 3,                         /* ID  */
  YYSYMBOL_HBLOCK = 4,                     /* HBLOCK  */
  YYSYMBOL_POUND = 5,                      /* POUND  */
  YYSYMBOL_STRING = 6,                     /* STRING  */
  YYSYMBOL_WSTRING = 7,                    /* WSTRING  */
  YYSYMBOL_INCLUDE = 8,                    /* INCLUDE  */
  YYSYMBOL_IMPORT = 9,                     /* IMPORT  */
  YYSYMBOL_INSERT = 10,                    /* INSERT  */
  YYSYMBOL_CHARCONST = 11,                 /* CHARCONST  */
  YYSYMBOL_WCHARCONST = 12,                /* WCHARCONST  */
  YYSYMBOL_NUM_INT = 13,                   /* NUM_INT  */
  YYSYMBOL_NUM_FLOAT = 14,                 /* NUM_FLOAT  */
  YYSYMBOL_NUM_UNSIGNED = 15,              /* NUM_UNSIGNED  */
  YYSYMBOL_NUM_LONG = 16,                  /* NUM_LONG  */
  YYSYMBOL_NUM_ULONG = 17,                 /* NUM_ULONG  */
  YYSYMBOL_NUM_LONGLONG = 18,              /* NUM_LONGLONG  */
  YYSYMBOL_NUM_ULONGLONG = 19,             /* NUM_ULONGLONG  */
  YYSYMBOL_NUM_BOOL = 20,                  /* NUM_BOOL  */
  YYSYMBOL_TYPEDEF = 21,                   /* TYPEDEF  */
  YYSYMBOL_TYPE_INT = 22,                  /* TYPE_INT  */
  YYSYMBOL_TYPE_UNSIGNED = 23,             /* TYPE_UNSIGNED  */
  YYSYMBOL_TYPE_SHORT = 24,                /* TYPE_SHORT  */
  YYSYMBOL_TYPE_LONG = 25,                 /* TYPE_LONG  */
  YYSYMBOL_TYPE_FLOAT = 26,                /* TYPE_FLOAT  */
  YYSYMBOL_TYPE_DOUBLE = 27,               /* TYPE_DOUBLE  */
  YYSYMBOL_TYPE_CHAR = 28,                 /* TYPE_CHAR  */
  YYSYMBOL_TYPE_WCHAR = 29,                /* TYPE_WCHAR  */
  YYSYMBOL_TYPE_VOID = 30,                 /* TYPE_VOID  */
  YYSYMBOL_TYPE_SIGNED = 31,               /* TYPE_SIGNED  */
  YYSYMBOL_TYPE_BOOL = 32,                 /* TYPE_BOOL  */
  YYSYMBOL_TYPE_COMPLEX = 33,              /* TYPE_COMPLEX  */
  YYSYMBOL_TYPE_TYPEDEF = 34,              /* TYPE_TYPEDEF  */
  YYSYMBOL_TYPE_RAW = 35,                  /* TYPE_RAW  */
  YYSYMBOL_TYPE_NON_ISO_INT8 = 36,         /* TYPE_NON_ISO_INT8  */
  YYSYMBOL_TYPE_NON_ISO_INT16 = 37,        /* TYPE_NON_ISO_INT16  */
  YYSYMBOL_TYPE_NON_ISO_INT32 = 38,        /* TYPE_NON_ISO_INT32  */
  YYSYMBOL_TYPE_NON_ISO_INT64 = 39,        /* TYPE_NON_ISO_INT64  */
  YYSYMBOL_LPAREN = 40,                    /* LPAREN  */
  YYSYMBOL_RPAREN = 41,                    /* RPAREN  */
  YYSYMBOL_COMMA = 42,                     /* COMMA  */
  YYSYMBOL_SEMI = 43,                      /* SEMI  */
  YYSYMBOL_EXTERN = 44,                    /* EXTERN  */
  YYSYMBOL_INIT = 45,                      /* INIT  */
  YYSYMBOL_LBRACE = 46,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 47,                    /* RBRACE  */
  YYSYMBOL_PERIOD = 48,                    /* PERIOD  */
  YYSYMBOL_CONST_QUAL = 49,                /* CONST_QUAL  */
  YYSYMBOL_VOLATILE = 50,                  /* VOLATILE  */
  YYSYMBOL_REGISTER = 51,                  /* REGISTER  */
  YYSYMBOL_STRUCT = 52,                    /* STRUCT  */
  YYSYMBOL_UNION = 53,                     /* UNION  */
  YYSYMBOL_EQUAL = 54,                     /* EQUAL  */
  YYSYMBOL_SIZEOF = 55,                    /* SIZEOF  */
  YYSYMBOL_MODULE = 56,                    /* MODULE  */
  YYSYMBOL_LBRACKET = 57,                  /* LBRACKET  */
  YYSYMBOL_RBRACKET = 58,                  /* RBRACKET  */
  YYSYMBOL_BEGINFILE = 59,                 /* BEGINFILE  */
  YYSYMBOL_ENDOFFILE = 60,                 /* ENDOFFILE  */
  YYSYMBOL_ILLEGAL = 61,                   /* ILLEGAL  */
  YYSYMBOL_CONSTANT = 62,                  /* CONSTANT  */
  YYSYMBOL_NAME = 63,                      /* NAME  */
  YYSYMBOL_RENAME = 64,                    /* RENAME  */
  YYSYMBOL_NAMEWARN = 65,                  /* NAMEWARN  */
  YYSYMBOL_EXTEND = 66,                    /* EXTEND  */
  YYSYMBOL_PRAGMA = 67,                    /* PRAGMA  */
  YYSYMBOL_FEATURE = 68,                   /* FEATURE  */
  YYSYMBOL_VARARGS = 69,                   /* VARARGS  */
  YYSYMBOL_ENUM = 70,                      /* ENUM  */
  YYSYMBOL_CLASS = 71,                     /* CLASS  */
  YYSYMBOL_TYPENAME = 72,                  /* TYPENAME  */
  YYSYMBOL_PRIVATE = 73,                   /* PRIVATE  */
  YYSYMBOL_PUBLIC = 74,                    /* PUBLIC  */
  YYSYMBOL_PROTECTED = 75,                 /* PROTECTED  */
  YYSYMBOL_COLON = 76,                     /* COLON  */
  YYSYMBOL_STATIC = 77,                    /* STATIC  */
  YYSYMBOL_VIRTUAL = 78,                   /* VIRTUAL  */
  YYSYMBOL_FRIEND = 79,                    /* FRIEND  */
  YYSYMBOL_THROW = 80,                     /* THROW  */
  YYSYMBOL_CATCH = 81,                     /* CATCH  */
  YYSYMBOL_EXPLICIT = 82,                  /* EXPLICIT  */
  YYSYMBOL_STATIC_ASSERT = 83,             /* STATIC_ASSERT  */
  YYSYMBOL_CONSTEXPR = 84,                 /* CONSTEXPR  */
  YYSYMBOL_THREAD_LOCAL = 85,              /* THREAD_LOCAL  */
  YYSYMBOL_DECLTYPE = 86,                  /* DECLTYPE  */
  YYSYMBOL_AUTO = 87,                      /* AUTO  */
  YYSYMBOL_NOEXCEPT = 88,                  /* NOEXCEPT  */
  YYSYMBOL_OVERRIDE = 89,                  /* OVERRIDE  */
  YYSYMBOL_FINAL = 90,                     /* FINAL  */
  YYSYMBOL_USING = 91,                     /* USING  */
  YYSYMBOL_NAMESPACE = 92,                 /* NAMESPACE  */
  YYSYMBOL_NATIVE = 93,                    /* NATIVE  */
  YYSYMBOL_INLINE = 94,                    /* INLINE  */
  YYSYMBOL_TYPEMAP = 95,                   /* TYPEMAP  */
  YYSYMBOL_EXCEPT = 96,                    /* EXCEPT  */
  YYSYMBOL_ECHO = 97,                      /* ECHO  */
  YYSYMBOL_APPLY = 98,                     /* APPLY  */
  YYSYMBOL_CLEAR = 99,                     /* CLEAR  */
  YYSYMBOL_SWIGTEMPLATE = 100,             /* SWIGTEMPLATE  */
  YYSYMBOL_FRAGMENT = 101,                 /* FRAGMENT  */
  YYSYMBOL_WARN = 102,                     /* WARN  */
  YYSYMBOL_LESSTHAN = 103,                 /* LESSTHAN  */
  YYSYMBOL_GREATERTHAN = 104,              /* GREATERTHAN  */
  YYSYMBOL_DELETE_KW = 105,                /* DELETE_KW  */
  YYSYMBOL_DEFAULT = 106,                  /* DEFAULT  */
  YYSYMBOL_LESSTHANOREQUALTO = 107,        /* LESSTHANOREQUALTO  */
  YYSYMBOL_GREATERTHANOREQUALTO = 108,     /* GREATERTHANOREQUALTO  */
  YYSYMBOL_EQUALTO = 109,                  /* EQUALTO  */
  YYSYMBOL_NOTEQUALTO = 110,               /* NOTEQUALTO  */
  YYSYMBOL_ARROW = 111,                    /* ARROW  */
  YYSYMBOL_QUESTIONMARK = 112,             /* QUESTIONMARK  */
  YYSYMBOL_TYPES = 113,                    /* TYPES  */
  YYSYMBOL_PARMS = 114,                    /* PARMS  */
  YYSYMBOL_NONID = 115,                    /* NONID  */
  YYSYMBOL_DSTAR = 116,                    /* DSTAR  */
  YYSYMBOL_DCNOT = 117,                    /* DCNOT  */
  YYSYMBOL_TEMPLATE = 118,                 /* TEMPLATE  */
  YYSYMBOL_OPERATOR = 119,                 /* OPERATOR  */
  YYSYMBOL_CONVERSIONOPERATOR = 120,       /* CONVERSIONOPERATOR  */
  YYSYMBOL_PARSETYPE = 121,                /* PARSETYPE  */
  YYSYMBOL_PARSEPARM = 122,                /* PARSEPARM  */
  YYSYMBOL_PARSEPARMS = 123,               /* PARSEPARMS  */
  YYSYMBOL_DOXYGENSTRING = 124,            /* DOXYGENSTRING  */
  YYSYMBOL_DOXYGENPOSTSTRING = 125,        /* DOXYGENPOSTSTRING  */
  YYSYMBOL_CAST = 126,                     /* CAST  */
  YYSYMBOL_LOR = 127,                      /* LOR  */
  YYSYMBOL_LAND = 128,                     /* LAND  */
  YYSYMBOL_OR = 129,                       /* OR  */
  YYSYMBOL_XOR = 130,                      /* XOR  */
  YYSYMBOL_AND = 131,                      /* AND  */
  YYSYMBOL_LSHIFT = 132,                   /* LSHIFT  */
  YYSYMBOL_RSHIFT = 133,                   /* RSHIFT  */
  YYSYMBOL_PLUS = 134,                     /* PLUS  */
  YYSYMBOL_MINUS = 135,                    /* MINUS  */
  YYSYMBOL_STAR = 136,                     /* STAR  */
  YYSYMBOL_SLASH = 137,                    /* SLASH  */
  YYSYMBOL_MODULO = 138,                   /* MODULO  */
  YYSYMBOL_UMINUS = 139,                   /* UMINUS  */
  YYSYMBOL_NOT = 140,                      /* NOT  */
  YYSYMBOL_LNOT = 141,                     /* LNOT  */
  YYSYMBOL_DCOLON = 142,                   /* DCOLON  */
  YYSYMBOL_YYACCEPT = 143,                 /* $accept  */
  YYSYMBOL_program = 144,                  /* program  */
  YYSYMBOL_interface = 145,                /* interface  */
  YYSYMBOL_declaration = 146,              /* declaration  */
  YYSYMBOL_swig_directive = 147,           /* swig_directive  */
  YYSYMBOL_extend_directive = 148,         /* extend_directive  */
  YYSYMBOL_149_1 = 149,                    /* $@1  */
  YYSYMBOL_apply_directive = 150,          /* apply_directive  */
  YYSYMBOL_clear_directive = 151,          /* clear_directive  */
  YYSYMBOL_constant_directive = 152,       /* constant_directive  */
  YYSYMBOL_echo_directive = 153,           /* echo_directive  */
  YYSYMBOL_except_directive = 154,         /* except_directive  */
  YYSYMBOL_stringtype = 155,               /* stringtype  */
  YYSYMBOL_fname = 156,                    /* fname  */
  YYSYMBOL_fragment_directive = 157,       /* fragment_directive  */
  YYSYMBOL_include_directive = 158,        /* include_directive  */
  YYSYMBOL_159_2 = 159,                    /* $@2  */
  YYSYMBOL_includetype = 160,              /* includetype  */
  YYSYMBOL_inline_directive = 161,         /* inline_directive  */
  YYSYMBOL_insert_directive = 162,         /* insert_directive  */
  YYSYMBOL_module_directive = 163,         /* module_directive  */
  YYSYMBOL_name_directive = 164,           /* name_directive  */
  YYSYMBOL_native_directive = 165,         /* native_directive  */
  YYSYMBOL_pragma_directive = 166,         /* pragma_directive  */
  YYSYMBOL_pragma_arg = 167,               /* pragma_arg  */
  YYSYMBOL_pragma_lang = 168,              /* pragma_lang  */
  YYSYMBOL_rename_directive = 169,         /* rename_directive  */
  YYSYMBOL_rename_namewarn = 170,          /* rename_namewarn  */
  YYSYMBOL_feature_directive = 171,        /* feature_directive  */
  YYSYMBOL_stringbracesemi = 172,          /* stringbracesemi  */
  YYSYMBOL_featattr = 173,                 /* featattr  */
  YYSYMBOL_varargs_directive = 174,        /* varargs_directive  */
  YYSYMBOL_varargs_parms = 175,            /* varargs_parms  */
  YYSYMBOL_typemap_directive = 176,        /* typemap_directive  */
  YYSYMBOL_typemap_type = 177,             /* typemap_type  */
  YYSYMBOL_tm_list = 178,                  /* tm_list  */
  YYSYMBOL_tm_tail = 179,                  /* tm_tail  */
  YYSYMBOL_typemap_parm = 180,             /* typemap_parm  */
  YYSYMBOL_types_directive = 181,          /* types_directive  */
  YYSYMBOL_template_directive = 182,       /* template_directive  */
  YYSYMBOL_warn_directive = 183,           /* warn_directive  */
  YYSYMBOL_c_declaration = 184,            /* c_declaration  */
  YYSYMBOL_185_3 = 185,                    /* $@3  */
  YYSYMBOL_c_decl = 186,                   /* c_decl  */
  YYSYMBOL_c_decl_tail = 187,              /* c_decl_tail  */
  YYSYMBOL_initializer = 188,              /* initializer  */
  YYSYMBOL_cpp_alternate_rettype = 189,    /* cpp_alternate_rettype  */
  YYSYMBOL_cpp_lambda_decl = 190,          /* cpp_lambda_decl  */
  YYSYMBOL_lambda_introducer = 191,        /* lambda_introducer  */
  YYSYMBOL_lambda_body = 192,              /* lambda_body  */
  YYSYMBOL_lambda_tail = 193,              /* lambda_tail  */
  YYSYMBOL_194_4 = 194,                    /* $@4  */
  YYSYMBOL_c_enum_key = 195,               /* c_enum_key  */
  YYSYMBOL_c_enum_inherit = 196,           /* c_enum_inherit  */
  YYSYMBOL_c_enum_forward_decl = 197,      /* c_enum_forward_decl  */
  YYSYMBOL_c_enum_decl = 198,              /* c_enum_decl  */
  YYSYMBOL_c_constructor_decl = 199,       /* c_constructor_decl  */
  YYSYMBOL_cpp_declaration = 200,          /* cpp_declaration  */
  YYSYMBOL_cpp_class_decl = 201,           /* cpp_class_decl  */
  YYSYMBOL_202_5 = 202,                    /* @5  */
  YYSYMBOL_203_6 = 203,                    /* @6  */
  YYSYMBOL_cpp_opt_declarators = 204,      /* cpp_opt_declarators  */
  YYSYMBOL_cpp_forward_class_decl = 205,   /* cpp_forward_class_decl  */
  YYSYMBOL_cpp_template_decl = 206,        /* cpp_template_decl  */
  YYSYMBOL_207_7 = 207,                    /* $@7  */
  YYSYMBOL_cpp_template_possible = 208,    /* cpp_template_possible  */
  YYSYMBOL_template_parms = 209,           /* template_parms  */
  YYSYMBOL_templateparameters = 210,       /* templateparameters  */
  YYSYMBOL_templateparameter = 211,        /* templateparameter  */
  YYSYMBOL_templateparameterstail = 212,   /* templateparameterstail  */
  YYSYMBOL_cpp_using_decl = 213,           /* cpp_using_decl  */
  YYSYMBOL_cpp_namespace_decl = 214,       /* cpp_namespace_decl  */
  YYSYMBOL_215_8 = 215,                    /* @8  */
  YYSYMBOL_216_9 = 216,                    /* $@9  */
  YYSYMBOL_cpp_members = 217,              /* cpp_members  */
  YYSYMBOL_218_10 = 218,                   /* $@10  */
  YYSYMBOL_219_11 = 219,                   /* $@11  */
  YYSYMBOL_220_12 = 220,                   /* $@12  */
  YYSYMBOL_cpp_member_no_dox = 221,        /* cpp_member_no_dox  */
  YYSYMBOL_cpp_member = 222,               /* cpp_member  */
  YYSYMBOL_cpp_constructor_decl = 223,     /* cpp_constructor_decl  */
  YYSYMBOL_cpp_destructor_decl = 224,      /* cpp_destructor_decl  */
  YYSYMBOL_cpp_conversion_operator = 225,  /* cpp_conversion_operator  */
  YYSYMBOL_cpp_catch_decl = 226,           /* cpp_catch_decl  */
  YYSYMBOL_cpp_static_assert = 227,        /* cpp_static_assert  */
  YYSYMBOL_cpp_protection_decl = 228,      /* cpp_protection_decl  */
  YYSYMBOL_cpp_swig_directive = 229,       /* cpp_swig_directive  */
  YYSYMBOL_cpp_end = 230,                  /* cpp_end  */
  YYSYMBOL_cpp_vend = 231,                 /* cpp_vend  */
  YYSYMBOL_anonymous_bitfield = 232,       /* anonymous_bitfield  */
  YYSYMBOL_anon_bitfield_type = 233,       /* anon_bitfield_type  */
  YYSYMBOL_extern_string = 234,            /* extern_string  */
  YYSYMBOL_storage_class = 235,            /* storage_class  */
  YYSYMBOL_parms = 236,                    /* parms  */
  YYSYMBOL_rawparms = 237,                 /* rawparms  */
  YYSYMBOL_ptail = 238,                    /* ptail  */
  YYSYMBOL_parm_no_dox = 239,              /* parm_no_dox  */
  YYSYMBOL_parm = 240,                     /* parm  */
  YYSYMBOL_valparms = 241,                 /* valparms  */
  YYSYMBOL_rawvalparms = 242,              /* rawvalparms  */
  YYSYMBOL_valptail = 243,                 /* valptail  */
  YYSYMBOL_valparm = 244,                  /* valparm  */
  YYSYMBOL_def_args = 245,                 /* def_args  */
  YYSYMBOL_parameter_declarator = 246,     /* parameter_declarator  */
  YYSYMBOL_plain_declarator = 247,         /* plain_declarator  */
  YYSYMBOL_declarator = 248,               /* declarator  */
  YYSYMBOL_notso_direct_declarator = 249,  /* notso_direct_declarator  */
  YYSYMBOL_direct_declarator = 250,        /* direct_declarator  */
  YYSYMBOL_abstract_declarator = 251,      /* abstract_declarator  */
  YYSYMBOL_direct_abstract_declarator = 252, /* direct_abstract_declarator  */
  YYSYMBOL_pointer = 253,                  /* pointer  */
  YYSYMBOL_cv_ref_qualifier = 254,         /* cv_ref_qualifier  */
  YYSYMBOL_ref_qualifier = 255,            /* ref_qualifier  */
  YYSYMBOL_type_qualifier = 256,           /* type_qualifier  */
  YYSYMBOL_type_qualifier_raw = 257,       /* type_qualifier_raw  */
  YYSYMBOL_type = 258,                     /* type  */
  YYSYMBOL_rawtype = 259,                  /* rawtype  */
  YYSYMBOL_type_right = 260,               /* type_right  */
  YYSYMBOL_decltype = 261,                 /* decltype  */
  YYSYMBOL_primitive_type = 262,           /* primitive_type  */
  YYSYMBOL_primitive_type_list = 263,      /* primitive_type_list  */
  YYSYMBOL_type_specifier = 264,           /* type_specifier  */
  YYSYMBOL_definetype = 265,               /* definetype  */
  YYSYMBOL_266_13 = 266,                   /* $@13  */
  YYSYMBOL_default_delete = 267,           /* default_delete  */
  YYSYMBOL_deleted_definition = 268,       /* deleted_definition  */
  YYSYMBOL_explicit_default = 269,         /* explicit_default  */
  YYSYMBOL_ename = 270,                    /* ename  */
  YYSYMBOL_constant_directives = 271,      /* constant_directives  */
  YYSYMBOL_optional_ignored_defines = 272, /* optional_ignored_defines  */
  YYSYMBOL_enumlist = 273,                 /* enumlist  */
  YYSYMBOL_enumlist_item = 274,            /* enumlist_item  */
  YYSYMBOL_edecl_with_dox = 275,           /* edecl_with_dox  */
  YYSYMBOL_edecl = 276,                    /* edecl  */
  YYSYMBOL_etype = 277,                    /* etype  */
  YYSYMBOL_expr = 278,                     /* expr  */
  YYSYMBOL_exprmem = 279,                  /* exprmem  */
  YYSYMBOL_valexpr = 280,                  /* valexpr  */
  YYSYMBOL_exprnum = 281,                  /* exprnum  */
  YYSYMBOL_exprcompound = 282,             /* exprcompound  */
  YYSYMBOL_ellipsis = 283,                 /* ellipsis  */
  YYSYMBOL_variadic = 284,                 /* variadic  */
  YYSYMBOL_inherit = 285,                  /* inherit  */
  YYSYMBOL_raw_inherit = 286,              /* raw_inherit  */
  YYSYMBOL_287_14 = 287,                   /* $@14  */
  YYSYMBOL_base_list = 288,                /* base_list  */
  YYSYMBOL_base_specifier = 289,           /* base_specifier  */
  YYSYMBOL_290_15 = 290,                   /* @15  */
  YYSYMBOL_291_16 = 291,                   /* @16  */
  YYSYMBOL_access_specifier = 292,         /* access_specifier  */
  YYSYMBOL_templcpptype = 293,             /* templcpptype  */
  YYSYMBOL_cpptype = 294,                  /* cpptype  */
  YYSYMBOL_classkey = 295,                 /* classkey  */
  YYSYMBOL_classkeyopt = 296,              /* classkeyopt  */
  YYSYMBOL_opt_virtual = 297,              /* opt_virtual  */
  YYSYMBOL_virt_specifier_seq = 298,       /* virt_specifier_seq  */
  YYSYMBOL_virt_specifier_seq_opt = 299,   /* virt_specifier_seq_opt  */
  YYSYMBOL_exception_specification = 300,  /* exception_specification  */
  YYSYMBOL_qualifiers_exception_specification = 301, /* qualifiers_exception_specification  */
  YYSYMBOL_cpp_const = 302,                /* cpp_const  */
  YYSYMBOL_ctor_end = 303,                 /* ctor_end  */
  YYSYMBOL_ctor_initializer = 304,         /* ctor_initializer  */
  YYSYMBOL_mem_initializer_list = 305,     /* mem_initializer_list  */
  YYSYMBOL_mem_initializer = 306,          /* mem_initializer  */
  YYSYMBOL_less_valparms_greater = 307,    /* less_valparms_greater  */
  YYSYMBOL_identifier = 308,               /* identifier  */
  YYSYMBOL_idstring = 309,                 /* idstring  */
  YYSYMBOL_idstringopt = 310,              /* idstringopt  */
  YYSYMBOL_idcolon = 311,                  /* idcolon  */
  YYSYMBOL_idcolontail = 312,              /* idcolontail  */
  YYSYMBOL_idtemplate = 313,               /* idtemplate  */
  YYSYMBOL_idtemplatetemplate = 314,       /* idtemplatetemplate  */
  YYSYMBOL_idcolonnt = 315,                /* idcolonnt  */
  YYSYMBOL_idcolontailnt = 316,            /* idcolontailnt  */
  YYSYMBOL_string = 317,                   /* string  */
  YYSYMBOL_wstring = 318,                  /* wstring  */
  YYSYMBOL_stringbrace = 319,              /* stringbrace  */
  YYSYMBOL_options = 320,                  /* options  */
  YYSYMBOL_options_ex = 321,               /* options_ex  */
  YYSYMBOL_kwargs = 322,                   /* kwargs  */
  YYSYMBOL_stringnum = 323,                /* stringnum  */
  YYSYMBOL_empty = 324                     /* empty  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

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


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
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

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
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
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
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
#define YYLAST   5736

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  143
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  182
/* YYNRULES -- Number of rules.  */
#define YYNRULES  613
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1195

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   397


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
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
static const yytype_int16 yyrline[] =
{
       0,  1713,  1713,  1725,  1729,  1732,  1735,  1738,  1741,  1746,
    1755,  1759,  1766,  1771,  1772,  1773,  1774,  1775,  1785,  1801,
    1811,  1812,  1813,  1814,  1815,  1816,  1817,  1818,  1819,  1820,
    1821,  1822,  1823,  1824,  1825,  1826,  1827,  1828,  1829,  1830,
    1831,  1838,  1838,  1920,  1930,  1941,  1961,  1985,  2009,  2020,
    2029,  2048,  2054,  2060,  2065,  2072,  2079,  2083,  2096,  2105,
    2120,  2133,  2133,  2189,  2190,  2197,  2216,  2247,  2251,  2262,
    2268,  2287,  2330,  2336,  2349,  2355,  2381,  2387,  2394,  2395,
    2398,  2399,  2406,  2452,  2498,  2509,  2512,  2539,  2545,  2551,
    2557,  2565,  2571,  2577,  2583,  2591,  2592,  2593,  2596,  2601,
    2611,  2647,  2648,  2683,  2700,  2708,  2721,  2746,  2752,  2756,
    2759,  2770,  2775,  2788,  2800,  3098,  3108,  3115,  3116,  3120,
    3120,  3145,  3151,  3161,  3173,  3182,  3262,  3325,  3329,  3354,
    3358,  3369,  3374,  3375,  3376,  3380,  3381,  3382,  3393,  3398,
    3403,  3410,  3416,  3421,  3424,  3424,  3437,  3440,  3443,  3452,
    3455,  3462,  3484,  3513,  3611,  3664,  3665,  3666,  3667,  3668,
    3669,  3674,  3674,  3923,  3923,  4070,  4071,  4083,  4101,  4101,
    4362,  4368,  4374,  4377,  4380,  4383,  4386,  4389,  4394,  4430,
    4434,  4437,  4440,  4445,  4449,  4454,  4464,  4495,  4495,  4553,
    4553,  4575,  4602,  4619,  4624,  4619,  4632,  4633,  4634,  4634,
    4650,  4651,  4668,  4669,  4670,  4671,  4672,  4673,  4674,  4675,
    4676,  4677,  4678,  4679,  4680,  4681,  4682,  4683,  4685,  4688,
    4692,  4704,  4733,  4763,  4796,  4812,  4830,  4849,  4869,  4889,
    4897,  4904,  4911,  4919,  4927,  4930,  4934,  4937,  4938,  4939,
    4940,  4941,  4942,  4943,  4944,  4947,  4958,  4969,  4982,  4993,
    5004,  5018,  5021,  5024,  5025,  5029,  5031,  5039,  5051,  5052,
    5053,  5054,  5055,  5056,  5057,  5058,  5059,  5060,  5061,  5062,
    5063,  5064,  5065,  5066,  5067,  5068,  5069,  5070,  5077,  5088,
    5092,  5099,  5103,  5108,  5112,  5124,  5134,  5144,  5147,  5151,
    5157,  5170,  5174,  5177,  5181,  5185,  5213,  5221,  5234,  5250,
    5261,  5271,  5283,  5287,  5291,  5298,  5320,  5337,  5356,  5375,
    5382,  5390,  5399,  5408,  5412,  5421,  5432,  5443,  5455,  5465,
    5479,  5487,  5496,  5505,  5509,  5518,  5529,  5540,  5552,  5562,
    5572,  5583,  5596,  5603,  5611,  5627,  5635,  5646,  5657,  5668,
    5687,  5695,  5712,  5720,  5727,  5734,  5745,  5757,  5768,  5780,
    5791,  5802,  5822,  5843,  5849,  5855,  5862,  5869,  5878,  5887,
    5890,  5899,  5908,  5915,  5922,  5929,  5937,  5947,  5958,  5969,
    5980,  5987,  5994,  5997,  6014,  6032,  6042,  6049,  6055,  6060,
    6067,  6071,  6076,  6083,  6087,  6093,  6097,  6103,  6104,  6105,
    6111,  6117,  6121,  6122,  6126,  6133,  6136,  6137,  6141,  6142,
    6144,  6147,  6150,  6155,  6166,  6191,  6194,  6248,  6252,  6256,
    6260,  6264,  6268,  6272,  6276,  6280,  6284,  6288,  6292,  6296,
    6300,  6306,  6306,  6322,  6327,  6330,  6336,  6351,  6367,  6368,
    6371,  6372,  6376,  6377,  6381,  6385,  6390,  6398,  6407,  6412,
    6417,  6420,  6426,  6434,  6446,  6461,  6462,  6482,  6486,  6496,
    6502,  6505,  6508,  6512,  6517,  6522,  6523,  6528,  6542,  6558,
    6568,  6586,  6593,  6600,  6607,  6615,  6623,  6627,  6631,  6637,
    6638,  6639,  6640,  6641,  6642,  6643,  6644,  6647,  6651,  6655,
    6659,  6663,  6667,  6671,  6675,  6679,  6683,  6687,  6691,  6695,
    6699,  6713,  6717,  6721,  6727,  6731,  6735,  6739,  6743,  6759,
    6764,  6767,  6772,  6777,  6777,  6778,  6781,  6798,  6807,  6807,
    6825,  6825,  6843,  6844,  6845,  6848,  6852,  6856,  6860,  6866,
    6869,  6873,  6879,  6883,  6887,  6893,  6896,  6901,  6902,  6905,
    6908,  6911,  6914,  6919,  6922,  6927,  6933,  6939,  6945,  6951,
    6957,  6965,  6973,  6978,  6985,  6988,  6998,  7009,  7020,  7030,
    7040,  7048,  7060,  7061,  7064,  7065,  7066,  7067,  7070,  7082,
    7088,  7097,  7098,  7099,  7102,  7103,  7104,  7107,  7108,  7111,
    7116,  7120,  7123,  7126,  7129,  7132,  7137,  7141,  7144,  7151,
    7157,  7160,  7165,  7168,  7174,  7179,  7183,  7186,  7189,  7192,
    7197,  7201,  7204,  7207,  7213,  7216,  7219,  7227,  7230,  7233,
    7237,  7242,  7255,  7257,  7270,  7274,  7279,  7285,  7289,  7294,
    7298,  7305,  7308,  7313
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ID", "HBLOCK",
  "POUND", "STRING", "WSTRING", "INCLUDE", "IMPORT", "INSERT", "CHARCONST",
  "WCHARCONST", "NUM_INT", "NUM_FLOAT", "NUM_UNSIGNED", "NUM_LONG",
  "NUM_ULONG", "NUM_LONGLONG", "NUM_ULONGLONG", "NUM_BOOL", "TYPEDEF",
  "TYPE_INT", "TYPE_UNSIGNED", "TYPE_SHORT", "TYPE_LONG", "TYPE_FLOAT",
  "TYPE_DOUBLE", "TYPE_CHAR", "TYPE_WCHAR", "TYPE_VOID", "TYPE_SIGNED",
  "TYPE_BOOL", "TYPE_COMPLEX", "TYPE_TYPEDEF", "TYPE_RAW",
  "TYPE_NON_ISO_INT8", "TYPE_NON_ISO_INT16", "TYPE_NON_ISO_INT32",
  "TYPE_NON_ISO_INT64", "LPAREN", "RPAREN", "COMMA", "SEMI", "EXTERN",
  "INIT", "LBRACE", "RBRACE", "PERIOD", "CONST_QUAL", "VOLATILE",
  "REGISTER", "STRUCT", "UNION", "EQUAL", "SIZEOF", "MODULE", "LBRACKET",
  "RBRACKET", "BEGINFILE", "ENDOFFILE", "ILLEGAL", "CONSTANT", "NAME",
  "RENAME", "NAMEWARN", "EXTEND", "PRAGMA", "FEATURE", "VARARGS", "ENUM",
  "CLASS", "TYPENAME", "PRIVATE", "PUBLIC", "PROTECTED", "COLON", "STATIC",
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
  "constant_directives", "optional_ignored_defines", "enumlist",
  "enumlist_item", "edecl_with_dox", "edecl", "etype", "expr", "exprmem",
  "valexpr", "exprnum", "exprcompound", "ellipsis", "variadic", "inherit",
  "raw_inherit", "$@14", "base_list", "base_specifier", "@15", "@16",
  "access_specifier", "templcpptype", "cpptype", "classkey", "classkeyopt",
  "opt_virtual", "virt_specifier_seq", "virt_specifier_seq_opt",
  "exception_specification", "qualifiers_exception_specification",
  "cpp_const", "ctor_end", "ctor_initializer", "mem_initializer_list",
  "mem_initializer", "less_valparms_greater", "identifier", "idstring",
  "idstringopt", "idcolon", "idcolontail", "idtemplate",
  "idtemplatetemplate", "idcolonnt", "idcolontailnt", "string", "wstring",
  "stringbrace", "options", "options_ex", "kwargs", "stringnum", "empty", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
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
#endif

#define YYPACT_NINF (-1036)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-614)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     399,  4602,  4705,   250,    89,  4046, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036, -1036, -1036, -1036,   127, -1036, -1036, -1036,
   -1036, -1036,   292,   153,   159,    72, -1036, -1036,   111,   189,
     243,  5334,   488,   137,   322,  5617,   619,  1265,   619, -1036,
   -1036, -1036,  2765, -1036,   488,   243, -1036,   163, -1036,   385,
     403,  5015, -1036,   344, -1036, -1036, -1036,   316, -1036, -1036,
      44,   444,  5118,   460, -1036, -1036,   444,   467,   496,   501,
      12, -1036, -1036,   515,   476,   531,   366,    39,   240,   412,
     560,   175,   570,   587,   602,  5405,  5405,   576,   578,   616,
     625,    15, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036, -1036,   444, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036,  1700, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036,    35,  5476, -1036,   608, -1036, -1036,   624,
     631,   488,   337,   592,  2272, -1036, -1036, -1036, -1036, -1036,
     619, -1036,  3640,   633,   220,  2408,  3224,    46,   110,   439,
      50,   488, -1036, -1036,   231,   237,   231,   317,  1857,   568,
   -1036, -1036, -1036, -1036, -1036,   245,   364, -1036, -1036, -1036,
     647, -1036,   650, -1036, -1036,   408, -1036, -1036,   592,   118,
     408,   408, -1036,   657,  3400, -1036,   161,  1003,   353,   245,
     245, -1036,   408,  4912, -1036, -1036,  5015, -1036, -1036, -1036,
   -1036, -1036, -1036,   488,   336, -1036,   172,   656,   245, -1036,
   -1036,   408,   245, -1036, -1036, -1036,   699,  5015,   662,  1471,
     667,   672,   408,   616,   699,  5015,  5015,   488,   616,  2244,
     571,  1039,   408,   321,  1387,   603, -1036, -1036,  3400,   488,
    3414,   558, -1036,   680,   682,   697,   245, -1036, -1036,   163,
     651,   669, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036,  3224,   432,  3224,  3224,  3224,  3224,  3224,
    3224,  3224, -1036,   654, -1036,   681,   754,  1933,  2918,    59,
      61, -1036, -1036,   699,   795, -1036, -1036,  3757,   806,   806,
     769,   770,    54,   701,   768, -1036, -1036, -1036,   762,  3224,
   -1036, -1036, -1036, -1036,  3821, -1036,  2918,   787,  3757,   781,
     488,   383,   317, -1036,   782,   383,   317, -1036,   702, -1036,
   -1036,  5015,  2544, -1036,  5015,  2680,   799,   886,  1015,   383,
     317,   725,   184, -1036, -1036,   163,   807,  4808, -1036, -1036,
   -1036, -1036,   814,   699,   488, -1036, -1036,   424,   808, -1036,
   -1036,   979,   231,   452,   480, -1036,   817, -1036, -1036, -1036,
   -1036,   488, -1036,   823,   811,   596,   826,   829, -1036,   831,
     832, -1036,  5547, -1036,   488, -1036,   836,   838, -1036,   843,
     847,  5405, -1036, -1036,   486, -1036, -1036, -1036,  5405, -1036,
   -1036, -1036,   849, -1036, -1036,   675,   254,   851,   789, -1036,
     852, -1036,    62, -1036, -1036,   104,   249,   249,   249,   709,
     784,   861,   380,   863,  5015,  1143,  1211,   788,  1916,  1214,
      74,   834,   299, -1036,  3874,  1214, -1036,   865, -1036,   255,
   -1036, -1036, -1036, -1036,   243, -1036,   592,   910,  2511,  5547,
     866,  3606,  2518, -1036, -1036, -1036, -1036, -1036, -1036,  2272,
   -1036, -1036, -1036,  3224,  3224,  3224,  3224,  3224,  3224,  3224,
    3224,  3224,  3224,  3224,  3224,  3224,  3224,  3224,  3224,  3224,
     917,   919, -1036,   494,   494,  1565,   815,   313, -1036,   372,
   -1036, -1036,   494,   494,   449,   816,   888,   249,  3224,  2918,
   -1036,  5015,   148,    18,   887, -1036,  5015,  2816,   892, -1036,
     907, -1036,  4549,   908, -1036,  4859,   905,   913,   383,   317,
     921,   383,   317,  3434,   924,   925,  1258,   383, -1036, -1036,
   -1036,  5015,   650,   408,   938, -1036, -1036, -1036,   408,  1202,
   -1036,   937,  5015,   940, -1036,   939, -1036,   645,   404,  2498,
     943,  5015,  3400,   942, -1036,  1471,  4159,   948, -1036,   880,
    5405,   265,   946,   947,  5015,   672,   614,   956,   408,  5015,
      84,   920,  5015, -1036, -1036, -1036,   888,  1624,   448,    23,
   -1036,   963,  2362,   968,   182,   933,   926, -1036, -1036,   793,
   -1036,   272, -1036, -1036, -1036,   914, -1036,   970,  5617,   502,
   -1036,   988,   784,   231,   955, -1036, -1036,   990, -1036,   488,
   -1036,  3224,  2952,  3088,  3360,    14,  1265,   992,   681,  1106,
    1106,  1590,  1590,  2782,  3190,  3606,  2791,  1817,  2518,  1069,
    1069,   648,   648, -1036, -1036, -1036, -1036, -1036,   816,   619,
   -1036, -1036, -1036,   494,  1000,  1002,  1471,   321,  4962,  1006,
     540,   816, -1036,   346,   448,  1008, -1036,  5494,   448,   219,
   -1036,   219, -1036,   448,  1004,  1009,  1012,  1013,  1485,   383,
     317,  1014,  1022,  1027,   383,   650, -1036, -1036,   314,  4272,
   -1036,  1035, -1036,   254,  1038, -1036,  1010, -1036, -1036, -1036,
   -1036,   699, -1036, -1036, -1036,  1016, -1036,  1214,   699, -1036,
    1028,   103,   694,   404, -1036,  1214, -1036,  1011, -1036, -1036,
    4385,    40,  5547,   586, -1036, -1036,  5015, -1036,  1043, -1036,
     944, -1036,   199,   986, -1036,  1050,  1049, -1036,   488,   953,
     852, -1036,  1471,  1214,   187,   448, -1036,  5015,  3224, -1036,
   -1036, -1036, -1036, -1036,  4777, -1036,   422, -1036, -1036,  1040,
    1812,   417, -1036, -1036,  1061, -1036,   724, -1036,  2142, -1036,
     231,  2918,  3224,  3224,  3360,  3944,  3224,  1065,  1070,  1072,
    1074, -1036,  3224, -1036, -1036,  1075,  1076, -1036, -1036, -1036,
     555,   383, -1036, -1036,   383, -1036, -1036,   383,   448,   448,
    1077,  1084,  1087,   383,   448,  1088,  1093, -1036, -1036, -1036,
     699, -1036,   408,   408,   219,  2142,  5015,    84,  1202,  1672,
     408,  1081, -1036,  1214,  1099, -1036, -1036,   699,  3400,   146,
   -1036,  5405, -1036,  1083,   219,   233,   245,   384, -1036,  2272,
     236, -1036,  1064,    44,   622, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036,  5189, -1036,  4498,  1104, -1036,  1107,  2647, -1036,
   -1036, -1036,   684, -1036, -1036, -1036,  5015, -1036,   510,  1040,
   -1036,   123,  1109,    16, -1036,  5015,   480,  1097,  1089, -1036,
   -1036,  3400, -1036, -1036, -1036,   955, -1036, -1036, -1036,   488,
   -1036, -1036, -1036,  1111,  1091,  1092,  1098,  1019,  3534,   245,
   -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036,  1113,  1044,  2142, -1036, -1036, -1036, -1036, -1036, -1036,
   -1036,  5261,  1131,  2142, -1036,  2918,  2918,  2918,  3224,  3224,
   -1036,  5547,  3054, -1036, -1036, -1036,   383,   383,   448,  1132,
    1133,   383,   448,   448, -1036, -1036,   231,  1137,  1144, -1036,
     699,  1146, -1036,  1214,  1753,    84, -1036,  1147, -1036,  1149,
   -1036, -1036, -1036,   199, -1036, -1036,   199,  1082, -1036, -1036,
    5547,  5015,  3400,  5547,  3432, -1036, -1036,   684, -1036, -1036,
     231, -1036,  1148, -1036, -1036, -1036, -1036,   245,  1040, -1036,
    1140,  1960,    94, -1036,  1154,  1153,   480,   488,   516, -1036,
    1214, -1036,  1150,   955,  2142, -1036, -1036, -1036, -1036,   245,
   -1036,  1161,  3378, -1036, -1036,  1134,  1135,  1136,  1151,  1152,
     261,  1166,  2918,  2918,  1265,   383,   448,   448,   383,   383,
    1181, -1036,  1182, -1036,  1183, -1036,  1214, -1036, -1036, -1036,
   -1036, -1036,  1187,  1471,  1141,    92,  3874, -1036,   417,  1214,
    1193, -1036, -1036,  3224, -1036,  1214,  1040, -1036,   612, -1036,
    1204,  1206,  1205,   431, -1036, -1036,   231,  1212, -1036, -1036,
   -1036,   488, -1036,  2142,  1210,  5015, -1036, -1036,  1214,  3224,
   -1036,  3378,  1225,   383,   383, -1036, -1036, -1036,  1224, -1036,
    1226, -1036,  5015,  1236,  1237,    22,  1234, -1036,    49, -1036,
   -1036,  2918,   231, -1036, -1036, -1036, -1036,   488,  1231, -1036,
   -1036,   417,  1238,  1150,  1240,  5015,  1243,   231,  1997, -1036,
   -1036, -1036, -1036,  1252,  5015,  5015,  5015,  1255,  1812,  5547,
     510,   417,  1248,  1249, -1036, -1036, -1036, -1036,  1266,  1214,
     417, -1036,  1214,  1267,  1268,  1269,  5015, -1036,  1253, -1036,
   -1036,  1263, -1036,  2142,  1214, -1036,   338, -1036, -1036,   376,
    1214,  1214,  1214,  1271,   510,  1270, -1036, -1036, -1036, -1036,
     480, -1036, -1036,   480, -1036, -1036, -1036,  1214, -1036, -1036,
    1272,  1273, -1036, -1036, -1036
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
     613,     0,     0,     0,     0,     0,    12,     4,   561,   407,
     415,   408,   409,   412,   413,   410,   411,   397,   414,   396,
     416,   399,   417,   418,   419,   420,     0,   387,   388,   389,
     520,   521,   146,   515,   516,     0,   562,   563,     0,     0,
     573,     0,     0,   287,     0,     0,   385,   613,   392,   402,
     395,   404,   405,   519,     0,   580,   400,   571,     6,     0,
       0,   613,     1,    17,    67,    63,    64,     0,   263,    16,
     258,   613,     0,     0,    85,    86,   613,   613,     0,     0,
     262,   264,   265,     0,   266,     0,   267,   272,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    10,    11,     9,    13,    20,    21,    22,    23,
      24,    25,    26,    27,   613,    28,    29,    30,    31,    32,
      33,    34,     0,    35,    36,    37,    38,    39,    40,    14,
     116,   121,   118,   117,    18,    15,   155,   156,   157,   158,
     159,   160,   124,   259,     0,   277,     0,   148,   147,     0,
       0,     0,     0,     0,   613,   574,   288,   398,   289,     3,
     391,   386,   613,     0,   421,     0,     0,   573,   363,   362,
     379,     0,   304,   284,   613,   313,   613,   359,   353,   340,
     301,   393,   406,   401,   581,     0,     0,   569,     5,     8,
       0,   278,   613,   280,    19,     0,   595,   275,     0,   257,
       0,     0,   602,     0,     0,   390,   580,     0,   613,     0,
       0,    81,     0,   613,   270,   274,   613,   268,   230,   271,
     269,   276,   273,     0,     0,   189,   580,     0,     0,    65,
      66,     0,     0,    54,    52,    49,    50,   613,     0,   613,
       0,   613,   613,     0,   115,   613,   613,     0,     0,     0,
       0,     0,     0,   313,     0,   340,   261,   260,     0,   613,
       0,   613,   286,     0,     0,     0,     0,   575,   582,   572,
       0,   561,   597,   457,   458,   469,   470,   471,   472,   473,
     474,   475,   476,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   295,     0,   290,   613,   446,   390,     0,   451,
     445,   450,   455,   452,   456,   292,   394,   613,   363,   362,
       0,     0,   353,   400,     0,   299,   426,   427,   297,     0,
     423,   424,   425,   370,     0,   445,   300,     0,   613,     0,
       0,   315,   361,   332,     0,   314,   360,   377,   378,   341,
     302,   613,     0,   303,   613,     0,     0,   356,   355,   310,
     354,   332,   364,   579,   578,   577,     0,     0,   279,   283,
     565,   564,   613,   566,     0,   594,   119,   607,     0,    71,
      48,     0,   613,   313,   421,    73,     0,   523,   524,   522,
     525,     0,   526,     0,    77,     0,     0,     0,   101,     0,
       0,   185,     0,   613,     0,   187,     0,     0,   106,     0,
       0,     0,   110,   306,   313,   307,   309,    44,     0,   107,
     109,   567,     0,   568,    57,     0,    56,     0,     0,   178,
     613,   182,   519,   180,   170,     0,     0,     0,     0,   564,
       0,     0,     0,     0,   613,     0,     0,   332,     0,   613,
     340,   613,   580,   429,   613,   613,   503,     0,   502,   401,
     505,   517,   518,   403,     0,   570,     0,     0,     0,     0,
       0,   467,   466,   495,   494,   468,   496,   497,   560,     0,
     291,   294,   498,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   596,   363,   362,   353,   400,     0,   340,     0,
     375,   372,   356,   355,     0,   340,   364,     0,     0,   422,
     371,   613,   353,   400,     0,   333,   613,     0,     0,   376,
       0,   349,     0,     0,   368,     0,     0,     0,   312,   358,
       0,   311,   357,   366,     0,     0,     0,   316,   365,   576,
       7,     0,   613,     0,     0,   604,   171,   613,     0,     0,
     601,     0,   613,     0,    72,     0,    80,     0,     0,     0,
       0,     0,     0,     0,   186,   613,     0,     0,   613,   613,
       0,     0,   111,     0,   613,   613,     0,     0,     0,     0,
       0,   168,     0,   179,   184,    61,     0,     0,     0,     0,
      82,     0,     0,     0,   536,   529,   530,   384,   383,   541,
     382,   380,   537,   542,   544,     0,   545,     0,     0,     0,
     150,     0,   400,   613,   613,   163,   167,     0,   583,     0,
     447,   459,     0,     0,   379,     0,   613,     0,   613,   492,
     491,   489,   490,     0,   488,   487,   483,   484,   482,   485,
     486,   477,   478,   479,   480,   481,   449,   448,     0,   364,
     344,   343,   342,   366,     0,     0,   365,   323,     0,     0,
       0,   332,   334,   364,     0,     0,   337,     0,     0,   351,
     350,   373,   369,     0,     0,     0,     0,     0,     0,   317,
     367,     0,     0,     0,   319,   613,   281,   603,     0,     0,
     608,   609,   612,   611,   605,    46,     0,    45,    41,    79,
      76,    78,   600,    96,   599,     0,    91,   613,   598,    95,
       0,   611,     0,     0,   102,   613,   229,     0,   190,   191,
       0,   258,     0,     0,    53,    51,   613,    43,     0,   108,
       0,   588,   586,     0,    60,     0,     0,   113,     0,   613,
     613,   613,     0,   613,     0,     0,   351,   613,     0,   539,
     532,   531,   543,   381,     0,   141,     0,   149,   151,   613,
     613,     0,   131,   527,   504,   506,   508,   528,     0,   161,
     613,   460,     0,     0,   379,   378,     0,     0,     0,     0,
       0,   293,     0,   345,   347,     0,     0,   298,   352,   335,
       0,   325,   339,   338,   324,   305,   374,   320,     0,     0,
       0,     0,     0,   318,     0,     0,     0,   282,    69,    70,
      68,   120,     0,     0,   351,     0,   613,     0,     0,     0,
       0,     0,    93,   613,     0,   122,   188,   257,     0,   580,
     104,     0,   103,     0,   351,     0,     0,     0,   584,   613,
       0,    55,     0,   258,     0,   172,   173,   176,   175,   169,
     174,   177,     0,   183,     0,     0,    84,     0,     0,   134,
     133,   135,   613,   137,   132,   136,   613,   142,     0,   430,
     432,   438,     0,   434,   433,   613,   421,   542,   613,   154,
     130,     0,   127,   129,   125,   613,   513,   512,   514,     0,
     510,   198,   217,     0,     0,     0,     0,   264,   613,     0,
     242,   243,   235,   244,   215,   196,   240,   236,   234,   237,
     238,   239,   241,   216,   212,   213,   200,   207,   206,   210,
     209,     0,   218,     0,   201,   202,   205,   211,   203,   204,
     214,     0,   277,     0,   285,   463,   462,   461,     0,     0,
     453,     0,   493,   346,   348,   336,   322,   321,     0,     0,
       0,   326,     0,     0,   610,   606,   613,     0,     0,    87,
     611,    98,    92,   613,     0,     0,   100,     0,    74,     0,
     112,   308,   589,   587,   593,   592,   591,     0,    58,    59,
       0,   613,     0,     0,     0,    62,    83,   535,   540,   533,
     613,   534,     0,   144,   143,   140,   431,     0,   613,   440,
     442,     0,   613,   435,     0,     0,     0,     0,     0,   553,
     613,   507,   613,   613,     0,   193,   232,   231,   233,     0,
     219,     0,     0,   220,   192,   397,   396,   399,     0,   395,
     400,     0,   465,   464,   613,   327,     0,     0,   331,   330,
       0,    42,     0,    99,     0,    94,   613,    89,    75,   105,
     585,   590,     0,   613,     0,     0,   613,   538,     0,   613,
       0,   441,   439,     0,   152,   613,   613,   436,     0,   550,
       0,   552,   554,     0,   546,   547,   613,     0,   500,   509,
     501,     0,   199,     0,     0,   613,   165,   164,   613,     0,
     208,     0,     0,   329,   328,    47,    97,    88,     0,   114,
       0,   168,   613,     0,     0,     0,     0,   126,     0,   145,
     443,   444,   613,   437,   548,   549,   551,     0,     0,   558,
     559,     0,     0,   613,     0,   613,     0,   613,     0,   162,
     454,    90,   123,     0,   613,   613,   613,     0,   613,     0,
       0,     0,   555,     0,   128,   499,   511,   194,     0,   613,
       0,   251,   613,     0,     0,     0,   613,   221,     0,   138,
     153,     0,   556,     0,   613,   222,     0,   166,   228,     0,
     613,   613,   613,     0,     0,     0,   195,   223,   245,   247,
       0,   248,   250,   421,   226,   225,   224,   613,   139,   557,
       0,     0,   227,   246,   249
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1036, -1036,  -364, -1036, -1036, -1036, -1036,    17,    43,    -2,
      55, -1036,   765, -1036,    58,    73, -1036, -1036, -1036,    75,
   -1036,    76, -1036,    87, -1036, -1036,    88, -1036,    93,  -561,
    -680,   100, -1036,   101, -1036,  -356,   745,   -89,   112,   114,
     116,   117, -1036,   582,  -973,  -921, -1036, -1036, -1036, -1035,
    -943, -1036,  -140, -1036, -1036, -1036, -1036, -1036,     6, -1036,
   -1036,   232,    32,    33, -1036, -1036,   348, -1036,   749,   593,
     133, -1036, -1036, -1036,  -787, -1036, -1036, -1036,   429, -1036,
     595, -1036,   597,   135, -1036, -1036, -1036, -1036,  -220, -1036,
   -1036, -1036,     7,   -35, -1036,  -501,  1291,    64,   498, -1036,
     707,   875,   -39,  -609,  -549,   575,  1235,     5,  -148,  1304,
     218,  -618,   748,    85, -1036,   -65,    41,   -24,   599,  -714,
    1294, -1036,  -365, -1036,  -163, -1036, -1036, -1036,   481,   358,
    -894, -1036, -1036,   363, -1036,  1159, -1036,  -141,  -525, -1036,
   -1036,   239,   916, -1036, -1036, -1036,   478, -1036, -1036, -1036,
    -231,   -83, -1036, -1036,   357,  -571, -1036,  -589, -1036,   700,
     234, -1036, -1036,   257,   -20,  1157,  -168, -1036,  1031,  -194,
    -150,  1185, -1036,  -372,  1298, -1036,   652,   207, -1036,  -177,
    -534,     0
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,     5,   104,   105,   106,   815,   900,   901,   902,
     903,   111,   414,   415,   904,   905,   741,   114,   115,   906,
     117,   907,   119,   908,   700,   210,   909,   122,   910,   706,
     560,   911,   387,   912,   397,   240,   409,   241,   913,   914,
     915,   916,   547,   130,   884,   761,   862,   131,   756,   868,
     995,  1060,    42,   609,   132,   133,   134,   135,   917,   933,
     768,  1087,   918,   919,   739,   849,   418,   419,   420,   583,
     920,   140,   568,   393,   921,  1083,  1163,  1014,   922,   923,
     924,   925,   926,   927,   142,   928,   929,  1165,  1168,   930,
    1028,   143,   931,   310,   191,   358,    43,   192,   293,   294,
     470,   295,   762,   173,   402,   174,   331,   253,   176,   177,
     254,   599,   600,    45,    46,   296,   205,    48,    49,    50,
      51,    52,   318,   319,   360,   321,   322,   441,   870,   871,
     872,   873,   998,   999,  1110,   298,   299,   325,   301,   302,
    1078,  1079,   447,   448,   614,   764,   765,   889,  1013,   890,
      53,    54,   380,   381,   766,   602,   990,   603,   604,  1169,
     879,  1008,  1071,  1072,   184,    55,   367,   412,    56,   187,
      57,   269,   733,   838,   303,   304,   709,   201,   544,   368,
     694,   193
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
       6,   320,   268,   109,   259,   145,   238,   204,   172,   553,
     752,   136,   144,   300,   311,   422,   717,   779,   247,   737,
     155,   160,   107,   749,   692,   712,   190,   362,   957,   566,
     239,   239,   821,   369,   692,   353,   268,   137,   138,   376,
     864,   686,    47,    47,   385,   573,   196,   180,   108,   327,
     196,   795,   175,   796,   398,   776,   256,     8,  1002,   662,
     110,   261,  1136,   112,   662,    44,    59,    30,    31,  1058,
     270,   202,   431,  1140,   411,   455,   202,   211,   113,   260,
     116,   118,    47,   221,   433,  1107,    33,    34,   702,    62,
     196,   405,   120,   121,   162,   867,   214,   215,   123,    27,
      28,    29,    47,  -296,  -181,   124,   125,   490,  1067,   365,
     365,   165,   151,     8,   202,   364,   222,   126,   246,   127,
     257,   128,   129,  1174,   365,   197,     8,   703,   607,   197,
     704,   161,  1102,   181,   663,   340,  1024,   343,   139,   745,
     141,  1003,   777,    36,    37,   778,  1031,   155,  1144,   154,
     328,     8,   822,  1137,   305,  1121,    72,  -566,   329,   311,
    1139,   539,   198,   585,   366,  -296,  -181,   165,  1160,    38,
     491,   877,  1113,   167,   180,   146,   180,  1167,   388,   229,
     311,   389,   502,   689,   807,   503,   170,     8,   328,   968,
     438,  1141,   359,   365,   171,   297,   956,  1159,   705,    36,
      37,   149,   400,    47,   720,   165,  1150,   150,   382,   373,
     417,   320,    36,    37,   723,   374,   971,  1029,   292,  1066,
    1103,   230,   748,  1104,   432,    38,   394,  1082,   170,    40,
     856,  1188,   534,    27,    28,    29,     8,    36,    37,   406,
     978,   410,   413,     8,   404,   306,   423,   997,     8,   154,
     330,    60,     8,   152,    47,   338,   959,    47,   172,   443,
     365,   450,   158,    38,   154,   178,   315,    40,    27,    28,
      29,   595,   596,    36,    37,   154,   502,   341,    47,   503,
     185,  1043,   979,   208,   961,   164,    47,    47,   330,   371,
      61,   989,   153,   692,   342,   471,  1124,   180,   616,    38,
     579,  -613,   175,    40,  1090,   186,   520,   166,   724,   523,
     421,   725,   535,   497,   499,   536,   836,   504,   808,   575,
     196,   248,    36,    37,   330,   316,   317,   565,   300,    36,
      37,   446,   223,   551,    36,    37,   239,  -256,    36,    37,
       8,   837,  -428,   239,   147,  -428,   154,   597,    47,     8,
     598,   422,   972,   434,   650,    38,   195,   344,   655,    40,
     809,   434,   545,   148,    38,   159,   687,     8,   167,    47,
     342,   690,   180,   619,   345,  -428,  1176,   854,   342,   391,
     312,  1178,    47,     8,  1179,    47,   432,     8,   337,   171,
     392,   710,  1180,     6,   626,    27,    28,    29,    47,   591,
     597,   735,   154,   598,  1047,   377,   378,     8,   702,   611,
     196,     8,   434,   651,   196,     8,  1057,   405,   880,  1181,
     584,   542,  1182,   516,   379,  1092,    36,    37,   188,   342,
    1183,   497,   499,   504,   618,    36,    37,   538,   655,   606,
     517,   610,     8,   219,   371,   606,   189,   703,   220,   450,
     704,     8,   163,    36,    37,   266,   267,   178,   225,   881,
     882,    38,   866,   883,   194,    40,   548,   428,   867,    36,
      37,  1119,   459,    36,    37,    47,   659,  1120,   549,   328,
     460,   665,   266,   354,   200,    47,   330,   334,   432,   434,
     652,     8,   552,    36,    37,    38,   165,    36,    37,    40,
     207,    36,    37,   975,  1100,   239,   342,   209,   786,   342,
     297,  1005,   657,   316,   317,   178,   170,   696,   705,    38,
       1,     2,     3,   167,   601,   495,   574,    38,    36,    37,
     601,    40,   250,   292,   307,   251,   212,    36,    37,   728,
     170,   213,   359,   342,   171,   758,   512,     6,   759,   877,
     993,   165,    47,   994,    38,   216,   519,    47,    40,  1074,
     217,     8,  1075,    38,   109,   406,   145,    40,     6,   145,
     404,   218,   136,   144,     8,   410,   722,    36,    37,   330,
     516,   789,    47,   107,   757,   316,   317,   172,   330,   428,
     702,   656,   196,    47,   786,   516,   945,   517,   137,   138,
     228,  1050,    47,    38,  1051,   685,   235,    40,   196,   108,
     231,   432,   517,   180,   767,    47,   242,     8,   243,   329,
      47,   110,   196,    47,   112,   714,   180,   232,   471,   830,
     233,   175,   704,   234,   446,   954,   955,   558,   559,   113,
     831,   116,   118,   736,    30,    31,   421,    36,    37,   699,
     588,   196,   710,   120,   121,  1114,   262,   828,  1115,   123,
      36,    37,   428,    33,    34,   245,   124,   125,    27,    28,
      29,   742,   263,    38,    30,    31,   625,    40,   126,   264,
     127,   314,   128,   129,   352,   359,    38,   109,   356,   145,
      40,   833,   357,    33,    34,   136,   144,   252,   300,   139,
     370,   141,   395,    36,    37,   365,   107,   606,   401,   775,
     407,   330,   857,   320,   408,   606,   577,   578,   109,   438,
     145,   137,   138,   469,   178,   981,   136,   144,   451,   730,
     452,   934,   108,   731,   538,   819,   820,   107,   453,   145,
     584,     6,   969,   606,   110,   846,   852,   112,   538,  1021,
     422,   160,   137,   138,   601,   456,   601,   869,   468,   874,
     606,   247,   113,   108,   116,   118,   239,    47,   932,   261,
     180,   847,   848,   595,   596,   110,   120,   121,   112,   372,
     457,   958,   123,   178,   487,   488,   489,   984,    47,   124,
     125,   259,   601,   113,   472,   116,   118,   886,   887,   888,
     601,   126,   492,   127,   178,   128,   129,   120,   121,     8,
     500,   501,   154,   123,   403,   932,   507,   506,  1191,   508,
     124,   125,   139,   606,   141,  -580,  -580,   511,   601,   514,
     518,   992,   126,   439,   127,   445,   128,   129,   170,   305,
    1004,   533,   337,  1070,   178,   601,   162,   526,   261,   550,
     540,  -580,   109,   139,   145,   141,   543,    47,   554,   775,
     136,   144,   991,   165,   556,   557,   984,   869,   561,  1084,
     562,   107,   563,   593,   178,   564,  1034,   569,  1009,   570,
     297,   594,   595,   596,   571,   767,   137,   138,   572,     8,
     576,     8,   580,   581,   582,    36,    37,   108,   145,   601,
     586,    68,   587,   292,   592,   405,   590,    47,   601,   110,
     608,   615,   112,   620,   627,  1053,    47,  1040,  1055,   601,
     646,    38,   647,   932,   721,   167,   328,   113,   162,   116,
     118,   649,   653,   932,   527,   664,   163,    27,    28,    29,
     668,   120,   121,   165,  1177,   165,   171,   123,   669,   671,
    1184,  1185,  1186,   673,   124,   125,   180,    80,    81,    82,
     178,   674,    84,   606,    86,    87,   126,  1192,   127,   675,
     128,   129,   681,   682,    68,    36,    37,    36,    37,   688,
     695,   423,     8,   697,   713,   698,   726,   139,   716,   141,
     180,   719,   337,   519,   727,   172,   869,   843,   874,   734,
     869,    38,   874,    38,   746,    40,     8,   167,   747,   196,
     606,   738,  1080,   767,   932,   751,   168,  1190,     8,   169,
     320,  1106,    47,   750,   170,   754,   330,   755,   171,   760,
      80,    81,    82,   763,   180,    84,   769,    86,    87,   175,
     780,   783,     8,   784,   375,   421,   606,   788,   601,   792,
    1126,   814,   798,   406,   825,   328,   816,   799,   404,   606,
     800,   801,   804,   530,   869,   606,   874,  1133,    36,    37,
     805,   844,   165,   157,  1158,   806,   180,   812,   179,   432,
     813,   654,   818,   932,   834,   183,   835,   334,   606,   839,
    1148,   840,    36,    37,    38,   601,   841,    47,    40,  1153,
    1154,  1155,    72,   885,    36,    37,   938,   426,   316,   317,
     427,   939,   180,   940,   941,   170,   943,   944,   980,   224,
     227,  1173,   964,  1080,   970,   948,    47,   180,    36,    37,
      38,   601,   949,   707,    40,   950,   952,   715,   606,   605,
     403,   953,   966,    47,   601,   613,     8,   986,   987,   606,
     601,  1006,   606,   255,    38,   330,  1001,  1015,    40,  1019,
    1022,   654,   743,   932,   606,  1007,    47,  1016,  1017,  1023,
     606,   606,   606,   601,  1018,    47,    47,    47,  -197,   330,
    1036,  1037,   265,   432,  1041,  1042,  1052,   606,   820,  1059,
    1048,   527,  1049,   313,  1063,  1068,  1069,    47,  1077,   333,
     333,  1085,   339,   485,   486,   487,   488,   489,   196,   351,
    -254,  -253,  -255,  1091,     8,   275,   276,   277,   278,   279,
     280,   281,   282,   601,  1095,  1096,  1097,  1089,  -252,   206,
    1099,   785,    36,    37,   601,   255,  1109,   601,   483,   484,
     485,   486,   487,   488,   489,  1101,   226,  1116,  1117,   601,
    1125,   432,   178,  1118,   390,   601,   601,   601,    38,   530,
    1122,     8,    40,    27,    28,    29,  1130,  1131,     8,  1132,
     179,   178,   601,  1105,   428,  1138,  1134,  1135,   424,  1143,
     430,   333,   333,   330,  1149,   437,  1145,  1147,   823,   440,
     157,   255,   449,  1152,   593,  1156,  1161,  1162,   432,   867,
      36,    37,   594,   595,   596,   162,   683,  1164,  1170,  1171,
    1172,  1175,  1187,   163,   691,  1193,  1194,   785,  1189,   164,
     729,   845,   165,  1129,   324,   326,    38,  1020,   179,  1054,
      40,   740,   156,   853,   850,   781,   851,   977,   496,   498,
     498,   166,   597,   505,   628,   598,   182,    36,    37,   753,
     996,   330,   361,   863,    36,    37,  1062,   361,   361,   513,
    1061,   515,  1146,  1011,   361,   617,   383,   384,   199,   361,
    1081,   355,  1157,    38,  1142,   832,     0,    40,   333,   333,
      38,     0,     0,   333,   167,   396,     0,     0,   361,   399,
       8,     0,   236,   168,   963,   546,   169,   244,   330,   361,
       0,   170,   430,   967,   335,   171,   429,   817,     0,   361,
       0,     0,   555,   349,     0,   824,   442,     0,     0,     0,
       0,     0,     0,   454,     0,   567,     0,   432,     0,     0,
       0,     0,     0,     0,     0,   346,     0,     0,     0,     0,
       0,     0,   458,   855,   461,   462,   463,   464,   465,   466,
     467,     0,     0,     0,     0,     0,  1010,   498,   498,   498,
     878,     0,     0,   589,     0,     0,   333,   333,     0,   333,
       0,     0,   332,   336,     8,   612,    36,    37,   509,     0,
       0,     0,   350,     0,     0,     0,   335,     0,     8,   349,
       0,     0,     0,   363,     0,     0,     0,     0,   363,   363,
       0,   522,    38,     0,   525,   363,    40,     0,     0,     0,
     363,   162,     0,     0,     0,   435,     0,     0,   436,   163,
       0,     0,     0,   965,     0,   432,   648,   330,   165,   363,
       0,     0,     0,   802,     0,     0,     0,   179,   498,  1046,
     363,   416,     0,   661,     0,     0,   425,   363,     0,     0,
     363,     0,     0,     0,     0,     0,     0,   439,     0,   445,
      36,    37,     0,     0,   333,     0,     0,   333,     8,     0,
       0,     0,     0,     0,    36,    37,  1065,     0,     0,     0,
       0,     0,   528,   531,     0,     0,    38,   537,     0,   255,
     167,     0,     0,   255,     0,     0,   179,  1088,     0,   168,
      38,     0,   169,     0,    40,   307,     0,   170,     0,     0,
       0,   171,   332,   336,     0,     0,   350,   179,   255,   333,
       0,     0,   165,   333,     0,   330,     0,     8,   403,     0,
     196,     0,   629,   630,   631,   632,   633,   634,   635,   636,
     637,   638,   639,   640,   641,   642,   643,   644,   645,     0,
     770,   529,   532,     0,    36,    37,     0,   179,     0,     0,
       0,     0,     0,  1044,   371,     0,  1088,   658,     0,     0,
     528,   531,   163,   537,     0,     8,   667,     0,     0,     0,
      38,     0,     0,     0,    40,     0,     0,   179,     0,     0,
       0,     0,     0,   502,   333,   333,   503,   473,   474,   333,
     361,     0,     0,     8,   333,   361,     0,     0,     0,   333,
    1076,     0,   371,    36,    37,   962,   361,     0,     0,     0,
     163,     0,   483,   484,   485,   486,   487,   488,   489,     0,
       0,     0,     0,   732,     0,   361,     0,     0,     0,    38,
     249,     0,     0,   167,   255,     0,  1098,   660,   163,     0,
       0,     0,   250,     0,     0,   251,     8,     0,     0,  1108,
     170,    36,    37,     0,   171,  1112,     0,     0,   679,   842,
       0,   684,     0,   179,     0,     0,   333,     0,     0,     0,
     771,   635,   638,   643,     0,   865,     0,    38,  1127,    36,
      37,   167,     0,   371,     0,     0,  1045,   332,   336,   350,
     250,   163,     0,   251,     0,     0,   529,   532,   170,     0,
       0,     0,   171,     0,     0,    38,   350,     0,     0,   167,
       0,     0,     0,   660,     0,     0,     0,   679,   250,   333,
     333,   251,     0,     0,     0,   333,   170,   680,   878,     0,
     171,   363,    36,    37,     0,     0,   363,   693,     0,  1166,
     255,     0,   875,     0,     0,   701,   708,   711,     0,   255,
       8,    27,    28,    29,     0,     0,   876,     0,    38,     0,
       0,     0,   167,     0,     0,     0,   363,     0,   708,   829,
       0,   250,     0,     0,   251,   744,     0,     0,     0,   170,
       0,     0,   593,   171,     0,     0,     0,   328,   790,   791,
     594,   595,   596,   794,     0,   346,     0,   858,   797,     0,
       0,     0,   255,   803,   165,     0,     0,     0,     0,     8,
    1012,     0,     0,     0,   473,   474,   475,   476,     0,     0,
       0,   935,   936,   465,     0,   937,     8,     0,     0,     0,
     597,   942,     0,   598,     0,     0,    36,    37,   482,   483,
     484,   485,   486,   487,   488,   489,   432,   680,     0,     0,
       0,     0,  1030,     8,   534,     0,     0,     0,     0,   361,
     361,     0,    38,   162,     0,  -613,    40,   361,     0,   333,
     790,   163,     0,   333,   333,   347,   810,   164,   348,     0,
     165,     0,   973,   974,   976,   255,     0,   330,     0,     0,
     371,     0,     0,  1064,     0,    36,    37,     0,   163,   166,
       0,   708,     0,   255,     0,   255,     0,     0,     0,   827,
       0,   708,    36,    37,     0,     0,     0,     0,  1000,     0,
       0,    38,   255,   946,   947,    40,     0,  -613,  1073,   951,
    1151,     0,     0,     0,   535,     0,     0,   536,    38,    36,
      37,     0,   167,   255,     0,     0,   330,     0,  -613,     0,
       0,   168,     0,     0,   169,   179,     0,   333,   333,   170,
       0,     0,     0,   171,     0,    38,     0,     0,     0,   167,
       0,     0,     0,     0,   179,     0,     0,   612,   250,     0,
       0,   251,     0,     0,     0,     0,   170,  1032,  1033,     0,
     171,     0,     0,     0,   473,   474,   475,   476,     0,   477,
     363,   363,  1123,     0,     0,   708,   960,     0,   363,     0,
       0,     0,   255,     0,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,     0,     0,     0,     0,
       0,   827,     0,   891,     0,  -613,    64,     0,  1073,     0,
      65,    66,    67,     0,  1000,     0,     0,     0,     0,     0,
       0,     0,     0,    68,  -613,  -613,  -613,  -613,  -613,  -613,
    -613,  -613,  -613,  -613,  -613,  -613,     0,  -613,  -613,  -613,
    -613,  -613,     0,  1035,     0,   892,    70,  1038,  1039,  -613,
       0,  -613,  -613,  -613,  -613,  -613,     0,     0,     0,     0,
       0,     0,     0,     0,    72,    73,    74,    75,   893,    77,
      78,    79,  -613,  -613,  -613,   894,   895,   896,     0,    80,
     897,    82,  1111,    83,    84,    85,    86,    87,  -613,  -613,
       0,  -613,  -613,    88,     0,     0,     0,    92,     0,    94,
      95,    96,    97,    98,    99,     0,     0,     8,  1128,     0,
     196,     0,     0,     0,     0,   100,     0,  -613,     0,     0,
     101,  -613,  -613,   708,     0,     0,   898,     0,     0,     0,
       0,  1093,  1094,     0,     0,   271,     0,     0,   196,   272,
       0,     0,   899,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,     0,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     0,    21,    22,    23,
      24,    25,   283,     0,     0,     0,     0,     0,     0,     0,
      26,    27,    28,    29,    30,    31,     0,   284,     0,     0,
       0,     0,     0,    36,    37,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,     0,     0,     0,     0,   316,
     317,     0,     0,     0,     0,     0,     0,     0,    35,    38,
       0,    36,    37,    40,     0,     8,     0,     0,     0,     0,
       0,     0,   426,     0,     0,   427,     0,     0,     0,     0,
     170,     0,     0,     0,     0,     0,     0,    38,     0,     0,
      39,    40,     0,     0,     0,     0,    41,     0,     0,     0,
     285,     0,   432,   286,     0,     0,   287,   288,   289,     0,
     676,   271,   290,   291,   196,   272,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,   283,     0,
       0,    36,    37,     0,     0,     0,     0,    27,    28,    29,
      30,    31,     0,   284,     0,     0,   323,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    38,    32,    33,
      34,    40,     0,     0,     0,     0,     0,     0,     0,     0,
     677,     0,     0,   678,    35,     0,     0,    36,    37,     0,
       0,     8,   330,     0,   196,     0,     0,     0,     0,     0,
       0,   275,   276,   277,   278,   279,   280,   281,   282,     0,
       0,     0,     0,    38,     0,     0,     0,    40,     0,     0,
       0,     0,     0,     0,     0,     0,   285,     0,     0,   286,
       0,     0,   287,   288,   289,     0,     0,   271,   290,   291,
     196,   272,   621,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,   283,     0,     0,    36,    37,     0,
       0,     0,     0,    27,    28,    29,    30,    31,     0,   284,
       0,     0,   521,   316,   317,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,     0,   473,   474,
     475,   476,     0,   477,     0,   473,   474,   475,   476,     0,
      35,     0,     0,    36,    37,     0,     0,     0,   478,   622,
     480,   481,   623,   483,   484,   485,   486,   624,   488,   489,
     483,   484,   485,   486,   487,   488,   489,     0,     0,    38,
       0,     0,     0,    40,     0,     0,     0,     0,     0,     0,
       0,     0,   285,     0,     0,   286,     0,     0,   287,   288,
     289,     0,     0,   271,   290,   291,   196,   272,   988,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,     0,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,     0,    21,    22,    23,    24,    25,
     283,     0,     0,     0,     0,     0,     0,     0,     0,    27,
      28,    29,    30,    31,     0,   284,     0,     0,   524,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      32,    33,    34,     0,   473,   474,   475,   476,     0,   477,
       0,     0,     0,     0,     0,     0,    35,     0,     0,    36,
      37,     0,     0,     0,   478,   479,   480,   481,   482,   483,
     484,   485,   486,   487,   488,   489,     0,     9,    10,    11,
      12,    13,    14,    15,    16,    38,    18,     0,    20,    40,
       0,    22,    23,    24,    25,     0,     0,     0,   285,     0,
       0,   286,     0,     0,   287,   288,   289,     0,     0,   271,
     290,   291,   196,   272,     0,     0,     0,   273,   274,   275,
     276,   277,   278,   279,   280,   281,   282,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
       0,    21,    22,    23,    24,    25,   283,     0,   782,     0,
       0,     0,     0,     0,     0,    27,    28,    29,    30,    31,
       0,   284,     0,     0,   666,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,   473,
     474,   475,   476,     0,   477,     0,     0,     0,   473,   474,
     475,   476,    35,     0,     0,    36,    37,     0,     0,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,   488,
     489,   481,   482,   483,   484,   485,   486,   487,   488,   489,
       0,    38,     0,     0,     0,    40,     0,     0,     0,     0,
       0,     0,     0,     0,   285,     0,     0,   286,     0,     0,
     287,   288,   289,     0,     0,   271,   290,   291,   196,   272,
       0,     0,     0,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,     0,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     0,    21,    22,    23,
      24,    25,   283,   772,     0,     0,     0,     0,     0,     0,
       0,    27,    28,    29,    30,    31,     0,   284,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,   473,   474,   475,   476,     0,
     477,     0,     0,     0,     0,     0,     0,     0,    35,     0,
       0,    36,    37,     0,     0,   478,   479,   480,   481,   482,
     483,   484,   485,   486,   487,   488,   489,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    38,     0,     0,
       0,    40,     0,     0,     0,     0,     0,     0,     0,     0,
     285,     0,     0,   286,     0,     0,   287,   288,   289,     0,
       0,   271,   290,   291,   196,   272,     0,     0,     0,   273,
     274,   275,   276,   277,   278,   279,   280,   281,   282,     0,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,   283,   773,
       0,     0,     0,     0,     0,     0,     0,    27,    28,    29,
      30,    31,     0,   284,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,   473,   474,   475,   476,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    36,    37,     0,
       0,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     487,   488,   489,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    38,     0,     0,     0,    40,     0,     0,
       0,     0,     0,     0,     0,     0,   285,     0,     0,   286,
       0,     0,   287,   288,   289,     0,     0,   271,   290,   291,
     196,   272,     0,     0,     0,   273,   274,   275,   276,   277,
     278,   279,   280,   281,   282,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,   283,     0,     0,     0,     0,     0,
       0,     0,     0,    27,    28,    29,    30,    31,     0,   284,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,   473,   474,   475,
     476,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      35,     0,     0,    36,    37,     0,     0,     0,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    38,
       0,     0,     0,    40,     0,     0,     0,     0,     0,     0,
       0,     0,   285,     0,     0,   286,     0,     0,   287,   288,
     289,     0,     0,   271,   290,   291,   196,   272,     0,     0,
       0,   273,   274,   275,   276,   277,   278,   279,   280,   281,
     282,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,     0,    21,    22,    23,    24,    25,
     283,     0,     0,     8,     0,     0,     0,     0,     0,    27,
      28,    29,    30,    31,     0,   284,     0,     8,   371,     0,
       0,  1086,     0,     0,     0,     0,   163,     0,     0,     0,
      32,    33,    34,     0,     0,     8,     0,     8,     0,     0,
     371,     0,     0,     0,     0,     0,    35,     0,   163,    36,
      37,     0,     0,     0,   444,     0,     0,     0,     0,     0,
       0,     0,   163,     0,     0,     0,     0,    36,    37,     0,
       0,     0,  1056,     0,   328,    38,     0,     0,     0,    40,
     163,     0,   676,     0,     0,     0,     0,     0,     0,    36,
      37,   165,     0,    38,   287,   288,   774,   167,     0,     0,
     290,   291,     0,    36,    37,     0,   250,     0,     0,   251,
       0,     0,     0,     0,   170,    38,     0,     0,   171,   167,
       0,    36,    37,    36,    37,     0,     0,     0,   250,    38,
       0,   251,     0,   167,     0,     0,   170,     0,    64,     0,
     171,     0,   250,     0,    67,   251,     0,    38,     0,    38,
     170,   167,     0,    40,   171,    68,     0,     0,     0,     0,
     250,     0,   677,   251,     0,   678,     0,     0,   170,     0,
       0,     0,   171,     0,   330,     0,     0,   892,    70,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    72,    73,    74,    75,
       0,    77,    78,    79,     0,     0,     0,   894,   895,   896,
       0,    80,   897,    82,     0,    83,    84,    85,    86,    87,
       0,     0,     0,     0,     0,    88,     0,     0,     0,    92,
       0,    94,    95,    96,    97,    98,    99,     0,     0,     0,
       0,     0,     0,     8,     0,     0,     0,   100,     0,     0,
       0,     0,   101,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,   899,    21,    22,    23,    24,    25,
     307,     0,     0,     0,     0,     0,     0,     0,    26,    27,
      28,    29,    30,    31,     0,     0,     0,   165,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      32,    33,    34,   473,   474,   475,   476,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    35,     0,     0,    36,
      37,     0,     0,     0,     0,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    38,     0,     0,    39,    40,
       8,     0,     0,     0,    41,     0,     0,     0,   308,     0,
       0,   309,     0,     0,     0,     0,   170,     0,     0,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     0,    21,    22,    23,    24,    25,   307,     0,     0,
       0,     0,     0,     0,     0,    26,    27,    28,    29,    30,
      31,     0,     0,     0,   165,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    32,    33,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    35,     0,     0,    36,    37,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    38,     0,     0,    39,    40,     8,     0,   510,
       0,    41,     0,     0,     0,   493,     0,     0,   494,     0,
       0,     0,     0,   170,     0,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,     0,     0,     0,     0,     0,     0,
       0,     0,    26,    27,    28,    29,    30,    31,   473,   474,
     475,   476,     0,   477,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,     8,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,   488,   489,
      35,     0,     0,    36,    37,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,     0,     0,     0,     0,     0,    38,
       0,     0,    39,    40,     0,     0,    30,    31,    41,     0,
       0,     0,   426,     0,     0,   427,     0,     0,     0,     0,
     170,     0,     0,     0,    32,    33,    34,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      35,     0,     0,    36,    37,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    -2,    63,     0,  -613,
      64,     0,     0,     0,    65,    66,    67,     0,     0,    38,
       0,     0,     0,    40,     0,     0,     0,    68,  -613,  -613,
    -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,
     170,  -613,  -613,  -613,  -613,  -613,     0,     0,     0,    69,
      70,     0,     0,     0,     0,  -613,  -613,  -613,  -613,  -613,
       0,     0,    71,     0,     0,     0,     0,     0,    72,    73,
      74,    75,    76,    77,    78,    79,  -613,  -613,  -613,     0,
       0,     0,     0,    80,    81,    82,     0,    83,    84,    85,
      86,    87,  -613,  -613,     0,  -613,  -613,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   100,
      63,  -613,  -613,    64,   101,  -613,     0,    65,    66,    67,
     102,   103,     0,     0,     0,     0,     0,     0,     0,     0,
      68,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,
    -613,  -613,  -613,     0,  -613,  -613,  -613,  -613,  -613,     0,
       0,     0,    69,    70,     0,     0,   718,     0,  -613,  -613,
    -613,  -613,  -613,     0,     0,    71,     0,     0,     0,     0,
       0,    72,    73,    74,    75,    76,    77,    78,    79,  -613,
    -613,  -613,     0,     0,     0,     0,    80,    81,    82,     0,
      83,    84,    85,    86,    87,  -613,  -613,     0,  -613,  -613,
      88,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   100,    63,  -613,  -613,    64,   101,  -613,     0,
      65,    66,    67,   102,   103,     0,     0,     0,     0,     0,
       0,     0,     0,    68,  -613,  -613,  -613,  -613,  -613,  -613,
    -613,  -613,  -613,  -613,  -613,  -613,     0,  -613,  -613,  -613,
    -613,  -613,     0,     0,     0,    69,    70,     0,     0,   811,
       0,  -613,  -613,  -613,  -613,  -613,     0,     0,    71,     0,
       0,     0,     0,     0,    72,    73,    74,    75,    76,    77,
      78,    79,  -613,  -613,  -613,     0,     0,     0,     0,    80,
      81,    82,     0,    83,    84,    85,    86,    87,  -613,  -613,
       0,  -613,  -613,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   100,    63,  -613,  -613,    64,
     101,  -613,     0,    65,    66,    67,   102,   103,     0,     0,
       0,     0,     0,     0,     0,     0,    68,  -613,  -613,  -613,
    -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,     0,
    -613,  -613,  -613,  -613,  -613,     0,     0,     0,    69,    70,
       0,     0,   826,     0,  -613,  -613,  -613,  -613,  -613,     0,
       0,    71,     0,     0,     0,     0,     0,    72,    73,    74,
      75,    76,    77,    78,    79,  -613,  -613,  -613,     0,     0,
       0,     0,    80,    81,    82,     0,    83,    84,    85,    86,
      87,  -613,  -613,     0,  -613,  -613,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   100,    63,
    -613,  -613,    64,   101,  -613,     0,    65,    66,    67,   102,
     103,     0,     0,     0,     0,     0,     0,     0,     0,    68,
    -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,  -613,
    -613,  -613,     0,  -613,  -613,  -613,  -613,  -613,     0,     0,
       0,    69,    70,     0,     0,     0,     0,  -613,  -613,  -613,
    -613,  -613,     0,     0,    71,     0,     0,     0,   985,     0,
      72,    73,    74,    75,    76,    77,    78,    79,  -613,  -613,
    -613,     0,     0,     0,     0,    80,    81,    82,     0,    83,
      84,    85,    86,    87,  -613,  -613,     0,  -613,  -613,    88,
      89,    90,    91,    92,    93,    94,    95,    96,    97,    98,
      99,     0,     0,     7,     0,     8,     0,   670,     0,     0,
       0,   100,     0,  -613,     0,     0,   101,  -613,     0,     0,
       0,     0,   102,   103,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     0,    21,    22,    23,
      24,    25,     0,     0,     0,     0,     0,     0,     0,     0,
      26,    27,    28,    29,    30,    31,   473,   474,   475,   476,
       0,   477,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,     0,   478,   479,   480,   481,
     482,   483,   484,   485,   486,   487,   488,   489,    35,     0,
       0,    36,    37,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     8,     0,
       0,     0,     0,     0,     0,     0,     0,    38,     0,     0,
      39,    40,     0,     0,     0,     0,    41,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,    24,    25,     0,     0,     0,     0,     0,
       0,     0,     0,    26,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    33,    34,     0,     0,
       8,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    35,     0,     0,    36,    37,     0,     0,     0,     9,
      10,    11,    12,    13,    14,    15,    16,   859,    18,   860,
      20,     8,   861,    22,    23,    24,    25,     0,     0,     0,
      38,     0,     0,    39,    40,     0,     0,     0,     0,    41,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,     0,     0,
       0,     0,     0,     0,     0,     0,    26,    27,    28,    29,
      30,    31,     0,    35,     0,     0,    36,    37,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    38,     0,    35,     0,    40,    36,    37,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     8,     0,   672,     0,     0,
       0,     0,     0,    38,     0,   386,    39,    40,     0,     0,
       0,     0,    41,   541,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,     0,    21,    22,    23,
      24,    25,     0,     0,     0,     0,     0,     0,     0,     0,
      26,    27,    28,    29,    30,    31,   473,   474,   475,   476,
       0,   477,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    32,    33,    34,     0,   478,   479,   480,   481,
     482,   483,   484,   485,   486,   487,   488,   489,    35,     0,
       0,    36,    37,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     8,     0,
     787,     0,     0,     0,     0,     0,     0,    38,     0,     0,
      39,    40,     0,     0,     0,     0,    41,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,    24,    25,     0,     0,     0,     0,     0,
       0,     0,     0,    26,    27,    28,    29,    30,    31,   473,
     474,   475,   476,     0,   477,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    33,    34,     0,   478,
     479,   480,   481,   482,   483,   484,   485,   486,   487,   488,
     489,    35,     0,     0,    36,    37,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   203,
       0,     8,     0,     0,     0,     0,     0,     0,     0,     0,
      38,     0,     0,    39,    40,     0,     0,     0,     0,    41,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      19,    20,     0,    21,    22,    23,    24,    25,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    27,    28,    29,
      30,    31,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    32,    33,
      34,     0,     8,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    35,     0,     0,    36,    37,     0,
       0,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,     0,    21,    22,    23,    24,    25,     0,
       0,     0,     0,    38,     0,     0,     0,    40,    27,    28,
      29,    30,    31,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    32,
      33,    34,     0,     0,     8,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    35,   982,     0,    36,    37,
       0,     0,     0,     9,    10,    11,    12,    13,    14,    15,
      16,  1025,    18,  1026,    20,     0,  1027,    22,    23,    24,
      25,     0,     0,     0,    38,     0,     0,     0,    40,   983,
      27,    28,    29,    30,    31,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    32,    33,    34,     0,     0,     0,     8,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    35,   258,     0,
      36,    37,     0,     0,     0,     0,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    19,    20,     0,    21,
      22,    23,    24,    25,     0,     0,    38,     0,     0,     0,
      40,   983,    26,    27,    28,    29,    30,    31,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    32,    33,    34,     0,     8,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      35,     0,     0,    36,    37,     0,     0,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,     0,
      21,    22,    23,    24,    25,   237,     0,     0,     0,    38,
       0,     0,    39,    40,    27,    28,    29,    30,    31,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    32,    33,    34,     0,     8,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    35,     0,     0,    36,    37,     0,     0,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    19,    20,
       0,    21,    22,    23,    24,    25,     0,     0,     0,     0,
      38,     0,     0,     0,    40,    27,    28,    29,    30,    31,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    32,    33,    34,     0,
       8,     0,   793,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    35,   258,     0,    36,    37,     0,     0,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     0,    21,    22,    23,    24,    25,     0,     0,     0,
       0,    38,     0,     0,     0,    40,    27,    28,    29,    30,
      31,   473,   474,   475,   476,     0,   477,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    32,    33,    34,
       8,   478,   479,   480,   481,   482,   483,   484,   485,   486,
     487,   488,   489,    35,     0,     0,    36,    37,     0,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     0,    21,    22,    23,    24,    25,     0,     0,     0,
       0,     0,    38,     0,     0,     0,    40,     0,     0,    30,
      31,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    32,    33,    34,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    35,     0,     0,    36,    37,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    38,     0,     0,     0,    40
};

static const yytype_int16 yycheck[] =
{
       0,   164,   152,     5,   144,     5,    95,    72,    47,   374,
     599,     5,     5,   154,   162,   246,   565,   626,   101,   580,
      40,    45,     5,   594,   549,   559,    61,   195,   815,   393,
      95,    96,   712,   201,   559,   185,   186,     5,     5,   207,
     754,   542,     1,     2,   212,   401,     6,    47,     5,     3,
       6,   669,    47,   671,   231,    41,    21,     3,    42,    41,
       5,   144,    40,     5,    41,     1,     2,    52,    53,   990,
     153,    71,   249,  1108,   242,   269,    76,    77,     5,   144,
       5,     5,    41,    44,   252,  1058,    71,    72,     4,     0,
       6,   239,     5,     5,    40,    46,    84,    85,     5,    49,
      50,    51,    61,    42,    42,     5,     5,    48,  1002,     6,
       6,    57,    40,     3,   114,   198,    77,     5,   103,     5,
      85,     5,     5,  1158,     6,    85,     3,    43,    54,    85,
      46,    46,    40,    48,   116,   174,   923,   176,     5,   116,
       5,   125,   128,    89,    90,   131,   933,   167,  1121,   103,
      40,     3,   713,   131,   154,  1076,    62,    54,    48,   307,
     111,   355,   118,    59,    46,   104,   104,    57,  1141,   115,
     111,   760,  1066,   119,   174,    48,   176,  1150,   213,     4,
     328,   216,   128,   547,   685,   131,   136,     3,    40,    43,
     116,  1112,   192,     6,   140,   154,   814,  1140,   114,    89,
      90,    48,   237,   162,   568,    57,  1127,    48,   208,   204,
     245,   374,    89,    90,   570,    54,   834,   931,   154,   125,
     128,    46,    40,   131,    40,   115,    54,  1014,   136,   119,
      43,  1174,    48,    49,    50,    51,     3,    89,    90,   239,
       4,   241,   242,     3,   239,   160,   246,   124,     3,   103,
     140,     1,     3,   142,   213,   170,   817,   216,   297,   259,
       6,   261,   125,   115,   103,    47,    46,   119,    49,    50,
      51,    89,    90,    89,    90,   103,   128,    40,   237,   131,
     117,   961,    46,    76,   818,    54,   245,   246,   140,    40,
      40,   862,   103,   818,    57,   295,  1083,   297,    43,   115,
      46,    46,   297,   119,    43,   142,   341,    76,    43,   344,
     246,    46,   128,   308,   309,   131,   117,   312,     4,   408,
       6,   114,    89,    90,   140,   105,   106,   392,   469,    89,
      90,    76,    92,   372,    89,    90,   401,    76,    89,    90,
       3,   142,    43,   408,    52,    46,   103,   128,   307,     3,
     131,   582,   119,    40,    41,   115,    40,    40,   506,   119,
      46,    40,   362,    71,   115,    43,   543,     3,   119,   328,
      57,   548,   372,   456,    57,    76,  1163,   741,    57,    43,
     162,    43,   341,     3,    46,   344,    40,     3,   170,   140,
      54,   559,    54,   393,   459,    49,    50,    51,   357,   434,
     128,   578,   103,   131,   965,    52,    53,     3,     4,   444,
       6,     3,    40,    41,     6,     3,   987,   565,     1,    43,
     420,   357,    46,    40,    71,  1034,    89,    90,    43,    57,
      54,   426,   427,   428,   454,    89,    90,   352,   586,   439,
      57,   441,     3,    77,    40,   445,    43,    43,    82,   449,
      46,     3,    48,    89,    90,   118,   119,   239,    46,    42,
      43,   115,    40,    46,   120,   119,    42,   249,    46,    89,
      90,    40,    40,    89,    90,   434,   511,    46,    54,    40,
      48,   516,   118,   119,    40,   444,   140,    48,    40,    40,
      41,     3,    40,    89,    90,   115,    57,    89,    90,   119,
      40,    89,    90,   119,  1053,   570,    57,    40,   656,    57,
     469,   876,   507,   105,   106,   297,   136,   552,   114,   115,
     121,   122,   123,   119,   439,   307,    40,   115,    89,    90,
     445,   119,   128,   469,    40,   131,    40,    89,    90,   574,
     136,    40,   542,    57,   140,    43,   328,   547,    46,  1138,
      40,    57,   511,    43,   115,    40,   338,   516,   119,    43,
      84,     3,    46,   115,   566,   565,   566,   119,   568,   569,
     565,    40,   566,   566,     3,   575,   569,    89,    90,   140,
      40,    41,   541,   566,   608,   105,   106,   626,   140,   371,
       4,   506,     6,   552,   742,    40,    41,    57,   566,   566,
      40,   973,   561,   115,   976,   541,     4,   119,     6,   566,
      40,    40,    57,   613,   614,   574,    40,     3,    40,    48,
     579,   566,     6,   582,   566,   561,   626,    40,   628,    43,
      43,   626,    46,    46,    76,   812,   813,    41,    42,   566,
      54,   566,   566,   579,    52,    53,   582,    89,    90,     4,
     432,     6,   820,   566,   566,    43,    48,   722,    46,   566,
      89,    90,   444,    71,    72,    40,   566,   566,    49,    50,
      51,   586,    48,   115,    52,    53,   458,   119,   566,    48,
     566,    48,   566,   566,   116,   685,   115,   689,    41,   689,
     119,   726,    42,    71,    72,   689,   689,   122,   839,   566,
      43,   566,    46,    89,    90,     6,   689,   707,    46,   624,
      43,   140,   747,   876,    42,   715,    41,    42,   720,   116,
     720,   689,   689,    42,   506,   103,   720,   720,    48,   115,
      48,   770,   689,   119,   649,    41,    42,   720,    41,   739,
     740,   741,   831,   743,   689,   739,   739,   689,   663,   899,
     981,   775,   720,   720,   669,   104,   671,   759,   104,   759,
     760,   844,   689,   720,   689,   689,   831,   726,   768,   852,
     770,   739,   739,    89,    90,   720,   689,   689,   720,   204,
     111,   816,   689,   565,   136,   137,   138,   852,   747,   689,
     689,   931,   707,   720,    40,   720,   720,    73,    74,    75,
     715,   689,     7,   689,   586,   689,   689,   720,   720,     3,
      41,    41,   103,   720,   239,   815,    48,   116,  1183,    57,
     720,   720,   689,   823,   689,   116,   117,    40,   743,    48,
      48,   866,   720,   258,   720,   260,   720,   720,   136,   839,
     875,   116,   624,  1006,   626,   760,    40,    48,   931,    41,
      43,   142,   854,   720,   854,   720,    42,   816,    41,   774,
     854,   854,   862,    57,    41,    54,   931,   869,    42,  1019,
      41,   854,    41,    80,   656,    43,   941,    41,   878,    41,
     839,    88,    89,    90,    41,   885,   854,   854,    41,     3,
      41,     3,    41,   104,    42,    89,    90,   854,   898,   814,
     116,    21,    41,   839,   116,  1053,    43,   866,   823,   854,
      76,    46,   854,     3,    48,   980,   875,   956,   983,   834,
       3,   115,     3,   923,    44,   119,    40,   854,    40,   854,
     854,   116,   116,   933,    48,    48,    48,    49,    50,    51,
      48,   854,   854,    57,  1164,    57,   140,   854,    41,    41,
    1170,  1171,  1172,    48,   854,   854,   956,    77,    78,    79,
     742,    48,    82,   963,    84,    85,   854,  1187,   854,    48,
     854,   854,    48,    48,    21,    89,    90,    89,    90,    41,
      43,   981,     3,    43,    41,    46,    40,   854,    46,   854,
     990,    43,   774,   775,    47,  1034,   998,    44,   998,    43,
    1002,   115,  1002,   115,    41,   119,     3,   119,    40,     6,
    1010,    91,  1012,  1013,  1014,    89,   128,  1180,     3,   131,
    1183,  1056,   981,    90,   136,   111,   140,    57,   140,    41,
      77,    78,    79,    78,  1034,    82,    46,    84,    85,  1034,
      48,    41,     3,    41,    41,   981,  1046,    41,   963,    41,
    1085,    41,    48,  1053,    43,    40,    40,    48,  1053,  1059,
      48,    48,    48,    48,  1066,  1065,  1066,  1102,    89,    90,
      48,   118,    57,    42,  1139,    48,  1076,    42,    47,    40,
      42,   506,    54,  1083,    41,    54,   142,    48,  1088,   103,
    1125,    41,    89,    90,   115,  1010,    47,  1056,   119,  1134,
    1135,  1136,    62,    42,    89,    90,    41,   128,   105,   106,
     131,    41,  1112,    41,    40,   136,    41,    41,    54,    88,
      89,  1156,    41,  1123,    41,    48,  1085,  1127,    89,    90,
     115,  1046,    48,   558,   119,    48,    48,   562,  1138,   439,
     565,    48,    43,  1102,  1059,   445,     3,    43,    41,  1149,
    1065,    54,  1152,   122,   115,   140,    47,    46,   119,   140,
      47,   586,   587,  1163,  1164,    76,  1125,    76,    76,   125,
    1170,  1171,  1172,  1088,    76,  1134,  1135,  1136,    47,   140,
      48,    48,   151,    40,    47,    41,   104,  1187,    42,    41,
      43,    48,    43,   162,    54,    41,    43,  1156,    48,   168,
     169,    40,   171,   134,   135,   136,   137,   138,     6,   178,
      76,    76,    76,    47,     3,    13,    14,    15,    16,    17,
      18,    19,    20,  1138,    43,    43,    43,    76,    76,    72,
      43,   656,    89,    90,  1149,   204,    43,  1152,   132,   133,
     134,   135,   136,   137,   138,   104,    89,    43,    42,  1164,
      40,    40,  1034,    48,   223,  1170,  1171,  1172,   115,    48,
      48,     3,   119,    49,    50,    51,    41,    43,     3,    43,
     239,  1053,  1187,  1055,  1056,    41,    40,    40,   247,    48,
     249,   250,   251,   140,    41,   254,    48,    47,   713,   258,
     259,   260,   261,    41,    80,    40,    48,    48,    40,    46,
      89,    90,    88,    89,    90,    40,    48,    41,    41,    41,
      41,    48,    41,    48,   549,    43,    43,   742,    48,    54,
     575,   739,    57,  1091,   165,   166,   115,   898,   297,   981,
     119,   582,    41,   740,   739,   628,   739,   839,   307,   308,
     309,    76,   128,   312,   469,   131,    52,    89,    90,   601,
     869,   140,   195,   754,    89,    90,   998,   200,   201,   328,
     997,   330,  1123,   885,   207,   449,   209,   210,    70,   212,
    1013,   186,  1138,   115,  1117,   723,    -1,   119,   347,   348,
     115,    -1,    -1,   352,   119,   228,    -1,    -1,   231,   232,
       3,    -1,    94,   128,   819,   364,   131,    99,   140,   242,
      -1,   136,   371,   828,   169,   140,   249,   707,    -1,   252,
      -1,    -1,   381,   178,    -1,   715,   259,    -1,    -1,    -1,
      -1,    -1,    -1,   266,    -1,   394,    -1,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    48,    -1,    -1,    -1,    -1,
      -1,    -1,   283,   743,   285,   286,   287,   288,   289,   290,
     291,    -1,    -1,    -1,    -1,    -1,   881,   426,   427,   428,
     760,    -1,    -1,   432,    -1,    -1,   435,   436,    -1,   438,
      -1,    -1,   168,   169,     3,   444,    89,    90,   319,    -1,
      -1,    -1,   178,    -1,    -1,    -1,   251,    -1,     3,   254,
      -1,    -1,    -1,   195,    -1,    -1,    -1,    -1,   200,   201,
      -1,   342,   115,    -1,   345,   207,   119,    -1,    -1,    -1,
     212,    40,    -1,    -1,    -1,   128,    -1,    -1,   131,    48,
      -1,    -1,    -1,   823,    -1,    40,   495,   140,    57,   231,
      -1,    -1,    -1,    48,    -1,    -1,    -1,   506,   507,   964,
     242,   243,    -1,   512,    -1,    -1,   248,   249,    -1,    -1,
     252,    -1,    -1,    -1,    -1,    -1,    -1,   982,    -1,   984,
      89,    90,    -1,    -1,   533,    -1,    -1,   536,     3,    -1,
      -1,    -1,    -1,    -1,    89,    90,  1001,    -1,    -1,    -1,
      -1,    -1,   347,   348,    -1,    -1,   115,   352,    -1,   558,
     119,    -1,    -1,   562,    -1,    -1,   565,  1022,    -1,   128,
     115,    -1,   131,    -1,   119,    40,    -1,   136,    -1,    -1,
      -1,   140,   308,   309,    -1,    -1,   312,   586,   587,   588,
      -1,    -1,    57,   592,    -1,   140,    -1,     3,  1053,    -1,
       6,    -1,   473,   474,   475,   476,   477,   478,   479,   480,
     481,   482,   483,   484,   485,   486,   487,   488,   489,    -1,
     619,   347,   348,    -1,    89,    90,    -1,   626,    -1,    -1,
      -1,    -1,    -1,   963,    40,    -1,  1091,   508,    -1,    -1,
     435,   436,    48,   438,    -1,     3,   517,    -1,    -1,    -1,
     115,    -1,    -1,    -1,   119,    -1,    -1,   656,    -1,    -1,
      -1,    -1,    -1,   128,   663,   664,   131,   107,   108,   668,
     543,    -1,    -1,     3,   673,   548,    -1,    -1,    -1,   678,
    1010,    -1,    40,    89,    90,    43,   559,    -1,    -1,    -1,
      48,    -1,   132,   133,   134,   135,   136,   137,   138,    -1,
      -1,    -1,    -1,   576,    -1,   578,    -1,    -1,    -1,   115,
      40,    -1,    -1,   119,   713,    -1,  1046,   512,    48,    -1,
      -1,    -1,   128,    -1,    -1,   131,     3,    -1,    -1,  1059,
     136,    89,    90,    -1,   140,  1065,    -1,    -1,   533,   738,
      -1,   536,    -1,   742,    -1,    -1,   745,    -1,    -1,    -1,
     621,   622,   623,   624,    -1,   754,    -1,   115,  1088,    89,
      90,   119,    -1,    40,    -1,    -1,    43,   493,   494,   495,
     128,    48,    -1,   131,    -1,    -1,   502,   503,   136,    -1,
      -1,    -1,   140,    -1,    -1,   115,   512,    -1,    -1,   119,
      -1,    -1,    -1,   588,    -1,    -1,    -1,   592,   128,   798,
     799,   131,    -1,    -1,    -1,   804,   136,   533,  1138,    -1,
     140,   543,    89,    90,    -1,    -1,   548,   549,    -1,  1149,
     819,    -1,    40,    -1,    -1,   557,   558,   559,    -1,   828,
       3,    49,    50,    51,    -1,    -1,    54,    -1,   115,    -1,
      -1,    -1,   119,    -1,    -1,    -1,   578,    -1,   580,   722,
      -1,   128,    -1,    -1,   131,   587,    -1,    -1,    -1,   136,
      -1,    -1,    80,   140,    -1,    -1,    -1,    40,   663,   664,
      88,    89,    90,   668,    -1,    48,    -1,   748,   673,    -1,
      -1,    -1,   881,   678,    57,    -1,    -1,    -1,    -1,     3,
     889,    -1,    -1,    -1,   107,   108,   109,   110,    -1,    -1,
      -1,   772,   773,   774,    -1,   776,     3,    -1,    -1,    -1,
     128,   782,    -1,   131,    -1,    -1,    89,    90,   131,   132,
     133,   134,   135,   136,   137,   138,    40,   653,    -1,    -1,
      -1,    -1,   931,     3,    48,    -1,    -1,    -1,    -1,   812,
     813,    -1,   115,    40,    -1,    42,   119,   820,    -1,   948,
     745,    48,    -1,   952,   953,   128,   688,    54,   131,    -1,
      57,    -1,   835,   836,   837,   964,    -1,   140,    -1,    -1,
      40,    -1,    -1,    43,    -1,    89,    90,    -1,    48,    76,
      -1,   713,    -1,   982,    -1,   984,    -1,    -1,    -1,   721,
      -1,   723,    89,    90,    -1,    -1,    -1,    -1,   871,    -1,
      -1,   115,  1001,   798,   799,   119,    -1,   104,  1007,   804,
      43,    -1,    -1,    -1,   128,    -1,    -1,   131,   115,    89,
      90,    -1,   119,  1022,    -1,    -1,   140,    -1,   125,    -1,
      -1,   128,    -1,    -1,   131,  1034,    -1,  1036,  1037,   136,
      -1,    -1,    -1,   140,    -1,   115,    -1,    -1,    -1,   119,
      -1,    -1,    -1,    -1,  1053,    -1,    -1,  1056,   128,    -1,
      -1,   131,    -1,    -1,    -1,    -1,   136,   938,   939,    -1,
     140,    -1,    -1,    -1,   107,   108,   109,   110,    -1,   112,
     812,   813,  1081,    -1,    -1,   817,   818,    -1,   820,    -1,
      -1,    -1,  1091,    -1,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,    -1,    -1,    -1,    -1,
      -1,   843,    -1,     1,    -1,     3,     4,    -1,  1117,    -1,
       8,     9,    10,    -1,   997,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    -1,   948,    -1,    43,    44,   952,   953,    47,
      -1,    49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    -1,    77,
      78,    79,  1063,    81,    82,    83,    84,    85,    86,    87,
      -1,    89,    90,    91,    -1,    -1,    -1,    95,    -1,    97,
      98,    99,   100,   101,   102,    -1,    -1,     3,  1089,    -1,
       6,    -1,    -1,    -1,    -1,   113,    -1,   115,    -1,    -1,
     118,   119,   120,   965,    -1,    -1,   124,    -1,    -1,    -1,
      -1,  1036,  1037,    -1,    -1,     3,    -1,    -1,     6,     7,
      -1,    -1,   140,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    89,    90,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    -1,    -1,    -1,    -1,   105,
     106,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,   115,
      -1,    89,    90,   119,    -1,     3,    -1,    -1,    -1,    -1,
      -1,    -1,   128,    -1,    -1,   131,    -1,    -1,    -1,    -1,
     136,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,   124,    -1,    -1,    -1,
     128,    -1,    40,   131,    -1,    -1,   134,   135,   136,    -1,
      48,     3,   140,   141,     6,     7,    -1,    -1,    -1,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    40,    -1,
      -1,    89,    90,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    53,    -1,    55,    -1,    -1,    58,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,    70,    71,
      72,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     128,    -1,    -1,   131,    86,    -1,    -1,    89,    90,    -1,
      -1,     3,   140,    -1,     6,    -1,    -1,    -1,    -1,    -1,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   128,    -1,    -1,   131,
      -1,    -1,   134,   135,   136,    -1,    -1,     3,   140,   141,
       6,     7,    41,    -1,    -1,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    40,    -1,    -1,    89,    90,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    -1,    55,
      -1,    -1,    58,   105,   106,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    -1,   107,   108,
     109,   110,    -1,   112,    -1,   107,   108,   109,   110,    -1,
      86,    -1,    -1,    89,    90,    -1,    -1,    -1,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
     132,   133,   134,   135,   136,   137,   138,    -1,    -1,   115,
      -1,    -1,    -1,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   128,    -1,    -1,   131,    -1,    -1,   134,   135,
     136,    -1,    -1,     3,   140,   141,     6,     7,    41,    -1,
      -1,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    -1,    55,    -1,    -1,    58,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    -1,   107,   108,   109,   110,    -1,   112,
      -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,    89,
      90,    -1,    -1,    -1,   127,   128,   129,   130,   131,   132,
     133,   134,   135,   136,   137,   138,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,   115,    31,    -1,    33,   119,
      -1,    36,    37,    38,    39,    -1,    -1,    -1,   128,    -1,
      -1,   131,    -1,    -1,   134,   135,   136,    -1,    -1,     3,
     140,   141,     6,     7,    -1,    -1,    -1,    11,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    40,    -1,    76,    -1,
      -1,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
      -1,    55,    -1,    -1,    58,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,   107,
     108,   109,   110,    -1,   112,    -1,    -1,    -1,   107,   108,
     109,   110,    86,    -1,    -1,    89,    90,    -1,    -1,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,   130,   131,   132,   133,   134,   135,   136,   137,   138,
      -1,   115,    -1,    -1,    -1,   119,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   128,    -1,    -1,   131,    -1,    -1,
     134,   135,   136,    -1,    -1,     3,   140,   141,     6,     7,
      -1,    -1,    -1,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    40,    41,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    49,    50,    51,    52,    53,    -1,    55,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,   107,   108,   109,   110,    -1,
     112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,
      -1,    89,    90,    -1,    -1,   127,   128,   129,   130,   131,
     132,   133,   134,   135,   136,   137,   138,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,
      -1,   119,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     128,    -1,    -1,   131,    -1,    -1,   134,   135,   136,    -1,
      -1,     3,   140,   141,     6,     7,    -1,    -1,    -1,    11,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    40,    41,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    53,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,   107,   108,   109,   110,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    86,    -1,    -1,    89,    90,    -1,
      -1,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   128,    -1,    -1,   131,
      -1,    -1,   134,   135,   136,    -1,    -1,     3,   140,   141,
       6,     7,    -1,    -1,    -1,    11,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    49,    50,    51,    52,    53,    -1,    55,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,   107,   108,   109,
     110,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      86,    -1,    -1,    89,    90,    -1,    -1,    -1,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,
      -1,    -1,    -1,   119,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   128,    -1,    -1,   131,    -1,    -1,   134,   135,
     136,    -1,    -1,     3,   140,   141,     6,     7,    -1,    -1,
      -1,    11,    12,    13,    14,    15,    16,    17,    18,    19,
      20,     3,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    37,    38,    39,
      40,    -1,    -1,     3,    -1,    -1,    -1,    -1,    -1,    49,
      50,    51,    52,    53,    -1,    55,    -1,     3,    40,    -1,
      -1,    43,    -1,    -1,    -1,    -1,    48,    -1,    -1,    -1,
      70,    71,    72,    -1,    -1,     3,    -1,     3,    -1,    -1,
      40,    -1,    -1,    -1,    -1,    -1,    86,    -1,    48,    89,
      90,    -1,    -1,    -1,    40,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    -1,    -1,    -1,    -1,    89,    90,    -1,
      -1,    -1,    40,    -1,    40,   115,    -1,    -1,    -1,   119,
      48,    -1,    48,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      90,    57,    -1,   115,   134,   135,   136,   119,    -1,    -1,
     140,   141,    -1,    89,    90,    -1,   128,    -1,    -1,   131,
      -1,    -1,    -1,    -1,   136,   115,    -1,    -1,   140,   119,
      -1,    89,    90,    89,    90,    -1,    -1,    -1,   128,   115,
      -1,   131,    -1,   119,    -1,    -1,   136,    -1,     4,    -1,
     140,    -1,   128,    -1,    10,   131,    -1,   115,    -1,   115,
     136,   119,    -1,   119,   140,    21,    -1,    -1,    -1,    -1,
     128,    -1,   128,   131,    -1,   131,    -1,    -1,   136,    -1,
      -1,    -1,   140,    -1,   140,    -1,    -1,    43,    44,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    62,    63,    64,    65,
      -1,    67,    68,    69,    -1,    -1,    -1,    73,    74,    75,
      -1,    77,    78,    79,    -1,    81,    82,    83,    84,    85,
      -1,    -1,    -1,    -1,    -1,    91,    -1,    -1,    -1,    95,
      -1,    97,    98,    99,   100,   101,   102,    -1,    -1,    -1,
      -1,    -1,    -1,     3,    -1,    -1,    -1,   113,    -1,    -1,
      -1,    -1,   118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,   140,    35,    36,    37,    38,    39,
      40,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    48,    49,
      50,    51,    52,    53,    -1,    -1,    -1,    57,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,   107,   108,   109,   110,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    86,    -1,    -1,    89,
      90,    -1,    -1,    -1,    -1,   129,   130,   131,   132,   133,
     134,   135,   136,   137,   138,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,   118,   119,
       3,    -1,    -1,    -1,   124,    -1,    -1,    -1,   128,    -1,
      -1,   131,    -1,    -1,    -1,    -1,   136,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    39,    40,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    48,    49,    50,    51,    52,
      53,    -1,    -1,    -1,    57,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,    -1,    -1,   118,   119,     3,    -1,    58,
      -1,   124,    -1,    -1,    -1,   128,    -1,    -1,   131,    -1,
      -1,    -1,    -1,   136,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    48,    49,    50,    51,    52,    53,   107,   108,
     109,   110,    -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,     3,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
      86,    -1,    -1,    89,    90,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,   115,
      -1,    -1,   118,   119,    -1,    -1,    52,    53,   124,    -1,
      -1,    -1,   128,    -1,    -1,   131,    -1,    -1,    -1,    -1,
     136,    -1,    -1,    -1,    70,    71,    72,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      86,    -1,    -1,    89,    90,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     0,     1,    -1,     3,
       4,    -1,    -1,    -1,     8,     9,    10,    -1,    -1,   115,
      -1,    -1,    -1,   119,    -1,    -1,    -1,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
     136,    35,    36,    37,    38,    39,    -1,    -1,    -1,    43,
      44,    -1,    -1,    -1,    -1,    49,    50,    51,    52,    53,
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
      -1,    -1,    43,    44,    -1,    -1,    47,    -1,    49,    50,
      51,    52,    53,    -1,    -1,    56,    -1,    -1,    -1,    -1,
      -1,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    -1,    -1,    -1,    -1,    77,    78,    79,    -1,
      81,    82,    83,    84,    85,    86,    87,    -1,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,     1,   115,     3,     4,   118,   119,    -1,
       8,     9,    10,   124,   125,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    43,    44,    -1,    -1,    47,
      -1,    49,    50,    51,    52,    53,    -1,    -1,    56,    -1,
      -1,    -1,    -1,    -1,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    -1,    -1,    -1,    -1,    77,
      78,    79,    -1,    81,    82,    83,    84,    85,    86,    87,
      -1,    89,    90,    91,    92,    93,    94,    95,    96,    97,
      98,    99,   100,   101,   102,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   113,     1,   115,     3,     4,
     118,   119,    -1,     8,     9,    10,   124,   125,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    43,    44,
      -1,    -1,    47,    -1,    49,    50,    51,    52,    53,    -1,
      -1,    56,    -1,    -1,    -1,    -1,    -1,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    -1,    -1,
      -1,    -1,    77,    78,    79,    -1,    81,    82,    83,    84,
      85,    86,    87,    -1,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,     1,
     115,     3,     4,   118,   119,    -1,     8,     9,    10,   124,
     125,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    43,    44,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    53,    -1,    -1,    56,    -1,    -1,    -1,    60,    -1,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    -1,    77,    78,    79,    -1,    81,
      82,    83,    84,    85,    86,    87,    -1,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,    -1,    -1,     1,    -1,     3,    -1,    58,    -1,    -1,
      -1,   113,    -1,   115,    -1,    -1,   118,   119,    -1,    -1,
      -1,    -1,   124,   125,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    50,    51,    52,    53,   107,   108,   109,   110,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    -1,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,    86,    -1,
      -1,    89,    90,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,     3,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,   124,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    49,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,    -1,
       3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,     3,    35,    36,    37,    38,    39,    -1,    -1,    -1,
     115,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,   124,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    48,    49,    50,    51,
      52,    53,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,    -1,    86,    -1,   119,    89,    90,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,    -1,    58,    -1,    -1,
      -1,    -1,    -1,   115,    -1,    13,   118,   119,    -1,    -1,
      -1,    -1,   124,   125,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    37,
      38,    39,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      48,    49,    50,    51,    52,    53,   107,   108,   109,   110,
      -1,   112,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    70,    71,    72,    -1,   127,   128,   129,   130,
     131,   132,   133,   134,   135,   136,   137,   138,    86,    -1,
      -1,    89,    90,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,    -1,
      58,    -1,    -1,    -1,    -1,    -1,    -1,   115,    -1,    -1,
     118,   119,    -1,    -1,    -1,    -1,   124,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    48,    49,    50,    51,    52,    53,   107,
     108,   109,   110,    -1,   112,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,   127,
     128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
     138,    86,    -1,    -1,    89,    90,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,
      -1,     3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,    -1,    -1,   118,   119,    -1,    -1,    -1,    -1,   124,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    -1,    35,    36,    37,    38,    39,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    49,    50,    51,
      52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,
      72,    -1,     3,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    86,    -1,    -1,    89,    90,    -1,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    -1,   115,    -1,    -1,    -1,   119,    49,    50,
      51,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,
      71,    72,    -1,    -1,     3,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    86,    87,    -1,    89,    90,
      -1,    -1,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    -1,    35,    36,    37,    38,
      39,    -1,    -1,    -1,   115,    -1,    -1,    -1,   119,   120,
      49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    70,    71,    72,    -1,    -1,    -1,     3,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    86,    87,    -1,
      89,    90,    -1,    -1,    -1,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    37,    38,    39,    -1,    -1,   115,    -1,    -1,    -1,
     119,   120,    48,    49,    50,    51,    52,    53,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    70,    71,    72,    -1,     3,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      86,    -1,    -1,    89,    90,    -1,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    37,    38,    39,    40,    -1,    -1,    -1,   115,
      -1,    -1,   118,   119,    49,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,     3,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    86,    -1,    -1,    89,    90,    -1,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    -1,
     115,    -1,    -1,    -1,   119,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,    -1,
       3,    -1,    58,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    86,    87,    -1,    89,    90,    -1,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      -1,   115,    -1,    -1,    -1,   119,    49,    50,    51,    52,
      53,   107,   108,   109,   110,    -1,   112,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
       3,   127,   128,   129,   130,   131,   132,   133,   134,   135,
     136,   137,   138,    86,    -1,    -1,    89,    90,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      -1,    -1,   115,    -1,    -1,    -1,   119,    -1,    -1,    52,
      53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    70,    71,    72,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    86,    -1,    -1,    89,    90,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,    -1,    -1,    -1,   119
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   121,   122,   123,   144,   145,   324,     1,     3,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    35,    36,    37,    38,    39,    48,    49,    50,    51,
      52,    53,    70,    71,    72,    86,    89,    90,   115,   118,
     119,   124,   195,   239,   240,   256,   257,   259,   260,   261,
     262,   263,   264,   293,   294,   308,   311,   313,     1,   240,
       1,    40,     0,     1,     4,     8,     9,    10,    21,    43,
      44,    56,    62,    63,    64,    65,    66,    67,    68,    69,
      77,    78,    79,    81,    82,    83,    84,    85,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     113,   118,   124,   125,   146,   147,   148,   150,   151,   152,
     153,   154,   157,   158,   160,   161,   162,   163,   164,   165,
     166,   169,   170,   171,   174,   176,   181,   182,   183,   184,
     186,   190,   197,   198,   199,   200,   201,   205,   206,   213,
     214,   226,   227,   234,   235,   324,    48,    52,    71,    48,
      48,    40,   142,   103,   103,   307,   239,   311,   125,    43,
     260,   256,    40,    48,    54,    57,    76,   119,   128,   131,
     136,   140,   245,   246,   248,   250,   251,   252,   253,   311,
     324,   256,   263,   311,   307,   117,   142,   312,    43,    43,
     236,   237,   240,   324,   120,    40,     6,    85,   118,   317,
      40,   320,   324,     1,   258,   259,   308,    40,   320,    40,
     168,   324,    40,    40,    84,    85,    40,    84,    40,    77,
      82,    44,    77,    92,   311,    46,   308,   311,    40,     4,
      46,    40,    40,    43,    46,     4,   317,    40,   180,   258,
     178,   180,    40,    40,   317,    40,   103,   294,   320,    40,
     128,   131,   248,   250,   253,   311,    21,    85,    87,   195,
     258,   294,    48,    48,    48,   311,   118,   119,   313,   314,
     294,     3,     7,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    40,    55,   128,   131,   134,   135,   136,
     140,   141,   240,   241,   242,   244,   258,   259,   278,   279,
     280,   281,   282,   317,   318,   324,   256,    40,   128,   131,
     236,   251,   253,   311,    48,    46,   105,   106,   265,   266,
     267,   268,   269,    58,   278,   280,   278,     3,    40,    48,
     140,   249,   252,   311,    48,   249,   252,   253,   256,   311,
     245,    40,    57,   245,    40,    57,    48,   128,   131,   249,
     252,   311,   116,   313,   119,   314,    41,    42,   238,   324,
     267,   308,   309,   317,   294,     6,    46,   309,   322,   309,
      43,    40,   248,   250,    54,    41,   309,    52,    53,    71,
     295,   296,   324,   308,   308,   309,    13,   175,   236,   236,
     311,    43,    54,   216,    54,    46,   308,   177,   322,   308,
     236,    46,   247,   248,   250,   251,   324,    43,    42,   179,
     324,   309,   310,   324,   155,   156,   317,   236,   209,   210,
     211,   240,   293,   324,   311,   317,   128,   131,   253,   308,
     311,   322,    40,   309,    40,   128,   131,   311,   116,   248,
     311,   270,   308,   324,    40,   248,    76,   285,   286,   311,
     324,    48,    48,    41,   308,   312,   104,   111,   278,    40,
      48,   278,   278,   278,   278,   278,   278,   278,   104,    42,
     243,   324,    40,   107,   108,   109,   110,   112,   127,   128,
     129,   130,   131,   132,   133,   134,   135,   136,   137,   138,
      48,   111,     7,   128,   131,   253,   311,   250,   311,   250,
      41,    41,   128,   131,   250,   311,   116,    48,    57,   278,
      58,    40,   253,   311,    48,   311,    40,    57,    48,   253,
     236,    58,   278,   236,    58,   278,    48,    48,   249,   252,
      48,   249,   252,   116,    48,   128,   131,   249,   256,   312,
      43,   125,   240,    42,   321,   324,   311,   185,    42,    54,
      41,   245,    40,   265,    41,   311,    41,    54,    41,    42,
     173,    42,    41,    41,    43,   258,   145,   311,   215,    41,
      41,    41,    41,   178,    40,   180,    41,    41,    42,    46,
      41,   104,    42,   212,   324,    59,   116,    41,   253,   311,
      43,   236,   116,    80,    88,    89,    90,   128,   131,   254,
     255,   256,   298,   300,   301,   302,   324,    54,    76,   196,
     324,   236,   311,   302,   287,    46,    43,   285,   307,   294,
       3,    41,   128,   131,   136,   253,   258,    48,   244,   278,
     278,   278,   278,   278,   278,   278,   278,   278,   278,   278,
     278,   278,   278,   278,   278,   278,     3,     3,   311,   116,
      41,    41,    41,   116,   248,   251,   256,   250,   278,   236,
     249,   311,    41,   116,    48,   236,    58,   278,    48,    41,
      58,    41,    58,    48,    48,    48,    48,   128,   131,   249,
     252,    48,    48,    48,   249,   240,   238,   322,    41,   145,
     322,   155,   281,   317,   323,    43,   236,    43,    46,     4,
     167,   317,     4,    43,    46,   114,   172,   248,   317,   319,
     309,   317,   323,    41,   240,   248,    46,   247,    47,    43,
     145,    44,   235,   178,    43,    46,    40,    47,   236,   179,
     115,   119,   308,   315,    43,   322,   240,   172,    91,   207,
     211,   159,   256,   248,   317,   116,    41,    40,    40,   298,
      90,    89,   300,   255,   111,    57,   191,   260,    43,    46,
      41,   188,   245,    78,   288,   289,   297,   324,   203,    46,
     311,   278,    41,    41,   136,   256,    41,   128,   131,   246,
      48,   243,    76,    41,    41,   248,   251,    58,    41,    41,
     249,   249,    41,    58,   249,   254,   254,   249,    48,    48,
      48,    48,    48,   249,    48,    48,    48,   238,     4,    46,
     317,    47,    42,    42,    41,   149,    40,   302,    54,    41,
      42,   173,   172,   248,   302,    43,    47,   317,   258,   308,
      43,    54,   319,   236,    41,   142,   117,   142,   316,   103,
      41,    47,   311,    44,   118,   186,   201,   205,   206,   208,
     223,   225,   235,   212,   145,   302,    43,   236,   278,    30,
      32,    35,   189,   261,   262,   311,    40,    46,   192,   152,
     271,   272,   273,   274,   324,    40,    54,   300,   302,   303,
       1,    42,    43,    46,   187,    42,    73,    74,    75,   290,
     292,     1,    43,    66,    73,    74,    75,    78,   124,   140,
     150,   151,   152,   153,   157,   158,   162,   164,   166,   169,
     171,   174,   176,   181,   182,   183,   184,   201,   205,   206,
     213,   217,   221,   222,   223,   224,   225,   226,   228,   229,
     232,   235,   324,   202,   245,   278,   278,   278,    41,    41,
      41,    40,   278,    41,    41,    41,   249,   249,    48,    48,
      48,   249,    48,    48,   322,   322,   254,   217,   236,   172,
     317,   323,    43,   248,    41,   302,    43,   248,    43,   180,
      41,   254,   119,   308,   308,   119,   308,   241,     4,    46,
      54,   103,    87,   120,   258,    60,    43,    41,    41,   298,
     299,   324,   236,    40,    43,   193,   271,   124,   275,   276,
     308,    47,    42,   125,   236,   265,    54,    76,   304,   324,
     248,   289,   311,   291,   220,    46,    76,    76,    76,   140,
     221,   313,    47,   125,   217,    30,    32,    35,   233,   262,
     311,   217,   278,   278,   258,   249,    48,    48,   249,   249,
     245,    47,    41,   173,   302,    43,   248,   172,    43,    43,
     316,   316,   104,   258,   209,   258,    40,   298,   188,    41,
     194,   276,   272,    54,    43,   248,   125,   273,    41,    43,
     267,   305,   306,   311,    43,    46,   302,    48,   283,   284,
     324,   297,   217,   218,   313,    40,    43,   204,   248,    76,
      43,    47,   246,   249,   249,    43,    43,    43,   302,    43,
     247,   104,    40,   128,   131,   253,   236,   187,   302,    43,
     277,   278,   302,   273,    43,    46,    43,    42,    48,    40,
      46,   188,    48,   311,   217,    40,   236,   302,   278,   204,
      41,    43,    43,   236,    40,    40,    40,   131,    41,   111,
     192,   188,   306,    48,   187,    48,   284,    47,   236,    41,
     188,    43,    41,   236,   236,   236,    40,   303,   258,   193,
     187,    48,    48,   219,    41,   230,   302,   187,   231,   302,
      41,    41,    41,   236,   192,    48,   217,   231,    43,    46,
      54,    43,    46,    54,   231,   231,   231,    41,   193,    48,
     267,   265,   231,    43,    43
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int16 yyr1[] =
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
     271,   271,   272,   272,   273,   273,   273,   273,   273,   274,
     275,   275,   276,   276,   277,   278,   278,   279,   279,   279,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   281,
     281,   281,   281,   281,   281,   281,   281,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   282,
     282,   282,   282,   282,   282,   282,   282,   282,   282,   283,
     284,   284,   285,   287,   286,   286,   288,   288,   290,   289,
     291,   289,   292,   292,   292,   293,   293,   293,   293,   294,
     294,   294,   295,   295,   295,   296,   296,   297,   297,   298,
     298,   298,   298,   299,   299,   300,   300,   300,   300,   300,
     300,   301,   301,   301,   302,   302,   303,   303,   303,   303,
     303,   303,   304,   304,   305,   305,   305,   305,   306,   306,
     307,   308,   308,   308,   309,   309,   309,   310,   310,   311,
     311,   311,   311,   311,   311,   311,   312,   312,   312,   312,
     313,   313,   314,   314,   315,   315,   315,   315,   315,   315,
     316,   316,   316,   316,   317,   317,   318,   318,   319,   319,
     319,   320,   320,   321,   321,   322,   322,   322,   322,   322,
     322,   323,   323,   324
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     3,     2,     3,     2,     5,     3,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     8,     5,     3,     5,     5,     9,     3,     2,
       2,     5,     2,     5,     2,     4,     1,     1,     7,     7,
       5,     0,     7,     1,     1,     2,     2,     1,     6,     6,
       6,     3,     4,     3,     7,     8,     5,     3,     1,     1,
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
       1,     2,     1,     1,     1,     2,     3,     4,     1,     3,
       1,     2,     1,     3,     1,     1,     1,     3,     3,     3,
       1,     1,     1,     5,     8,     1,     1,     1,     1,     3,
       4,     5,     5,     5,     6,     6,     2,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     5,     2,     2,     2,     2,     2,     3,
       1,     1,     1,     0,     3,     1,     1,     3,     0,     4,
       0,     6,     1,     1,     1,     1,     1,     4,     4,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     1,     1,     4,     1,     1,     5,     2,
       4,     1,     1,     2,     1,     1,     3,     3,     4,     4,
       3,     4,     2,     1,     1,     3,     4,     6,     2,     2,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       4,     1,     3,     1,     2,     3,     3,     2,     2,     2,
       1,     2,     1,     3,     2,     4,     1,     3,     1,     3,
       3,     2,     2,     2,     2,     1,     2,     1,     1,     1,
       1,     3,     1,     2,     1,     3,     5,     1,     3,     3,
       5,     1,     1,     0
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
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
| yyreduce -- do a reduction.  |
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
  case 2: /* program: interface  */
#line 1713 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 4708 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 3: /* program: PARSETYPE parm SEMI  */
#line 1725 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     {
                 top = Copy(Getattr((yyvsp[-1].p),"type"));
		 Delete((yyvsp[-1].p));
               }
#line 4717 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 4: /* program: PARSETYPE error  */
#line 1729 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
                 top = 0;
               }
#line 4725 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 5: /* program: PARSEPARM parm SEMI  */
#line 1732 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     {
                 top = (yyvsp[-1].p);
               }
#line 4733 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 6: /* program: PARSEPARM error  */
#line 1735 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
                 top = 0;
               }
#line 4741 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 7: /* program: PARSEPARMS LPAREN parms RPAREN SEMI  */
#line 1738 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                     {
                 top = (yyvsp[-2].pl);
               }
#line 4749 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 8: /* program: PARSEPARMS error SEMI  */
#line 1741 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
                 top = 0;
               }
#line 4757 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 9: /* interface: interface declaration  */
#line 1746 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {  
                   /* add declaration to end of linked list (the declaration isn't always a single declaration, sometimes it is a linked list itself) */
                   if (currentDeclComment != NULL) {
		     set_comment((yyvsp[0].node), currentDeclComment);
		     currentDeclComment = NULL;
                   }                                      
                   appendChild((yyvsp[-1].node),(yyvsp[0].node));
                   (yyval.node) = (yyvsp[-1].node);
               }
#line 4771 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 10: /* interface: interface DOXYGENSTRING  */
#line 1755 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
                   currentDeclComment = (yyvsp[0].str); 
                   (yyval.node) = (yyvsp[-1].node);
               }
#line 4780 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 11: /* interface: interface DOXYGENPOSTSTRING  */
#line 1759 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                             {
                   Node *node = lastChild((yyvsp[-1].node));
                   if (node) {
                     set_comment(node, (yyvsp[0].str));
                   }
                   (yyval.node) = (yyvsp[-1].node);
               }
#line 4792 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 12: /* interface: empty  */
#line 1766 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                   (yyval.node) = new_node("top");
               }
#line 4800 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 13: /* declaration: swig_directive  */
#line 1771 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 4806 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 14: /* declaration: c_declaration  */
#line 1772 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.node) = (yyvsp[0].node); }
#line 4812 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 15: /* declaration: cpp_declaration  */
#line 1773 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 4818 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 16: /* declaration: SEMI  */
#line 1774 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                      { (yyval.node) = 0; }
#line 4824 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 17: /* declaration: error  */
#line 1775 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                  (yyval.node) = 0;
		  if (cparse_unknown_directive) {
		      Swig_error(cparse_file, cparse_line, "Unknown directive '%s'.\n", cparse_unknown_directive);
		  } else {
		      Swig_error(cparse_file, cparse_line, "Syntax error in input(1).\n");
		  }
		  exit(1);
               }
#line 4838 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 18: /* declaration: c_constructor_decl  */
#line 1785 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { 
                  if ((yyval.node)) {
   		      add_symbols((yyval.node));
                  }
                  (yyval.node) = (yyvsp[0].node); 
	       }
#line 4849 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 19: /* declaration: error CONVERSIONOPERATOR  */
#line 1801 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                          {
                  (yyval.node) = 0;
                  skip_decl();
               }
#line 4858 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 20: /* swig_directive: extend_directive  */
#line 1811 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4864 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 21: /* swig_directive: apply_directive  */
#line 1812 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 4870 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 22: /* swig_directive: clear_directive  */
#line 1813 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 4876 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 23: /* swig_directive: constant_directive  */
#line 1814 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 4882 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 24: /* swig_directive: echo_directive  */
#line 1815 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 4888 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 25: /* swig_directive: except_directive  */
#line 1816 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4894 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 26: /* swig_directive: fragment_directive  */
#line 1817 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 4900 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 27: /* swig_directive: include_directive  */
#line 1818 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 4906 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 28: /* swig_directive: inline_directive  */
#line 1819 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4912 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 29: /* swig_directive: insert_directive  */
#line 1820 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4918 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 30: /* swig_directive: module_directive  */
#line 1821 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4924 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 31: /* swig_directive: name_directive  */
#line 1822 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 4930 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 32: /* swig_directive: native_directive  */
#line 1823 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4936 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 33: /* swig_directive: pragma_directive  */
#line 1824 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4942 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 34: /* swig_directive: rename_directive  */
#line 1825 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4948 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 35: /* swig_directive: feature_directive  */
#line 1826 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 4954 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 36: /* swig_directive: varargs_directive  */
#line 1827 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 4960 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 37: /* swig_directive: typemap_directive  */
#line 1828 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 4966 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 38: /* swig_directive: types_directive  */
#line 1829 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 4972 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 39: /* swig_directive: template_directive  */
#line 1830 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 4978 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 40: /* swig_directive: warn_directive  */
#line 1831 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 4984 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 41: /* $@1: %empty  */
#line 1838 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5030 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 42: /* extend_directive: EXTEND options classkeyopt idcolon LBRACE $@1 cpp_members RBRACE  */
#line 1878 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5071 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 43: /* apply_directive: APPLY typemap_parm LBRACE tm_list RBRACE  */
#line 1920 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                           {
                    (yyval.node) = new_node("apply");
                    Setattr((yyval.node),"pattern",Getattr((yyvsp[-3].p),"pattern"));
		    appendChild((yyval.node),(yyvsp[-1].p));
               }
#line 5081 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 44: /* clear_directive: CLEAR tm_list SEMI  */
#line 1930 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     {
		 (yyval.node) = new_node("clear");
		 appendChild((yyval.node),(yyvsp[-1].p));
               }
#line 5090 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 45: /* constant_directive: CONSTANT identifier EQUAL definetype SEMI  */
#line 1941 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5115 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 46: /* constant_directive: CONSTANT type declarator def_args SEMI  */
#line 1961 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5142 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 47: /* constant_directive: CONSTANT type direct_declarator LPAREN parms RPAREN cv_ref_qualifier def_args SEMI  */
#line 1985 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5171 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 48: /* constant_directive: CONSTANT error SEMI  */
#line 2009 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     {
		 Swig_warning(WARN_PARSE_BAD_VALUE,cparse_file,cparse_line,"Bad constant value (ignored).\n");
		 (yyval.node) = 0;
	       }
#line 5180 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 49: /* echo_directive: ECHO HBLOCK  */
#line 2020 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		 char temp[64];
		 Replace((yyvsp[0].str),"$file",cparse_file, DOH_REPLACE_ANY);
		 sprintf(temp,"%d", cparse_line);
		 Replace((yyvsp[0].str),"$line",temp,DOH_REPLACE_ANY);
		 Printf(stderr,"%s\n", (yyvsp[0].str));
		 Delete((yyvsp[0].str));
                 (yyval.node) = 0;
	       }
#line 5194 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 50: /* echo_directive: ECHO string  */
#line 2029 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5209 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 51: /* except_directive: EXCEPT LPAREN identifier RPAREN LBRACE  */
#line 2048 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                          {
                    skip_balanced('{','}');
		    (yyval.node) = 0;
		    Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
	       }
#line 5219 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 52: /* except_directive: EXCEPT LBRACE  */
#line 2054 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
                    skip_balanced('{','}');
		    (yyval.node) = 0;
		    Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
               }
#line 5229 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 53: /* except_directive: EXCEPT LPAREN identifier RPAREN SEMI  */
#line 2060 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                      {
		 (yyval.node) = 0;
		 Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
               }
#line 5238 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 54: /* except_directive: EXCEPT SEMI  */
#line 2065 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		 (yyval.node) = 0;
		 Swig_warning(WARN_DEPRECATED_EXCEPT,cparse_file, cparse_line, "%%except is deprecated.  Use %%exception instead.\n");
	       }
#line 5247 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 55: /* stringtype: string LBRACE parm RBRACE  */
#line 2072 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                          {		 
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"value",(yyvsp[-3].str));
		 Setattr((yyval.node),"type",Getattr((yyvsp[-1].p),"type"));
               }
#line 5257 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 56: /* fname: string  */
#line 2079 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"value",(yyvsp[0].str));
              }
#line 5266 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 57: /* fname: stringtype  */
#line 2083 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
                (yyval.node) = (yyvsp[0].node);
              }
#line 5274 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 58: /* fragment_directive: FRAGMENT LPAREN fname COMMA kwargs RPAREN HBLOCK  */
#line 2096 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                     {
                   Hash *p = (yyvsp[-2].node);
		   (yyval.node) = new_node("fragment");
		   Setattr((yyval.node),"value",Getattr((yyvsp[-4].node),"value"));
		   Setattr((yyval.node),"type",Getattr((yyvsp[-4].node),"type"));
		   Setattr((yyval.node),"section",Getattr(p,"name"));
		   Setattr((yyval.node),"kwargs",nextSibling(p));
		   Setattr((yyval.node),"code",(yyvsp[0].str));
                 }
#line 5288 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 59: /* fragment_directive: FRAGMENT LPAREN fname COMMA kwargs RPAREN LBRACE  */
#line 2105 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5308 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 60: /* fragment_directive: FRAGMENT LPAREN fname RPAREN SEMI  */
#line 2120 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                     {
		   (yyval.node) = new_node("fragment");
		   Setattr((yyval.node),"value",Getattr((yyvsp[-2].node),"value"));
		   Setattr((yyval.node),"type",Getattr((yyvsp[-2].node),"type"));
		   Setattr((yyval.node),"emitonly","1");
		 }
#line 5319 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 61: /* $@2: %empty  */
#line 2133 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5334 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 62: /* include_directive: includetype options string BEGINFILE $@2 interface ENDOFFILE  */
#line 2142 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5384 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 63: /* includetype: INCLUDE  */
#line 2189 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.loc).type = "include"; }
#line 5390 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 64: /* includetype: IMPORT  */
#line 2190 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.loc).type = "import"; ++import_mode;}
#line 5396 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 65: /* inline_directive: INLINE HBLOCK  */
#line 2197 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5420 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 66: /* inline_directive: INLINE LBRACE  */
#line 2216 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5446 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 67: /* insert_directive: HBLOCK  */
#line 2247 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"code",(yyvsp[0].str));
	       }
#line 5455 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 68: /* insert_directive: INSERT LPAREN idstring options_ex RPAREN string  */
#line 2251 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
		 String *code = NewStringEmpty();
		 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"section",(yyvsp[-3].id));
		 Setattr((yyval.node),"code",code);
		 Setattr((yyval.node),"options",(yyvsp[-2].node));
		 if (Swig_insert_file((yyvsp[0].str),code) < 0) {
		   Swig_error(cparse_file, cparse_line, "Couldn't find '%s'.\n", (yyvsp[0].str));
		   (yyval.node) = 0;
		 } 
               }
#line 5471 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 69: /* insert_directive: INSERT LPAREN idstring options_ex RPAREN HBLOCK  */
#line 2262 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
		 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"section",(yyvsp[-3].id));
		 Setattr((yyval.node),"options",(yyvsp[-2].node));
		 Setattr((yyval.node),"code",(yyvsp[0].str));
               }
#line 5482 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 70: /* insert_directive: INSERT LPAREN idstring options_ex RPAREN LBRACE  */
#line 2268 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
		 String *code;
                 skip_balanced('{','}');
		 (yyval.node) = new_node("insert");
		 Setattr((yyval.node),"section",(yyvsp[-3].id));
		 Setattr((yyval.node),"options",(yyvsp[-2].node));
		 Delitem(scanner_ccode,0);
		 Delitem(scanner_ccode,DOH_END);
		 code = Copy(scanner_ccode);
		 Setattr((yyval.node),"code", code);
		 Delete(code);
	       }
#line 5499 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 71: /* module_directive: MODULE options idstring  */
#line 2287 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5540 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 72: /* name_directive: NAME LPAREN idstring RPAREN  */
#line 2330 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                             {
                 Swig_warning(WARN_DEPRECATED_NAME,cparse_file,cparse_line, "%%name is deprecated.  Use %%rename instead.\n");
		 Delete(yyrename);
                 yyrename = NewString((yyvsp[-1].id));
		 (yyval.node) = 0;
               }
#line 5551 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 73: /* name_directive: NAME LPAREN RPAREN  */
#line 2336 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    {
		 Swig_warning(WARN_DEPRECATED_NAME,cparse_file,cparse_line, "%%name is deprecated.  Use %%rename instead.\n");
		 (yyval.node) = 0;
		 Swig_error(cparse_file,cparse_line,"Missing argument to %%name directive.\n");
	       }
#line 5561 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 74: /* native_directive: NATIVE LPAREN identifier RPAREN storage_class identifier SEMI  */
#line 2349 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                 {
                 (yyval.node) = new_node("native");
		 Setattr((yyval.node),"name",(yyvsp[-4].id));
		 Setattr((yyval.node),"wrap:name",(yyvsp[-1].id));
	         add_symbols((yyval.node));
	       }
#line 5572 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 75: /* native_directive: NATIVE LPAREN identifier RPAREN storage_class type declarator SEMI  */
#line 2355 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5594 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 76: /* pragma_directive: PRAGMA pragma_lang identifier EQUAL pragma_arg  */
#line 2381 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                  {
                 (yyval.node) = new_node("pragma");
		 Setattr((yyval.node),"lang",(yyvsp[-3].id));
		 Setattr((yyval.node),"name",(yyvsp[-2].id));
		 Setattr((yyval.node),"value",(yyvsp[0].str));
	       }
#line 5605 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 77: /* pragma_directive: PRAGMA pragma_lang identifier  */
#line 2387 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                              {
		(yyval.node) = new_node("pragma");
		Setattr((yyval.node),"lang",(yyvsp[-1].id));
		Setattr((yyval.node),"name",(yyvsp[0].id));
	      }
#line 5615 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 78: /* pragma_arg: string  */
#line 2394 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.str) = (yyvsp[0].str); }
#line 5621 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 79: /* pragma_arg: HBLOCK  */
#line 2395 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.str) = (yyvsp[0].str); }
#line 5627 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 80: /* pragma_lang: LPAREN identifier RPAREN  */
#line 2398 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         { (yyval.id) = (yyvsp[-1].id); }
#line 5633 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 81: /* pragma_lang: empty  */
#line 2399 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                      { (yyval.id) = (char *) "swig"; }
#line 5639 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 82: /* rename_directive: rename_namewarn declarator idstring SEMI  */
#line 2406 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5690 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 83: /* rename_directive: rename_namewarn LPAREN kwargs RPAREN declarator cpp_const SEMI  */
#line 2452 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5741 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 84: /* rename_directive: rename_namewarn LPAREN kwargs RPAREN string SEMI  */
#line 2498 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
		if ((yyvsp[-5].intvalue)) {
		  Swig_name_rename_add(Namespaceprefix,(yyvsp[-1].str),0,(yyvsp[-3].node),0);
		} else {
		  Swig_name_namewarn_add(Namespaceprefix,(yyvsp[-1].str),0,(yyvsp[-3].node));
		}
		(yyval.node) = 0;
		scanner_clear_rename();
              }
#line 5755 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 85: /* rename_namewarn: RENAME  */
#line 2509 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		    (yyval.intvalue) = 1;
                }
#line 5763 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 86: /* rename_namewarn: NAMEWARN  */
#line 2512 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
                    (yyval.intvalue) = 0;
                }
#line 5771 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 87: /* feature_directive: FEATURE LPAREN idstring RPAREN declarator cpp_const stringbracesemi  */
#line 2539 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                        {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-4].id), val, 0, (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5782 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 88: /* feature_directive: FEATURE LPAREN idstring COMMA stringnum RPAREN declarator cpp_const SEMI  */
#line 2545 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                             {
                    String *val = Len((yyvsp[-4].str)) ? (yyvsp[-4].str) : 0;
                    new_feature((yyvsp[-6].id), val, 0, (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5793 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 89: /* feature_directive: FEATURE LPAREN idstring featattr RPAREN declarator cpp_const stringbracesemi  */
#line 2551 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                                 {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-5].id), val, (yyvsp[-4].node), (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5804 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 90: /* feature_directive: FEATURE LPAREN idstring COMMA stringnum featattr RPAREN declarator cpp_const SEMI  */
#line 2557 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                                      {
                    String *val = Len((yyvsp[-5].str)) ? (yyvsp[-5].str) : 0;
                    new_feature((yyvsp[-7].id), val, (yyvsp[-4].node), (yyvsp[-2].decl).id, (yyvsp[-2].decl).type, (yyvsp[-2].decl).parms, (yyvsp[-1].dtype).qualifier);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5815 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 91: /* feature_directive: FEATURE LPAREN idstring RPAREN stringbracesemi  */
#line 2565 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                   {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-2].id), val, 0, 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5826 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 92: /* feature_directive: FEATURE LPAREN idstring COMMA stringnum RPAREN SEMI  */
#line 2571 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                        {
                    String *val = Len((yyvsp[-2].str)) ? (yyvsp[-2].str) : 0;
                    new_feature((yyvsp[-4].id), val, 0, 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5837 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 93: /* feature_directive: FEATURE LPAREN idstring featattr RPAREN stringbracesemi  */
#line 2577 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                            {
                    String *val = (yyvsp[0].str) ? NewString((yyvsp[0].str)) : NewString("1");
                    new_feature((yyvsp[-3].id), val, (yyvsp[-2].node), 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5848 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 94: /* feature_directive: FEATURE LPAREN idstring COMMA stringnum featattr RPAREN SEMI  */
#line 2583 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                 {
                    String *val = Len((yyvsp[-3].str)) ? (yyvsp[-3].str) : 0;
                    new_feature((yyvsp[-5].id), val, (yyvsp[-2].node), 0, 0, 0, 0);
                    (yyval.node) = 0;
                    scanner_clear_rename();
                  }
#line 5859 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 95: /* stringbracesemi: stringbrace  */
#line 2591 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.str) = (yyvsp[0].str); }
#line 5865 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 96: /* stringbracesemi: SEMI  */
#line 2592 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.str) = 0; }
#line 5871 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 97: /* stringbracesemi: PARMS LPAREN parms RPAREN SEMI  */
#line 2593 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 { (yyval.str) = (yyvsp[-2].pl); }
#line 5877 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 98: /* featattr: COMMA idstring EQUAL stringnum  */
#line 2596 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 {
		  (yyval.node) = NewHash();
		  Setattr((yyval.node),"name",(yyvsp[-2].id));
		  Setattr((yyval.node),"value",(yyvsp[0].str));
                }
#line 5887 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 99: /* featattr: COMMA idstring EQUAL stringnum featattr  */
#line 2601 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                          {
		  (yyval.node) = NewHash();
		  Setattr((yyval.node),"name",(yyvsp[-3].id));
		  Setattr((yyval.node),"value",(yyvsp[-1].str));
                  set_nextSibling((yyval.node),(yyvsp[0].node));
                }
#line 5898 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 100: /* varargs_directive: VARARGS LPAREN varargs_parms RPAREN declarator cpp_const SEMI  */
#line 2611 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5938 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 101: /* varargs_parms: parms  */
#line 2647 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.pl) = (yyvsp[0].pl); }
#line 5944 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 102: /* varargs_parms: NUM_INT COMMA parm  */
#line 2648 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5973 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 103: /* typemap_directive: TYPEMAP LPAREN typemap_type RPAREN tm_list stringbrace  */
#line 2683 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 5995 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 104: /* typemap_directive: TYPEMAP LPAREN typemap_type RPAREN tm_list SEMI  */
#line 2700 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
		 (yyval.node) = 0;
		 if ((yyvsp[-3].tmap).method) {
		   (yyval.node) = new_node("typemap");
		   Setattr((yyval.node),"method",(yyvsp[-3].tmap).method);
		   appendChild((yyval.node),(yyvsp[-1].p));
		 }
	       }
#line 6008 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 105: /* typemap_directive: TYPEMAP LPAREN typemap_type RPAREN tm_list EQUAL typemap_parm SEMI  */
#line 2708 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                    {
		   (yyval.node) = 0;
		   if ((yyvsp[-5].tmap).method) {
		     (yyval.node) = new_node("typemapcopy");
		     Setattr((yyval.node),"method",(yyvsp[-5].tmap).method);
		     Setattr((yyval.node),"pattern", Getattr((yyvsp[-1].p),"pattern"));
		     appendChild((yyval.node),(yyvsp[-3].p));
		   }
	       }
#line 6022 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 106: /* typemap_type: kwargs  */
#line 2721 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6050 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 107: /* tm_list: typemap_parm tm_tail  */
#line 2746 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                      {
                 (yyval.p) = (yyvsp[-1].p);
		 set_nextSibling((yyval.p),(yyvsp[0].p));
		}
#line 6059 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 108: /* tm_tail: COMMA typemap_parm tm_tail  */
#line 2752 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            {
                 (yyval.p) = (yyvsp[-1].p);
		 set_nextSibling((yyval.p),(yyvsp[0].p));
                }
#line 6068 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 109: /* tm_tail: empty  */
#line 2756 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.p) = 0;}
#line 6074 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 110: /* typemap_parm: type plain_declarator  */
#line 2759 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6090 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 111: /* typemap_parm: LPAREN parms RPAREN  */
#line 2770 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     {
                  (yyval.p) = new_node("typemapitem");
		  Setattr((yyval.p),"pattern",(yyvsp[-1].pl));
		  /*		  Setattr($$,"multitype",$2); */
               }
#line 6100 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 112: /* typemap_parm: LPAREN parms RPAREN LPAREN parms RPAREN  */
#line 2775 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                         {
		 (yyval.p) = new_node("typemapitem");
		 Setattr((yyval.p),"pattern", (yyvsp[-4].pl));
		 /*                 Setattr($$,"multitype",$2); */
		 Setattr((yyval.p),"parms",(yyvsp[-1].pl));
               }
#line 6111 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 113: /* types_directive: TYPES LPAREN parms RPAREN stringbracesemi  */
#line 2788 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                            {
                   (yyval.node) = new_node("types");
		   Setattr((yyval.node),"parms",(yyvsp[-2].pl));
                   if ((yyvsp[0].str))
		     Setattr((yyval.node),"convcode",NewString((yyvsp[0].str)));
               }
#line 6122 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 114: /* template_directive: SWIGTEMPLATE LPAREN idstringopt RPAREN idcolonnt LESSTHAN valparms GREATERTHAN SEMI  */
#line 2800 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
                            Setattr(templnode,"feature:onlychildren", "typemap,typemapitem,typemapcopy,typedef,types,fragment,apply");
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
                    update_abstracts(linkliststart);
		  }
	          Swig_symbol_setscope(tscope);
		  Delete(Namespaceprefix);
		  Namespaceprefix = Swig_symbol_qualifiedscopename(0);
                }
#line 6418 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 115: /* warn_directive: WARN string  */
#line 3098 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		  Swig_warning(0,cparse_file, cparse_line,"%s\n", (yyvsp[0].str));
		  (yyval.node) = 0;
               }
#line 6427 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 116: /* c_declaration: c_decl  */
#line 3108 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
                    (yyval.node) = (yyvsp[0].node); 
                    if ((yyval.node)) {
   		      add_symbols((yyval.node));
                      default_arguments((yyval.node));
   	            }
                }
#line 6439 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 117: /* c_declaration: c_enum_decl  */
#line 3115 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 6445 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 118: /* c_declaration: c_enum_forward_decl  */
#line 3116 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                      { (yyval.node) = (yyvsp[0].node); }
#line 6451 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 119: /* $@3: %empty  */
#line 3120 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
		  if (Strcmp((yyvsp[-1].str),"C") == 0) {
		    cparse_externc = 1;
		  }
		}
#line 6461 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 120: /* c_declaration: EXTERN string LBRACE $@3 interface RBRACE  */
#line 3124 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6487 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 121: /* c_declaration: cpp_lambda_decl  */
#line 3145 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		  (yyval.node) = (yyvsp[0].node);
		  SWIG_WARN_NODE_BEGIN((yyval.node));
		  Swig_warning(WARN_CPP11_LAMBDA, cparse_file, cparse_line, "Lambda expressions and closures are not fully supported yet.\n");
		  SWIG_WARN_NODE_END((yyval.node));
		}
#line 6498 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 122: /* c_declaration: USING idcolon EQUAL type plain_declarator SEMI  */
#line 3151 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6513 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 123: /* c_declaration: TEMPLATE LESSTHAN template_parms GREATERTHAN USING idcolon EQUAL type plain_declarator SEMI  */
#line 3161 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6530 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 124: /* c_declaration: cpp_static_assert  */
#line 3173 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    {
                   (yyval.node) = (yyvsp[0].node);
                }
#line 6538 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 125: /* c_decl: storage_class type declarator cpp_const initializer c_decl_tail  */
#line 3182 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
	      Setattr((yyval.node),"final",(yyvsp[-2].dtype).final);
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
#line 6621 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 126: /* c_decl: storage_class AUTO declarator cpp_const ARROW cpp_alternate_rettype virt_specifier_seq_opt initializer c_decl_tail  */
#line 3262 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
	      Setattr((yyval.node),"final",(yyvsp[-5].dtype).final);
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
#line 6685 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 127: /* c_decl_tail: SEMI  */
#line 3325 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                      { 
                   (yyval.node) = 0;
                   Clear(scanner_ccode); 
               }
#line 6694 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 128: /* c_decl_tail: COMMA declarator cpp_const initializer c_decl_tail  */
#line 3329 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		 Setattr((yyval.node),"final",(yyvsp[-2].dtype).final);
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
#line 6724 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 129: /* c_decl_tail: LBRACE  */
#line 3354 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { 
                   skip_balanced('{','}');
                   (yyval.node) = 0;
               }
#line 6733 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 130: /* c_decl_tail: error  */
#line 3358 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
		   (yyval.node) = 0;
		   if (yychar == RPAREN) {
		       Swig_error(cparse_file, cparse_line, "Unexpected ')'.\n");
		   } else {
		       Swig_error(cparse_file, cparse_line, "Syntax error - possibly a missing semicolon.\n");
		   }
		   exit(1);
               }
#line 6747 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 131: /* initializer: def_args  */
#line 3369 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
                   (yyval.dtype) = (yyvsp[0].dtype);
              }
#line 6755 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 132: /* cpp_alternate_rettype: primitive_type  */
#line 3374 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       { (yyval.node) = (yyvsp[0].type); }
#line 6761 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 133: /* cpp_alternate_rettype: TYPE_BOOL  */
#line 3375 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.node) = (yyvsp[0].type); }
#line 6767 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 134: /* cpp_alternate_rettype: TYPE_VOID  */
#line 3376 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.node) = (yyvsp[0].type); }
#line 6773 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 135: /* cpp_alternate_rettype: TYPE_RAW  */
#line 3380 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.node) = (yyvsp[0].type); }
#line 6779 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 136: /* cpp_alternate_rettype: idcolon  */
#line 3381 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.node) = (yyvsp[0].str); }
#line 6785 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 137: /* cpp_alternate_rettype: decltype  */
#line 3382 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.node) = (yyvsp[0].type); }
#line 6791 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 138: /* cpp_lambda_decl: storage_class AUTO idcolon EQUAL lambda_introducer LPAREN parms RPAREN cpp_const lambda_body lambda_tail  */
#line 3393 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                                                           {
		  (yyval.node) = new_node("lambda");
		  Setattr((yyval.node),"name",(yyvsp[-8].str));
		  add_symbols((yyval.node));
	        }
#line 6801 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 139: /* cpp_lambda_decl: storage_class AUTO idcolon EQUAL lambda_introducer LPAREN parms RPAREN cpp_const ARROW type lambda_body lambda_tail  */
#line 3398 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                                                                      {
		  (yyval.node) = new_node("lambda");
		  Setattr((yyval.node),"name",(yyvsp[-10].str));
		  add_symbols((yyval.node));
		}
#line 6811 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 140: /* cpp_lambda_decl: storage_class AUTO idcolon EQUAL lambda_introducer lambda_body lambda_tail  */
#line 3403 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                             {
		  (yyval.node) = new_node("lambda");
		  Setattr((yyval.node),"name",(yyvsp[-4].str));
		  add_symbols((yyval.node));
		}
#line 6821 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 141: /* lambda_introducer: LBRACKET  */
#line 3410 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		  skip_balanced('[',']');
		  (yyval.node) = 0;
	        }
#line 6830 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 142: /* lambda_body: LBRACE  */
#line 3416 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                     {
		  skip_balanced('{','}');
		  (yyval.node) = 0;
		}
#line 6839 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 143: /* lambda_tail: SEMI  */
#line 3421 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                     {
		  (yyval.pl) = 0;
		}
#line 6847 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 144: /* $@4: %empty  */
#line 3424 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		  skip_balanced('(',')');
		}
#line 6855 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 145: /* lambda_tail: LPAREN $@4 SEMI  */
#line 3426 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
		  (yyval.pl) = 0;
		}
#line 6863 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 146: /* c_enum_key: ENUM  */
#line 3437 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                  {
		   (yyval.node) = (char *)"enum";
	      }
#line 6871 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 147: /* c_enum_key: ENUM CLASS  */
#line 3440 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
		   (yyval.node) = (char *)"enum class";
	      }
#line 6879 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 148: /* c_enum_key: ENUM STRUCT  */
#line 3443 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            {
		   (yyval.node) = (char *)"enum struct";
	      }
#line 6887 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 149: /* c_enum_inherit: COLON type_right  */
#line 3452 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
                   (yyval.node) = (yyvsp[0].type);
              }
#line 6895 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 150: /* c_enum_inherit: empty  */
#line 3455 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                      { (yyval.node) = 0; }
#line 6901 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 151: /* c_enum_forward_decl: storage_class c_enum_key ename c_enum_inherit SEMI  */
#line 3462 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6920 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 152: /* c_enum_decl: storage_class c_enum_key ename c_enum_inherit LBRACE enumlist RBRACE SEMI  */
#line 3484 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 6954 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 153: /* c_enum_decl: storage_class c_enum_key ename c_enum_inherit LBRACE enumlist RBRACE declarator cpp_const initializer c_decl_tail  */
#line 3513 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7055 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 154: /* c_constructor_decl: storage_class type LPAREN parms RPAREN ctor_end  */
#line 3611 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
			Setattr((yyval.node),"final",(yyvsp[0].decl).final);
			err = 0;
		      }
		    }
		    if (err) {
		      Swig_error(cparse_file,cparse_line,"Syntax error in input(2).\n");
		      exit(1);
		    }
                }
#line 7107 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 155: /* cpp_declaration: cpp_class_decl  */
#line 3664 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {  (yyval.node) = (yyvsp[0].node); }
#line 7113 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 156: /* cpp_declaration: cpp_forward_class_decl  */
#line 3665 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         { (yyval.node) = (yyvsp[0].node); }
#line 7119 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 157: /* cpp_declaration: cpp_template_decl  */
#line 3666 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.node) = (yyvsp[0].node); }
#line 7125 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 158: /* cpp_declaration: cpp_using_decl  */
#line 3667 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 7131 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 159: /* cpp_declaration: cpp_namespace_decl  */
#line 3668 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { (yyval.node) = (yyvsp[0].node); }
#line 7137 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 160: /* cpp_declaration: cpp_catch_decl  */
#line 3669 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = 0; }
#line 7143 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 161: /* @5: %empty  */
#line 3674 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7242 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 162: /* cpp_class_decl: storage_class cpptype idcolon inherit LBRACE @5 cpp_members RBRACE cpp_opt_declarators  */
#line 3767 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		       Swig_symbol_setscope(cscope); /* declaration goes in the parent scope */
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
#line 7400 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 163: /* @6: %empty  */
#line 3923 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7445 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 164: /* cpp_class_decl: storage_class cpptype inherit LBRACE @6 cpp_members RBRACE cpp_opt_declarators  */
#line 3962 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7556 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 165: /* cpp_opt_declarators: SEMI  */
#line 4070 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { (yyval.node) = 0; }
#line 7562 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 166: /* cpp_opt_declarators: declarator cpp_const initializer c_decl_tail  */
#line 4071 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                    {
                        (yyval.node) = new_node("cdecl");
                        Setattr((yyval.node),"name",(yyvsp[-3].decl).id);
                        Setattr((yyval.node),"decl",(yyvsp[-3].decl).type);
                        Setattr((yyval.node),"parms",(yyvsp[-3].decl).parms);
			set_nextSibling((yyval.node), (yyvsp[0].node));
                    }
#line 7574 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 167: /* cpp_forward_class_decl: storage_class cpptype idcolon SEMI  */
#line 4083 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7591 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 168: /* $@7: %empty  */
#line 4101 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 { 
		    if (currentOuterClass)
		      Setattr(currentOuterClass, "template_parameters", template_parameters);
		    template_parameters = (yyvsp[-1].tparms); 
		    parsing_template_declaration = 1;
		  }
#line 7602 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 169: /* cpp_template_decl: TEMPLATE LESSTHAN template_parms GREATERTHAN $@7 cpp_template_possible  */
#line 4106 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7861 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 170: /* cpp_template_decl: TEMPLATE cpptype idcolon  */
#line 4362 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
		  Swig_warning(WARN_PARSE_EXPLICIT_TEMPLATE, cparse_file, cparse_line, "Explicit template instantiation ignored.\n");
                  (yyval.node) = 0; 
		}
#line 7870 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 171: /* cpp_template_decl: EXTERN TEMPLATE cpptype idcolon  */
#line 4368 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                  {
		  Swig_warning(WARN_PARSE_EXPLICIT_TEMPLATE, cparse_file, cparse_line, "Explicit template instantiation ignored.\n");
                  (yyval.node) = 0; 
                }
#line 7879 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 172: /* cpp_template_possible: c_decl  */
#line 4374 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
		  (yyval.node) = (yyvsp[0].node);
                }
#line 7887 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 173: /* cpp_template_possible: cpp_class_decl  */
#line 4377 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
                   (yyval.node) = (yyvsp[0].node);
                }
#line 7895 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 174: /* cpp_template_possible: cpp_constructor_decl  */
#line 4380 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
                   (yyval.node) = (yyvsp[0].node);
                }
#line 7903 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 175: /* cpp_template_possible: cpp_template_decl  */
#line 4383 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    {
		  (yyval.node) = 0;
                }
#line 7911 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 176: /* cpp_template_possible: cpp_forward_class_decl  */
#line 4386 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
                  (yyval.node) = (yyvsp[0].node);
                }
#line 7919 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 177: /* cpp_template_possible: cpp_conversion_operator  */
#line 4389 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                          {
                  (yyval.node) = (yyvsp[0].node);
                }
#line 7927 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 178: /* template_parms: templateparameters  */
#line 4394 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 7966 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 179: /* templateparameters: templateparameter templateparameterstail  */
#line 4430 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                              {
                      set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
                      (yyval.pl) = (yyvsp[-1].p);
                   }
#line 7975 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 180: /* templateparameters: empty  */
#line 4434 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.pl) = 0; }
#line 7981 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 181: /* templateparameter: templcpptype  */
#line 4437 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		    (yyval.p) = NewParmWithoutFileLineInfo(NewString((yyvsp[0].id)), 0);
                  }
#line 7989 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 182: /* templateparameter: parm  */
#line 4440 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
                    (yyval.p) = (yyvsp[0].p);
                  }
#line 7997 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 183: /* templateparameterstail: COMMA templateparameter templateparameterstail  */
#line 4445 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                        {
                         set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
                         (yyval.pl) = (yyvsp[-1].p);
                       }
#line 8006 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 184: /* templateparameterstail: empty  */
#line 4449 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.pl) = 0; }
#line 8012 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 185: /* cpp_using_decl: USING idcolon SEMI  */
#line 4454 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8027 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 186: /* cpp_using_decl: USING NAMESPACE idcolon SEMI  */
#line 4464 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8061 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 187: /* @8: %empty  */
#line 4495 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8110 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 188: /* cpp_namespace_decl: NAMESPACE idcolon LBRACE @8 interface RBRACE  */
#line 4538 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8130 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 189: /* $@9: %empty  */
#line 4553 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8148 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 190: /* cpp_namespace_decl: NAMESPACE LBRACE $@9 interface RBRACE  */
#line 4565 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8163 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 191: /* cpp_namespace_decl: NAMESPACE identifier EQUAL idcolon SEMI  */
#line 4575 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8193 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 192: /* cpp_members: cpp_member cpp_members  */
#line 4602 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8215 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 193: /* $@10: %empty  */
#line 4619 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             { 
	       extendmode = 1;
	       if (cplus_mode != CPLUS_PUBLIC) {
		 Swig_error(cparse_file,cparse_line,"%%extend can only be used in a public section\n");
	       }
             }
#line 8226 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 194: /* $@11: %empty  */
#line 4624 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
	       extendmode = 0;
	     }
#line 8234 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 195: /* cpp_members: EXTEND LBRACE $@10 cpp_members RBRACE $@11 cpp_members  */
#line 4626 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
	       (yyval.node) = new_node("extend");
	       mark_nodes_as_extend((yyvsp[-3].node));
	       appendChild((yyval.node),(yyvsp[-3].node));
	       set_nextSibling((yyval.node),(yyvsp[0].node));
	     }
#line 8245 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 196: /* cpp_members: include_directive  */
#line 4632 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 8251 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 197: /* cpp_members: empty  */
#line 4633 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                     { (yyval.node) = 0;}
#line 8257 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 198: /* $@12: %empty  */
#line 4634 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                     {
	       int start_line = cparse_line;
	       skip_decl();
	       Swig_error(cparse_file,start_line,"Syntax error in input(3).\n");
	       exit(1);
	       }
#line 8268 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 199: /* cpp_members: error $@12 cpp_members  */
#line 4639 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             { 
		 (yyval.node) = (yyvsp[0].node);
   	     }
#line 8276 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 200: /* cpp_member_no_dox: c_declaration  */
#line 4650 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 8282 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 201: /* cpp_member_no_dox: cpp_constructor_decl  */
#line 4651 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8304 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 202: /* cpp_member_no_dox: cpp_destructor_decl  */
#line 4668 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 8310 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 203: /* cpp_member_no_dox: cpp_protection_decl  */
#line 4669 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { (yyval.node) = (yyvsp[0].node); }
#line 8316 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 204: /* cpp_member_no_dox: cpp_swig_directive  */
#line 4670 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 8322 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 205: /* cpp_member_no_dox: cpp_conversion_operator  */
#line 4671 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       { (yyval.node) = (yyvsp[0].node); }
#line 8328 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 206: /* cpp_member_no_dox: cpp_forward_class_decl  */
#line 4672 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                      { (yyval.node) = (yyvsp[0].node); }
#line 8334 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 207: /* cpp_member_no_dox: cpp_class_decl  */
#line 4673 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 8340 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 208: /* cpp_member_no_dox: storage_class idcolon SEMI  */
#line 4674 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                          { (yyval.node) = 0; }
#line 8346 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 209: /* cpp_member_no_dox: cpp_using_decl  */
#line 4675 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 8352 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 210: /* cpp_member_no_dox: cpp_template_decl  */
#line 4676 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 8358 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 211: /* cpp_member_no_dox: cpp_catch_decl  */
#line 4677 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = 0; }
#line 8364 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 212: /* cpp_member_no_dox: template_directive  */
#line 4678 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 8370 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 213: /* cpp_member_no_dox: warn_directive  */
#line 4679 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 8376 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 214: /* cpp_member_no_dox: anonymous_bitfield  */
#line 4680 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = 0; }
#line 8382 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 215: /* cpp_member_no_dox: fragment_directive  */
#line 4681 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {(yyval.node) = (yyvsp[0].node); }
#line 8388 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 216: /* cpp_member_no_dox: types_directive  */
#line 4682 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {(yyval.node) = (yyvsp[0].node); }
#line 8394 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 217: /* cpp_member_no_dox: SEMI  */
#line 4683 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                    { (yyval.node) = 0; }
#line 8400 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 218: /* cpp_member: cpp_member_no_dox  */
#line 4685 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		(yyval.node) = (yyvsp[0].node);
	     }
#line 8408 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 219: /* cpp_member: DOXYGENSTRING cpp_member_no_dox  */
#line 4688 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                               {
	         (yyval.node) = (yyvsp[0].node);
		 set_comment((yyvsp[0].node), (yyvsp[-1].str));
	     }
#line 8417 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 220: /* cpp_member: cpp_member_no_dox DOXYGENPOSTSTRING  */
#line 4692 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                   {
	         (yyval.node) = (yyvsp[-1].node);
		 set_comment((yyvsp[-1].node), (yyvsp[0].str));
	     }
#line 8426 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 221: /* cpp_constructor_decl: storage_class type LPAREN parms RPAREN ctor_end  */
#line 4704 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		Setattr((yyval.node),"final",(yyvsp[0].decl).final);
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
#line 8456 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 222: /* cpp_destructor_decl: NOT idtemplate LPAREN parms RPAREN cpp_end  */
#line 4733 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
	       Setattr((yyval.node),"final",(yyvsp[0].dtype).final);
	       if ((yyvsp[0].dtype).val)
	         Setattr((yyval.node),"value",(yyvsp[0].dtype).val);
	       if ((yyvsp[0].dtype).qualifier)
		 Swig_error(cparse_file, cparse_line, "Destructor %s %s cannot have a qualifier.\n", Swig_name_decl((yyval.node)), SwigType_str((yyvsp[0].dtype).qualifier, 0));
	       add_symbols((yyval.node));
	      }
#line 8488 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 223: /* cpp_destructor_decl: VIRTUAL NOT idtemplate LPAREN parms RPAREN cpp_vend  */
#line 4763 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		Setattr((yyval.node),"final",(yyvsp[0].dtype).final);
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
#line 8522 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 224: /* cpp_conversion_operator: storage_class CONVERSIONOPERATOR type pointer LPAREN parms RPAREN cpp_vend  */
#line 4796 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8543 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 225: /* cpp_conversion_operator: storage_class CONVERSIONOPERATOR type AND LPAREN parms RPAREN cpp_vend  */
#line 4812 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8566 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 226: /* cpp_conversion_operator: storage_class CONVERSIONOPERATOR type LAND LPAREN parms RPAREN cpp_vend  */
#line 4830 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8589 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 227: /* cpp_conversion_operator: storage_class CONVERSIONOPERATOR type pointer AND LPAREN parms RPAREN cpp_vend  */
#line 4849 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8613 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 228: /* cpp_conversion_operator: storage_class CONVERSIONOPERATOR type LPAREN parms RPAREN cpp_vend  */
#line 4869 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8634 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 229: /* cpp_catch_decl: CATCH LPAREN parms RPAREN LBRACE  */
#line 4889 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                  {
                 skip_balanced('{','}');
                 (yyval.node) = 0;
               }
#line 8643 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 230: /* cpp_static_assert: STATIC_ASSERT LPAREN  */
#line 4897 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
                skip_balanced('(',')');
                (yyval.node) = 0;
              }
#line 8652 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 231: /* cpp_protection_decl: PUBLIC COLON  */
#line 4904 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { 
                (yyval.node) = new_node("access");
		Setattr((yyval.node),"kind","public");
                cplus_mode = CPLUS_PUBLIC;
              }
#line 8662 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 232: /* cpp_protection_decl: PRIVATE COLON  */
#line 4911 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { 
                (yyval.node) = new_node("access");
                Setattr((yyval.node),"kind","private");
		cplus_mode = CPLUS_PRIVATE;
	      }
#line 8672 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 233: /* cpp_protection_decl: PROTECTED COLON  */
#line 4919 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { 
		(yyval.node) = new_node("access");
		Setattr((yyval.node),"kind","protected");
		cplus_mode = CPLUS_PROTECTED;
	      }
#line 8682 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 234: /* cpp_swig_directive: pragma_directive  */
#line 4927 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { (yyval.node) = (yyvsp[0].node); }
#line 8688 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 235: /* cpp_swig_directive: constant_directive  */
#line 4930 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.node) = (yyvsp[0].node); }
#line 8694 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 236: /* cpp_swig_directive: name_directive  */
#line 4934 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 8700 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 237: /* cpp_swig_directive: rename_directive  */
#line 4937 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 8706 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 238: /* cpp_swig_directive: feature_directive  */
#line 4938 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 8712 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 239: /* cpp_swig_directive: varargs_directive  */
#line 4939 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 8718 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 240: /* cpp_swig_directive: insert_directive  */
#line 4940 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.node) = (yyvsp[0].node); }
#line 8724 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 241: /* cpp_swig_directive: typemap_directive  */
#line 4941 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.node) = (yyvsp[0].node); }
#line 8730 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 242: /* cpp_swig_directive: apply_directive  */
#line 4942 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.node) = (yyvsp[0].node); }
#line 8736 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 243: /* cpp_swig_directive: clear_directive  */
#line 4943 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.node) = (yyvsp[0].node); }
#line 8742 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 244: /* cpp_swig_directive: echo_directive  */
#line 4944 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.node) = (yyvsp[0].node); }
#line 8748 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 245: /* cpp_end: cpp_const SEMI  */
#line 4947 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
	            Clear(scanner_ccode);
		    (yyval.dtype).val = 0;
		    (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
		    (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = (yyvsp[-1].dtype).throws;
		    (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf;
		    (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
		    (yyval.dtype).final = (yyvsp[-1].dtype).final;
               }
#line 8764 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 246: /* cpp_end: cpp_const EQUAL default_delete SEMI  */
#line 4958 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                     {
	            Clear(scanner_ccode);
		    (yyval.dtype).val = (yyvsp[-1].dtype).val;
		    (yyval.dtype).qualifier = (yyvsp[-3].dtype).qualifier;
		    (yyval.dtype).refqualifier = (yyvsp[-3].dtype).refqualifier;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = (yyvsp[-3].dtype).throws;
		    (yyval.dtype).throwf = (yyvsp[-3].dtype).throwf;
		    (yyval.dtype).nexcept = (yyvsp[-3].dtype).nexcept;
		    (yyval.dtype).final = (yyvsp[-3].dtype).final;
               }
#line 8780 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 247: /* cpp_end: cpp_const LBRACE  */
#line 4969 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { 
		    skip_balanced('{','}'); 
		    (yyval.dtype).val = 0;
		    (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
		    (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
		    (yyval.dtype).bitfield = 0;
		    (yyval.dtype).throws = (yyvsp[-1].dtype).throws;
		    (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf;
		    (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
		    (yyval.dtype).final = (yyvsp[-1].dtype).final;
	       }
#line 8796 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 248: /* cpp_vend: cpp_const SEMI  */
#line 4982 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { 
                     Clear(scanner_ccode);
                     (yyval.dtype).val = 0;
                     (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
                     (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
                     (yyval.dtype).bitfield = 0;
                     (yyval.dtype).throws = (yyvsp[-1].dtype).throws;
                     (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf;
                     (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
                     (yyval.dtype).final = (yyvsp[-1].dtype).final;
                }
#line 8812 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 249: /* cpp_vend: cpp_const EQUAL definetype SEMI  */
#line 4993 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 { 
                     Clear(scanner_ccode);
                     (yyval.dtype).val = (yyvsp[-1].dtype).val;
                     (yyval.dtype).qualifier = (yyvsp[-3].dtype).qualifier;
                     (yyval.dtype).refqualifier = (yyvsp[-3].dtype).refqualifier;
                     (yyval.dtype).bitfield = 0;
                     (yyval.dtype).throws = (yyvsp[-3].dtype).throws; 
                     (yyval.dtype).throwf = (yyvsp[-3].dtype).throwf; 
                     (yyval.dtype).nexcept = (yyvsp[-3].dtype).nexcept;
                     (yyval.dtype).final = (yyvsp[-3].dtype).final;
               }
#line 8828 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 250: /* cpp_vend: cpp_const LBRACE  */
#line 5004 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { 
                     skip_balanced('{','}');
                     (yyval.dtype).val = 0;
                     (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
                     (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
                     (yyval.dtype).bitfield = 0;
                     (yyval.dtype).throws = (yyvsp[-1].dtype).throws; 
                     (yyval.dtype).throwf = (yyvsp[-1].dtype).throwf; 
                     (yyval.dtype).nexcept = (yyvsp[-1].dtype).nexcept;
                     (yyval.dtype).final = (yyvsp[-1].dtype).final;
               }
#line 8844 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 251: /* anonymous_bitfield: storage_class anon_bitfield_type COLON expr SEMI  */
#line 5018 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                       { }
#line 8850 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 252: /* anon_bitfield_type: primitive_type  */
#line 5021 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.type) = (yyvsp[0].type);
                  /* Printf(stdout,"primitive = '%s'\n", $$);*/
                }
#line 8858 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 253: /* anon_bitfield_type: TYPE_BOOL  */
#line 5024 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.type) = (yyvsp[0].type); }
#line 8864 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 254: /* anon_bitfield_type: TYPE_VOID  */
#line 5025 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.type) = (yyvsp[0].type); }
#line 8870 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 255: /* anon_bitfield_type: TYPE_RAW  */
#line 5029 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.type) = (yyvsp[0].type); }
#line 8876 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 256: /* anon_bitfield_type: idcolon  */
#line 5031 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		  (yyval.type) = (yyvsp[0].str);
               }
#line 8884 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 257: /* extern_string: EXTERN string  */
#line 5039 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 8899 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 258: /* storage_class: EXTERN  */
#line 5051 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.id) = "extern"; }
#line 8905 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 259: /* storage_class: extern_string  */
#line 5052 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.id) = (yyvsp[0].id); }
#line 8911 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 260: /* storage_class: extern_string THREAD_LOCAL  */
#line 5053 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            { (yyval.id) = "thread_local"; }
#line 8917 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 261: /* storage_class: extern_string TYPEDEF  */
#line 5054 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       { (yyval.id) = "typedef"; }
#line 8923 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 262: /* storage_class: STATIC  */
#line 5055 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.id) = "static"; }
#line 8929 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 263: /* storage_class: TYPEDEF  */
#line 5056 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.id) = "typedef"; }
#line 8935 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 264: /* storage_class: VIRTUAL  */
#line 5057 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.id) = "virtual"; }
#line 8941 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 265: /* storage_class: FRIEND  */
#line 5058 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.id) = "friend"; }
#line 8947 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 266: /* storage_class: EXPLICIT  */
#line 5059 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.id) = "explicit"; }
#line 8953 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 267: /* storage_class: CONSTEXPR  */
#line 5060 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.id) = "constexpr"; }
#line 8959 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 268: /* storage_class: EXPLICIT CONSTEXPR  */
#line 5061 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.id) = "explicit constexpr"; }
#line 8965 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 269: /* storage_class: CONSTEXPR EXPLICIT  */
#line 5062 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.id) = "explicit constexpr"; }
#line 8971 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 270: /* storage_class: STATIC CONSTEXPR  */
#line 5063 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.id) = "static constexpr"; }
#line 8977 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 271: /* storage_class: CONSTEXPR STATIC  */
#line 5064 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { (yyval.id) = "static constexpr"; }
#line 8983 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 272: /* storage_class: THREAD_LOCAL  */
#line 5065 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.id) = "thread_local"; }
#line 8989 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 273: /* storage_class: THREAD_LOCAL STATIC  */
#line 5066 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { (yyval.id) = "static thread_local"; }
#line 8995 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 274: /* storage_class: STATIC THREAD_LOCAL  */
#line 5067 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { (yyval.id) = "static thread_local"; }
#line 9001 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 275: /* storage_class: EXTERN THREAD_LOCAL  */
#line 5068 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { (yyval.id) = "extern thread_local"; }
#line 9007 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 276: /* storage_class: THREAD_LOCAL EXTERN  */
#line 5069 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { (yyval.id) = "extern thread_local"; }
#line 9013 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 277: /* storage_class: empty  */
#line 5070 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.id) = 0; }
#line 9019 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 278: /* parms: rawparms  */
#line 5077 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                 Parm *p;
		 (yyval.pl) = (yyvsp[0].pl);
		 p = (yyvsp[0].pl);
                 while (p) {
		   Replace(Getattr(p,"type"),"typename ", "", DOH_REPLACE_ANY);
		   p = nextSibling(p);
                 }
               }
#line 9033 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 279: /* rawparms: parm ptail  */
#line 5088 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
                  set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
                  (yyval.pl) = (yyvsp[-1].p);
		}
#line 9042 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 280: /* rawparms: empty  */
#line 5092 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
		  (yyval.pl) = 0;
		  previousNode = currentNode;
		  currentNode=0;
	       }
#line 9052 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 281: /* ptail: COMMA parm ptail  */
#line 5099 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
                 set_nextSibling((yyvsp[-1].p),(yyvsp[0].pl));
		 (yyval.pl) = (yyvsp[-1].p);
                }
#line 9061 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 282: /* ptail: COMMA DOXYGENPOSTSTRING parm ptail  */
#line 5103 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                    {
		 set_comment(previousNode, (yyvsp[-2].str));
                 set_nextSibling((yyvsp[-1].p), (yyvsp[0].pl));
		 (yyval.pl) = (yyvsp[-1].p);
               }
#line 9071 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 283: /* ptail: empty  */
#line 5108 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.pl) = 0; }
#line 9077 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 284: /* parm_no_dox: rawtype parameter_declarator  */
#line 5112 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9093 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 285: /* parm_no_dox: TEMPLATE LESSTHAN cpptype GREATERTHAN cpptype idcolon def_args  */
#line 5124 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9108 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 286: /* parm_no_dox: PERIOD PERIOD PERIOD  */
#line 5134 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
		  SwigType *t = NewString("v(...)");
		  (yyval.p) = NewParmWithoutFileLineInfo(t, 0);
		  previousNode = currentNode;
		  currentNode = (yyval.p);
		  Setfile((yyval.p),cparse_file);
		  Setline((yyval.p),cparse_line);
		}
#line 9121 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 287: /* parm: parm_no_dox  */
#line 5144 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
		  (yyval.p) = (yyvsp[0].p);
		}
#line 9129 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 288: /* parm: DOXYGENSTRING parm_no_dox  */
#line 5147 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            {
		  (yyval.p) = (yyvsp[0].p);
		  set_comment((yyvsp[0].p), (yyvsp[-1].str));
		}
#line 9138 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 289: /* parm: parm_no_dox DOXYGENPOSTSTRING  */
#line 5151 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                {
		  (yyval.p) = (yyvsp[-1].p);
		  set_comment((yyvsp[-1].p), (yyvsp[0].str));
		}
#line 9147 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 290: /* valparms: rawvalparms  */
#line 5157 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9163 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 291: /* rawvalparms: valparm valptail  */
#line 5170 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   {
                  set_nextSibling((yyvsp[-1].p),(yyvsp[0].p));
                  (yyval.p) = (yyvsp[-1].p);
		}
#line 9172 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 292: /* rawvalparms: empty  */
#line 5174 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.p) = 0; }
#line 9178 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 293: /* valptail: COMMA valparm valptail  */
#line 5177 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                        {
                 set_nextSibling((yyvsp[-1].p),(yyvsp[0].p));
		 (yyval.p) = (yyvsp[-1].p);
                }
#line 9187 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 294: /* valptail: empty  */
#line 5181 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.p) = 0; }
#line 9193 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 295: /* valparm: parm  */
#line 5185 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9226 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 296: /* valparm: valexpr  */
#line 5213 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
                  (yyval.p) = NewParmWithoutFileLineInfo(0,0);
                  Setfile((yyval.p),cparse_file);
		  Setline((yyval.p),cparse_line);
		  Setattr((yyval.p),"value",(yyvsp[0].dtype).val);
               }
#line 9237 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 297: /* def_args: EQUAL definetype  */
#line 5221 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		    (yyval.dtype).final = 0;
		  }
               }
#line 9255 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 298: /* def_args: EQUAL definetype LBRACKET expr RBRACKET  */
#line 5234 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		    (yyval.dtype).final = 0;
		  } else {
		    (yyval.dtype).val = NewStringf("%s[%s]",(yyvsp[-3].dtype).val,(yyvsp[-1].dtype).val); 
		  }		  
               }
#line 9276 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 299: /* def_args: EQUAL LBRACE  */
#line 5250 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
		 skip_balanced('{','}');
		 (yyval.dtype).val = NewString(scanner_ccode);
		 (yyval.dtype).rawval = 0;
                 (yyval.dtype).type = T_INT;
		 (yyval.dtype).bitfield = 0;
		 (yyval.dtype).throws = 0;
		 (yyval.dtype).throwf = 0;
		 (yyval.dtype).nexcept = 0;
		 (yyval.dtype).final = 0;
	       }
#line 9292 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 300: /* def_args: COLON expr  */
#line 5261 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { 
		 (yyval.dtype).val = 0;
		 (yyval.dtype).rawval = 0;
		 (yyval.dtype).type = 0;
		 (yyval.dtype).bitfield = (yyvsp[0].dtype).val;
		 (yyval.dtype).throws = 0;
		 (yyval.dtype).throwf = 0;
		 (yyval.dtype).nexcept = 0;
		 (yyval.dtype).final = 0;
	       }
#line 9307 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 301: /* def_args: empty  */
#line 5271 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                 (yyval.dtype).val = 0;
                 (yyval.dtype).rawval = 0;
                 (yyval.dtype).type = T_INT;
		 (yyval.dtype).bitfield = 0;
		 (yyval.dtype).throws = 0;
		 (yyval.dtype).throwf = 0;
		 (yyval.dtype).nexcept = 0;
		 (yyval.dtype).final = 0;
               }
#line 9322 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 302: /* parameter_declarator: declarator def_args  */
#line 5283 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
                 (yyval.decl) = (yyvsp[-1].decl);
		 (yyval.decl).defarg = (yyvsp[0].dtype).rawval ? (yyvsp[0].dtype).rawval : (yyvsp[0].dtype).val;
            }
#line 9331 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 303: /* parameter_declarator: abstract_declarator def_args  */
#line 5287 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
              (yyval.decl) = (yyvsp[-1].decl);
	      (yyval.decl).defarg = (yyvsp[0].dtype).rawval ? (yyvsp[0].dtype).rawval : (yyvsp[0].dtype).val;
            }
#line 9340 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 304: /* parameter_declarator: def_args  */
#line 5291 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
   	      (yyval.decl).type = 0;
              (yyval.decl).id = 0;
	      (yyval.decl).defarg = (yyvsp[0].dtype).rawval ? (yyvsp[0].dtype).rawval : (yyvsp[0].dtype).val;
            }
#line 9350 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 305: /* parameter_declarator: direct_declarator LPAREN parms RPAREN cv_ref_qualifier  */
#line 5298 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9375 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 306: /* plain_declarator: declarator  */
#line 5320 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9397 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 307: /* plain_declarator: abstract_declarator  */
#line 5337 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9419 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 308: /* plain_declarator: direct_declarator LPAREN parms RPAREN cv_ref_qualifier  */
#line 5356 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9443 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 309: /* plain_declarator: empty  */
#line 5375 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                    {
   	      (yyval.decl).type = 0;
              (yyval.decl).id = 0;
	      (yyval.decl).parms = 0;
	      }
#line 9453 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 310: /* declarator: pointer notso_direct_declarator  */
#line 5382 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                              {
              (yyval.decl) = (yyvsp[0].decl);
	      if ((yyval.decl).type) {
		SwigType_push((yyvsp[-1].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-1].type);
           }
#line 9466 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 311: /* declarator: pointer AND notso_direct_declarator  */
#line 5390 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_reference((yyvsp[-2].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-2].type);
           }
#line 9480 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 312: /* declarator: pointer LAND notso_direct_declarator  */
#line 5399 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                  {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_rvalue_reference((yyvsp[-2].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-2].type);
           }
#line 9494 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 313: /* declarator: direct_declarator  */
#line 5408 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
              (yyval.decl) = (yyvsp[0].decl);
	      if (!(yyval.decl).type) (yyval.decl).type = NewStringEmpty();
           }
#line 9503 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 314: /* declarator: AND notso_direct_declarator  */
#line 5412 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
	     (yyval.decl) = (yyvsp[0].decl);
	     (yyval.decl).type = NewStringEmpty();
	     SwigType_add_reference((yyval.decl).type);
	     if ((yyvsp[0].decl).type) {
	       SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
	       Delete((yyvsp[0].decl).type);
	     }
           }
#line 9517 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 315: /* declarator: LAND notso_direct_declarator  */
#line 5421 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9533 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 316: /* declarator: idcolon DSTAR notso_direct_declarator  */
#line 5432 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9549 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 317: /* declarator: pointer idcolon DSTAR notso_direct_declarator  */
#line 5443 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9566 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 318: /* declarator: pointer idcolon DSTAR AND notso_direct_declarator  */
#line 5455 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9581 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 319: /* declarator: idcolon DSTAR AND notso_direct_declarator  */
#line 5465 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9597 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 320: /* declarator: pointer PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5479 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                   {
              (yyval.decl) = (yyvsp[0].decl);
	      if ((yyval.decl).type) {
		SwigType_push((yyvsp[-4].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-4].type);
           }
#line 9610 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 321: /* declarator: pointer AND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5487 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                      {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_reference((yyvsp[-5].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-5].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-5].type);
           }
#line 9624 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 322: /* declarator: pointer LAND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5496 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                       {
              (yyval.decl) = (yyvsp[0].decl);
	      SwigType_add_rvalue_reference((yyvsp[-5].type));
              if ((yyval.decl).type) {
		SwigType_push((yyvsp[-5].type),(yyval.decl).type);
		Delete((yyval.decl).type);
	      }
	      (yyval.decl).type = (yyvsp[-5].type);
           }
#line 9638 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 323: /* declarator: PERIOD PERIOD PERIOD direct_declarator  */
#line 5505 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                    {
              (yyval.decl) = (yyvsp[0].decl);
	      if (!(yyval.decl).type) (yyval.decl).type = NewStringEmpty();
           }
#line 9647 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 324: /* declarator: AND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5509 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                              {
	     (yyval.decl) = (yyvsp[0].decl);
	     (yyval.decl).type = NewStringEmpty();
	     SwigType_add_reference((yyval.decl).type);
	     if ((yyvsp[0].decl).type) {
	       SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
	       Delete((yyvsp[0].decl).type);
	     }
           }
#line 9661 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 325: /* declarator: LAND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5518 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9677 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 326: /* declarator: idcolon DSTAR PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5529 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9693 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 327: /* declarator: pointer idcolon DSTAR PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5540 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9710 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 328: /* declarator: pointer idcolon DSTAR AND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5552 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9725 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 329: /* declarator: pointer idcolon DSTAR LAND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5562 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9740 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 330: /* declarator: idcolon DSTAR AND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5572 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9756 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 331: /* declarator: idcolon DSTAR LAND PERIOD PERIOD PERIOD notso_direct_declarator  */
#line 5583 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9772 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 332: /* notso_direct_declarator: idcolon  */
#line 5596 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
                /* Note: This is non-standard C.  Template declarator is allowed to follow an identifier */
                 (yyval.decl).id = Char((yyvsp[0].str));
		 (yyval.decl).type = 0;
		 (yyval.decl).parms = 0;
		 (yyval.decl).have_parms = 0;
                  }
#line 9784 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 333: /* notso_direct_declarator: NOT idcolon  */
#line 5603 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
                  (yyval.decl).id = Char(NewStringf("~%s",(yyvsp[0].str)));
                  (yyval.decl).type = 0;
                  (yyval.decl).parms = 0;
                  (yyval.decl).have_parms = 0;
                  }
#line 9795 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 334: /* notso_direct_declarator: LPAREN idcolon RPAREN  */
#line 5611 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
                  (yyval.decl).id = Char((yyvsp[-1].str));
                  (yyval.decl).type = 0;
                  (yyval.decl).parms = 0;
                  (yyval.decl).have_parms = 0;
                  }
#line 9806 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 335: /* notso_direct_declarator: LPAREN pointer notso_direct_declarator RPAREN  */
#line 5627 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                  {
		    (yyval.decl) = (yyvsp[-1].decl);
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 9819 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 336: /* notso_direct_declarator: LPAREN idcolon DSTAR notso_direct_declarator RPAREN  */
#line 5635 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9835 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 337: /* notso_direct_declarator: notso_direct_declarator LBRACKET RBRACKET  */
#line 5646 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9851 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 338: /* notso_direct_declarator: notso_direct_declarator LBRACKET expr RBRACKET  */
#line 5657 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9867 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 339: /* notso_direct_declarator: notso_direct_declarator LPAREN parms RPAREN  */
#line 5668 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9889 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 340: /* direct_declarator: idcolon  */
#line 5687 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            {
                /* Note: This is non-standard C.  Template declarator is allowed to follow an identifier */
                 (yyval.decl).id = Char((yyvsp[0].str));
		 (yyval.decl).type = 0;
		 (yyval.decl).parms = 0;
		 (yyval.decl).have_parms = 0;
                  }
#line 9901 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 341: /* direct_declarator: NOT idcolon  */
#line 5695 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
                  (yyval.decl).id = Char(NewStringf("~%s",(yyvsp[0].str)));
                  (yyval.decl).type = 0;
                  (yyval.decl).parms = 0;
                  (yyval.decl).have_parms = 0;
                  }
#line 9912 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 342: /* direct_declarator: LPAREN pointer direct_declarator RPAREN  */
#line 5712 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                            {
		    (yyval.decl) = (yyvsp[-1].decl);
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 9925 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 343: /* direct_declarator: LPAREN AND direct_declarator RPAREN  */
#line 5720 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                        {
                    (yyval.decl) = (yyvsp[-1].decl);
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = NewStringEmpty();
		    }
		    SwigType_add_reference((yyval.decl).type);
                  }
#line 9937 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 344: /* direct_declarator: LPAREN LAND direct_declarator RPAREN  */
#line 5727 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                         {
                    (yyval.decl) = (yyvsp[-1].decl);
		    if (!(yyval.decl).type) {
		      (yyval.decl).type = NewStringEmpty();
		    }
		    SwigType_add_rvalue_reference((yyval.decl).type);
                  }
#line 9949 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 345: /* direct_declarator: LPAREN idcolon DSTAR declarator RPAREN  */
#line 5734 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9965 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 346: /* direct_declarator: LPAREN idcolon DSTAR type_qualifier declarator RPAREN  */
#line 5745 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9982 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 347: /* direct_declarator: LPAREN idcolon DSTAR abstract_declarator RPAREN  */
#line 5757 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 9998 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 348: /* direct_declarator: LPAREN idcolon DSTAR type_qualifier abstract_declarator RPAREN  */
#line 5768 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10015 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 349: /* direct_declarator: direct_declarator LBRACKET RBRACKET  */
#line 5780 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10031 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 350: /* direct_declarator: direct_declarator LBRACKET expr RBRACKET  */
#line 5791 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10047 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 351: /* direct_declarator: direct_declarator LPAREN parms RPAREN  */
#line 5802 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10069 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 352: /* direct_declarator: OPERATOR ID LPAREN parms RPAREN  */
#line 5822 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10093 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 353: /* abstract_declarator: pointer  */
#line 5843 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
		    (yyval.decl).type = (yyvsp[0].type);
                    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
                  }
#line 10104 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 354: /* abstract_declarator: pointer direct_abstract_declarator  */
#line 5849 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                       { 
                     (yyval.decl) = (yyvsp[0].decl);
                     SwigType_push((yyvsp[-1].type),(yyvsp[0].decl).type);
		     (yyval.decl).type = (yyvsp[-1].type);
		     Delete((yyvsp[0].decl).type);
                  }
#line 10115 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 355: /* abstract_declarator: pointer AND  */
#line 5855 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
		    (yyval.decl).type = (yyvsp[-1].type);
		    SwigType_add_reference((yyval.decl).type);
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		  }
#line 10127 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 356: /* abstract_declarator: pointer LAND  */
#line 5862 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		    (yyval.decl).type = (yyvsp[-1].type);
		    SwigType_add_rvalue_reference((yyval.decl).type);
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		  }
#line 10139 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 357: /* abstract_declarator: pointer AND direct_abstract_declarator  */
#line 5869 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                           {
		    (yyval.decl) = (yyvsp[0].decl);
		    SwigType_add_reference((yyvsp[-2].type));
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 10153 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 358: /* abstract_declarator: pointer LAND direct_abstract_declarator  */
#line 5878 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                            {
		    (yyval.decl) = (yyvsp[0].decl);
		    SwigType_add_rvalue_reference((yyvsp[-2].type));
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-2].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-2].type);
                  }
#line 10167 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 359: /* abstract_declarator: direct_abstract_declarator  */
#line 5887 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                               {
		    (yyval.decl) = (yyvsp[0].decl);
                  }
#line 10175 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 360: /* abstract_declarator: AND direct_abstract_declarator  */
#line 5890 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                   {
		    (yyval.decl) = (yyvsp[0].decl);
		    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_reference((yyval.decl).type);
		    if ((yyvsp[0].decl).type) {
		      SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
		      Delete((yyvsp[0].decl).type);
		    }
                  }
#line 10189 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 361: /* abstract_declarator: LAND direct_abstract_declarator  */
#line 5899 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                    {
		    (yyval.decl) = (yyvsp[0].decl);
		    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_rvalue_reference((yyval.decl).type);
		    if ((yyvsp[0].decl).type) {
		      SwigType_push((yyval.decl).type,(yyvsp[0].decl).type);
		      Delete((yyvsp[0].decl).type);
		    }
                  }
#line 10203 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 362: /* abstract_declarator: AND  */
#line 5908 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
                    (yyval.decl).id = 0;
                    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
                    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_reference((yyval.decl).type);
                  }
#line 10215 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 363: /* abstract_declarator: LAND  */
#line 5915 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
                    (yyval.decl).id = 0;
                    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
                    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_rvalue_reference((yyval.decl).type);
                  }
#line 10227 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 364: /* abstract_declarator: idcolon DSTAR  */
#line 5922 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  { 
		    (yyval.decl).type = NewStringEmpty();
                    SwigType_add_memberpointer((yyval.decl).type,(yyvsp[-1].str));
                    (yyval.decl).id = 0;
                    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
      	          }
#line 10239 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 365: /* abstract_declarator: idcolon DSTAR type_qualifier  */
#line 5929 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 {
		    (yyval.decl).type = NewStringEmpty();
		    SwigType_add_memberpointer((yyval.decl).type, (yyvsp[-2].str));
		    SwigType_push((yyval.decl).type, (yyvsp[0].str));
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		  }
#line 10252 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 366: /* abstract_declarator: pointer idcolon DSTAR  */
#line 5937 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10267 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 367: /* abstract_declarator: pointer idcolon DSTAR direct_abstract_declarator  */
#line 5947 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                     { 
		    (yyval.decl) = (yyvsp[0].decl);
		    SwigType_add_memberpointer((yyvsp[-3].type),(yyvsp[-2].str));
		    if ((yyval.decl).type) {
		      SwigType_push((yyvsp[-3].type),(yyval.decl).type);
		      Delete((yyval.decl).type);
		    }
		    (yyval.decl).type = (yyvsp[-3].type);
                  }
#line 10281 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 368: /* direct_abstract_declarator: direct_abstract_declarator LBRACKET RBRACKET  */
#line 5958 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10297 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 369: /* direct_abstract_declarator: direct_abstract_declarator LBRACKET expr RBRACKET  */
#line 5969 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10313 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 370: /* direct_abstract_declarator: LBRACKET RBRACKET  */
#line 5980 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                      { 
		    (yyval.decl).type = NewStringEmpty();
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		    SwigType_add_array((yyval.decl).type,"");
                  }
#line 10325 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 371: /* direct_abstract_declarator: LBRACKET expr RBRACKET  */
#line 5987 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           { 
		    (yyval.decl).type = NewStringEmpty();
		    (yyval.decl).id = 0;
		    (yyval.decl).parms = 0;
		    (yyval.decl).have_parms = 0;
		    SwigType_add_array((yyval.decl).type,(yyvsp[-1].dtype).val);
		  }
#line 10337 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 372: /* direct_abstract_declarator: LPAREN abstract_declarator RPAREN  */
#line 5994 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                      {
                    (yyval.decl) = (yyvsp[-1].decl);
		  }
#line 10345 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 373: /* direct_abstract_declarator: direct_abstract_declarator LPAREN parms RPAREN  */
#line 5997 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10367 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 374: /* direct_abstract_declarator: direct_abstract_declarator LPAREN parms RPAREN cv_ref_qualifier  */
#line 6014 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10390 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 375: /* direct_abstract_declarator: LPAREN parms RPAREN  */
#line 6032 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                        {
                    (yyval.decl).type = NewStringEmpty();
                    SwigType_add_function((yyval.decl).type,(yyvsp[-1].pl));
		    (yyval.decl).parms = (yyvsp[-1].pl);
		    (yyval.decl).have_parms = 1;
		    (yyval.decl).id = 0;
                  }
#line 10402 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 376: /* pointer: STAR type_qualifier pointer  */
#line 6042 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         { 
             (yyval.type) = NewStringEmpty();
             SwigType_add_pointer((yyval.type));
	     SwigType_push((yyval.type),(yyvsp[-1].str));
	     SwigType_push((yyval.type),(yyvsp[0].type));
	     Delete((yyvsp[0].type));
           }
#line 10414 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 377: /* pointer: STAR pointer  */
#line 6049 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
	     (yyval.type) = NewStringEmpty();
	     SwigType_add_pointer((yyval.type));
	     SwigType_push((yyval.type),(yyvsp[0].type));
	     Delete((yyvsp[0].type));
	   }
#line 10425 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 378: /* pointer: STAR type_qualifier  */
#line 6055 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { 
	     (yyval.type) = NewStringEmpty();
	     SwigType_add_pointer((yyval.type));
	     SwigType_push((yyval.type),(yyvsp[0].str));
           }
#line 10435 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 379: /* pointer: STAR  */
#line 6060 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                  {
	     (yyval.type) = NewStringEmpty();
	     SwigType_add_pointer((yyval.type));
           }
#line 10444 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 380: /* cv_ref_qualifier: type_qualifier  */
#line 6067 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		  (yyval.dtype).qualifier = (yyvsp[0].str);
		  (yyval.dtype).refqualifier = 0;
	       }
#line 10453 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 381: /* cv_ref_qualifier: type_qualifier ref_qualifier  */
#line 6071 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                              {
		  (yyval.dtype).qualifier = (yyvsp[-1].str);
		  (yyval.dtype).refqualifier = (yyvsp[0].str);
		  SwigType_push((yyval.dtype).qualifier, (yyvsp[0].str));
	       }
#line 10463 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 382: /* cv_ref_qualifier: ref_qualifier  */
#line 6076 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
		  (yyval.dtype).qualifier = NewStringEmpty();
		  (yyval.dtype).refqualifier = (yyvsp[0].str);
		  SwigType_push((yyval.dtype).qualifier, (yyvsp[0].str));
	       }
#line 10473 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 383: /* ref_qualifier: AND  */
#line 6083 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                    {
	          (yyval.str) = NewStringEmpty();
	          SwigType_add_reference((yyval.str));
	       }
#line 10482 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 384: /* ref_qualifier: LAND  */
#line 6087 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                      {
	          (yyval.str) = NewStringEmpty();
	          SwigType_add_rvalue_reference((yyval.str));
	       }
#line 10491 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 385: /* type_qualifier: type_qualifier_raw  */
#line 6093 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    {
	          (yyval.str) = NewStringEmpty();
	          if ((yyvsp[0].id)) SwigType_add_qualifier((yyval.str),(yyvsp[0].id));
               }
#line 10500 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 386: /* type_qualifier: type_qualifier_raw type_qualifier  */
#line 6097 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                   {
		  (yyval.str) = (yyvsp[0].str);
	          if ((yyvsp[-1].id)) SwigType_add_qualifier((yyval.str),(yyvsp[-1].id));
               }
#line 10509 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 387: /* type_qualifier_raw: CONST_QUAL  */
#line 6103 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { (yyval.id) = "const"; }
#line 10515 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 388: /* type_qualifier_raw: VOLATILE  */
#line 6104 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.id) = "volatile"; }
#line 10521 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 389: /* type_qualifier_raw: REGISTER  */
#line 6105 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.id) = 0; }
#line 10527 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 390: /* type: rawtype  */
#line 6111 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                   (yyval.type) = (yyvsp[0].type);
                   Replace((yyval.type),"typename ","", DOH_REPLACE_ANY);
                }
#line 10536 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 391: /* rawtype: type_qualifier type_right  */
#line 6117 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
                   (yyval.type) = (yyvsp[0].type);
	           SwigType_push((yyval.type),(yyvsp[-1].str));
               }
#line 10545 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 392: /* rawtype: type_right  */
#line 6121 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { (yyval.type) = (yyvsp[0].type); }
#line 10551 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 393: /* rawtype: type_right type_qualifier  */
#line 6122 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
		  (yyval.type) = (yyvsp[-1].type);
	          SwigType_push((yyval.type),(yyvsp[0].str));
	       }
#line 10560 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 394: /* rawtype: type_qualifier type_right type_qualifier  */
#line 6126 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                          {
		  (yyval.type) = (yyvsp[-1].type);
	          SwigType_push((yyval.type),(yyvsp[0].str));
	          SwigType_push((yyval.type),(yyvsp[-2].str));
	       }
#line 10570 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 395: /* type_right: primitive_type  */
#line 6133 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.type) = (yyvsp[0].type);
                  /* Printf(stdout,"primitive = '%s'\n", $$);*/
               }
#line 10578 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 396: /* type_right: TYPE_BOOL  */
#line 6136 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.type) = (yyvsp[0].type); }
#line 10584 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 397: /* type_right: TYPE_VOID  */
#line 6137 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.type) = (yyvsp[0].type); }
#line 10590 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 398: /* type_right: c_enum_key idcolon  */
#line 6141 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { (yyval.type) = NewStringf("enum %s", (yyvsp[0].str)); }
#line 10596 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 399: /* type_right: TYPE_RAW  */
#line 6142 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.type) = (yyvsp[0].type); }
#line 10602 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 400: /* type_right: idcolon  */
#line 6144 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		  (yyval.type) = (yyvsp[0].str);
               }
#line 10610 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 401: /* type_right: cpptype idcolon  */
#line 6147 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 { 
		 (yyval.type) = NewStringf("%s %s", (yyvsp[-1].id), (yyvsp[0].str));
               }
#line 10618 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 402: /* type_right: decltype  */
#line 6150 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                 (yyval.type) = (yyvsp[0].type);
               }
#line 10626 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 403: /* decltype: DECLTYPE LPAREN idcolon RPAREN  */
#line 6155 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                {
                 Node *n = Swig_symbol_clookup((yyvsp[-1].str),0);
                 if (!n) {
		   Swig_error(cparse_file, cparse_line, "Identifier %s not defined.\n", (yyvsp[-1].str));
                   (yyval.type) = (yyvsp[-1].str);
                 } else {
                   (yyval.type) = Getattr(n, "type");
                 }
               }
#line 10640 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 404: /* primitive_type: primitive_type_list  */
#line 6166 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10668 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 405: /* primitive_type_list: type_specifier  */
#line 6191 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     { 
                 (yyval.ptype) = (yyvsp[0].ptype);
               }
#line 10676 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 406: /* primitive_type_list: type_specifier primitive_type_list  */
#line 6194 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 10732 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 407: /* type_specifier: TYPE_INT  */
#line 6248 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { 
		    (yyval.ptype).type = NewString("int");
                    (yyval.ptype).us = 0;
               }
#line 10741 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 408: /* type_specifier: TYPE_SHORT  */
#line 6252 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { 
                    (yyval.ptype).type = NewString("short");
                    (yyval.ptype).us = 0;
                }
#line 10750 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 409: /* type_specifier: TYPE_LONG  */
#line 6256 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { 
                    (yyval.ptype).type = NewString("long");
                    (yyval.ptype).us = 0;
                }
#line 10759 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 410: /* type_specifier: TYPE_CHAR  */
#line 6260 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { 
                    (yyval.ptype).type = NewString("char");
                    (yyval.ptype).us = 0;
                }
#line 10768 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 411: /* type_specifier: TYPE_WCHAR  */
#line 6264 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { 
                    (yyval.ptype).type = NewString("wchar_t");
                    (yyval.ptype).us = 0;
                }
#line 10777 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 412: /* type_specifier: TYPE_FLOAT  */
#line 6268 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { 
                    (yyval.ptype).type = NewString("float");
                    (yyval.ptype).us = 0;
                }
#line 10786 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 413: /* type_specifier: TYPE_DOUBLE  */
#line 6272 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             { 
                    (yyval.ptype).type = NewString("double");
                    (yyval.ptype).us = 0;
                }
#line 10795 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 414: /* type_specifier: TYPE_SIGNED  */
#line 6276 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             { 
                    (yyval.ptype).us = NewString("signed");
                    (yyval.ptype).type = 0;
                }
#line 10804 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 415: /* type_specifier: TYPE_UNSIGNED  */
#line 6280 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { 
                    (yyval.ptype).us = NewString("unsigned");
                    (yyval.ptype).type = 0;
                }
#line 10813 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 416: /* type_specifier: TYPE_COMPLEX  */
#line 6284 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { 
                    (yyval.ptype).type = NewString("complex");
                    (yyval.ptype).us = 0;
                }
#line 10822 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 417: /* type_specifier: TYPE_NON_ISO_INT8  */
#line 6288 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   { 
                    (yyval.ptype).type = NewString("__int8");
                    (yyval.ptype).us = 0;
                }
#line 10831 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 418: /* type_specifier: TYPE_NON_ISO_INT16  */
#line 6292 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { 
                    (yyval.ptype).type = NewString("__int16");
                    (yyval.ptype).us = 0;
                }
#line 10840 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 419: /* type_specifier: TYPE_NON_ISO_INT32  */
#line 6296 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { 
                    (yyval.ptype).type = NewString("__int32");
                    (yyval.ptype).us = 0;
                }
#line 10849 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 420: /* type_specifier: TYPE_NON_ISO_INT64  */
#line 6300 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    { 
                    (yyval.ptype).type = NewString("__int64");
                    (yyval.ptype).us = 0;
                }
#line 10858 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 421: /* $@13: %empty  */
#line 6306 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                 { /* scanner_check_typedef(); */ }
#line 10864 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 422: /* definetype: $@13 expr  */
#line 6306 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		   (yyval.dtype).final = 0;
		   scanner_ignore_typedef();
                }
#line 10885 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 423: /* definetype: default_delete  */
#line 6322 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		  (yyval.dtype) = (yyvsp[0].dtype);
		}
#line 10893 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 424: /* default_delete: deleted_definition  */
#line 6327 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    {
		  (yyval.dtype) = (yyvsp[0].dtype);
		}
#line 10901 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 425: /* default_delete: explicit_default  */
#line 6330 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   {
		  (yyval.dtype) = (yyvsp[0].dtype);
		}
#line 10909 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 426: /* deleted_definition: DELETE_KW  */
#line 6336 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		  (yyval.dtype).final = 0;
		}
#line 10926 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 427: /* explicit_default: DEFAULT  */
#line 6351 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		  (yyval.dtype).final = 0;
		}
#line 10943 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 428: /* ename: identifier  */
#line 6367 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             { (yyval.id) = (yyvsp[0].id); }
#line 10949 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 429: /* ename: empty  */
#line 6368 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.id) = (char *) 0;}
#line 10955 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 434: /* enumlist: enumlist_item  */
#line 6381 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
		  Setattr((yyvsp[0].node),"_last",(yyvsp[0].node));
		  (yyval.node) = (yyvsp[0].node);
		}
#line 10964 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 435: /* enumlist: enumlist_item DOXYGENPOSTSTRING  */
#line 6385 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                  {
		  Setattr((yyvsp[-1].node),"_last",(yyvsp[-1].node));
		  set_comment((yyvsp[-1].node), (yyvsp[0].str));
		  (yyval.node) = (yyvsp[-1].node);
		}
#line 10974 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 436: /* enumlist: enumlist_item COMMA enumlist  */
#line 6390 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                               {
		  if ((yyvsp[0].node)) {
		    set_nextSibling((yyvsp[-2].node), (yyvsp[0].node));
		    Setattr((yyvsp[-2].node),"_last",Getattr((yyvsp[0].node),"_last"));
		    Setattr((yyvsp[0].node),"_last",NULL);
		  }
		  (yyval.node) = (yyvsp[-2].node);
		}
#line 10987 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 437: /* enumlist: enumlist_item COMMA DOXYGENPOSTSTRING enumlist  */
#line 6398 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
		  if ((yyvsp[0].node)) {
		    set_nextSibling((yyvsp[-3].node), (yyvsp[0].node));
		    Setattr((yyvsp[-3].node),"_last",Getattr((yyvsp[0].node),"_last"));
		    Setattr((yyvsp[0].node),"_last",NULL);
		  }
		  set_comment((yyvsp[-3].node), (yyvsp[-1].str));
		  (yyval.node) = (yyvsp[-3].node);
		}
#line 11001 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 438: /* enumlist: optional_ignored_defines  */
#line 6407 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
		  (yyval.node) = 0;
		}
#line 11009 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 439: /* enumlist_item: optional_ignored_defines edecl_with_dox optional_ignored_defines  */
#line 6412 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                   {
		  (yyval.node) = (yyvsp[-1].node);
		}
#line 11017 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 440: /* edecl_with_dox: edecl  */
#line 6417 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
		  (yyval.node) = (yyvsp[0].node);
		}
#line 11025 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 441: /* edecl_with_dox: DOXYGENSTRING edecl  */
#line 6420 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                      {
		  (yyval.node) = (yyvsp[0].node);
		  set_comment((yyvsp[0].node), (yyvsp[-1].str));
		}
#line 11034 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 442: /* edecl: identifier  */
#line 6426 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		   SwigType *type = NewSwigType(T_INT);
		   (yyval.node) = new_node("enumitem");
		   Setattr((yyval.node),"name",(yyvsp[0].id));
		   Setattr((yyval.node),"type",type);
		   SetFlag((yyval.node),"feature:immutable");
		   Delete(type);
		 }
#line 11047 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 443: /* edecl: identifier EQUAL etype  */
#line 6434 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11062 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 444: /* etype: expr  */
#line 6446 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11078 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 445: /* expr: valexpr  */
#line 6461 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11084 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 446: /* expr: type  */
#line 6462 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11106 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 447: /* exprmem: ID ARROW ID  */
#line 6482 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		 (yyval.dtype).val = NewStringf("%s->%s", (yyvsp[-2].id), (yyvsp[0].id));
		 (yyval.dtype).type = 0;
	       }
#line 11115 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 448: /* exprmem: exprmem ARROW ID  */
#line 6486 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		 (yyval.dtype) = (yyvsp[-2].dtype);
		 Printf((yyval.dtype).val, "->%s", (yyvsp[0].id));
	       }
#line 11124 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 449: /* exprmem: exprmem PERIOD ID  */
#line 6496 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   {
		 (yyval.dtype) = (yyvsp[-2].dtype);
		 Printf((yyval.dtype).val, ".%s", (yyvsp[0].id));
	       }
#line 11133 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 450: /* valexpr: exprnum  */
#line 6502 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		    (yyval.dtype) = (yyvsp[0].dtype);
               }
#line 11141 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 451: /* valexpr: exprmem  */
#line 6505 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		    (yyval.dtype) = (yyvsp[0].dtype);
               }
#line 11149 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 452: /* valexpr: string  */
#line 6508 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
		    (yyval.dtype).val = (yyvsp[0].str);
                    (yyval.dtype).type = T_STRING;
               }
#line 11158 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 453: /* valexpr: SIZEOF LPAREN type parameter_declarator RPAREN  */
#line 6512 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                {
		  SwigType_push((yyvsp[-2].type),(yyvsp[-1].decl).type);
		  (yyval.dtype).val = NewStringf("sizeof(%s)",SwigType_str((yyvsp[-2].type),0));
		  (yyval.dtype).type = T_ULONG;
               }
#line 11168 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 454: /* valexpr: SIZEOF PERIOD PERIOD PERIOD LPAREN type parameter_declarator RPAREN  */
#line 6517 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                                     {
		  SwigType_push((yyvsp[-2].type),(yyvsp[-1].decl).type);
		  (yyval.dtype).val = NewStringf("sizeof...(%s)",SwigType_str((yyvsp[-2].type),0));
		  (yyval.dtype).type = T_ULONG;
               }
#line 11178 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 455: /* valexpr: exprcompound  */
#line 6522 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11184 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 456: /* valexpr: wstring  */
#line 6523 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
		    (yyval.dtype).val = (yyvsp[0].str);
		    (yyval.dtype).rawval = NewStringf("L\"%s\"", (yyval.dtype).val);
                    (yyval.dtype).type = T_WSTRING;
	       }
#line 11194 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 457: /* valexpr: CHARCONST  */
#line 6528 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		  (yyval.dtype).final = 0;
	       }
#line 11213 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 458: /* valexpr: WCHARCONST  */
#line 6542 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
		  (yyval.dtype).final = 0;
	       }
#line 11232 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 459: /* valexpr: LPAREN expr RPAREN  */
#line 6558 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                {
		    (yyval.dtype).val = NewStringf("(%s)",(yyvsp[-1].dtype).val);
		    if ((yyvsp[-1].dtype).rawval) {
		      (yyval.dtype).rawval = NewStringf("(%s)",(yyvsp[-1].dtype).rawval);
		    }
		    (yyval.dtype).type = (yyvsp[-1].dtype).type;
	       }
#line 11244 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 460: /* valexpr: LPAREN expr RPAREN expr  */
#line 6568 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11267 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 461: /* valexpr: LPAREN expr pointer RPAREN expr  */
#line 6586 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                            {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_push((yyvsp[-3].dtype).val,(yyvsp[-2].type));
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-3].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11279 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 462: /* valexpr: LPAREN expr AND RPAREN expr  */
#line 6593 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                        {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_add_reference((yyvsp[-3].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-3].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11291 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 463: /* valexpr: LPAREN expr LAND RPAREN expr  */
#line 6600 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                         {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_add_rvalue_reference((yyvsp[-3].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-3].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11303 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 464: /* valexpr: LPAREN expr pointer AND RPAREN expr  */
#line 6607 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_push((yyvsp[-4].dtype).val,(yyvsp[-3].type));
		   SwigType_add_reference((yyvsp[-4].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-4].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11316 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 465: /* valexpr: LPAREN expr pointer LAND RPAREN expr  */
#line 6615 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                 {
                 (yyval.dtype) = (yyvsp[0].dtype);
		 if ((yyvsp[0].dtype).type != T_STRING) {
		   SwigType_push((yyvsp[-4].dtype).val,(yyvsp[-3].type));
		   SwigType_add_rvalue_reference((yyvsp[-4].dtype).val);
		   (yyval.dtype).val = NewStringf("(%s) %s", SwigType_str((yyvsp[-4].dtype).val,0), (yyvsp[0].dtype).val);
		 }
 	       }
#line 11329 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 466: /* valexpr: AND expr  */
#line 6623 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
		 (yyval.dtype) = (yyvsp[0].dtype);
                 (yyval.dtype).val = NewStringf("&%s",(yyvsp[0].dtype).val);
	       }
#line 11338 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 467: /* valexpr: LAND expr  */
#line 6627 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
		 (yyval.dtype) = (yyvsp[0].dtype);
                 (yyval.dtype).val = NewStringf("&&%s",(yyvsp[0].dtype).val);
	       }
#line 11347 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 468: /* valexpr: STAR expr  */
#line 6631 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
		 (yyval.dtype) = (yyvsp[0].dtype);
                 (yyval.dtype).val = NewStringf("*%s",(yyvsp[0].dtype).val);
	       }
#line 11356 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 469: /* exprnum: NUM_INT  */
#line 6637 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11362 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 470: /* exprnum: NUM_FLOAT  */
#line 6638 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11368 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 471: /* exprnum: NUM_UNSIGNED  */
#line 6639 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11374 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 472: /* exprnum: NUM_LONG  */
#line 6640 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11380 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 473: /* exprnum: NUM_ULONG  */
#line 6641 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11386 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 474: /* exprnum: NUM_LONGLONG  */
#line 6642 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11392 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 475: /* exprnum: NUM_ULONGLONG  */
#line 6643 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11398 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 476: /* exprnum: NUM_BOOL  */
#line 6644 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.dtype) = (yyvsp[0].dtype); }
#line 11404 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 477: /* exprcompound: expr PLUS expr  */
#line 6647 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
		 (yyval.dtype).val = NewStringf("%s+%s", COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11413 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 478: /* exprcompound: expr MINUS expr  */
#line 6651 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		 (yyval.dtype).val = NewStringf("%s-%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11422 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 479: /* exprcompound: expr STAR expr  */
#line 6655 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
		 (yyval.dtype).val = NewStringf("%s*%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11431 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 480: /* exprcompound: expr SLASH expr  */
#line 6659 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		 (yyval.dtype).val = NewStringf("%s/%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11440 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 481: /* exprcompound: expr MODULO expr  */
#line 6663 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		 (yyval.dtype).val = NewStringf("%s%%%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11449 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 482: /* exprcompound: expr AND expr  */
#line 6667 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
		 (yyval.dtype).val = NewStringf("%s&%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11458 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 483: /* exprcompound: expr OR expr  */
#line 6671 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
		 (yyval.dtype).val = NewStringf("%s|%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11467 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 484: /* exprcompound: expr XOR expr  */
#line 6675 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
		 (yyval.dtype).val = NewStringf("%s^%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type,(yyvsp[0].dtype).type);
	       }
#line 11476 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 485: /* exprcompound: expr LSHIFT expr  */
#line 6679 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		 (yyval.dtype).val = NewStringf("%s << %s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote_type((yyvsp[-2].dtype).type);
	       }
#line 11485 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 486: /* exprcompound: expr RSHIFT expr  */
#line 6683 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		 (yyval.dtype).val = NewStringf("%s >> %s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = promote_type((yyvsp[-2].dtype).type);
	       }
#line 11494 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 487: /* exprcompound: expr LAND expr  */
#line 6687 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
		 (yyval.dtype).val = NewStringf("%s&&%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11503 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 488: /* exprcompound: expr LOR expr  */
#line 6691 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               {
		 (yyval.dtype).val = NewStringf("%s||%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11512 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 489: /* exprcompound: expr EQUALTO expr  */
#line 6695 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   {
		 (yyval.dtype).val = NewStringf("%s==%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11521 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 490: /* exprcompound: expr NOTEQUALTO expr  */
#line 6699 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                      {
		 (yyval.dtype).val = NewStringf("%s!=%s",COMPOUND_EXPR_VAL((yyvsp[-2].dtype)),COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11530 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 491: /* exprcompound: expr GREATERTHANOREQUALTO expr  */
#line 6713 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                {
		 (yyval.dtype).val = NewStringf("%s >= %s", COMPOUND_EXPR_VAL((yyvsp[-2].dtype)), COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11539 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 492: /* exprcompound: expr LESSTHANOREQUALTO expr  */
#line 6717 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                             {
		 (yyval.dtype).val = NewStringf("%s <= %s", COMPOUND_EXPR_VAL((yyvsp[-2].dtype)), COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = cparse_cplusplus ? T_BOOL : T_INT;
	       }
#line 11548 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 493: /* exprcompound: expr QUESTIONMARK expr COLON expr  */
#line 6721 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                      {
		 (yyval.dtype).val = NewStringf("%s?%s:%s", COMPOUND_EXPR_VAL((yyvsp[-4].dtype)), COMPOUND_EXPR_VAL((yyvsp[-2].dtype)), COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 /* This may not be exactly right, but is probably good enough
		  * for the purposes of parsing constant expressions. */
		 (yyval.dtype).type = promote((yyvsp[-2].dtype).type, (yyvsp[0].dtype).type);
	       }
#line 11559 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 494: /* exprcompound: MINUS expr  */
#line 6727 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
		 (yyval.dtype).val = NewStringf("-%s",(yyvsp[0].dtype).val);
		 (yyval.dtype).type = (yyvsp[0].dtype).type;
	       }
#line 11568 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 495: /* exprcompound: PLUS expr  */
#line 6731 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                        {
                 (yyval.dtype).val = NewStringf("+%s",(yyvsp[0].dtype).val);
		 (yyval.dtype).type = (yyvsp[0].dtype).type;
	       }
#line 11577 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 496: /* exprcompound: NOT expr  */
#line 6735 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
		 (yyval.dtype).val = NewStringf("~%s",(yyvsp[0].dtype).val);
		 (yyval.dtype).type = (yyvsp[0].dtype).type;
	       }
#line 11586 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 497: /* exprcompound: LNOT expr  */
#line 6739 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
                 (yyval.dtype).val = NewStringf("!%s",COMPOUND_EXPR_VAL((yyvsp[0].dtype)));
		 (yyval.dtype).type = T_INT;
	       }
#line 11595 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 498: /* exprcompound: type LPAREN  */
#line 6743 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11614 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 499: /* ellipsis: PERIOD PERIOD PERIOD  */
#line 6759 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                     {
	        (yyval.str) = NewString("...");
	      }
#line 11622 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 500: /* variadic: ellipsis  */
#line 6764 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
	        (yyval.str) = (yyvsp[0].str);
	      }
#line 11630 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 501: /* variadic: empty  */
#line 6767 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                      {
	        (yyval.str) = 0;
	      }
#line 11638 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 502: /* inherit: raw_inherit  */
#line 6772 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		 (yyval.bases) = (yyvsp[0].bases);
               }
#line 11646 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 503: /* $@14: %empty  */
#line 6777 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { inherit_list = 1; }
#line 11652 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 504: /* raw_inherit: COLON $@14 base_list  */
#line 6777 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                        { (yyval.bases) = (yyvsp[0].bases); inherit_list = 0; }
#line 11658 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 505: /* raw_inherit: empty  */
#line 6778 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.bases) = 0; }
#line 11664 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 506: /* base_list: base_specifier  */
#line 6781 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11685 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 507: /* base_list: base_list COMMA base_specifier  */
#line 6798 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                {
		   Hash *list = (yyvsp[-2].bases);
		   Node *base = (yyvsp[0].node);
		   Node *name = Getattr(base,"name");
		   Append(Getattr(list,Getattr(base,"access")),name);
                   (yyval.bases) = list;
               }
#line 11697 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 508: /* @15: %empty  */
#line 6807 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                             {
		 (yyval.intvalue) = cparse_line;
	       }
#line 11705 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 509: /* base_specifier: opt_virtual @15 idcolon variadic  */
#line 6809 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11726 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 510: /* @16: %empty  */
#line 6825 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                              {
		 (yyval.intvalue) = cparse_line;
	       }
#line 11734 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 511: /* base_specifier: opt_virtual access_specifier @16 opt_virtual idcolon variadic  */
#line 6827 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 11753 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 512: /* access_specifier: PUBLIC  */
#line 6843 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.id) = (char*)"public"; }
#line 11759 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 513: /* access_specifier: PRIVATE  */
#line 6844 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.id) = (char*)"private"; }
#line 11765 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 514: /* access_specifier: PROTECTED  */
#line 6845 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           { (yyval.id) = (char*)"protected"; }
#line 11771 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 515: /* templcpptype: CLASS  */
#line 6848 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { 
                   (yyval.id) = (char*)"class"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11780 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 516: /* templcpptype: TYPENAME  */
#line 6852 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { 
                   (yyval.id) = (char *)"typename"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11789 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 517: /* templcpptype: CLASS PERIOD PERIOD PERIOD  */
#line 6856 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            { 
                   (yyval.id) = (char *)"class..."; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11798 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 518: /* templcpptype: TYPENAME PERIOD PERIOD PERIOD  */
#line 6860 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                               { 
                   (yyval.id) = (char *)"typename..."; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11807 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 519: /* cpptype: templcpptype  */
#line 6866 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
                 (yyval.id) = (yyvsp[0].id);
               }
#line 11815 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 520: /* cpptype: STRUCT  */
#line 6869 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { 
                   (yyval.id) = (char*)"struct"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11824 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 521: /* cpptype: UNION  */
#line 6873 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                   (yyval.id) = (char*)"union"; 
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11833 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 522: /* classkey: CLASS  */
#line 6879 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                   (yyval.id) = (char*)"class";
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11842 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 523: /* classkey: STRUCT  */
#line 6883 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
                   (yyval.id) = (char*)"struct";
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11851 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 524: /* classkey: UNION  */
#line 6887 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                   (yyval.id) = (char*)"union";
		   if (!inherit_list) last_cpptype = (yyval.id);
               }
#line 11860 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 525: /* classkeyopt: classkey  */
#line 6893 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
		   (yyval.id) = (yyvsp[0].id);
               }
#line 11868 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 526: /* classkeyopt: empty  */
#line 6896 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
		   (yyval.id) = 0;
               }
#line 11876 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 529: /* virt_specifier_seq: OVERRIDE  */
#line 6905 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
                   (yyval.str) = 0;
	       }
#line 11884 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 530: /* virt_specifier_seq: FINAL  */
#line 6908 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                   (yyval.str) = NewString("1");
	       }
#line 11892 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 531: /* virt_specifier_seq: FINAL OVERRIDE  */
#line 6911 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
                   (yyval.str) = NewString("1");
	       }
#line 11900 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 532: /* virt_specifier_seq: OVERRIDE FINAL  */
#line 6914 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
                   (yyval.str) = NewString("1");
	       }
#line 11908 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 533: /* virt_specifier_seq_opt: virt_specifier_seq  */
#line 6919 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            {
                   (yyval.str) = (yyvsp[0].str);
               }
#line 11916 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 534: /* virt_specifier_seq_opt: empty  */
#line 6922 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
                   (yyval.str) = 0;
               }
#line 11924 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 535: /* exception_specification: THROW LPAREN parms RPAREN  */
#line 6927 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                    {
                    (yyval.dtype).throws = (yyvsp[-1].pl);
                    (yyval.dtype).throwf = NewString("1");
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).final = 0;
	       }
#line 11935 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 536: /* exception_specification: NOEXCEPT  */
#line 6933 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = NewString("true");
                    (yyval.dtype).final = 0;
	       }
#line 11946 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 537: /* exception_specification: virt_specifier_seq  */
#line 6939 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                    {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).final = (yyvsp[0].str);
	       }
#line 11957 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 538: /* exception_specification: THROW LPAREN parms RPAREN virt_specifier_seq  */
#line 6945 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                              {
                    (yyval.dtype).throws = (yyvsp[-2].pl);
                    (yyval.dtype).throwf = NewString("1");
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).final = (yyvsp[0].str);
	       }
#line 11968 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 539: /* exception_specification: NOEXCEPT virt_specifier_seq  */
#line 6951 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                             {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = NewString("true");
                    (yyval.dtype).final = (yyvsp[0].str);
	       }
#line 11979 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 540: /* exception_specification: NOEXCEPT LPAREN expr RPAREN  */
#line 6957 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                             {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = (yyvsp[-1].dtype).val;
                    (yyval.dtype).final = 0;
	       }
#line 11990 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 541: /* qualifiers_exception_specification: cv_ref_qualifier  */
#line 6965 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                      {
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).final = 0;
                    (yyval.dtype).qualifier = (yyvsp[0].dtype).qualifier;
                    (yyval.dtype).refqualifier = (yyvsp[0].dtype).refqualifier;
               }
#line 12003 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 542: /* qualifiers_exception_specification: exception_specification  */
#line 6973 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
		    (yyval.dtype) = (yyvsp[0].dtype);
                    (yyval.dtype).qualifier = 0;
                    (yyval.dtype).refqualifier = 0;
               }
#line 12013 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 543: /* qualifiers_exception_specification: cv_ref_qualifier exception_specification  */
#line 6978 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                          {
		    (yyval.dtype) = (yyvsp[0].dtype);
                    (yyval.dtype).qualifier = (yyvsp[-1].dtype).qualifier;
                    (yyval.dtype).refqualifier = (yyvsp[-1].dtype).refqualifier;
               }
#line 12023 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 544: /* cpp_const: qualifiers_exception_specification  */
#line 6985 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                    {
                    (yyval.dtype) = (yyvsp[0].dtype);
               }
#line 12031 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 545: /* cpp_const: empty  */
#line 6988 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { 
                    (yyval.dtype).throws = 0;
                    (yyval.dtype).throwf = 0;
                    (yyval.dtype).nexcept = 0;
                    (yyval.dtype).final = 0;
                    (yyval.dtype).qualifier = 0;
                    (yyval.dtype).refqualifier = 0;
               }
#line 12044 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 546: /* ctor_end: cpp_const ctor_initializer SEMI  */
#line 6998 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 { 
                    Clear(scanner_ccode); 
                    (yyval.decl).have_parms = 0; 
                    (yyval.decl).defarg = 0; 
		    (yyval.decl).throws = (yyvsp[-2].dtype).throws;
		    (yyval.decl).throwf = (yyvsp[-2].dtype).throwf;
		    (yyval.decl).nexcept = (yyvsp[-2].dtype).nexcept;
		    (yyval.decl).final = (yyvsp[-2].dtype).final;
                    if ((yyvsp[-2].dtype).qualifier)
                      Swig_error(cparse_file, cparse_line, "Constructor cannot have a qualifier.\n");
               }
#line 12060 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 547: /* ctor_end: cpp_const ctor_initializer LBRACE  */
#line 7009 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                   { 
                    skip_balanced('{','}'); 
                    (yyval.decl).have_parms = 0; 
                    (yyval.decl).defarg = 0; 
                    (yyval.decl).throws = (yyvsp[-2].dtype).throws;
                    (yyval.decl).throwf = (yyvsp[-2].dtype).throwf;
                    (yyval.decl).nexcept = (yyvsp[-2].dtype).nexcept;
                    (yyval.decl).final = (yyvsp[-2].dtype).final;
                    if ((yyvsp[-2].dtype).qualifier)
                      Swig_error(cparse_file, cparse_line, "Constructor cannot have a qualifier.\n");
               }
#line 12076 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 548: /* ctor_end: LPAREN parms RPAREN SEMI  */
#line 7020 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                          { 
                    Clear(scanner_ccode); 
                    (yyval.decl).parms = (yyvsp[-2].pl); 
                    (yyval.decl).have_parms = 1; 
                    (yyval.decl).defarg = 0; 
		    (yyval.decl).throws = 0;
		    (yyval.decl).throwf = 0;
		    (yyval.decl).nexcept = 0;
		    (yyval.decl).final = 0;
               }
#line 12091 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 549: /* ctor_end: LPAREN parms RPAREN LBRACE  */
#line 7030 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            {
                    skip_balanced('{','}'); 
                    (yyval.decl).parms = (yyvsp[-2].pl); 
                    (yyval.decl).have_parms = 1; 
                    (yyval.decl).defarg = 0; 
                    (yyval.decl).throws = 0;
                    (yyval.decl).throwf = 0;
                    (yyval.decl).nexcept = 0;
                    (yyval.decl).final = 0;
               }
#line 12106 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 550: /* ctor_end: EQUAL definetype SEMI  */
#line 7040 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       { 
                    (yyval.decl).have_parms = 0; 
                    (yyval.decl).defarg = (yyvsp[-1].dtype).val; 
                    (yyval.decl).throws = 0;
                    (yyval.decl).throwf = 0;
                    (yyval.decl).nexcept = 0;
                    (yyval.decl).final = 0;
               }
#line 12119 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 551: /* ctor_end: exception_specification EQUAL default_delete SEMI  */
#line 7048 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                                   {
                    (yyval.decl).have_parms = 0;
                    (yyval.decl).defarg = (yyvsp[-1].dtype).val;
                    (yyval.decl).throws = (yyvsp[-3].dtype).throws;
                    (yyval.decl).throwf = (yyvsp[-3].dtype).throwf;
                    (yyval.decl).nexcept = (yyvsp[-3].dtype).nexcept;
                    (yyval.decl).final = (yyvsp[-3].dtype).final;
                    if ((yyvsp[-3].dtype).qualifier)
                      Swig_error(cparse_file, cparse_line, "Constructor cannot have a qualifier.\n");
               }
#line 12134 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 558: /* mem_initializer: idcolon LPAREN  */
#line 7070 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		  skip_balanced('(',')');
		  Clear(scanner_ccode);
		}
#line 12143 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 559: /* mem_initializer: idcolon LBRACE  */
#line 7082 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
		  skip_balanced('{','}');
		  Clear(scanner_ccode);
		}
#line 12152 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 560: /* less_valparms_greater: LESSTHAN valparms GREATERTHAN  */
#line 7088 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                      {
                     String *s = NewStringEmpty();
                     SwigType_add_template(s,(yyvsp[-1].p));
                     (yyval.id) = Char(s);
		     scanner_last_id(1);
                }
#line 12163 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 561: /* identifier: ID  */
#line 7097 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                    { (yyval.id) = (yyvsp[0].id); }
#line 12169 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 562: /* identifier: OVERRIDE  */
#line 7098 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.id) = Swig_copy_string("override"); }
#line 12175 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 563: /* identifier: FINAL  */
#line 7099 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.id) = Swig_copy_string("final"); }
#line 12181 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 564: /* idstring: identifier  */
#line 7102 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            { (yyval.id) = (yyvsp[0].id); }
#line 12187 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 565: /* idstring: default_delete  */
#line 7103 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                { (yyval.id) = Char((yyvsp[0].dtype).val); }
#line 12193 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 566: /* idstring: string  */
#line 7104 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.id) = Char((yyvsp[0].str)); }
#line 12199 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 567: /* idstringopt: idstring  */
#line 7107 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          { (yyval.id) = (yyvsp[0].id); }
#line 12205 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 568: /* idstringopt: empty  */
#line 7108 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.id) = 0; }
#line 12211 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 569: /* idcolon: idtemplate idcolontail  */
#line 7111 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                        { 
                  (yyval.str) = 0;
		  if (!(yyval.str)) (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str),(yyvsp[0].str));
      	          Delete((yyvsp[0].str));
               }
#line 12221 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 570: /* idcolon: NONID DCOLON idtemplatetemplate idcolontail  */
#line 7116 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                             {
		 (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].str),(yyvsp[0].str));
                 Delete((yyvsp[0].str));
               }
#line 12230 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 571: /* idcolon: idtemplate  */
#line 7120 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            {
		 (yyval.str) = NewString((yyvsp[0].str));
   	       }
#line 12238 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 572: /* idcolon: NONID DCOLON idtemplatetemplate  */
#line 7123 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 {
		 (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12246 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 573: /* idcolon: OPERATOR  */
#line 7126 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                 (yyval.str) = NewStringf("%s", (yyvsp[0].str));
	       }
#line 12254 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 574: /* idcolon: OPERATOR less_valparms_greater  */
#line 7129 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                {
                 (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str), (yyvsp[0].id));
	       }
#line 12262 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 575: /* idcolon: NONID DCOLON OPERATOR  */
#line 7132 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
                 (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12270 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 576: /* idcolontail: DCOLON idtemplatetemplate idcolontail  */
#line 7137 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                       {
                   (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].str),(yyvsp[0].str));
		   Delete((yyvsp[0].str));
               }
#line 12279 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 577: /* idcolontail: DCOLON idtemplatetemplate  */
#line 7141 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                           {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12287 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 578: /* idcolontail: DCOLON OPERATOR  */
#line 7144 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12295 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 579: /* idcolontail: DCNOT idtemplate  */
#line 7151 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		 (yyval.str) = NewStringf("::~%s",(yyvsp[0].str));
               }
#line 12303 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 580: /* idtemplate: identifier  */
#line 7157 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                           {
		(yyval.str) = NewStringf("%s", (yyvsp[0].id));
	      }
#line 12311 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 581: /* idtemplate: identifier less_valparms_greater  */
#line 7160 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                 {
		(yyval.str) = NewStringf("%s%s", (yyvsp[-1].id), (yyvsp[0].id));
	      }
#line 12319 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 582: /* idtemplatetemplate: idtemplate  */
#line 7165 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                {
		(yyval.str) = (yyvsp[0].str);
	      }
#line 12327 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 583: /* idtemplatetemplate: TEMPLATE identifier less_valparms_greater  */
#line 7168 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                          {
		(yyval.str) = NewStringf("%s%s", (yyvsp[-1].id), (yyvsp[0].id));
	      }
#line 12335 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 584: /* idcolonnt: identifier idcolontailnt  */
#line 7174 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
                  (yyval.str) = 0;
		  if (!(yyval.str)) (yyval.str) = NewStringf("%s%s", (yyvsp[-1].id),(yyvsp[0].str));
      	          Delete((yyvsp[0].str));
               }
#line 12345 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 585: /* idcolonnt: NONID DCOLON identifier idcolontailnt  */
#line 7179 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                       {
		 (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].id),(yyvsp[0].str));
                 Delete((yyvsp[0].str));
               }
#line 12354 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 586: /* idcolonnt: identifier  */
#line 7183 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                            {
		 (yyval.str) = NewString((yyvsp[0].id));
   	       }
#line 12362 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 587: /* idcolonnt: NONID DCOLON identifier  */
#line 7186 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                         {
		 (yyval.str) = NewStringf("::%s",(yyvsp[0].id));
               }
#line 12370 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 588: /* idcolonnt: OPERATOR  */
#line 7189 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                 (yyval.str) = NewString((yyvsp[0].str));
	       }
#line 12378 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 589: /* idcolonnt: NONID DCOLON OPERATOR  */
#line 7192 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
                 (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12386 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 590: /* idcolontailnt: DCOLON identifier idcolontailnt  */
#line 7197 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                  {
                   (yyval.str) = NewStringf("::%s%s",(yyvsp[-1].id),(yyvsp[0].str));
		   Delete((yyvsp[0].str));
               }
#line 12395 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 591: /* idcolontailnt: DCOLON identifier  */
#line 7201 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                   {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].id));
               }
#line 12403 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 592: /* idcolontailnt: DCOLON OPERATOR  */
#line 7204 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                 {
                   (yyval.str) = NewStringf("::%s",(yyvsp[0].str));
               }
#line 12411 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 593: /* idcolontailnt: DCNOT identifier  */
#line 7207 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
		 (yyval.str) = NewStringf("::~%s",(yyvsp[0].id));
               }
#line 12419 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 594: /* string: string STRING  */
#line 7213 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                               { 
                   (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str), (yyvsp[0].id));
               }
#line 12427 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 595: /* string: STRING  */
#line 7216 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        { (yyval.str) = NewString((yyvsp[0].id));}
#line 12433 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 596: /* wstring: wstring WSTRING  */
#line 7219 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                  {
                   (yyval.str) = NewStringf("%s%s", (yyvsp[-1].str), (yyvsp[0].id));
               }
#line 12441 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 597: /* wstring: WSTRING  */
#line 7227 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         { (yyval.str) = NewString((yyvsp[0].id));}
#line 12447 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 598: /* stringbrace: string  */
#line 7230 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
		 (yyval.str) = (yyvsp[0].str);
               }
#line 12455 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 599: /* stringbrace: LBRACE  */
#line 7233 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
                  skip_balanced('{','}');
		  (yyval.str) = NewString(scanner_ccode);
               }
#line 12464 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 600: /* stringbrace: HBLOCK  */
#line 7237 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       {
		 (yyval.str) = (yyvsp[0].str);
              }
#line 12472 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 601: /* options: LPAREN kwargs RPAREN  */
#line 7242 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
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
#line 12490 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 602: /* options: empty  */
#line 7255 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.node) = 0; }
#line 12496 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 603: /* options_ex: COMMA kwargs  */
#line 7257 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                              {
                  Hash *n;
                  (yyval.node) = NewHash();
                  n = (yyvsp[0].node);
                  while(n) {
                     String *name, *value;
                     name = Getattr(n,"name");
                     value = Getattr(n,"value");
		     if (!value) value = (String *) "1";
                     Setattr((yyval.node),name, value);
		     n = nextSibling(n);
		  }
               }
#line 12514 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 604: /* options_ex: empty  */
#line 7270 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                       { (yyval.node) = 0; }
#line 12520 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 605: /* kwargs: idstring EQUAL stringnum  */
#line 7274 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                          {
		 (yyval.node) = NewHash();
		 Setattr((yyval.node),"name",(yyvsp[-2].id));
		 Setattr((yyval.node),"value",(yyvsp[0].str));
               }
#line 12530 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 606: /* kwargs: idstring EQUAL stringnum COMMA kwargs  */
#line 7279 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                       {
		 (yyval.node) = NewHash();
		 Setattr((yyval.node),"name",(yyvsp[-4].id));
		 Setattr((yyval.node),"value",(yyvsp[-2].str));
		 set_nextSibling((yyval.node),(yyvsp[0].node));
               }
#line 12541 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 607: /* kwargs: idstring  */
#line 7285 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                          {
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"name",(yyvsp[0].id));
	       }
#line 12550 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 608: /* kwargs: idstring COMMA kwargs  */
#line 7289 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                       {
                 (yyval.node) = NewHash();
                 Setattr((yyval.node),"name",(yyvsp[-2].id));
                 set_nextSibling((yyval.node),(yyvsp[0].node));
               }
#line 12560 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 609: /* kwargs: idstring EQUAL stringtype  */
#line 7294 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                            {
                 (yyval.node) = (yyvsp[0].node);
		 Setattr((yyval.node),"name",(yyvsp[-2].id));
               }
#line 12569 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 610: /* kwargs: idstring EQUAL stringtype COMMA kwargs  */
#line 7298 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                                                        {
                 (yyval.node) = (yyvsp[-2].node);
		 Setattr((yyval.node),"name",(yyvsp[-4].id));
		 set_nextSibling((yyval.node),(yyvsp[0].node));
               }
#line 12579 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 611: /* stringnum: string  */
#line 7305 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                        {
		 (yyval.str) = (yyvsp[0].str);
               }
#line 12587 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;

  case 612: /* stringnum: exprnum  */
#line 7308 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"
                         {
                 (yyval.str) = Char((yyvsp[0].dtype).val);
               }
#line 12595 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"
    break;


#line 12599 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.c"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

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


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 7315 "/home/rk/src/games/Projects/rbfx/Source/ThirdParty/swig/Source/CParse/parser.y"


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

