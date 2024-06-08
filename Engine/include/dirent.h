/****************************************************************************

    Declaration of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
             Reviewed by Ramon Santamaria for raylib on January 2020.

    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.
    
    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

****************************************************************************/

#ifndef DIRENT_H
#define DIRENT_H

// Allow custom memory allocators
#ifndef DIRENT_MALLOC
    #define DIRENT_MALLOC(sz)   malloc(sz)
#endif
#ifndef DIRENT_FREE
    #define DIRENT_FREE(p)      free(p)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------

// Fordward declaration of DIR, implementation below
typedef struct DIR DIR;

struct dirent
{
    char *d_name;
};

#endif      // DIRENT_H

/****************************************************************************

    Implementation of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
             Reviewed by Ramon Santamaria for raylib on January 2020.

    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.
    
    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

****************************************************************************/

#include <io.h>         // _findfirst and _findnext set errno iff they return -1
#include <stdlib.h>
#include <string.h>
#include <errno.h>

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef ptrdiff_t handle_type;  // C99's intptr_t not sufficiently portable

struct DIR {
    handle_type handle;         // -1 for failed rewind
    struct _finddata_t info;
    struct dirent result;       // d_name null iff first time
    char *name;                 // null-terminated char string
};

DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent *readdir(DIR *dir);
void rewinddir(DIR *dir);
