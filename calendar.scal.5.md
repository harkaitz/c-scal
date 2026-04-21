# CALENDAR FILES

Calendar files for the *scal(1)* utrility.

# Introduction

Calendars for the *scal(1)* program can be defined in the following
places.

    -o OPTIONS               : (1)
    $SCAL_INFO,$SCAL_FILE    : (2)
    ~/.config/calendar.scal  : (3)
    /etc/calendar.scal       : (4)

Options are specified one after another, separated by commas, spaces
and newlines.

    @CATEGORY Starts the definition of a category.
    :STYLE    Sets the style for the category.
    !nice     Weekday style prevails to category.
    y2020     Select a year, for year dependent days.
    m12       Selects the month, from 1 to 12.
    20        Mark day category, requires a month before.
    10-20     Mark day range to category.
    TODAY     Add today date to category.
    l=,TXT=   Assign a label to category, use any terminator.
    %m        Weeks start on Monday. (-m option)
    %s        Weeks start on Sunday. (-s option)
    %l        Print labels by default on -y. (-l option)

Supported styles are:

    bold, underline, reverse, blink, red, green, purple
    yellow, blue.

# Examples

Color weekends:

    @weekend,:red,w0,w6     : Color weekends red.

Full example:

    %m
    %l
    @reduced,:underline,l=,Intensive 7h=,!nice,w5
    @weekend,:red,w0,w6,l=,Weekends=
    y2026
    @reduced
    m1,5
    m3,18
    m4,1
    m6,22-30
    m7,1-31
    m8,1-31
    m9,1-10
    @holiday,:purple,l=,Holidays=
    m1,1,6,20
    m3,19
    m4,2,3,6
    m5,1
    m7,25,31
    m8,15
    m10,12
    m12,8,25
    @adjust,:yellow,l=,Bridges=
    m4,3,7-10
    m12,24,28-31
    @vacation,:blue,l=,Vacation=
    m8,3-7,10-14
    m12,21-23
    m10,13-16,19-23
    m1,2,5,19
    m3,20

# Authors

Harkaitz Agirre Ezama <harkaitz.aguirre@gmail.com>.

# Collaboration

Feel free to open bug reports and feature/pull requests.

More software like this here:

1. [https://harkadev.com/prj/](https://harkadev.com/prj/)
2. [https://devreal.org](https://devreal.org)

# See also

**CALENDAR.SCAL(5)**, **REMIND(1)**

