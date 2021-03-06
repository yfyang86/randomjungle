/*
  Logistic Regression using Truncated Iteratively Re-weighted Least Squares
  (includes several programs)
  Copyright (C) 2005  Paul Komarek

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Author: Paul Komarek, komarek@cmu.edu
  Alternate contact: Andrew Moore, awm@cs.cmu.edu

*/


/* **
   File:            ambs.c
   Author:          Andrew William Moore
   Created:         7th Feb 1990
   Updated:         8 Dec 96
   Description:     Standard andrew tools. These are trivial and tedious.

   Copyright 1996, Schenley Park Research.
*/

/* ------------------- includes ------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <setjmp.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include "amma.h"
#include "ambs.h"
#include "am_string.h"

#ifdef OLD_SPR
#include "adgui.h"
#endif

/* ------------------- definitions ----------------------------*/
#ifdef PC_MVIS_PLATFORM
extern char GetcBuf;
extern HWND hEditIn;
extern HANDLE KeyPress;
extern CRITICAL_SECTION critsect;
#endif

#define PD_A 55
#define PD_B 24
typedef struct pd_state
{
  int cursor;
  unsigned int val[PD_A];
} pd_state;

pd_state Pd_state;
bool Pd_state_defined = FALSE;

bool Currently_saving_state = FALSE;
pd_state Saved_pd_state;


/* Private prototypes  -- A lot of these maybe should be public */
void copy_pd_state(pd_state *src,pd_state *dst);
unsigned int pd_random_29bit(void);
unsigned int pd_random_32bit(void);
void andrews_randomize(void); /* OBSOLETE */
double pd_random_double(void);
double basic_range_random(double lo, double hi);
double basic_gen_gauss(void);
int very_random_int(int n); /* OBSOLETE */
int very_basic_index_of_arg(const char *opt_string, int argc, char *argv[]);
int basic_index_of_arg(const char *opt_string, int argc, char *argv[]);
char *basic_string_from_args(const char *key, int argc, char *argv[], char *default_value);



double Verbosity = 0.0;

int Ignore_next_n = 0;

void my_error(const char *string)
{
  printf("***** Auton software error:\n***** %s\n",string);
  exit(-1);
}

/* Make sure the assert statement has no side effects that you need
   because it won't be executed in fast mode */
void my_assert_always(bool b)
{
  if ( !b )
    my_error("my_assert failed");
}

void my_errorf(const char *format, ...)
{
  VA_MAGIC_INTO_BIG_ARRAY;
  my_error(big_array);
}

bool eq_string(const char *s1, const char *s2)
{
  my_assert(s1 != NULL);
  my_assert(s2 != NULL);
  return(strcmp(s1,s2) == 0 );
}

bool eq_string_with_length(const char *s1, const char *s2, int n)
{
  return(strncmp(s1,s2,n) == 0 );
}

/*
   New Random Functions
   Author:      Darrell Kindred + a few mods by Justin Boyan
                 + tiny final adjustments by Andrew Moore
   Created:     Sun. Mar  3, 1996 16:22
   Description: the random number generator recommended by Persi Diaconis
                in his lecture at CMU, February 1996.

		The numbers (x_n) in the sequence are generated by
		  x_n = x_{n-PD_A} * x_{n-PD_B}   (mod 2^32)
		where PD_A=55, PD_B=24.  This gives a pseudorandom
		sequence of ODD integers in the interval [1, 2^32 - 1].

   Modified:    Mon Feb 17 13:42:19 EST 1997
                Scott identified hideous correlations in the low-order bit.
		As a temporary hack, I've killed the lowest 3 bits of
		each number generated.  This seems to help, but we should
		look into getting a more trustworthy RNG.  persi_diaconis--
		The old version is backed up in xdamut/backup/97-02-17.ambs.c
*/




void copy_pd_state(pd_state *src,pd_state *dst)
{
  int i;
  dst -> cursor = src -> cursor;
  for ( i = 0 ; i < PD_A ; i++ )
    dst -> val[i] = src -> val[i];
}

void push_current_am_srand_state(void)
{
  if ( Currently_saving_state )
    my_error("save current am_srand_state: already being pushed. Implement a real stack.");
  copy_pd_state(&Pd_state,&Saved_pd_state);
  Currently_saving_state = TRUE;
}

void pop_current_am_srand_state(void)
{
  if ( !Currently_saving_state )
    my_error("pop_current am_srand_state: nothing pushed.");
  copy_pd_state(&Saved_pd_state,&Pd_state);
  Currently_saving_state = FALSE;
}

unsigned int pd_random_29bit(void) 
{
    int c = Pd_state.cursor;
    if (!Pd_state_defined) 
    {
      /* not seeded */
      am_srand(5297);
    }
    /* this assumes 32-bit unsigned int */
    Pd_state.val[c] *= Pd_state.val[(c + PD_A - PD_B) % PD_A];

    Pd_state.cursor = ((c + 1) % PD_A);

    return Pd_state.val[c] >> 3;
}

unsigned int pd_random_32bit(void) 
{
    unsigned int x = pd_random_29bit(), y = pd_random_29bit();
    return x ^ (y<<3);
}


/**********************************************************************
 * interface for the outside world...  (jab)
 **********************************************************************/


/* Pass in a single seed, which can be any unsigned int */
void am_srand(int seed)
{
  unsigned int us_seed = (unsigned int) seed & 0x3FFFFFFF;
  unsigned int i;
  int four = 4;

    /* this code requires 32-bit ints */
  if (sizeof(unsigned int) != four) 
    my_error("this code requires 32-bit integers");

    /* make it odd */
  us_seed = us_seed * 2 + 1;
    /* add +0,+2,+4,... */

  for (i=0; i<PD_A; i++) {
    Pd_state.val[i] = us_seed + 2 * i;
  }
  Pd_state.cursor = 0;
  Pd_state_defined = TRUE;

    /* Throw out the first few numbers (enough to be sure that 
     * all values in the state have "wrapped around" at least twice). */
    for (i=0; i<400+(us_seed % 100); i++) {
	(void) pd_random_29bit();
    }
}

void am_randomize(void)
{
    time_t my_time = time(NULL);
    char *time_string = ctime(&my_time);
    int i = 0;
    int seed = 0;
    
    while ( time_string[i] != '\0' )  {
	seed += i * (int) (time_string[i]);
	i += 1;
    }
    am_srand(seed);
}

/* Synonmym for am_randomize. For backwards compat. Dont use */
void andrews_randomize(void)
{
  am_randomize();
}

double pd_random_double(void) 
{
  /* returns a double chosen uniformly from the interval [0,1) */
  double r29 = pd_random_29bit();
  return (r29 / ((unsigned int) (1<<29)));
}


double basic_range_random(double lo, double hi)
{
    return lo + (hi-lo)*pd_random_double();
}

int rrcalls = 0;

double range_random(double lo, double hi)
{
  double result = basic_range_random(lo,hi);
  /*  if ( rrcalls < 10 )
  //    printf("rrcall %d: range_random(%g,%g) = %g\n",rrcalls,lo,hi,result);
  */
  rrcalls += 1;
  return result;
}

int int_random(int n)
{
  int result = -1;
  if ( n < 1 ) 
  {
    printf("int_random(%d) ?\n",n);
    my_error("You called int_random(<=0)");
    return(0);
  }
  else
    result = pd_random_29bit() % n;

  return(result);
}

int GG_iset = 0;
double GG_gset = -77.77;

int int_min(int x, int y)
{
  return( (x < y) ? x : y );
}

double real_min(double x, double y)
{
  return( (x < y) ? x : y );
}

double real_max(double x, double y)
{
  return( (x < y) ? y : x );
}

double real_square(double x)
{
  return( x * x );
}

int int_max(int x, int y)
{
  return( (x < y) ? y : x );
}

int int_abs(int x)
{
  return( (x < 0) ? -x : x );
}

int int_square(int x)
{
  return( x * x );
}

int int_cube(int x)
{
  return( x * x * x );
}

bool am_isnan( double val)
{
  if (val < 0.0 || val >= 0.0) return FALSE;
  else return TRUE;
}

bool am_isinf( double val)
{
  if (val + 1.0 == val) return TRUE;
  else return FALSE;
}

bool am_isnum( double val)
{
  return !(am_isnan( val) || am_isinf( val));
}

int very_basic_index_of_arg(const char *opt_string, int argc, char *argv[])
/* Searches for the string among the argv[]
   arguments. If it finds it, returns the index,
   in the argv[] array, of that following argument.
   Otherwise it returns -1.

   argv[] may contain NULL.
*/
{
  int i;
  int result = -1;
  for ( i = 0 ; result < 0 && i < argc ; i++ )
    if ( argv[i] != NULL && strcmp(argv[i],opt_string) == 0 )
      result = i;

  if ( Verbosity > 350.0 )
  {
    if ( result >= 0 )
      printf("index_of_arg(): Key `%s' from in argument number %d\n",
             argv[result],result
            );
    else
      printf("index_of_arg(): Key `%s' not found\n",opt_string);
  }

  return(result);
}

#define MAX_ARG_CHARS 200

int basic_index_of_arg(const char *opt_string, int argc, char *argv[])
/* Searches for the string among the argv[]
   arguments. If it finds it, returns the index,
   in the argv[] array, of that following argument.
   Otherwise it returns -1.

   argv[] may contain NULL.

   It is INSENSITIVE TO THE INITIAL - sign in front of a key
*/
{
  int result;

  if ( Verbosity > 2000.0 )
  {
    int i;
    fprintf(stderr,"Welcome to basic_index_of_arg\n");
    fprintf(stderr,"opt_string = %s\n",
            (opt_string==NULL)?"NULL" : opt_string);
    fprintf(stderr,"argc = %d\n",argc);
    for ( i = 0 ; i < argc ; i++ )
      fprintf(stderr,"argv[%d] = %s\n",i,
              (argv[i]==NULL) ? "NULL" : argv[i]);
  }

  result = very_basic_index_of_arg(opt_string,argc,argv);
  if ( result < 0 )
  {
    int i;
    char other_key[MAX_ARG_CHARS+2];
    if ( opt_string[0] == '-' )
    {
      for ( i = 0 ; i < MAX_ARG_CHARS && opt_string[i+1] != '\0' ; i++ )
        other_key[i] = opt_string[i+1];
      other_key[i] = '\0';
    }
    else
    {
      other_key[0] = '-';
      for ( i = 0 ; i < MAX_ARG_CHARS && opt_string[i] != '\0' ; i++ )
        other_key[i+1] = opt_string[i];
      other_key[i+1] = '\0';
    }

    result = very_basic_index_of_arg(other_key,argc,argv);
  }

  return(result);
}
    
char *basic_string_from_args(const char *key, int argc, char *argv[], char *default_value)
{
  int idx = basic_index_of_arg(key,argc,argv);
  char *result;

  if ( idx >= 0 && idx < argc-1 )
    result = argv[idx+1];
  else
    result = default_value;

  return(result);
}

/**** Now for public, talkative, ones ****/

bool arghelp_wanted(int argc, char *argv[])
{
  bool result = basic_index_of_arg("arghelp",argc,argv) >= 0 ||
                basic_index_of_arg("-arghelp",argc,argv) >= 0;
  return(result);
}

int index_of_arg(const char *opt_string, int argc, char *argv[])
/* Searches for the string among the argv[]
   arguments. If it finds it, returns the index,
   in the argv[] array, of that following argument.
   Otherwise it returns -1.

   argv[] may contain NULL.
*/
{
  int result = basic_index_of_arg(opt_string,argc,argv);
  if ( arghelp_wanted(argc,argv) )
    printf("ARGV %21s %8s         %8s %s on commandline\n",
           opt_string,"","",
           (result >= 0) ? "   " : "not"
          );

  return(result);
}

char *mk_string_from_args(const char *key,int argc,char *argv[],char *default_value)
{
  return (mk_copy_string(string_from_args(key,argc,argv,default_value)));
}

char *string_from_args(const char *key, int argc, char *argv[], char *default_value)
{
  char *result = basic_string_from_args(key,argc,argv,default_value);
  if ( arghelp_wanted(argc,argv) )
  {
    printf("ARGV %21s %8s default %8s ",
           key,"<string>",
           (default_value == NULL) ? "NULL" : default_value
          );
    if ( (default_value == NULL && result == NULL) || 
         (default_value != NULL && eq_string(default_value,result))
       )
      printf("\n");
    else
      printf("commandline %s\n",result);
  }

  return(result);
}

char *string_from_args_insist(char *key,int argc,char *argv[])
{
  char *s = string_from_args(key,argc,argv,NULL);
  if ( s == NULL )
    my_errorf("I insist that keyword %s is on the command line followed\n"
	      "by some value. Please add %s <value> to the command line\n",
	      key,key);
  return s;
}

double double_from_args(const char *key, int argc, char *argv[], double default_value)
{
  const char *string = basic_string_from_args(key,argc,argv,(char *)NULL);
  double result = (string == NULL) ? default_value : atof(string);

  if ( arghelp_wanted(argc,argv) )
  {
    printf("ARGV %21s %8s default %8g ",
           key,"<double>",default_value
          );
    if ( string == NULL )
      printf("\n");
    else
      printf("commandline %g\n",result);
  }

  return(result);
}

int int_from_args(const char *key, int argc, char *argv[], int default_value)
{
  const char *string = basic_string_from_args(key,argc,argv,(char *)NULL);
  int result = (string == NULL) ? default_value : atoi(string);

  if ( arghelp_wanted(argc,argv) )
  {
    printf("ARGV %21s %8s default %8d ",
           key,"<int>",default_value
          );
    if ( string == NULL )
      printf("\n");
    else
      printf("commandline %d\n",result);
  }

  return(result);
}

bool bool_from_args(const char *key, int argc, char *argv[], bool default_value)
/*
   Searches for two adjacent lexical items in argv in which the first 
   item equals the string 'key'.

   Returns TRUE if and only if the second item is a string beginning
   with any of the following characters: t T y Y 1
   (this is meant to accomodate any reasonable user idea of words for
    specifying boolean Truthhood)


   If doesn't find anything matching key then returns default_value.
*/
{
  const char *string = basic_string_from_args(key,argc,argv,(char *)NULL);
  bool result = (string == NULL) ? default_value : 
                (string[0] == 't' || string[0] == 'T' ||
                 string[0] == 'y' || string[0] == 'Y' ||
                 string[0] == '1'
                );

  if ( arghelp_wanted(argc,argv) )
  {
    printf("ARGV %21s %8s default %8s ",
           key,"<bool>",(default_value) ? "TRUE" : "FALSE"
          );
    if ( string == NULL )
      printf("\n");
    else
      printf("commandline %s\n",(result)?"TRUE":"FALSE");
  }

  return(result);
}

int my_irint(double x)
/* Returns the closest integer to x */
{
  int returner = (int)floor(x + 0.5);
  return returner;
}

void fprintf_int(FILE *s,char *m1,int x,char *m2)
{
  fprintf(s,"%s = %d%s",m1,x,m2);
}

void fprintf_realnum(FILE *s,char *m1,double x,char *m2)
{
  fprintf(s,"%s = %g%s",m1,x,m2);
}

void fprintf_float(FILE *s,char *m1,double x,char *m2)
{
  fprintf_realnum(s,m1,x,m2);
}

void fprintf_double(FILE *s,char *m1,double x,char *m2)
{
  fprintf_realnum(s,m1,x,m2);
}

void fprintf_bool(FILE *s,char *m1,bool x,char *m2)
{
  fprintf(s,"%s = %s%s",m1,(x)?"True":"False",m2);
}

void fprintf_string(FILE *s,char *m1,char *x,char *m2)
{
  fprintf(s,"%s = \"%s\"%s",m1,x,m2);
}

int index_of_char(char *s,char c)
/*
   Returns the least index i such that s[i] == c.
   If no such index exists in string, returns -1
*/
{
  int result = -1;

  if ( s != NULL )
  {
    int i;
    for ( i = 0 ; result < 0 && s[i] != '\0' ; i++ )
      if ( s[i] == c ) result = i;
  }

  return(result);
}

bool char_is_in(char *s,char c)
{
  bool result = index_of_char(s,c) >= 0;
  return(result);
}

/* This returns the number of occurences of character c in string s. 
   It is legal for s to be NULL (in which case the result will be 0).
   
   Added by Mary on 8 Dec 96.
*/
int num_of_char_in_string(const char *s, char c)
{
  int res = 0;

  if (s != NULL)
  {
    int i;

    for (i = 0; i < (int) strlen(s); i++)
      if (s[i] == c) res++;
  }
  return(res);
}

FILE *am_fopen(char *filename,char *mode)
{
  FILE *s = NULL;
  bool all_whitespace = TRUE;
  int i;

  if ( filename == NULL )
	  my_error("am_fopen: Called with filename == NULL");
  
  for ( i = 0 ; filename[i] != '\0' && all_whitespace ; i++ )
	all_whitespace = filename[i] == ' ';

  if ( !all_whitespace )
	s = fopen(filename,mode);
  return(s);
}

bool is_all_digits(const char *string)
{
        if (!string || !*string)
                return FALSE;  /* null pointer, or pointer to null char */
        while (*string)
                if (!isdigit(*string++))
                        return FALSE;  /* found a non-digit! */
        return TRUE;  /* must all be digits */
}

bool is_a_number(const char *string)
/*
   uses a finite state machine with 8 states.
   These are the symbols in the FSM alphabet:
    S     + or -
    P     .
    D     0 or 1 or .. or 9
    E     e or E
    X     anything else

  In the following descriptions of states, any unmentioned
  symbol goes to an absorbing, non-accepting, error state
       State 1: S --> 2 , D --> 4 , P --> 3
       State 2: P --> 3 , D --> 4 
       State 3: D --> 5
    *  State 4: D --> 4 , E --> 6 , P --> 5
    *  State 5: D --> 5 , E --> 6
       State 6: D --> 8 , S --> 7
       State 7: D --> 8
    *  State 8: D --> 8
  The starred states are acceptors.
*/
{
  int state = 1;
  int i = 0;
  bool err_state = FALSE;
  bool result;

  while ( string[i] != '\0' && !err_state )
  {
    char c = string[i];
    char symbol = ( c == '.' ) ? 'P' :
                  ( c == 'e' || c == 'E' ) ? 'E' :
                  ( c == '+' || c == '-' ) ? 'S' :
                  ( c >= '0' && c <= '9' ) ? 'D' : 'X';
/*
    printf("i = %d , c = %c , symbol = %c , state = %d\n",i,c,symbol,state);
*/
    if ( state == 1 )
    {
      if ( symbol == 'S' ) state = 2;
      else if ( symbol == 'D' ) state = 4;
      else if ( symbol == 'P' ) state = 3;
      else err_state = TRUE;
    }
    else if ( state == 2 )
    {
      if      ( symbol == 'P' ) state = 3 ;
      else if ( symbol == 'D' ) state = 4 ;
      else err_state = TRUE;
    }
    else if ( state == 3 )
    {
      if      ( symbol == 'D' ) state = 5;
      else err_state = TRUE;
    }
    else if ( state == 4 )
    {
      if      ( symbol == 'D' ) state = 4 ;
      else if ( symbol == 'E' ) state = 6 ;
      else if ( symbol == 'P' ) state = 5;
      else err_state = TRUE;
    }
    else if ( state == 5 )
    {
      if      ( symbol == 'D' ) state = 5 ;
      else if ( symbol == 'E' ) state = 6;
      else err_state = TRUE;
    }
    else if ( state == 6 )
    {
      if      ( symbol == 'D' ) state = 8 ;
      else if ( symbol == 'S' ) state = 7;
      else err_state = TRUE;
    }
    else if ( state == 7 )
    {
      if      ( symbol == 'D' ) state = 8;
      else err_state = TRUE;
    }
    else if ( state == 8 )
    {
      if      ( symbol == 'D' ) state = 8;
      else err_state = TRUE;
    }

    i += 1;
  }

  result = !err_state && ( state==4 || state==5 || state==8 );
  return(result);
}

bool bool_from_string(char *s,bool *r_ok)
{
  bool result = FALSE;
  *r_ok = FALSE;

  if ( s != NULL && s[0] != '\0' )
  {
    char c = s[0];
    if ( c >= 'a' && c <= 'z' ) c += 'A' - 'a';
    if ( c == 'Y' || c == 'T' || c == '1' )
    {
      *r_ok = TRUE;
      result = TRUE;
    }
    if ( c == 'N' || c == 'F' || c == '0' )
    {
      *r_ok = TRUE;
      result = FALSE;
    }
  }

  return(result);
}

/*------------------------------------------------*/
int int_from_string(char *s,bool *r_ok)
{
  int result;
  if ( is_a_number(s) )
  {
    result = atoi(s);
    *r_ok = TRUE;
  }
  else
  {
    result = 0;
    *r_ok = FALSE;
  }
  return(result);
}

/*------------------------------------------------*/
double double_from_string(char *s,bool *r_ok)
{
  double result;
  if ( is_a_number(s) )
  {
    result = atof(s);
    *r_ok = TRUE;
  }
  else
  {
    result = 0;
    *r_ok = FALSE;
  }
  return(result);
}

void sensible_limits( double xlo, double xhi, double *res_lo, double *res_hi, double *res_delta )
{
  double scale,rel_hi,rel_delta;

  if ( xlo > xhi )
  {
    double temp = xlo; xlo = xhi; xhi = temp;
  }

  if ( xhi - xlo < 1e-50 )
  {
    double xmid = (xlo + xhi)/2.0;
    xlo = xmid - 1e-50;
    xhi = xmid + 1e-50;
  }
    
  scale = pow(10.0,ceil(-log10(xhi - xlo)));
  rel_hi = scale * (xhi - xlo);

  rel_delta = ( rel_hi < 1.5 ) ? 0.2 :
              ( rel_hi < 2.5 ) ? 0.5 :
              ( rel_hi < 5.0 ) ? 1.0 :
              2.0;

  *res_delta = rel_delta / scale;
  *res_lo = *res_delta * floor(xlo / *res_delta);
  *res_hi = *res_delta * ceil(xhi / *res_delta);
}

/* This version is about 3 times as fast.  -jab  */
int next_highest_power_of_two(int n)
{
  int result;

  if (n<=0) return 0;
  if (n<=1) return 1;
  if (n<=2) return 2;
  if (n<=4) return 4;
  if (n<=8) return 8;
  if (n<=16) return 16;
  if (n<=32) return 32;
  if (n<=64) return 64;
  if (n<=128) return 128;
  if (n<=256) return 256;
  if (n<=512) return 512;
  if (n<=1024) return 1024;
  if (n<=2048) return 2048;
  if (n<=4096) return 4096;
  if (n<=8192) return 8192;
  if (n<=16384) return 16384;
  if (n<=32768) return 32768;

  result = 65536;
  n--;
  n >>= 16;
  
  while (n)
  {
    result <<= 1;
    n >>= 1;
  }
  return result;
}


/* Returns TRUE if and only if x is NaN or Inf (i.e. returns FALSE
   if and only if x is a completely legal number) */         
bool is_ill_defined(double x)
{
  return am_isnan(x);
}

/* Returns TRUE iff n is a power of two. Note that 0 is not defined
   to be a power of two. */
bool is_power_of_two(int x)
{
  bool result = x > 0;
  while ( result && x > 1 )
  {
    result = (x & 1) == 0;
    x = x >> 1;
  }
  return result;
}

const char *bufstr(buftab *bt,int i,int j)
{
  const char *result;

  if ( i < 0 || i >= bt->rows || j < 0 || j >= bt->cols )
  {
    result = NULL;
    my_error("bufstr()");
  }
  else
    result = bt->strings[i][j];

  if ( result == NULL )
    result = "-";

  return(result);
}


void fprint_buftab( FILE *s, buftab *bt)
{
  int *widths = AM_MALLOC_ARRAY(int,bt->cols);
  int i,j;

  for (i=0; i < bt->cols; ++i) widths[i] = 0;

  for ( i = 0 ; i < bt->rows ; i++ )
    for ( j = 0 ; j < bt->cols ; j++ )
      widths[j] = int_max(widths[j],strlen(bufstr(bt,i,j)));

  for ( i = 0 ; i < bt->rows ; i++ )
    for ( j = 0 ; j < bt->cols ; j++ )
    {
      char ford[20];
      sprintf(ford,"%%%ds%s",widths[j],(j==bt->cols-1) ? "\n" : " ");
      fprintf(s,ford,bufstr(bt,i,j));
    }

  AM_FREE_ARRAY(widths,int,bt->cols);
}

void init_buftab( buftab *bt, int rows, int cols)
{
  if ( rows < 0 || cols < 0 )
    my_error("init_buftab()");
  else
  {
    int i,j;
    bt -> rows = rows;
    bt -> cols = cols;
    bt -> strings = AM_MALLOC_ARRAY(char_ptr_ptr,rows);
    for ( i = 0 ; i < rows ; i++ )
      bt->strings[i] = AM_MALLOC_ARRAY(char_ptr,cols);
    for ( i = 0 ; i < rows ; i++ )
      for ( j = 0 ; j < cols ; j++ )
        bt->strings[i][j] = NULL;
  }
}

void free_buftab_contents(buftab *bt)
{
  int i,j;
  for ( i = 0 ; i < bt->rows ; i++ )
    for ( j = 0 ; j < bt->cols ; j++ )
      if ( bt->strings[i][j] != NULL )
        AM_FREE_ARRAY(bt->strings[i][j],char, strlen(bt->strings[i][j]) + 1);

  for ( i = 0 ; i < bt->rows ; i++ )
    AM_FREE_ARRAY(bt->strings[i],char_ptr,bt->cols);
    
  AM_FREE_ARRAY(bt->strings,char_ptr_ptr,bt->rows);
}

void set_buftab( buftab *bt, int i, int j, const char *str)
{
  if ( i < 0 || i >= bt->rows || j < 0 || j >= bt->cols )
    my_error("set_buftab()");
  else if ( bt->strings[i][j] != NULL )
    my_error("set_buftab: non null string");
  else
    bt->strings[i][j] = make_copy_string(str);
}

/*-----------------------------------------------------------------*/
