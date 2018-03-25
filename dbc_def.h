/*
 * File: dbc_def.h
 *
 * Description:
 * This module provides a simple Design by Contract system for use in C.
 *
 * (c) Delaney & Morgan Computing 2009-2010.
 * <admin@delaneymorgan.com.au>
 * Anyone is free to copy this software as is, or modified as long as
 * this attribution remains. i.e. no complicated licences, no lawyers,
 * and no restrictions. Just use it, enjoy it, and relax.
 * We won't hassle you about it if you don't hassle us.
 *
 * Usage:
 * Simply include the header into your module body.
 *
 * DBC Usage:
 * =========
 * PRECONDITION( assertion) - validate parameter
 * MIDCONDITION( assertion) - validate function call return values
 * MIDCONDITION_EX( assertion, exception_type) - validate with special exception
 * POSTCONDITION( assertion) - validate return values
 * FAIL - fail unconditionally
 * EXCEPTION - exception handler start
 * EXCEPTION_EX( exception_type) - specialised exception handler start
 * FREEIF( automatic_pointer) - conditionally free automatic pointer
 * CLOSEFDIF( file_descriptor) - conditionally file descriptor
 * CLOSEIF( file_pointer) - conditionally file pointer
 * RELEASE( object_reference) - release & null object reference
 * RELEASEIF( object_reference) - conditionally release & null object reference
 *
 * Only use DBC for sanity checks, like you would "assert".  Do not use DBC to detect
 * conditions that might occur in normal situations.  i.e., DBC should not be used to
 * implement normal program logic.
 *
 * STYLE:
 * functions should be structured as follows:
 * 1. parameter checks (PRECONDIITION)
 * 2. gathering of resources - check child functions for failure (MIDCONDIITION)
 * 3. assembling of components - check results are consistent (MIDCONDIITION)
 * 4. produce final product - check product (POSTCONDITION)
 * 5. return
 *
 * char* DupString( const char* const qString1) {
 *    char* ret = NULL;      // initialise return value to error indicator
 *    char* newPtr = NULL;   // all pointer vars initialised to NULL
 *    char* result = NULL;
 *
 *    PRECONDITION( qString);      // check input parameter
 *    // preconditions can check parameter combinations as well as single parameters
 *
 *    newPtr = malloc( strlen( qString) + 1);  // obtain component
 *    MIDCONDITION( newPtr);                   // check component
 *    result = strcpy( newPtr, qString1);
 *    MIDCONDITION_EX( result == newPtr, SPECIAL);      // use special handler if needed
 *
 *    ret = newPtr;               // assemble components
 *    POSTCONDITION( ret);        // check final product
 * END:                            // normal exit label
 *    return ret;
 *
 * EXCEPTION:
 *    // if any condition fails, execution jumps here
 *    FREEIF( newPtr);         // clean up any intermediate components
 *    // returning NULL => exception occurred.
 *    return ret;
 *
 * EXCEPTION(SPECIAL):
 *    // special exception handler.  Have as many as you dare.
 *    FREEIF( newPtr);         // clean up any intermediate components
 *    // returning NULL => exception occurred.
 *    return ret;
 * }
 *
 * CODE SIZE & PERFORMANCE:
 * When logging, each condition check embeds a reference to the file name function name
 * string constants which will expand code-size.  This is almost always not a
 * problem.  When logging is disabled this string does not exist, so code-size is only
 * increased by an "if" test, which is minimal.
 *
 * Each condition check evaulates the speficied expression just once.  It is safe,
 * but probably poor form, to pass modifying expressions (i.e. i++) into them.  Remember
 * that any exception handling only occurs if there is a failure - not during normal
 * execution.
 *
 * LOGGING:
 * Logging can be directed to various locations and even disabled.
 * The console is the best destination during normal development, however
 * after deployment the system console should be used.
 * Logging can be disabled (all checks are still functional) if desired.
 * Finally, logging defaults to ON, with output going to the console.
 *
 * MacOSX logging is always directed to NSLog when logging is enabled.
 *
 * If NDEBUG flag is set to TRUE, then logging is disabled.
 *
 * If DBC_LOGGING_SYSLOG is set, then logging will be sent to the syslog.  Otherwise
 * it will appear on the console.
 *
 * If DBC_USEASSERTS is defined, then DBC will resort to using normal C assert behaviour.
 *
 */

#ifndef dbc_def__h
#define dbc_def__h


#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <syslog.h>

// =====================================================================


// define your own or modify this to identify your organisation in DBC logs
// NOTE: make sure string is in double quotes
//#ifndef ORGANISATION
//    #define ORGANISATION	"damco"
//#endif

#define END endLabel

#define EXIT() \
    do \
    { \
        goto END; \
    } while (0)

#define EXITIF(e) \
    do \
    { \
        if ((e)) \
        { \
            goto END; \
        } \
    } while (0)

#define EXCEPTION errorLabel
#define EXCEPTION_EX(where) EXCEPTION##where

#ifdef DBC_LOGGING_SYSLOG
    // send logs to syslog
    // NOTE: logging will be separately controlled by NDEBUG flag
    #define ERRLOGPRINTF syslog
    #define ERRLOGDEST LOG_ERR
#else
    // use stdout console
    #define ERRLOGPRINTF fprintf
    #define ERRLOGDEST stdout
#endif


#ifdef NDEBUG

    // FALSE => DBC exceptions will NOT be logged
    #define DBC_LOGGING false

#else

    // TRUE => DBC exceptions will be logged, FALSE => not logged.
    // logging will expand code-size with file and function name strings
    #define DBC_LOGGING true

#endif // NDEBUG

#ifdef ORGANISATION
#define ORGPREFIX  ORGANISATION ": "
#else
#define ORGPREFIX
#endif

// use C compiler's __func__ enhancement if under GNU C
#ifdef __GNUC__

	#if defined( __APPLE__) && defined( __MACH__) && defined (__OBJC__)

		// macosx - use NSLog instead of other mechanisms
		#define CHECK(exp,str,context) \
			do \
			{ \
			if (!(exp)) \
			{ \
				NSLog( @ #ORGPREFIX #context " failed - %s:%s:%d, " #str " is false", __FILE__, __func__, __LINE__); \
			} \
			} while (0)

		#define FAIL \
			do \
			{ \
				NSLog( @ #ORGPREFIX "failed - %s:%s:%d", __FILE__, __func__, __LINE__); \
				goto EXCEPTION; \
			} while (0)

	#else		// defined( __APPLE__) && defined( __MACH__)

		#define CHECK(exp,str,context) \
			do \
			{ \
			if (!(exp)) \
			{ \
				ERRLOGPRINTF( ERRLOGDEST, ORGPREFIX #context " failed - %s:%s:%d, " #str " is false\n", __FILE__, __func__, __LINE__); \
			} \
			} while (0)

		#define FAIL \
			do \
			{ \
				ERRLOGPRINTF( ERRLOGDEST, ORGPREFIX "failed - %s:%s:%d\n", __FILE__, __func__, __LINE__); \
				goto EXCEPTION; \
			} while (0)

	#endif		// defined( __APPLE__) && defined( __MACH__)

#else  // __GNUC__

    #define CHECK(exp,str,context) \
        do \
        { \
            if (!(exp)) \
            { \
                ERRLOGPRINTF( ERRLOGDEST, #ORGPREFIX #context " failed - %s:%d, " #str " is false\n", __FILE__, __LINE__); \
            } \
        } while (0)

    #define FAIL \
        do \
        { \
            ERRLOGPRINTF( ERRLOGDEST, ORGPREFIX "failed - %s:%d\n", __FILE__, __LINE__); \
            goto EXCEPTION; \
        } while (0)

#endif  // __GNUC__


// the PRE, MID, and POST condition tests.  If the expresssion evaluates to TRUE, they do nothing.
// otherwise they abort the flow of execution by jumping to a standard exception label and log the event
#ifdef DBC_USEASSERTS

    #define PRECONDITION(exp) \
    	do \
    	{ \
    		assert((exp)); \
    		if (0) \
    		{ \
    			goto EXCEPTION; \
    		} \
    	} while (0)
    	
    #define MIDCONDITION(exp) \
    	do \
    	{ \
    		assert((exp)); \
    		if (0) \
    		{ \
    			goto EXCEPTION; \
    		} \
    	} while (0)
    	
	#define MIDCONDITION_EX(exp,where) \
    	do \
    	{ \
    		assert((exp)); \
    		if (0) \
    		{ \
				goto EXCEPTION##where; \
    		} \
    	} while (0)
    	
    #define POSTCONDITION(exp) \
    	do \
    	{ \
    		assert((exp)); \
    		if (0) \
    		{ \
    			goto EXCEPTION; \
    		} \
    	} while (0)

#else

	#if DBC_LOGGING
		#define PRECONDITION(exp) \
			do \
			{ \
				bool dbc_temp_var_do_not_use; \
				dbc_temp_var_do_not_use = (exp) ? true: false; \
				CHECK( dbc_temp_var_do_not_use, #exp, PRECONDITION); \
				if (!dbc_temp_var_do_not_use) \
				{ \
					goto EXCEPTION; \
				} \
			} while (0)
	
		#define MIDCONDITION(exp) \
			do \
			{ \
				bool dbc_temp_var_do_not_use; \
				dbc_temp_var_do_not_use = (exp) ? true : false; \
				CHECK( dbc_temp_var_do_not_use, #exp, MIDCONDITION); \
				if (!dbc_temp_var_do_not_use) \
				{ \
					goto EXCEPTION; \
				} \
			} while (0)
	
		#define MIDCONDITION_EX(exp,where) \
			do \
			{ \
				bool dbc_temp_var_do_not_use; \
				dbc_temp_var_do_not_use = (exp) ? true : false; \
				CHECK( dbc_temp_var_do_not_use, #exp, MIDCONDITION); \
				if (!dbc_temp_var_do_not_use) \
				{ \
					goto EXCEPTION##where; \
				} \
			} while (0)
	
		#define POSTCONDITION(exp) \
			do \
			{ \
				bool dbc_temp_var_do_not_use; \
				dbc_temp_var_do_not_use = (exp) ? true : false; \
				CHECK( dbc_temp_var_do_not_use, #exp, POSTCONDITION); \
				if (!dbc_temp_var_do_not_use) \
				{ \
					goto EXCEPTION; \
				} \
			} while (0)
		  
	#else
	
		#define PRECONDITION(exp) \
			do \
			{ \
				if (!(exp)) \
				{ \
					goto EXCEPTION; \
				} \
			} while (0)
	
		#define MIDCONDITION(exp) \
			do \
			{ \
				if (!(exp)) \
				{ \
					goto EXCEPTION; \
				} \
			} while (0)
	
		#define MIDCONDITION_EX(exp,where) \
			do \
			{ \
				if (!(exp)) \
				{ \
					goto EXCEPTION##where; \
				} \
			} while (0)
		  
		#define POSTCONDITION(exp) \
			do \
			{ \
				if (!(exp)) \
				{ \
					goto EXCEPTION; \
				} \
			} while (0)
	   
	#endif

#endif


// stops compilers from rejecting redundant variables/parameters
#define UNUSED(x) ((void)(x))


// conditionally free memory pointer
#define FREEIF(x) \
    do \
    { \
        if ((x)) \
        { \
            free((x)); \
        } \
        x = NULL; \
    } while (0)


// conditionally free file descriptor
#define CLOSEFDIF(fd) \
    do \
    { \
        if ((fd)) \
        { \
            close((fd)); \
        } \
        fd = NULL; \
    } while (0)


// conditionally free file stream
#define CLOSEFILEIF(f) \
    do \
    { \
        if ((f)) \
        { \
            fclose((f)); \
        } \
        f = NULL; \
    } while (0)


// release & null reference
#define RELEASE(o) \
    do \
    { \
        [o release]; \
        o = nil; \
    } while (0)


// conditionally release obj-c object
#define RELEASEIF(o) \
    do \
    { \
        if ((o)) \
        { \
            RELEASE(o); \
        } \
    } while (0)


#endif  // dbc_def__h

