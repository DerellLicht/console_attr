THIS ZIP FILE INCLUDES:

    PRISM.PAS    - main PRISM file, declarations and main body of program
    PRISM.INC    - PRISM include file, the guts of the program
    PRISMENU.INC - PRISM include file for menu/help system
    PRISM.ASC    - raw help text
    PRISM.OBJ    - processed help text, ready for linking
    README       - this file
    DGTOOLS.ZIP  - The David Gerrold Toolbox, a set of supplementary units


TO COMPILE PRISM, YOU WILL NEED

    Turbo Pascal 5.5 or later.  (From Borland International)
    Object Professional.        (From TurboPower Software)
    The David Gerrold Toolbox   (included)


HOW TO COMPILE PRISM

1)  First, unzip DGTOOLS.ZIP.  The David Gerrold Toolbox is a "work in
progress".  It is a set of units containing a variety of reusable functions.
The code contains its own documentation.

2)  You will need to have Object Professional's units compiled and in a
    directory that Turbo Pascal can find.

3)  You will need an appropriately configured TPC.CFG file in the same
    directory as PRISM's source code.

    Enter:

      TPC PRISM <Enter>

    and Turbo Pascal's command line compiler will automatically compile
    PRISM.

4)  If you wish to recompile the help file you will need to perform these
    two steps before compiling PRISM:

    First, enter:

      MAKEHELP PRISM.ASC PRISM.HLP <Enter>

    This will use Object Professional's MakeHelp utility to process the
    MAGHLP.ASC file into MAGHLP.HLP, a compressed text file.

    Now, enter:

      BINOBJ PRISM.HLP PRISM.OBJ HELPTEXT

    This will use Turbo Pascal's Binobj utility to process MAGHLP.HLP into
    MAGHLP.OBJ so that it can be directly linked into the program;  this will
    allow the help text to be a part of the compiled program instead of a
    separate file.

    Now, enter:

      TPC PRISM <Enter>

    Assuming that you have properly installed Turbo Pascal on your system,
    this should allow PRISM to compile without problem.



COPYRIGHT

  PRISM, v1.0, a VGA palette editor was written by David Gerrold.
  This version is copyright (c) 1990 Ziff Davis Communications Co.
