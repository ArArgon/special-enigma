/* A Bison parser, made by GNU Bison 3.5.1.  */

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
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1





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

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_LRPARSER_TAB_HPP_INCLUDED
# define YY_YY_LRPARSER_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "lrparser.y"

#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

extern AST *astRoot;
void yyparse_init(const char* filename);
void yyparse_cleanup();

#line 121 "lrparser.tab.cpp"

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    VOID = 258,
    CONST = 259,
    INT = 260,
    RETURN = 261,
    WHILE = 262,
    BREAK = 263,
    IF = 264,
    ELSE = 265,
    CONTINUE = 266,
    IDENTIFIER = 267,
    CONSTANT = 268,
    LB = 269,
    LA = 270,
    LP = 271,
    RB = 272,
    RA = 273,
    RP = 274,
    COMMA = 275,
    SC = 276,
    ADD = 277,
    SUB = 278,
    MUL = 279,
    DIV = 280,
    MOD = 281,
    NOT = 282,
    ASSIGN = 283,
    LT = 284,
    GT = 285,
    LE_OP = 286,
    GE_OP = 287,
    EQ_OP = 288,
    NE_OP = 289,
    AND_OP = 290,
    OR_OP = 291,
    LOWER_THAN_ELSE = 292
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 11 "lrparser.y"

struct AST* node;
int d;

#line 175 "lrparser.tab.cpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_LRPARSER_TAB_HPP_INCLUDED  */



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
typedef yytype_uint8 yy_state_t;

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
#define YYFINAL  11
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   213

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  39
/* YYNRULES -- Number of rules.  */
#define YYNRULES  92
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  149

#define YYUNDEFTOK  2
#define YYMAXUTOK   292


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
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
      35,    36,    37
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,    55,    55,    56,    60,    61,    65,    66,    70,    71,
      72,    73,    77,    81,    82,    86,    87,    91,    92,    93,
      94,    95,    99,   100,   101,   105,   106,   110,   114,   115,
     119,   123,   124,   128,   132,   133,   137,   138,   142,   143,
     147,   148,   149,   150,   151,   155,   156,   160,   161,   165,
     166,   170,   174,   175,   179,   180,   184,   185,   186,   190,
     191,   192,   193,   194,   198,   199,   200,   204,   205,   206,
     207,   211,   212,   216,   217,   218,   219,   223,   224,   225,
     229,   230,   234,   235,   236,   240,   244,   245,   249,   253,
     254,   255,   256
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "VOID", "CONST", "INT", "RETURN",
  "WHILE", "BREAK", "IF", "ELSE", "CONTINUE", "IDENTIFIER", "CONSTANT",
  "\"{\"", "\"[\"", "\"(\"", "\"}\"", "\"]\"", "\")\"", "\",\"", "\";\"",
  "\"+\"", "\"-\"", "\"*\"", "\"/\"", "\"%\"", "\"!\"", "\"=\"", "\"<\"",
  "\">\"", "\"<=\"", "\">=\"", "\"==\"", "\"!=\"", "\"&&\"", "\"||\"",
  "LOWER_THAN_ELSE", "$accept", "compile_unit", "external_declaration",
  "declaration", "declaration_specifiers", "type_qualifier",
  "init_declarator_list", "init_declarator", "direct_declarator",
  "initializer", "initializer_list", "constant_expression",
  "type_specifier", "function_definition", "parameter_list",
  "parameter_declaration", "block", "blockitem_list", "blockitem",
  "statement", "expression_statement", "expression",
  "assignment_expression", "conditional_expression",
  "logical_or_expression", "logical_and_expression", "equality_expression",
  "relational_expression", "additive_expression",
  "multiplicative_expression", "unary_expression", "postfix_expression",
  "primary_expression", "argument_expression_list", "unary_operator",
  "assignment_operator", "selection_statement", "iteration_statement",
  "jump_statement", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292
};
# endif

#define YYPACT_NINF (-71)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      66,   -71,   -71,   -71,   139,   -71,   -71,     7,    66,    66,
     -71,   -71,   -71,   -71,   -71,    16,   -71,    37,   -71,   -71,
      -3,   -71,    18,   144,    39,    50,   -71,   -71,    -8,   152,
      -4,   -17,     0,    25,   -71,   -71,   172,   -71,   -71,   -71,
     -71,   -71,   -71,     7,   -71,   102,   -71,   -71,   -71,    54,
     -71,   -71,    14,    19,    47,    68,    80,    96,    31,   111,
     -71,   172,   -71,   -71,   -71,   -71,    43,   -71,   -71,   -71,
      -3,   135,   -71,   136,   -71,   -71,   -71,   149,   172,   -71,
     172,   -71,   162,   -71,   -71,   172,   -71,   172,   172,   172,
     172,   172,   172,   172,   172,   172,   172,   172,   172,   172,
     -71,   172,   172,   164,   -71,   -71,   174,   -71,    66,   -71,
     -71,    -6,   -71,   173,   177,   -71,   -71,    19,    47,    68,
      68,    80,    80,    80,    80,    96,    96,   -71,   -71,   -71,
     -71,    29,   -71,   -71,   185,   -71,   -71,    50,   124,   124,
     -71,   -71,   172,   -71,   -71,   118,   -71,   124,   -71
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       0,    28,    12,    29,     0,     3,     5,     0,    10,     8,
       4,     1,     2,    17,     6,     0,    13,    15,    11,     9,
       0,     7,     0,     0,     0,     0,    30,    14,    15,     0,
       0,     0,     0,     0,    77,    78,     0,    34,    45,    82,
      83,    84,    39,     0,    40,     0,    36,    38,    41,     0,
      47,    49,    51,    52,    54,    56,    59,    64,    67,    71,
      73,     0,    42,    43,    44,    19,     0,    27,    67,    21,
       0,     0,    31,     0,    16,    22,    91,     0,     0,    90,
       0,    89,     0,    35,    37,     0,    46,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      85,     0,     0,     0,    72,    18,    33,    20,     0,    23,
      25,     0,    92,     0,     0,    79,    48,    53,    55,    57,
      58,    60,    61,    62,    63,    65,    66,    68,    69,    70,
      50,     0,    75,    80,     0,    32,    24,     0,     0,     0,
      74,    76,     0,    26,    88,    86,    81,     0,    87
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
     -71,   -71,   130,   -12,    -7,   -71,   -71,   121,    -2,   -70,
     -71,   -71,   -71,   -71,   -71,    53,   155,   -71,   133,   -43,
     -71,   -23,   -25,   175,   -71,   123,   125,   117,   109,   113,
      -5,   -71,   -71,   -71,   -71,   -71,   -71,   -71,   -71
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,     5,     6,     7,     8,    15,    16,    28,    74,
     111,    66,     9,    10,    71,    72,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,   134,    61,   101,    62,    63,    64
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      75,    18,    19,   110,    79,    17,    77,    23,    24,    13,
      42,   136,    78,    82,   137,    43,    80,    70,    68,    13,
      25,     1,     2,     3,    29,    30,    31,    32,    14,    33,
      34,    35,    22,    42,    36,    37,    20,    21,    43,    38,
      39,    40,     1,     2,     3,    41,    81,   140,    75,    85,
      87,    22,    23,    24,    88,   113,   104,   114,    69,   100,
     116,   105,    34,    35,    73,    25,    36,   143,   106,     1,
       2,     3,    39,    40,    85,    86,   130,    41,   133,   131,
      89,    90,    68,    68,    68,    68,    68,    68,    68,    68,
      68,    68,   127,   128,   129,   144,   145,    91,    92,    93,
      94,    70,    95,    96,   148,     1,     2,     3,    29,    30,
      31,    32,    75,    33,    34,    35,    22,   146,    36,    83,
      97,    98,    99,    38,    39,    40,   102,   103,   147,    41,
      29,    30,    31,    32,    12,    33,    34,    35,    22,    11,
      36,    27,     1,     2,     3,    38,    39,    40,    34,    35,
      73,    41,    36,   109,   107,   108,    34,    35,    39,    40,
      36,   135,    65,    41,    34,    35,    39,    40,    36,    85,
     112,    41,    26,    76,    39,    40,    34,    35,    84,    41,
      36,   115,    85,   132,    34,    35,    39,    40,    36,    23,
      24,    41,   138,    85,    39,    40,   139,    85,    67,    41,
     121,   122,   123,   124,   141,   142,   119,   120,   125,   126,
     117,     0,     0,   118
};

static const yytype_int16 yycheck[] =
{
      25,     8,     9,    73,    21,     7,    29,    15,    16,    12,
      22,    17,    16,    36,    20,    22,    16,    24,    23,    12,
      28,     3,     4,     5,     6,     7,     8,     9,    21,    11,
      12,    13,    14,    45,    16,    17,    20,    21,    45,    21,
      22,    23,     3,     4,     5,    27,    21,    18,    73,    20,
      36,    14,    15,    16,    35,    78,    61,    80,    19,    28,
      85,    18,    12,    13,    14,    28,    16,   137,    70,     3,
       4,     5,    22,    23,    20,    21,   101,    27,   103,   102,
      33,    34,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   138,   139,    29,    30,    31,
      32,   108,    22,    23,   147,     3,     4,     5,     6,     7,
       8,     9,   137,    11,    12,    13,    14,   142,    16,    17,
      24,    25,    26,    21,    22,    23,    15,    16,    10,    27,
       6,     7,     8,     9,     4,    11,    12,    13,    14,     0,
      16,    20,     3,     4,     5,    21,    22,    23,    12,    13,
      14,    27,    16,    17,    19,    20,    12,    13,    22,    23,
      16,   108,    18,    27,    12,    13,    22,    23,    16,    20,
      21,    27,    17,    21,    22,    23,    12,    13,    45,    27,
      16,    19,    20,    19,    12,    13,    22,    23,    16,    15,
      16,    27,    19,    20,    22,    23,    19,    20,    23,    27,
      91,    92,    93,    94,    19,    20,    89,    90,    95,    96,
      87,    -1,    -1,    88
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,    39,    40,    41,    42,    43,    50,
      51,     0,    40,    12,    21,    44,    45,    46,    42,    42,
      20,    21,    14,    15,    16,    28,    54,    45,    46,     6,
       7,     8,     9,    11,    12,    13,    16,    17,    21,    22,
      23,    27,    41,    42,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    72,    74,    75,    76,    18,    49,    61,    68,    19,
      42,    52,    53,    14,    47,    60,    21,    59,    16,    21,
      16,    21,    59,    17,    56,    20,    21,    36,    35,    33,
      34,    29,    30,    31,    32,    22,    23,    24,    25,    26,
      28,    73,    15,    16,    68,    18,    46,    19,    20,    17,
      47,    48,    21,    59,    59,    19,    60,    63,    64,    65,
      65,    66,    66,    66,    66,    67,    67,    68,    68,    68,
      60,    59,    19,    60,    71,    53,    17,    20,    19,    19,
      18,    19,    20,    47,    57,    57,    60,    10,    57
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    38,    39,    39,    40,    40,    41,    41,    42,    42,
      42,    42,    43,    44,    44,    45,    45,    46,    46,    46,
      46,    46,    47,    47,    47,    48,    48,    49,    50,    50,
      51,    52,    52,    53,    54,    54,    55,    55,    56,    56,
      57,    57,    57,    57,    57,    58,    58,    59,    59,    60,
      60,    61,    62,    62,    63,    63,    64,    64,    64,    65,
      65,    65,    65,    65,    66,    66,    66,    67,    67,    67,
      67,    68,    68,    69,    69,    69,    69,    70,    70,    70,
      71,    71,    72,    72,    72,    73,    74,    74,    75,    76,
      76,    76,    76
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     2,     3,     1,     2,
       1,     2,     1,     1,     3,     1,     3,     1,     4,     3,
       4,     3,     1,     2,     3,     1,     3,     1,     1,     1,
       3,     1,     3,     2,     2,     3,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     1,     3,     1,
       3,     1,     1,     3,     1,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       3,     1,     2,     1,     4,     3,     4,     1,     1,     3,
       1,     3,     1,     1,     1,     1,     5,     7,     5,     2,
       2,     2,     3
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

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
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
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
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
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
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
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
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
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
            else
              goto append;

          append:
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

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
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
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

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
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
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
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
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
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
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
          ++yyp;
          ++yyformat;
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
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

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
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
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
# undef YYSTACK_RELOCATE
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
  case 2:
#line 55 "lrparser.y"
                                      {(yyval.node)=new_node("compile_unit",1,(yyvsp[-1].node)); (yyval.node)->right=(yyvsp[0].node); astRoot=(yyval.node);}
#line 1478 "lrparser.tab.cpp"
    break;

  case 3:
#line 56 "lrparser.y"
                           {(yyval.node)=new_node_r("compile_unit",(yyvsp[0].node)); astRoot=(yyval.node);}
#line 1484 "lrparser.tab.cpp"
    break;

  case 4:
#line 60 "lrparser.y"
                        {(yyval.node)=(yyvsp[0].node);}
#line 1490 "lrparser.tab.cpp"
    break;

  case 5:
#line 61 "lrparser.y"
                  {(yyval.node)=(yyvsp[0].node);}
#line 1496 "lrparser.tab.cpp"
    break;

  case 6:
#line 65 "lrparser.y"
                                   {(yyval.node)=new_node("declaration",1,(yyvsp[-1].node));}
#line 1502 "lrparser.tab.cpp"
    break;

  case 7:
#line 66 "lrparser.y"
                                                          {(yyval.node)=new_node("declaration",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[-1].node);}
#line 1508 "lrparser.tab.cpp"
    break;

  case 8:
#line 70 "lrparser.y"
                       {(yyval.node)=(yyvsp[0].node);}
#line 1514 "lrparser.tab.cpp"
    break;

  case 9:
#line 71 "lrparser.y"
                                            {(yyval.node)=(yyvsp[-1].node); (yyval.node)->right=(yyvsp[-1].node);}
#line 1520 "lrparser.tab.cpp"
    break;

  case 10:
#line 72 "lrparser.y"
                         {(yyval.node)=(yyvsp[0].node);}
#line 1526 "lrparser.tab.cpp"
    break;

  case 11:
#line 73 "lrparser.y"
                                            {(yyval.node)=(yyvsp[0].node); (yyval.node)->left=(yyvsp[-1].node);}
#line 1532 "lrparser.tab.cpp"
    break;

  case 12:
#line 77 "lrparser.y"
          {(yyval.node)=(yyvsp[0].node);}
#line 1538 "lrparser.tab.cpp"
    break;

  case 13:
#line 81 "lrparser.y"
                        {(yyval.node)=new_node_r("init_declarator_list",(yyvsp[0].node));}
#line 1544 "lrparser.tab.cpp"
    break;

  case 14:
#line 82 "lrparser.y"
                                                   {(yyval.node)=new_node("init_declarator_list",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1550 "lrparser.tab.cpp"
    break;

  case 15:
#line 86 "lrparser.y"
                          {(yyval.node)=(yyvsp[0].node);}
#line 1556 "lrparser.tab.cpp"
    break;

  case 16:
#line 87 "lrparser.y"
                                            {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1562 "lrparser.tab.cpp"
    break;

  case 17:
#line 91 "lrparser.y"
                   {(yyval.node)=(yyvsp[0].node);}
#line 1568 "lrparser.tab.cpp"
    break;

  case 18:
#line 92 "lrparser.y"
                                                        {(yyval.node)=new_node("direct_declarator",1,(yyvsp[-3].node)); (yyval.node)->right=(yyvsp[-1].node);}
#line 1574 "lrparser.tab.cpp"
    break;

  case 19:
#line 93 "lrparser.y"
                                    {(yyval.node)=new_node("direct_declarator",1,(yyvsp[-2].node)); (yyval.node)->right=NULL;}
#line 1580 "lrparser.tab.cpp"
    break;

  case 20:
#line 94 "lrparser.y"
                                                   {(yyval.node)=new_node("direct_declarator",1,(yyvsp[-3].node)); (yyval.node)->right=(yyvsp[-1].node);}
#line 1586 "lrparser.tab.cpp"
    break;

  case 21:
#line 95 "lrparser.y"
                                    {(yyval.node)=new_node("direct_declarator",1,(yyvsp[-2].node)); (yyval.node)->right=NULL;}
#line 1592 "lrparser.tab.cpp"
    break;

  case 22:
#line 99 "lrparser.y"
                              {(yyval.node)=(yyvsp[0].node);}
#line 1598 "lrparser.tab.cpp"
    break;

  case 23:
#line 100 "lrparser.y"
              {(yyval.node)=NULL;}
#line 1604 "lrparser.tab.cpp"
    break;

  case 24:
#line 101 "lrparser.y"
                                   {(yyval.node)=(yyvsp[-1].node);}
#line 1610 "lrparser.tab.cpp"
    break;

  case 25:
#line 105 "lrparser.y"
                    {(yyval.node)=new_node_r("initializer_list",(yyvsp[0].node));}
#line 1616 "lrparser.tab.cpp"
    break;

  case 26:
#line 106 "lrparser.y"
                                           {(yyval.node)=new_node("initializer_list",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1622 "lrparser.tab.cpp"
    break;

  case 27:
#line 110 "lrparser.y"
                           {(yyval.node)=(yyvsp[0].node);}
#line 1628 "lrparser.tab.cpp"
    break;

  case 28:
#line 114 "lrparser.y"
         {(yyval.node)=(yyvsp[0].node);}
#line 1634 "lrparser.tab.cpp"
    break;

  case 29:
#line 115 "lrparser.y"
          {(yyval.node)=(yyvsp[0].node);}
#line 1640 "lrparser.tab.cpp"
    break;

  case 30:
#line 119 "lrparser.y"
                                                       {(yyval.node)=new_node("function_definition",2,(yyvsp[-2].node),(yyvsp[-1].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1646 "lrparser.tab.cpp"
    break;

  case 31:
#line 123 "lrparser.y"
                              {(yyval.node)=new_node_r("parameter_list",(yyvsp[0].node));}
#line 1652 "lrparser.tab.cpp"
    break;

  case 32:
#line 124 "lrparser.y"
                                                   {(yyval.node)=new_node("parameter_list",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1658 "lrparser.tab.cpp"
    break;

  case 33:
#line 128 "lrparser.y"
                                                 {(yyval.node)=new_node("parameter_declaration",1,(yyvsp[-1].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1664 "lrparser.tab.cpp"
    break;

  case 34:
#line 132 "lrparser.y"
                {(yyval.node)=NULL;}
#line 1670 "lrparser.tab.cpp"
    break;

  case 35:
#line 133 "lrparser.y"
                                 {(yyval.node)=(yyvsp[-1].node);}
#line 1676 "lrparser.tab.cpp"
    break;

  case 36:
#line 137 "lrparser.y"
              {(yyval.node)=new_node_r("blockitem_list",(yyvsp[0].node));}
#line 1682 "lrparser.tab.cpp"
    break;

  case 37:
#line 138 "lrparser.y"
                               {(yyval.node)=new_node("blockitem_list",1,(yyvsp[-1].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1688 "lrparser.tab.cpp"
    break;

  case 38:
#line 142 "lrparser.y"
              {(yyval.node)=(yyvsp[0].node);}
#line 1694 "lrparser.tab.cpp"
    break;

  case 39:
#line 143 "lrparser.y"
                  {(yyval.node)=(yyvsp[0].node);}
#line 1700 "lrparser.tab.cpp"
    break;

  case 40:
#line 147 "lrparser.y"
              {(yyval.node)=(yyvsp[0].node);}
#line 1706 "lrparser.tab.cpp"
    break;

  case 41:
#line 148 "lrparser.y"
                               {(yyval.node)=(yyvsp[0].node);}
#line 1712 "lrparser.tab.cpp"
    break;

  case 42:
#line 149 "lrparser.y"
                              {(yyval.node)=(yyvsp[0].node);}
#line 1718 "lrparser.tab.cpp"
    break;

  case 43:
#line 150 "lrparser.y"
                              {(yyval.node)=(yyvsp[0].node);}
#line 1724 "lrparser.tab.cpp"
    break;

  case 44:
#line 151 "lrparser.y"
                         {(yyval.node)=(yyvsp[0].node);}
#line 1730 "lrparser.tab.cpp"
    break;

  case 45:
#line 155 "lrparser.y"
            {(yyval.node)=NULL;}
#line 1736 "lrparser.tab.cpp"
    break;

  case 46:
#line 156 "lrparser.y"
                         {(yyval.node)=new_node_r("expression_statement",(yyvsp[-1].node));}
#line 1742 "lrparser.tab.cpp"
    break;

  case 47:
#line 160 "lrparser.y"
                              {(yyval.node)=(yyvsp[0].node);}
#line 1748 "lrparser.tab.cpp"
    break;

  case 48:
#line 161 "lrparser.y"
                                               {(yyval.node)=new_node("expression",3,(yyvsp[-2].node),(yyvsp[-1].node),(yyvsp[0].node));}
#line 1754 "lrparser.tab.cpp"
    break;

  case 49:
#line 165 "lrparser.y"
                               {(yyval.node)=(yyvsp[0].node);}
#line 1760 "lrparser.tab.cpp"
    break;

  case 50:
#line 166 "lrparser.y"
                                                                     {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1766 "lrparser.tab.cpp"
    break;

  case 51:
#line 170 "lrparser.y"
                              {(yyval.node)=(yyvsp[0].node);}
#line 1772 "lrparser.tab.cpp"
    break;

  case 52:
#line 174 "lrparser.y"
                               {(yyval.node)=(yyvsp[0].node);}
#line 1778 "lrparser.tab.cpp"
    break;

  case 53:
#line 175 "lrparser.y"
                                                            {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1784 "lrparser.tab.cpp"
    break;

  case 54:
#line 179 "lrparser.y"
                            {(yyval.node)=(yyvsp[0].node);}
#line 1790 "lrparser.tab.cpp"
    break;

  case 55:
#line 180 "lrparser.y"
                                                          {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1796 "lrparser.tab.cpp"
    break;

  case 56:
#line 184 "lrparser.y"
                              {(yyval.node)=(yyvsp[0].node);}
#line 1802 "lrparser.tab.cpp"
    break;

  case 57:
#line 185 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1808 "lrparser.tab.cpp"
    break;

  case 58:
#line 186 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1814 "lrparser.tab.cpp"
    break;

  case 59:
#line 190 "lrparser.y"
                            {(yyval.node)=(yyvsp[0].node);}
#line 1820 "lrparser.tab.cpp"
    break;

  case 60:
#line 191 "lrparser.y"
                                                        {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1826 "lrparser.tab.cpp"
    break;

  case 61:
#line 192 "lrparser.y"
                                                        {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1832 "lrparser.tab.cpp"
    break;

  case 62:
#line 193 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1838 "lrparser.tab.cpp"
    break;

  case 63:
#line 194 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1844 "lrparser.tab.cpp"
    break;

  case 64:
#line 198 "lrparser.y"
                                  {(yyval.node)=(yyvsp[0].node);}
#line 1850 "lrparser.tab.cpp"
    break;

  case 65:
#line 199 "lrparser.y"
                                                            {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1856 "lrparser.tab.cpp"
    break;

  case 66:
#line 200 "lrparser.y"
                                                            {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1862 "lrparser.tab.cpp"
    break;

  case 67:
#line 204 "lrparser.y"
                         {(yyval.node)=(yyvsp[0].node);}
#line 1868 "lrparser.tab.cpp"
    break;

  case 68:
#line 205 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1874 "lrparser.tab.cpp"
    break;

  case 69:
#line 206 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1880 "lrparser.tab.cpp"
    break;

  case 70:
#line 207 "lrparser.y"
                                                         {(yyval.node)=(yyvsp[-1].node); (yyval.node)->left=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1886 "lrparser.tab.cpp"
    break;

  case 71:
#line 211 "lrparser.y"
                           {(yyval.node)=(yyvsp[0].node);}
#line 1892 "lrparser.tab.cpp"
    break;

  case 72:
#line 212 "lrparser.y"
                                          {(yyval.node)=(yyvsp[-1].node); (yyval.node)->right=(yyvsp[0].node);}
#line 1898 "lrparser.tab.cpp"
    break;

  case 73:
#line 216 "lrparser.y"
                           {(yyval.node)=(yyvsp[0].node);}
#line 1904 "lrparser.tab.cpp"
    break;

  case 74:
#line 217 "lrparser.y"
                                                {(yyval.node)=new_node("arr_postfix_expression",1,(yyvsp[-3].node)); (yyval.node)->right=(yyvsp[-1].node);}
#line 1910 "lrparser.tab.cpp"
    break;

  case 75:
#line 218 "lrparser.y"
                                     {(yyval.node)=new_node("func_postfix_expression",1,(yyvsp[-2].node)); (yyval.node)->right=NULL;}
#line 1916 "lrparser.tab.cpp"
    break;

  case 76:
#line 219 "lrparser.y"
                                                              {(yyval.node)=new_node("func_postfix_expression",1,(yyvsp[-3].node)); (yyval.node)->right=(yyvsp[-1].node);}
#line 1922 "lrparser.tab.cpp"
    break;

  case 77:
#line 223 "lrparser.y"
                   {(yyval.node)=(yyvsp[0].node);}
#line 1928 "lrparser.tab.cpp"
    break;

  case 78:
#line 224 "lrparser.y"
                   {(yyval.node)=(yyvsp[0].node);}
#line 1934 "lrparser.tab.cpp"
    break;

  case 79:
#line 225 "lrparser.y"
                             {(yyval.node)=(yyvsp[-1].node);}
#line 1940 "lrparser.tab.cpp"
    break;

  case 80:
#line 229 "lrparser.y"
                              {(yyval.node)=new_node_r("argument_expression_list",(yyvsp[0].node));}
#line 1946 "lrparser.tab.cpp"
    break;

  case 81:
#line 230 "lrparser.y"
                                                             {(yyval.node)=new_node("argument_expression_list",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1952 "lrparser.tab.cpp"
    break;

  case 82:
#line 234 "lrparser.y"
            {(yyval.node)=(yyvsp[0].node);}
#line 1958 "lrparser.tab.cpp"
    break;

  case 83:
#line 235 "lrparser.y"
              {(yyval.node)=(yyvsp[0].node);}
#line 1964 "lrparser.tab.cpp"
    break;

  case 84:
#line 236 "lrparser.y"
              {(yyval.node)=(yyvsp[0].node);}
#line 1970 "lrparser.tab.cpp"
    break;

  case 85:
#line 240 "lrparser.y"
        {(yyval.node)=(yyvsp[0].node);}
#line 1976 "lrparser.tab.cpp"
    break;

  case 86:
#line 244 "lrparser.y"
                                                              {(yyval.node)=new_node("if_statement",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1982 "lrparser.tab.cpp"
    break;

  case 87:
#line 245 "lrparser.y"
                                                         {(yyval.node)=new_node("if_statement",1,(yyvsp[-4].node)); (yyval.node)->right=(yyvsp[-1].node); (yyvsp[-1].node)->left=(yyvsp[-2].node); (yyvsp[-1].node)->right=(yyvsp[0].node);}
#line 1988 "lrparser.tab.cpp"
    break;

  case 88:
#line 249 "lrparser.y"
                                           {(yyval.node)=new_node("while_statement",1,(yyvsp[-2].node)); (yyval.node)->right=(yyvsp[0].node);}
#line 1994 "lrparser.tab.cpp"
    break;

  case 89:
#line 253 "lrparser.y"
                     {(yyval.node)=(yyvsp[-1].node);}
#line 2000 "lrparser.tab.cpp"
    break;

  case 90:
#line 254 "lrparser.y"
                    {(yyval.node)=(yyvsp[-1].node);}
#line 2006 "lrparser.tab.cpp"
    break;

  case 91:
#line 255 "lrparser.y"
                     {(yyval.node)=(yyvsp[-1].node);}
#line 2012 "lrparser.tab.cpp"
    break;

  case 92:
#line 256 "lrparser.y"
                                {(yyval.node)=(yyvsp[-2].node); (yyval.node)->right=(yyvsp[-1].node);}
#line 2018 "lrparser.tab.cpp"
    break;


#line 2022 "lrparser.tab.cpp"

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
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
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


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
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
                  yystos[+*yyssp], yyvsp);
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
#line 259 "lrparser.y"


void yyerror(const char*s)
{
	printf("error(line %d): %s at %s \n", yylineno, s, yytext);
}

AST *astRoot;

AST *parseAST(const char* filename)
{
    astRoot = NULL;
	yyparse_init(filename);

    yyparse();

	yyparse_cleanup();
    return astRoot;
}
