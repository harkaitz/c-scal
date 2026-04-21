#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>
#include <stdbool.h>

#define NL "\n"
#define URL_REPO "https://github.com/harkaitz/scal"

#ifdef __unix__
extern int isatty(int fd);
extern int fileno(FILE *stream);
#define WIN_UNIX(WIN,UNIX) UNIX
#endif

#ifdef _WIN32
#define WIN_UNIX(WIN,UNIX) WIN
#endif



char const HELP[] =
    "Usage: scal [-mycH3o:][OPTS...] [SCAL_FILE] [[month] year]"              NL
    ""                                                                        NL
    "The cal utility writes a calendar to the standard output. European style"NL
    "monday first calendar with `-m`. Full year with `-y`. Three month `-3`." NL
    ""                                                                        NL
    "The program supports categorizing days and coloring them. Read the"      NL
    "extended help by running `scal -H`. When piping force colors with `-c`." NL
    ""                                                                        NL
    "Copyright (c) 2026 Harkaitz Agirre Ezama, GPLv2 licensed."               NL
    ;

char const MANUAL[] = 
    "Options are specified one after another, separated by commas, spaces"  NL
    "and newlines. Options are readen from the $SCAL_INFO environment"      NL
    "variable, $SCAL_FILE, -o OPTIONS and "
    #ifdef __unix__
    ""                   "{~/.config,/etc}/calendar.scal ."                 NL
    #endif
    #ifdef _WIN32
    ""                   "%USERPROFILE%/calendar.scal ."                    NL
    #endif
    ""                                                                      NL
    "    @CATEGORY : Starts the definition of a category."                  NL
    "    :STYLE    : Sets the style for the category."                      NL
    "    !nice     : Weekday style prevails to category."                   NL
    "    y2020     : Select a year, for year dependent days."               NL
    "    w0,w6     : Weekday (0: Sunday, 1: Monday)."
    "    m12       : Selects the month, from 1 to 12."                      NL
    "    20        : Mark day category, requires a month before."           NL
    "    10-20     : Mark day range to category."                           NL
    "    TODAY     : Add today date to category."                           NL
    "    l=,TXT=   : Assigns a label to the category, use any terminator."  NL
    "    %m        : Weeks start on Monday. (-m option)"                    NL
    "    %s        : Weeks start on Sunday. (-s option)"                    NL
    "    %l        : Print labels by default on -y. (-l option)"            NL
    ""                                                                      NL
    "Supported styles are: bold, underline, reverse, blink, red, green,"    NL
    "purple, yellow, blue."                                                 NL
    ""                                                                      NL
    "Examples:"                                                             NL
    ""                                                                      NL
    "    @weekend,:red,w0,w6     : Color weekends red."                     NL
    ""                                                                      NL
    "For more information visit the official code repository here:"         NL
    "" URL_REPO ""                                                          NL;

#define errorf(FMT, ...) fprintf(stderr, "scal: error: " FMT "\n", ##__VA_ARGS__)
#define	MAXDAYS 42  /* max slots in a month array */
#define	SPACE   -1  /* used in day array */

typedef struct cat_s cat_t;

struct cat_s {
	char *name;
	char *label;
	char const *style;
	bool  nice;
};

static cat_t *style_day[13][32] = { 0 };
static cat_t *style_wd[7] = { 0 };

static cat_t  categories[10] = { 0 };
static size_t categoriesz    = 0;

static unsigned g_year = 0;
static unsigned g_month;
static unsigned g_day;

static bool opt_monday = false;
static bool opt_all_year = false;
static bool opt_labels = false;
static bool opt_is_tty = false;
static bool opt_3 = false;
static char const *opt_file_source = NULL;

/* Calculation. */
static int   shift_left(int dw);
static int   shift_right(int dw);
static void  day_array(unsigned, unsigned, unsigned *);
static char *build_row(char *p, unsigned *dp, unsigned month);

/* Auxiliary. */
static char *xstrdup(char const *s);
static void *xmalloc(size_t l);
static void  puts_center(char *, unsigned, unsigned);
static void  blank_string(char *buf, size_t buflen);
static void  puts_trim(char *);
static int   xatoi(char const *s);
static char const *fsuffix(char const *f);

/* Styling. */
static char const *get_escapes(char const *name);
static cat_t      *get_category(char const *catname);
static void        read_settings(char const source[], char settings[]);
static void        read_settings_file(char const file[], bool required);
static char const *get_colors(int month, int day, int wd);


#define	WEEK_LEN	20	/* 7 * 3 - one space at the end */
#define	HEAD_SEP	2	/* spaces between day headings */

int
main(int argc, char *argv[])
{
	if (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
		fputs(HELP, stdout);
		return 0;
	}

	if (argc > 1 && (!strcmp(argv[1], "-H") || !strcmp(argv[1], "--manual"))) {
		fputs(MANUAL, stdout);
		return 0;
	}

	#ifdef __unix__
	opt_is_tty = isatty(fileno(stdout));
	#else
	opt_is_tty = 1;
	#endif

	int opt;
	char *opt_o = NULL;
	while((opt = getopt(argc, argv, "o:mslcy3")) != -1) {
		switch (opt) {
		case 'o': opt_o = optarg; break;
		case 'm': opt_monday = true; break;
		case 's': opt_monday = false; break;
		case 'l': opt_labels = true; break;
		case 'c': opt_is_tty = true; break;
		case 'y': opt_all_year = true; break;
		case '3': opt_3 = true; opt_all_year = true; break;
		case '?':
		default:
			return 1;
		}
	}

	time_t now;
	time(&now);
	struct tm *ptm = localtime(&now);

	if (argv[optind] && !strcmp(fsuffix(argv[optind]),".scal")) {
		opt_file_source = argv[optind++];
		opt_all_year = true;
	}

	char *arg1 = argv[optind];
	char *arg2 = (arg1)?argv[optind+1]:NULL;
	if (arg1 && arg2) {
		g_month = xatoi(arg1);
		if (g_month < 1 || g_month > 12) {
			errorf("Invalid month: %s", arg1);
			return 1;
		}
		g_year = xatoi(arg2);
		if (g_year < 1) {
			errorf("Invalid year: %s", arg2);
			return 1;
		}
		g_day = 0;
		opt_all_year = false;
	} else if (arg1) {
		g_year = xatoi(arg1);
		if (g_year < 1) {
			errorf("Invalid year: %s", arg1);
			return 1;
		}
		g_month = 0;
		g_day = 0;
		opt_all_year = true;
	} else {
		g_year = ptm->tm_year + 1900;
		g_month = ptm->tm_mon + 1;
		g_day = ptm->tm_mday;
	}

	/* 1. /etc/calendar.scal */
	#ifdef __unix__
	read_settings_file("/etc/calendar.scal", false);
	#endif

	/* 2. Either $SCAL_FILE or ~/.config/calendar.scal. */
	char *env_scal_file, *env_home;
	if ((env_scal_file = getenv("SCAL_FILE"))) {
		read_settings_file(env_scal_file, true);
	} else if ((env_home = getenv(WIN_UNIX("USERPROFILE","HOME")))) {
		char filename[strlen(env_home)+64];
		sprintf(filename, "%s/.config/calendar.scal", env_home);
		read_settings_file(filename, false);
	}

	/* 3. $SCAL_INFO. */
	char *env_scal_info = xstrdup(getenv("SCAL_INFO"));
	if (env_scal_info) {
		read_settings("$SCAL_INFO", env_scal_info);
	}
	/* 4. Argument -o */
	if (opt_o) {
		read_settings("-o", opt_o);
	}

	/* Select calendar. */
	if (opt_file_source) {
		char const *p = opt_file_source;
		opt_file_source = NULL;
		read_settings_file(p, true);
	}

	char *month_names[12]; 
	for (unsigned i=0; i<12; i++) {
		struct tm zero_tm;
		char buf[40];
		zero_tm.tm_mon = i;
		strftime(buf, sizeof(buf), "%B", &zero_tm);
		month_names[i] = xstrdup(buf);
	}

	char day_headings[28]; /* "Su Mo Tu We Th Fr Sa" */
	blank_string(day_headings, sizeof(day_headings) - 7);
	for (unsigned i=0; i<7; i++) {
		struct tm zero_tm;
		char buf[40];
		zero_tm.tm_wday = shift_right(i);
		strftime(buf, sizeof(buf), "%a", &zero_tm);
		strncpy(day_headings + i * 3, buf, 2);
	}

	if (!opt_all_year) {
		
		unsigned row, len, days[MAXDAYS];
		unsigned *dp = days;
		char lineout[512];

		day_array(g_month, g_year, dp);
		len = sprintf(lineout, "%s %u", month_names[g_month - 1], g_year);
		printf(
		    "%*s%s\n%s\n",
		    ((WEEK_LEN) - len) / 2, "",
		    lineout, day_headings
		);
		for (row = 0; row < 6; row++) {
			build_row(lineout, dp, g_month)[0] = '\0';
			dp += 7;
			puts_trim(lineout);
		}
	} else {
		unsigned row, which_cal, week_len, days[12][MAXDAYS];
		unsigned *dp;
		char lineout[512];

		if (!opt_3) {
			sprintf(lineout, "%u", g_year);
			puts_center(
			    lineout,
			    (WEEK_LEN * 3 + HEAD_SEP * 2)
			    ,
			    0
			);
			puts("\n");
		}
		for (unsigned i = 0; i < 12; i++) {
			day_array(i + 1, g_year, days[i]);
		}
		blank_string(lineout, sizeof(lineout));
		week_len = WEEK_LEN;
		for (unsigned month = 0; month < 12; month += (opt_3)?1:3) {
			if (opt_3 && (month+2)!=g_month) {
				continue;
			}
			puts_center(month_names[month]    , week_len, HEAD_SEP);
			puts_center(month_names[month + 1], week_len, HEAD_SEP);
			puts_center(month_names[month + 2], week_len, 0);
			printf("\n%s%*s%s", day_headings, HEAD_SEP, "", day_headings);
			printf("%*s%s", HEAD_SEP, "", day_headings);
			putchar('\n');
			for (row = 0; row < (6*7); row += 7) {
				lineout[0]='\0';
				char *p = lineout;
				for (which_cal = 0; which_cal < 3; which_cal++) {
					dp = days[month + which_cal] + row;
					p = build_row(p, dp, (month+1+which_cal));
					*(p++) = ' ';
					*p = '\0';
				}
				/* blank_string took care of nul termination. */
				puts_trim(lineout);
			}
		}
	}

	if (opt_labels && opt_is_tty) {
		for (size_t i=0; i<categoriesz; i++) {
			if (categories[i].style) {
				printf("%s%s%s ",
				    categories[i].style,
				    (categories[i].label)?categories[i].label:categories[i].name,
				    "\x1b[0m"
				);
			}
		}
		printf("\n");
	}
}

/* --------------------------------------------------------------------------
 * ---- STYLING -------------------------------------------------------------
 * -------------------------------------------------------------------------- */

static char const *
get_escapes(char const *name)
{
	if (!strcmp(name, "bold")) {
		return "\x1b[1m";
	} else if (!strcmp(name, "underline")) {
		return "\x1b[4m";
	} else if (!strcmp(name, "reverse")) {
		return "\x1b[7m\x1b[1m";
	} else if (!strcmp(name, "blink")) {
		return "\x1b[5m\x1b[7m";
	} else if (!strcmp(name, "red")) { /* fg: Black, bg: red. */
		return "\x1b[30m\x1b[41m";
	} else if (!strcmp(name, "green")) { /* fg: Black, bg: green. */
		return "\x1b[30m\x1b[42m";
	} else if (!strcmp(name, "purple")) { /* fg: Black, bg: purple. */
		return "\x1b[30m\x1b[45m";
	} else if (!strcmp(name, "yellow")) { /* fg: Black, bg: yellow. */
		return "\x1b[30m\x1b[43m";
	} else if (!strcmp(name, "blue")) { /* fg: Yellow, bg: blue. */
		return "\x1b[33m\x1b[44m";
	} else {
		return NULL;
	}
}

static cat_t *
get_category(char const *catname)
{
	for (size_t i=0; i<categoriesz; i++) {
		if (!strcmp(categories[i].name, catname)) {
			return &categories[i];
		}
	}
	if (categoriesz == 10) {
		errorf("To much '@', the maximun is 10.");
		return NULL;
	}
	categories[categoriesz].name = xstrdup(catname);
	return &categories[categoriesz++];
}

static void
read_settings(char const source[], char settings[])
{
	char            *dash;
	static cat_t    *sel_category = NULL;
	static unsigned  sel_year     = 0;
	static unsigned  sel_month    = 0;
	int              number;
	for (char *w = strtok(settings, ", \n\t\r"); w; w = strtok(NULL, ", \n\t\r")) {
		if (w[0] == '%') { /* Global options. */
			if (w[1] == 'm') {
				opt_monday = true;
			} else if (w[1] == 's') {
				opt_monday = false;
			} else if (w[1] == 'l') {
				if (opt_all_year && !opt_3) {
					opt_labels = true;
				}
			}
		} else if (opt_file_source) {
			/* Only read global options from environment. */
		} else if (w[0] == '@') { /* Category. */
			sel_category = get_category(w+1);
			sel_year     = g_year;
			sel_month    = 0;
		} else if (w[0] == 'y') { /* Year. */
			number = xatoi(w+1);
			if (number >= 1) {
				sel_year = xatoi(w+1);
			} else {
				errorf("%s: %s: Invalid year.", source, w);
				sel_year = 0;
			}
		} else if (w[0] == 'm') { /* Month. */
			number = xatoi( ((w[1])=='0') ? w+2 : w+1);
			if(number >= 1 && number <= 12) {
				sel_month = number;
			} else {
				errorf("%s: %s: Valid months are from 1 to 12", source, w);
			}
		} else if (!sel_category) {
			errorf("%s: %s: Specify a category with '@' before.", source, w);
		} else if (w[0] == ':') { /* Category style. */
			sel_category->style = get_escapes(w+1);
		} else if (w[0] == '!') { /* Category options. */
			if (!strcmp(w+1, "nice")) {
				sel_category->nice = 1;
			}
		} else if (w[0] == 'l') { /* Category label. */
			char sep[2] = { (w[1])?w[1]:',', '\0' };
			sel_category->label = xstrdup(strtok(NULL, sep));
		} else if (sel_year != g_year) {
			/* Do nothing, the year doesn't match. */
		} else if (!strcmp(w, "TODAY")) { /* Today. */
			style_day[g_month][g_day] = sel_category;
		} else if (w[0] == 'w') { /* Weekday. */
			style_wd[xatoi(w+1)] = sel_category;
		} else if (!sel_month) {
			/* Do nothing, the month is not selected. */
		} else if ((dash = strchr(w, '-'))) { /* Day range. */
			int number1 = xatoi(w);
			int number2 = xatoi(dash + 1);
			*dash = '\0';
			if (number1 > number2 || number1 < 1 || number2 > 31) {
				errorf("%s: %s: Invalid day range.", source, w);
			} else {
				for (int i=number1; i<=number2; i++) {
					if (!(sel_category->nice && style_day[sel_month][i])) {
						style_day[sel_month][i] = sel_category;
					}
				}
			}
		} else { /* Single day. */
			number = xatoi(w);
			if (number >= 1 && number <= 31) {
				style_day[sel_month][number] = sel_category;
			} else {
				errorf("%s: %s: Invalid day.", source, w);
			}
		}
	}
}

static void
read_settings_file(char const file[], bool required)
{
	FILE *fp = fopen(file, "rb");
	if (!fp && required) {
		errorf("%s: %s", file, strerror(errno));
		exit(1);
	} else if (!fp) {
		return;
	}
	char line[1024];
	while (fgets(line, sizeof(line)-1, fp)) {
		line[sizeof(line)-1] = '\0';
		char *com = strchr(line, '#');
		if (com) {
			*com = '\0';
		}
		read_settings(file, line);
	}
	fclose(fp);
}

static char const *
get_colors(int month, int day, int wd)
{
	if (!opt_is_tty) {
		return NULL;
	}
	cat_t *c1 = style_day[month][day];
	cat_t *c2 = style_wd[shift_right(wd)];
	if (c1 && c2 && c1->nice) {
		return c2->style;
	} else if (c1) {
		return c1->style;
	} else if (c2) {
		return c2->style;
	} else {
		return NULL;
	}
	return NULL;
}

/* --------------------------------------------------------------------------
 * ---- CALCULATION ---------------------------------------------------------
 * -------------------------------------------------------------------------- */

static int
shift_left(int dw)
{
	return opt_monday ? (dw-1)%7 : dw;
}

static int
shift_right(int dw)
{
	return opt_monday ? (dw+1)%7 : dw;
}

static int
leap_year(unsigned yr)
{
	if (yr <= 1752) /* Gregorian reformation in 1752 */
		return !(yr % 4);
	return (!(yr % 4) && (yr % 100)) || !(yr % 400);
}

static void
day_array(unsigned month, unsigned year, unsigned *days)
{
	/*
	 * Fill in an array of 42 integers with a calendar.  Assume for a moment
	 * that you took the (maximum) 6 rows in a calendar and stretched them
	 * out end to end.  You would have 42 numbers or spaces.  This routine
	 * builds that array for any month from Jan. 1 through Dec. 9999.
	 */
	static const unsigned char days_in_month[] = {
		0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	static const unsigned char sep1752[] = {
				1,	2,	14,	15,	16,
		17,	18,	19,	20,	21,	22,	23,
		24,	25,	26,	27,	28,	29,	30
	};
	#define	centuries_since_1700(yr) ((yr) > 1700 ? (yr) / 100 - 17 : 0)
	/* number of centuries since 1700 whose modulo of 400 is 0 */
	#define	quad_centuries_since_1700(yr) ((yr) > 1600 ? ((yr) - 1600) / 400 : 0)
	#define	leap_years_since_year_1(yr) ((yr) / 4 - centuries_since_1700(yr) + quad_centuries_since_1700(yr))

	unsigned long temp;
	unsigned i;
	unsigned day, dw, dm;

	memset(days, SPACE, MAXDAYS * sizeof(int));

	if ((month == 9) && (year == 1752)) {
		/* Assumes the Gregorian reformation eliminates
		 * 3 Sep. 1752 through 13 Sep. 1752.
		 */
		size_t oday = 0;

		do {
			days[oday+2] = sep1752[oday];
		} while (++oday < sizeof(sep1752));

		return;
	}

	/* day_in_year
	 * return the 1 based day number within the year
	 */
	day = 1;
	if ((month > 2) && leap_year(year)) {
		++day;
	}

	i = month;
	while (i) {
		day += days_in_month[--i];
	}

	/* day_in_week
	 * return the 0 based day number for any date from 1 Jan. 1 to
	 * 31 Dec. 9999.  Assumes the Gregorian reformation eliminates
	 * 3 Sep. 1752 through 13 Sep. 1752.  Returns Thursday for all
	 * missing days.
	 */
	temp = (long)(year - 1) * 365 + leap_years_since_year_1(year - 1) + day;
	if (temp < 639787 /* 3 Sep 1752 (First missing day) */) {
		dw = ((temp - 1 + shift_left(6 /* 1 Jan 1 was saturday */)) % 7);
	} else {
		dw = (((temp - 1 + shift_left(6 /* 1 Jan 1 was saturday */)) - 11 /* 11 day correction */) % 7);
	}

        day = 1;

	dm = days_in_month[month];
	if ((month == 2) && leap_year(year)) {
		++dm;
	}

	do {
		days[dw++] = (day++);
	} while (--dm);
}

static char *
build_row(char *p, unsigned *dp, unsigned month)
{
	unsigned col, val, day;

	col = 0;
	do {
		day = *dp++;
		if (day != SPACE) {
			char const *s = get_colors(month, day, col);
			if (s) {
				strcpy(p, s);
				p += strlen(s);
			}
			val = day / 10;
			if (val > 0) {
				*(p++) = val + '0';
			} else {
				*(p++) = ' ';
			}
			*(p++) = day % 10 + '0';
			if (month == g_month && day == g_day) {
				if (opt_is_tty) {
					strcpy(p, "\x1b[7m\x1b[1m");
					p += strlen("\x1b[7m\x1b[1m");
				}
				*(p++) = '<';
				if (opt_is_tty) {
					strcpy(p, "\x1b[0m");
					p += strlen("\x1b[0m");
				}
			} else {
				*(p++) = ' ';
				if (s) {
					strcpy(p, "\x1b[0m");
					p += strlen("\x1b[0m");
				}
			}
			
		} else {
			memset(p, ' ', 3);
			p += 3;
		}
	} while (++col < 7);

	*p = '\0';
	return p;
}

/* --------------------------------------------------------------------------
 * ---- AUXILIARY FUNCTIONS -------------------------------------------------
 * -------------------------------------------------------------------------- */

static char *
xstrdup(char const *s)
{
	char *r; size_t l;
	if (s) {
		l = strlen(s);
		r = xmalloc(l+1);
		memcpy(r, s, l+1);
		return r;
	} else {
		return NULL;
	}
}

static void *
xmalloc(size_t l)
{
	char *r = malloc(l);
	if (!r) {
		errorf("Not enough memory.");
		exit(1);
	}
	return r;
}

static int
xatoi(char const *s)
{
	while (*s == '0') s++;
	return atoi(s);
}

static void
puts_center(char *str, unsigned len, unsigned separate)
{
	unsigned n = strlen(str);
	len -= n;
	printf("%*s%*s", (len/2) + n, str, (len/2) + (len % 2) + separate, "");
}

static void
blank_string(char *buf, size_t buflen)
{
	memset(buf, ' ', buflen);
	buf[buflen-1] = '\0';
}

static void
puts_trim(char *s)
{
	char *p = s;

	while (*p) {
		++p;
	}
	while (p != s) {
		--p;
		if (!isspace(*p)) {
			p[1] = '\0';
			break;
		}
	}

	puts(s);
}

static char const *
fsuffix(char const *f)
{
	char const *suffix = "";
	while (*f) {
		if (*f == '.') {
			suffix = f;
		}
		f++;
	}
	return suffix;
}
