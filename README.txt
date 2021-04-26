Design By Contract is a useful technique for developing correct programs.

Much of how software components interact is implied.  i.e., what are valid
values for various parameters, and what are the expected return values.
Sometimes these are commented, but comments still rely on programmers to pay
attention to them.  It would be much better these assumptions could be written
as code.  Code doesn’t have lapses of concentration, code doesn’t drift from
itself the way comments drift from code.

Secondly, unlike more modern languages, C doesn’t have a simple method for
handling exception conditions.  Any DBC facility should also provide a simple
method for recovering from exceptions.  We’ve tried to address Joel Spolsky’s
objections to traditional exceptions.  Hi Joel!  Often code seems to spend
more time checking for error codes from called functions than it does dealing
with its own logic.  We’ve attempted to make that easier too.

This technique can be used in C programs to great effect.  It only requires a
handful of simple macros (yes, evil macros!) to do the work.  DBC provides the
following checks:

  Preconditions
  Invariants
  Postconditions

It always bugged us that programmers had to rely on comments in code that
drifted with time to be completely misleading.  In particular, it bugged us
that comments were needed to say what was happening in the code at all.  We
thought there must be a better way.  So after a few early efforts we ended up
with this.

Our DBC module provides Preconditions and Postconditions.  Invariants we figure
can be managed by remembering that “const” is your friend.  We’ve also added
Midconditions, which you’ll see later.

Let’s look at a simple (and very contrived!) C function:

TWidget* Widget_New( const char* qString1, const char* qString2) {
    TWidget* newWidget;
    char* newString1;
    char* newString2;
	
    if (!qString1)
        return NULL;
    if (!qString2)
        return NULL;
    if (!qString3)
        return NULL;
    newWidget = malloc( sizeof( TWidget));
    if (!newWidget)
        return NULL;
    newString1 = strdup( qString1);
    if (!newString1) {
        free( newWidget);
        return NULL;
    }
    newString2 = strdup( qString2);
    if (!newString2) {
        free( newString1);
        free( newWidget);
        return NULL;
    }
    newWidget->fString1 = newString1;
    newWidget->fString2 = newString2;
    return newWidget;
}


Notice how each subsequent check complicates our recovery?  As these checks
build up, the function consists of more check/recovery than normal logic.  Each
subsequent check has to undo any previously created intermediate product.
Also, the permitted parameter values are not obvious at first glance - you have
to trawl through the code to see what values are permitted.

Lets formalise the normally implied structure of most functions:

  initialise return value to fail condition
  initialise all local pointers
  check parameters - including combinations of parameters
  gather resources
  process resources into product
  set return value to success condition
  check product
  return product

So the DBC version of our function would look like:
  
TWidget* Widget_New( const char* qString1, const char* qString2) {
    TWidget* ret = NULL;     // initialise return value to fail
    char* newString1 = NULL; // initialise local pointers to NULL
    char* newString2 = NULL;
    TWidget* newWidget = NULL;
	
    PRECONDITION( qString1);  // check parameters
    PRECONDITION( qString2);
	
    newWidget = malloc( sizeof( TWidget));   // gather resources
    MIDCONDITION( newWidget);      // check resource gathering
    newString1 = strdup( qString1);
    MIDCONDITION( newString1);     // check resource gathering
    newString2 = strdup( qString2);
    MIDCONDITION( newString2);     // check resource gathering

    newWidget->fString1 = newString1;    // build product
    newWidget->fString2 = newString2;
	
    ret = newWidget;          // set return code to success
    POSTCONDITION( ret);      // check product
    return ret;               // return success status
	
EXCEPTION:                    // all failed conditions go here
    FREEIF( newWidget);       // conditionally free local pointers
    FREEIF( newString1);
    FREEIF( newString2);
    return ret;               // return fail status
}

Notice how “flat” the second version is?  There are no obvious “if” statements
(in fact they’re all hidden under the hood).  We could add a fourth, fifth, or
umpteenth dynamically allocated resource and our code would still be flat.

As a result the code is much easier to read, and in particular the exception
handling is much simpler.  Further, all permitted parameter values are
explicitly stated at the top of the function via PRECONDITIONs.  Lastly, the
DBC macros can provide logging when problems occur - something the first
example doesn’t even bother with.

Note that DBC, like “assert”, is only used for sanity checking - it should not
be used for branches which are part of a function’s normal logic.

So why this and not assert?  This DBC implementation allows a program to
recover from errors, whereas the normal assert behaviour is to simply crash the
program with an error message.  Which is better?  Whatever’s most appropriate
in your case.

Assert could be modified, but instead of modifying its agreed behaviour beyond
recognition it’s better to supplant it with DBC instead.  This DBC module
provides a DBC_USEASSERTS flag, which if defined will cause DBC to revert to
normal C assert behaviour, so you get the best of both worlds.

POSTCONDITIONs are provided mainly for completeness, but in practice you’ll
probably find them the least valuable.  You’ll use PRE & MIDCONDITIONS
extensively.

What about multiple exception handlers?  This can be achieved via the
MIDCONDITION_EX/EXCEPTION_EX macros:

char* MultipleExceptionFunction( void) {
    char* ret = NULL;    // initialise return value to fail
    char* path = NULL;   // initialise local pointers to NULL
    time_t now;
    struct tm* ts = NULL;

    // gather resources
    path = getenv( “HOME”);
    MIDCONDITION( path);      // check resource gathering

    // detect some other error condition
    now = time( NULL);
    ts = localtime( &now);
    // we should never be running in 20th century!!!
    MIDCONDITION_EX( ts->tm_year < 2000, CENTURY);

    // build products
    ret = path;               // set return code to success

    // check products
    POSTCONDITION( ret);

    // return products
    return ret;        // return success status

EXCEPTION:
    // all failed conditions go here
    FREEIF( path);     // conditionally free local pointers
    return ret;        // return fail status

EXCEPTION_EX( CENTURY):
    // CENTURY exceptions go here
    FREEIF( path);     // conditionally free local pointers
    // perform some other recovery
    return ret;        // return fail status
}

A FAIL instruction is also provided.  This is useful where there is no
condition to evaluate, but the program is executing where it shouldn’t.  The
only place you’re likely to use this is in the “default” handler of a switch
statement:

int RateOS( void) {
    int ret = -1;    // -1 => error
    int coolness;
    TOperatingSystemCode osCode;

    // gather resources
    osCode = GetOperatingSystemCode();
    switch (osCode) {
        case kMacOSXOperatingSystemCode:
        case kLinuxOperatingSystemCode:
        case kChromeOperatingSystemCode:
            coolness = 100;
            break;

        case kWindowsOperatingSystemCode:
            coolness = 30;
            break;

        default:
            FAIL;    // what other OS?
            break;
    }

    ret = coolness;    // set return code to success
    return ret;        // return success status

EXCEPTION:
    return ret;        // return fail status
}

DBC is a lot easier if you define validation macros for any enums in your
program, ideally straight after the enum definition itself:

typedef enum {
    kChromeOperatingSystemCode,
    kLinuxOperatingSystemCode,
    kMacOSXOperatingSystemCode,
    kWindowsOperatingSystemCode,
    kMaxOperatingSystemCode
} TOperatingSystemCode;

#define ValidOperatingSystemCode(os) \
    (((os) >= 0) && ((os) < kMaxOperatingSystemCode))
    
Remember that enums can be negative even though the first member defaults to 0.
This validation macro can then be used in PRECONDITIONs throughout the program
wherever the enum is passed as a parameter.

BTW, if you’re still using #defines to identify a bunch of like-minded
constants, you should strongly consider turning them into a single enum.  Your
code will be much more self-documenting if you do.

The following provides an example of the validation macro’s usage:

void PrintOS( const TOperatingSystemCode qOSCode) {
    TOperatingSystemCode osCode;

    PRECONDITION( ValidOperatingSystemCode( qOSCode));

    printf( “Operating System Code = %d\n”, qOSCode);
    return;

EXCEPTION:
    return;
}

Exception handlers rely on two things:

  initialisation of dynamically allocated variables to NULL or 0
  Various conditional “free” macros to free these variables

Our DBC module provides:

  FREEIF() - conditionally free dynamically allocated memory
  CLOSEFDIF() - conditionally close a file descriptor
  CLOSEFILEIF() - conditionally close a file stream

These macros do nothing if the passed parameter is 0 or NULL.  Otherwise they
free or close the specified item.  We probably need some other macros, but off
the top of our heads, we can’t think of any.

NOTE: DBC4C is strictly for C.  It's not for C++ or Objective-C.  Why?

    1. C++/Objective-C built-in exceptions.
    2. Reference-counted and other smart pointers

Built-in exceptions bypass any of the DBC exception handling and thus avoid
freeing dynamically allocated objects.

Then there are reference-counted pointers.  Normal C-like pointers are easy -
they’re either allocated, or they’re not.  How can you tell after the fact
if a reference was incremented or not unless you checked initially, and
stashed the count somewhere to check later?  Just because a reference was
incremented, is it correct to decrement it?  That's a can of worms.

Nevertheless, PRECONDITIONs that raise exceptions may still be useful for
those languages.  That's a whole 'nother project though.

DBC messages can be directed to the console, syslog, or suppressed depending
on flag settings.  Messages include: filename; function name; line number and
also the test expression which evaluated as false.

One last fringe benefit of this technique is that it makes it simple to
identify white-box tests in your code.  Basically every CONDITION is a possible
point of failure that you can grep and wc for.

To build the test program:

    mkdir build
    cd build
    cmake ..
    make
    ./dbctest

The test program should print the following:

damco: PRECONDITION failed - /home/craig/project/dbc4c/dbctest.c:PRECONDITION_Fail:43, "NULL" is false
damco: MIDCONDITION failed - /home/craig/project/dbc4c/dbctest.c:MIDCONDITION_Fail:58, "NULL" is false
damco: POSTCONDITION failed - /home/craig/project/dbc4c/dbctest.c:POSTCONDITION_Fail:73, "false" is false
damco: failed - /home/craig/project/dbc4c/dbctest.c:FAIL_Fail:91
DBC module is working correctly

We realise this is a rapid-fire overview of something that would permeate every
corner of your C programs if you use it, but it’s really not much more
complicated than that.  Take it. Enjoy it.  Let us know how it could be
improved.

