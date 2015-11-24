/* The C RiPorFS Utility

The Public File License (see https://github.com/Gip-Gip/PFL for info)

Copyright Charles "Gip-Gip" Thompson, November 23, 2015

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

/* -- USEFUL COMMENTS COMING IN THE NEXT CENTUREY OR SO -- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum errors {
    none,
    err_fread,
    err_fwrite,
    err_signature
};

#define BYTE unsigned char
#define MSG const char
#define STR char
#define NUM int

#define RPFS_END 0xFF
#define RPFS_FIL 0xFE
#define RPFS_DIS 0xFD
#define RPFS_DIE 0xFC
#define RPFS_TIM 0xFB

BYTE RPFS_END_OEC = RPFS_END;
BYTE RPFS_DIE_OEC = RPFS_DIE;

#define RPFS_SIG "ext128rpfs"

STR *archiveName;

STR sigCheck[] = RPFS_SIG;

STR pathDelimiter[] = "|";

MSG startup[] = "C RiPorFS Utilities, v1.2015\n";
MSG initializing[] = "Initializing variables...\n";
MSG done[] = "Done!\n";
MSG EOFSfound[] = "EOFS found!\n";
MSG foundDir[] = "Found the directory \"%s\"!\n";
MSG foundFile[] = "Found the file \"%s\"!\n";
MSG help[] = "\n\
USAGE:\n\
crutil archive.file [arguments]\n\
\n\
ARGUMENTS:\n\
\n\
-h or --help = display this message and exit\n\
-d / or --path-delimiter / = set the path delimiter to /\n\
-v or --veiw = veiw the files packed inside archive.file\n\
\n";

MSG archiveSet[] = "The new archive file is \"%s\"!\n";
MSG delimiterSet[] = "The new path delimiter is \"%c\"!\n";

MSG noArchive[] = "Archive file not set!\n";
MSG noFile[] = "File does not exist!\n";
MSG badSig[] = "Bad signature!\n";
MSG fileIOerror[] = "File I/O error!\n";
MSG badOEC[] = "Bad OEC! Check @ ridge %d";
MSG badFilename[] = "Bad Filename!\n";
MSG mallocError[] = "Could not allocate memory!\n";
MSG badTimestamp[] = "Bad timestamp!\n";
MSG badDirectory[] = "Bad directory!\n";

NUM strsize = 0;

BYTE OEC8(BYTE *data, NUM size)
{
    BYTE checksum = 0;
    NUM cursor = 0;
    BYTE dataClone;
    NUM offsetCount;
    while(cursor < size)
    {
        dataClone = *(data + cursor);
        if(dataClone & 1 == 1)
        {
            offsetCount = cursor;
            while(offsetCount > 0)
            {
                dataClone = *(data + cursor);
                while(dataClone > 0)
                {
                    if(checksum == 255) checksum = 0;
                    checksum ++;
                    dataClone --;
                }
                offsetCount --;
            }
        }
        else if(checksum > 0)
        {
            offsetCount = cursor;
            while(offsetCount > 0)
            {
                dataClone = *(data + cursor);
                while(dataClone > 0)
                {
                    if(checksum == 0) checksum = 255;
                    checksum --;
                    dataClone --;
                }
                offsetCount --;
            }
        }
        cursor ++;
    }
    return checksum;
}

NUM checkSig(FILE *in)
{
    if(fread(sigCheck, 1, sizeof(sigCheck) - 1, in) != sizeof(sigCheck) - 1) return err_fread;
    if(strncmp(sigCheck, RPFS_SIG, sizeof(sigCheck))) return err_signature;
    return none;
}

STR * fmakestr(BYTE term, FILE *data)
{
    BYTE buffer = '0';
    strsize = 0;
    while(buffer != term)
    {
        if(fread(&buffer, 1, 1, data) != 1) return NULL;
        strsize ++;
    }
    fseek(data, -strsize, SEEK_CUR);
    STR *outstr = malloc(strsize);
    if(outstr == NULL) return NULL;
    if(fread(outstr, 1, strsize, data) != strsize)
    {
        free(outstr);
        return NULL;
    }
    
    return outstr;
}

STR * appendStr(STR *str, STR *srcstr)
{
    STR * outstr = malloc(strlen(str) + strlen(srcstr) + 1);
    strcpy(outstr, str);
    strcat(outstr, srcstr);
    free(str);
    return outstr;
}

STR * removeParentString(STR *str, STR delimiter)
{
    NUM cursor = strlen(str);
    while(*(str + cursor) != delimiter && cursor != 0)
    {
        cursor --;
    }
    if(cursor == 0) return str;
    *(str + cursor) = 0;
    STR * outstr = malloc(strlen(str) + 1);
    strcpy(outstr, str);
    free(str);
    return outstr;
}

NUM veiw()
{
    FILE *archiveFile = fopen(archiveName, "rb");
    if(archiveFile == NULL)
    {
        printf(noFile);
        return 0;
    }
    
    NUM sigState = checkSig(archiveFile);

    if(sigState == err_signature)
    {
        printf(badSig);
        return 0;
    }
    
    if(sigState == err_fread)
    {
        printf(fileIOerror);
        return 0;
    }

    BYTE dataChecksum = 0;
    BYTE ridge = 0;

    BYTE *dataBuffer = NULL;

    NUM EOFStrue = 0;

    NUM ridgeCount = 0;
    
    STR *root = malloc(strlen(archiveName) + 1);
    strcpy(root, archiveName);
    root = appendStr(root, pathDelimiter);
    
    STR * fullName = NULL;
    
    while(fread(&dataChecksum, 1, 1, archiveFile) == 1 && EOFStrue == 0)
    {
        fread(&ridge, 1, 1, archiveFile);
        switch(ridge)
        {
            case RPFS_END:
                if(dataChecksum != 255)
                {
                    printf(badOEC, ridgeCount);
                    free(dataBuffer);
                    return 0;
                }
                printf(EOFSfound);
                EOFStrue = 1;
                break;
            case RPFS_FIL:
                fseek(archiveFile, -1, SEEK_CUR);
                dataBuffer = fmakestr(0, archiveFile);
                
                if(dataBuffer == NULL)
                {
                    printf(badFilename);
                    return 0;
                }
                
                if(OEC8(dataBuffer, strsize) != dataChecksum)
                {
                    printf(badOEC, ridgeCount);
                    free(dataBuffer);
                    return 0;
                }
                
                fullName = malloc(strlen(root) + 1);
                strcpy(fullName, root);
                fullName = appendStr(fullName, (dataBuffer + 1));
                
                printf(foundFile, fullName);
                free(dataBuffer);
                free(fullName);
                break;
            case RPFS_DIS:
                fseek(archiveFile, -1, SEEK_CUR);
                dataBuffer = fmakestr(0, archiveFile);
                
                if(dataBuffer == NULL)
                {
                    printf(badDirectory);
                    return 0;
                }
                
                if(OEC8(dataBuffer, strsize) != dataChecksum)
                {
                    printf(badOEC, ridgeCount);
                    free(dataBuffer);
                    return 0;
                }
                
                root = appendStr(root, (dataBuffer + 1));
                root = appendStr(root, pathDelimiter);
                
                printf(foundDir, root);
                free(dataBuffer);
                break;
            case RPFS_DIE:
                if(dataChecksum != RPFS_DIE_OEC)
                {
                    printf(badOEC, ridgeCount);
                    return 0;
                }
                root = removeParentString(root, pathDelimiter[0]);
                root = removeParentString(root, pathDelimiter[0]);
                root = appendStr(root, pathDelimiter);
                break;
            case RPFS_TIM:
                fseek(archiveFile, -1, SEEK_CUR);
                dataBuffer = fmakestr(0, archiveFile);

                if(dataBuffer == NULL)
                {
                    printf(badTimestamp);
                    return 0;
                }

                if(OEC8(dataBuffer, strsize) != dataChecksum)
                {
                    printf(badOEC, ridgeCount);
                    free(dataBuffer);
                    return 0;
                }

                printf("The following files were created %s Unix time\n"
                    , (dataBuffer + 1));
                free(dataBuffer);
                break;
            default:
                dataBuffer = malloc(ridge + 1);
                if(dataBuffer == NULL)
                {
                    printf(mallocError);
                    return 0;
                }

                fseek(archiveFile, -1, SEEK_CUR);
                
                if(fread(dataBuffer, 1, ridge + 1, archiveFile) != ridge + 1)
                {
                    printf(fileIOerror);
                    free(dataBuffer);
                    return 0;
                }
                
                if(OEC8(dataBuffer, ridge + 1) != dataChecksum)
                {
                    printf(badOEC, ridgeCount);
                    free(dataBuffer);
                    return 0;
                }
                free(dataBuffer);
                break;
        }
        ridgeCount ++;
    }
    
    fclose(archiveFile);
    return 0;
}

/* Initialize all the required globals, along with a few more things */
NUM initialize()
{
    printf(initializing);
    RPFS_END_OEC = OEC8(&RPFS_END_OEC, 1);
    RPFS_DIE_OEC = OEC8(&RPFS_DIE_OEC, 1);
    return none;
}

NUM main(NUM argc, char *argv[])
{
    printf(startup);
    initialize();
    NUM argumentNum = 1;
    while(argumentNum < argc)
    {
        if(!strcmp(argv[argumentNum], "-h") || \
            !strcmp(argv[argumentNum], "--help"))
        {
            printf(help);
            return 0;
        }
        
        if(!strcmp(argv[argumentNum], "-d") ||
            !strcmp(argv[argumentNum], "--path-delimiter"))
        {
            argumentNum ++;
            pathDelimiter[0] = *argv[argumentNum];
            printf(delimiterSet, *pathDelimiter);
        }
        
        else if(!strcmp(argv[argumentNum], "-v") || 
            !strcmp(argv[argumentNum], "--veiw"))
        {
            if(archiveName != NULL) veiw();
            else printf(noArchive);
        }
        
        else
        {
            archiveName = argv[argumentNum];
            printf(archiveSet, archiveName);
        }
        argumentNum ++;
    }
    printf(done);
    return 0;
}
