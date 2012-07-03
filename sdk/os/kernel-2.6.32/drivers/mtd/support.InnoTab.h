#define INNOTAB

/*
** Some useful color colde
*/
#define TERMCOL_black			"\033[22;30m"
#define TERMCOL_red				"\033[22;31m"
#define TERMCOL_green			"\033[22;32m"
#define TERMCOL_brown			"\033[22;33m"
#define TERMCOL_blue			"\033[22;34m"
#define TERMCOL_magenta			"\033[22;35m"
#define TERMCOL_cyan			"\033[22;36m"
#define TERMCOL_gray			"\033[22;37m"
#define TERMCOL_dark_gray		"\033[01;30m"
#define TERMCOL_light_red		"\033[01;31m"
#define TERMCOL_light_green		"\033[01;32m"
#define TERMCOL_yellow			"\033[01;33m"
#define TERMCOL_light_blue		"\033[01;34m"
#define TERMCOL_light_magenta	"\033[01;35m"
#define TERMCOL_light_cyan		"\033[01;36m"
#define TERMCOL_white			"\033[01;37m"


#ifdef INNOTAB
/*
 * For debugging: continuously print out error message when invoked.
 */
void innotab_endlessWarning( const char *str, int line, char *msg );
#define INNOTAB_HANG innotab_endlessWarning( __PRETTY_FUNCTION__, __LINE__, 0 );
//#define INNOTAB_HANG 

void innotab_notice( const char *str, int line, char *msg );
#define INNOTAB_REACH_HERE innotab_notice( __PRETTY_FUNCTION__, __LINE__, 0 );
#else
// Do nothing
#define INNOTAB_DBG
#define INNOTAB_REACH_HERE
#endif