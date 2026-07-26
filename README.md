# DBC4C

A lightweight Design by Contract utility for C.

## Introduction

Design By Contract is a useful technique for developing correct programs.

Much of how software components interact is implied. i.e. what are valid values for various parameters, and what are the expected return values. Sometimes these are commented, but comments still rely on programmers to pay attention to them. It would be much better these assumptions could be written as code. Code doesn’t have lapses of concentration, code doesn’t drift from itself the way comments drift from code.

Secondly, unlike more modern languages, C doesn’t have a simple method for handling exception conditions. Any DBC facility should also provide a simple method for recovering from exceptions. We’ve tried to address Joel Spolsky’s objections to traditional exceptions. Hi Joel! Often code seems to spend more time checking for error codes from called functions than it does dealing with its own logic. We’ve attempted to make that easier too.

This technique can be used in C programs to great effect. It only requires a handful of simple macros (yes, evil macros!) to do the work. DBC provides the following checks:

- Preconditions
- Invariants
- Postconditions

Our DBC module provides Preconditions and Postconditions. Invariants we figure can be managed by remembering that “const” is your friend. We’ve also added Midconditions, which you’ll see below.

## Overview

`DBC4C` is implemented as a single header. It provides macros that can be used for sanity validation in C programs, while keeping normal logic paths flat.

### Provided macros

- `PRECONDITION(expression)` — validate input parameters.
- `MIDCONDITION(expression)` — validate intermediate results.
- `MIDCONDITION_EX(expression, exception_type)` — validate with a special exception label.
- `POSTCONDITION(expression)` — validate return values.
- `FAIL` — fail unconditionally.
- `EXCEPTION` / `EXCEPTION_EX(exception_type)` — exception handler labels.
- `FREEIF(pointer)` — free pointers safely if non-null.
- `CLOSEFDIF(fd)` — close file descriptors safely.
- `CLOSEFILEIF(file)` — close `FILE*` streams safely.

## Example

Consider a conventional C function that allocates resources and must clean up on failure:

```c
TWidget* Widget_New(const char* qString1, const char* qString2) {
    TWidget* newWidget;
    char* newString1;
    char* newString2;

    if (!qString1)
        return NULL;
    if (!qString2)
        return NULL;
    newWidget = malloc(sizeof(TWidget));
    if (!newWidget)
        return NULL;
    newString1 = strdup(qString1);
    if (!newString1) {
        free(newWidget);
        return NULL;
    }
    newString2 = strdup(qString2);
    if (!newString2) {
        free(newString1);
        free(newWidget);
        return NULL;
    }
    newWidget->fString1 = newString1;
    newWidget->fString2 = newString2;
    return newWidget;
}
```

Notice how each subsequent check complicates recovery. As these checks build up, the function consists of more check/recovery than normal logic. Each subsequent check has to undo any previously created intermediate product. Also, the permitted parameter values are not obvious at first glance — you have to trawl through the code to see what values are permitted.

Let’s formalise the normally implied structure of most functions:

- initialise return value to fail condition
- initialise all local pointers
- check parameters — including combinations of parameters
- gather resources
- process resources into product
- set return value to success condition
- check product
- return product

With DBC, the same function is simpler and flatter:

```c
TWidget* Widget_New(const char* qString1, const char* qString2) {
    TWidget* ret = NULL;     // initialise return value to fail
    char* newString1 = NULL; // initialise local pointers to NULL
    char* newString2 = NULL;
    TWidget* newWidget = NULL;

    PRECONDITION(qString1);  // check parameters
    PRECONDITION(qString2);

    newWidget = malloc(sizeof(TWidget));   // gather resources
    MIDCONDITION(newWidget);      // check resource gathering
    newString1 = strdup(qString1);
    MIDCONDITION(newString1);     // check resource gathering
    newString2 = strdup(qString2);
    MIDCONDITION(newString2);     // check resource gathering

    newWidget->fString1 = newString1;    // build product
    newWidget->fString2 = newString2;

    ret = newWidget;          // set return code to success
    POSTCONDITION(ret);      // check product
    return ret;               // return success status

EXCEPTION:                    // all failed conditions go here
    FREEIF(newWidget);       // conditionally free local pointers
    FREEIF(newString1);
    FREEIF(newString2);
    return ret;               // return fail status
}
```

Notice how “flat” the second version is. There are no obvious `if` statements (in fact they’re all hidden under the hood). We could add more dynamically allocated resources and the code would still remain flat. As a result, the code is easier to read, and exception handling is simpler.

## Why not `assert()`?

Note that DBC, like `assert`, is only used for sanity checking. Do not use DBC to implement normal program logic.

This DBC implementation allows a program to recover from errors, whereas the normal `assert` behaviour is to crash the program with an error message. Which is better? Whatever’s most appropriate in your case.

If `DBC_USEASSERTS` is defined, DBC reverts to normal C assert behaviour. That gives you the best of both worlds.

`POSTCONDITION`s are provided mainly for completeness, but in practice you’ll probably find them the least valuable. You’ll use `PRECONDITION` and `MIDCONDITION` extensively.

## Multiple exception handlers

A special exception handler can be achieved via `MIDCONDITION_EX` and `EXCEPTION_EX`:

```c
char* MultipleExceptionFunction(void) {
    char* ret = NULL;    // initialise return value to fail
    char* path = NULL;   // initialise local pointers to NULL
    time_t now;
    struct tm* ts = NULL;

    path = getenv("HOME");
    MIDCONDITION(path);      // check resource gathering

    now = time(NULL);
    ts = localtime(&now);
    MIDCONDITION_EX(ts->tm_year < 2000, CENTURY);

    ret = path;
    POSTCONDITION(ret);
    return ret;

EXCEPTION:
    FREEIF(path);
    return ret;

EXCEPTION_EX(CENTURY):
    FREEIF(path);
    return ret;
}
```

## `FAIL`

`FAIL` is useful where there is no condition to evaluate, such as the `default` branch of a `switch` statement:

```c
int RateOS(void) {
    int ret = -1;    // -1 => error
    int coolness;
    TOperatingSystemCode osCode;

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
    return ret;
}
```

## Build

This project is configured with CMake and builds the `dbctest` example/test target.

```bash
mkdir -p build
cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

## Configuration

- Define `DBC_USEASSERTS` to make DBC behave like standard `assert()`.
- Define `DBC_LOGGING_SYSLOG` to send failure logs to syslog on POSIX platforms.
- Define `NDEBUG` to disable DBC logging while keeping status checks.
- Define `ORGANISATION` to add a prefix to DBC log messages.

## Notes

`dbc_def.h` is designed for simple C programs and prefers DBC checks for sanity validation instead of ordinary control flow.
