# SCAL

cal, scal - displays a calendar

# Synopsis

    cal [[month] year]
    scal [-mycH3o:] [[month] year]
    scal CALENDAR.scal

# Description

The cal utility writes a calendar to the standard output. The program
supports categorizing days and coloring them.

- *-m* : Monday first calendar.
- *-y* : Display the full year.
- *-c* : Force colored output.
- *-3* : Display 3 months.

# Options

Coloring can be defined in the *\$SCAL_INFO* environment variable, the
*\$SCAL_FILE*, user `~/.config/calendar.scal`, system `/etc/calendar.scal`
or the `-o` option. Read *calendar.scal(5)* manpage for more information.

# Authors

Harkaitz Agirre Ezama <harkaitz.aguirre@gmail.com>.

# Collaboration

Feel free to open bug reports and feature/pull requests.

More software like this here:

1. [https://harkadev.com/prj/](https://harkadev.com/prj/)
2. [https://devreal.org](https://devreal.org)

# See also

**CALENDAR.SCAL(5)**, **REMIND(1)**
