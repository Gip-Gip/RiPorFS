/* The C RiPorFS Utility

The Public File License (see https://github.com/Gip-Gip/PFL for info)

Copyright Charles "Gip-Gip" Thompson, December 18th, 2015

In this case, a file is a group of digital data that can be transferred and
used.

The copyright holder of the file crutil.c has declared that the file and
everything taken from it, unless stated otherwise, is free for any use by any
one, with the exception of preventing the free use of the unmodified file,
including but not limited to patenting and/or claiming further copyright on
the unmodified file.

THE FILE crutil.c IS PROVIDED WITHOUT ANY WARRANTY OR GUARANTEE AT ALL. THE
AUTHOR IS NOT LIABLE FOR CLAIMS, DAMAGES, OR REALLY ANYTHING ELSE IN
CONNECTION TO THIS FILE, UNLESS EXPLICITLY STATED OTHERWISE.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The common types used in place of int, char, and so on */

#define BYTE unsigned char
#define STR char
#define MSG const char
#define NUM int

/* The size definitions. These should stay the same, but we can never tell what
    the new C standard will describe */

#define BYTE_SZ sizeof(BYTE)
#define STR_SZ sizeof(STR)
#define MSG_SZ sizeof(MSG)
#define NUM_SZ sizeof(NUM)
#define SIG_SZ 10

/* RPFS declarations */

#define RPFS_SIG "ext128rpfs"

/* The minimum value argc can have */

#define MINARGC 1

/* The special types for main, argv, and argc */

#define MAIN int main
#define ARGV char *argv[]
#define ARGC int argc

/* The paremeters to certian functions, such as fopen */

#define READMODE "rb" /* The mode used by fopen to open read-only files */

/* The return values */

enum retvals {
    none, /* When there is no error */
    err_unknown, /* When we don't know the error */
    err_noparms, /* When there are no paremeters given */
    err_filednopen, /* When a file didn't open */
    err_nosig, /* When the archive doesn't have a good signature */
    err_malloc, /* When we can't allocate memory */
    err_fread /* When we can't read the file */
};

/* The regular messages */

MSG msg_splash[] = "\n\
The C RiPorFS Utility, v3.2015. Run with --help for a quick guide\n\
\n";

MSG msg_help[] = "\n\
USAGE:\n\
\n\
crutil archive.file (arguments)\n\
\n\
argument \"-v\" or \"--veiw\" displays the contents of archive.file\n\
\n\
*because of the way crutil handles arguments, you can put multiple arguments\n\
 and archives in a simgle command. for example:\n\
\n\
 crutil archive.file -v -f file -v\n\
\n\
 would display the contents of archive.file, add a file to archive.file, and\n\
 then display the archive file with the new file inside of it.\n\
\n";

MSG msg_archset[] = "The archive file is now \"%s\"!\n";
MSG msg_sigcheckin[] = "Checking %s's RiPorFS signature...\n";
MSG msg_validsig[] = "Valid signature!\n";
MSG msg_done[] = "Done!\n";

/* The error messages */

MSG errmsg_noargs[] = "No arguments provided!\n";
MSG errmsg_archnotset[] = "The archive file has not been specified!\n";
MSG errmsg_filednopen[] = "Could not open \"%s\"!\n";
MSG errmsg_malloc[] = "Could not allocate enough memory!\n";
MSG errmsg_fread[] = "Could not read from %s!\n";
MSG errmsg_nosig[] = "The signature is invalid!\n";
MSG errmsg_unknown[] = "An unknown error has occured!!!\n";

/* The globals! */

STR *archiveName = NULL;
FILE *archiveFile = NULL;

/* The functions! */

/* void swapSB(BYTE *_dta_, NUM _sz_) - convert the LSB data to MSB data and
                                        vice versa, at the pointer _dta_

VARIABLES:

  BYTE *_dta_ - the data being converted
  NUM _sz_ - the size of the data being converted

  BYTE _invdta_ - the variable used for inverting the data
  NUM _off_ - the variable that holds the offset to the data being converted

*/

void swapSB(BYTE *_dta_, NUM _sz_)
{
    BYTE _invdta_ = 0;
    NUM _off_ = 0;

    /* While the data offset is less than the size of the data... */
    while(_off_ < _sz_)
    {
        /* Set the invert variable to zero */
        _invdta_ = 0;

        /* Add numbers to the invert variable in relation to AND results */
        if((*(_dta_ + _off_) & 1) == 1) _invdta_ += 128;
        if((*(_dta_ + _off_) & 2) == 2) _invdta_ += 64;
        if((*(_dta_ + _off_) & 4) == 4) _invdta_ += 32;
        if((*(_dta_ + _off_) & 8) == 8) _invdta_ += 16;
        if((*(_dta_ + _off_) & 16) == 16) _invdta_ += 8;
        if((*(_dta_ + _off_) & 32) == 32) _invdta_ += 4;
        if((*(_dta_ + _off_) & 64) == 64) _invdta_ += 2;
        if((*(_dta_ + _off_) & 128) == 128) _invdta_ += 1;

        /* Set the data at the offset to the invert variable */
        *(_dta_ + _off_) = _invdta_;

        /* Increment the offsetr */
        _off_ ++;
    }
}

/* void closeAGfiles( void ) - close all (hard coded) global files */

void closeAGfiles( void )
{
    /* If the archiveFile file is open(not null...) */
    if(archiveFile != NULL)
    {
        /* Close archiveFile... */
        fclose(archiveFile);

        /* and set it to null */
        archiveFile = NULL;
    }
}

/* NUM checkSig(FILE *_arf_) - check for the RiPorFS signature in the file
                               _arf_.

VARIABLES:

  FILE *_arf_ - the archive file to check the signature of

  STR sigBuff - the buffer that holds the signature

*/

NUM checkSig(FILE *_arf_)
{
    STR *sigBuff = malloc(SIG_SZ);

    /* If the signature buffer was not alloctated, return err_malloc */
    if(sigBuff == NULL) return err_malloc;

    /* If there is a comfirmed fread error,
        free sigBuff and return err_fread */
    if(fread(sigBuff, BYTE_SZ, SIG_SZ, _arf_) != SIG_SZ && ferror(_arf_))
    {
        free(sigBuff);
        return err_fread;
    }

    /* If the string read into the signature buffer is equal to the
       archive signature, frees sigBuff and return no errors */

    if(!strncmp(sigBuff, RPFS_SIG, SIG_SZ))
    {
        free(sigBuff);
        return none;
    }

    /* Else, we have to check if the bits are ordered differently... */

    /* Swap the SB! */
    swapSB(sigBuff, SIG_SZ);

    /* If that worked, free sigBuff and return no errors */
    if(!strncmp(sigBuff, RPFS_SIG, SIG_SZ))
    {
        free(sigBuff);
        return none;
    }

    /* If that still isn't the case, then there's no signature! */

    free(sigBuff);
    return err_nosig;
}

/* NUM veiw( void ) - prints the contents of the archive file given in
                      archiveName.
*/

NUM veiw( void )
{
    FILE *archiveFile = fopen(archiveName, READMODE);

    /* if the archive file could not be opened... */
    if(archiveFile == NULL)
    {
        /* print the error message... */
        printf(errmsg_filednopen, archiveName);

        /* and return err_filednopen */
        return err_filednopen;
    }

    /* Tell the veiwer we are checking the archive's signature... */
    printf(msg_sigcheckin, archiveName);

    /* Get the outcome of the signature check */
    switch(checkSig(archiveFile))
    {
        case err_malloc: /* If checkSig couldn't allocate memory,
            print the malloc error, close all global files,
            and return err_malloc */
            printf(errmsg_malloc);
            closeAGfiles();
            return err_malloc;
            break;

        case err_fread: /* If checkSig couldn't read the file,
            print the fread error, close all global files,
            and return err_fread */
            printf(errmsg_fread, archiveName);
            closeAGfiles();
            return err_fread;
            break;

        case err_nosig: /* If the signature wasn't detected,
            print the no signature error, close all global files,
            and return err_nosig */
            printf(errmsg_nosig);
            closeAGfiles();
            return err_nosig;
            break;

        case none: /* If the signature is valid, print the valid sig message */
            printf(msg_validsig);
            break;

        default: /* If the return value isn't here,
            print the unknown error message, close all global files,
            and return err_unknown */
            printf(errmsg_unknown);
            closeAGfiles();
            return err_unknown;
            break;
    }

    /* We have reached the end of veiw, so there were no errors! */

    /* close all the global files... */
    closeAGfiles();

    /* and return no errors! */
    return none;
}

/* MAIN - prints the startup message and interprets the command line
          arguments. If there are no arguments, it returns err_noparms

VARIABLES:

  ARGV - A pre-defined argv. An array that containes the command line arguments
  ARGC - A pre-defined argc. A number which holds the amount of arguments

  NUM argn - the number used for navigating argv

*/

MAIN (ARGC, ARGV)
{
    /* Since argc is not normally zero(I would bet it's never zero),
        we have to set argn to the minimum value argc can be */

    NUM argn = MINARGC;

    /* Print the splash! */
    printf(msg_splash);

    /* If for some reason there are no arguments... */
    if(argn == argc)
    {
        /* Just print the error and return err_noparms */
        printf(errmsg_noargs);
        return err_noparms;
    }

    /* Here, we go though all the arguments and interpret them */
    while(argn < argc)
    {
        /* If the argument is -h or --help... */
        if(!strcmp(argv[argn], "-h") || !strcmp(argv[argn], "--help"))
        {
            /* print the help message ... */
            printf(msg_help);

            /* and since there were no errors, return none */
            return none;
        }

        /* If the argument is -v or --veiw... */
        if(!strcmp(argv[argn], "-v") || !strcmp(argv[argn], "--veiw"))
        {
            /* Call the veiw function if the global archiveName has been set */
            if(archiveName != NULL) veiw();

            /* else, print a soft-error */
            else printf(errmsg_archnotset);
        }

        /* if no arguments are detected,
            assume it is the archive's name being specified */

        else
        {
            /* set the global archiveName to the argument... */
            archiveName = argv[argn];

            /* and print the name */
            printf(msg_archset, archiveName);
        }

        /* Increment argn to go though argv */
        argn ++;
    }

    /* Since the program has reached the end of main, there where no errors! */
    printf(msg_done);
    return none;
}
