#include <stdbool.h>
#include <string.h>

#define ORGANISATION "damco"

#include "dbc_def.h"

#define kTestString "This is a test"

static bool ALL_Success( const char* const qString) {
	bool ret = false;
	char* newString = NULL;
	
	PRECONDITION( qString);
	
	newString = strdup( kTestString);
	MIDCONDITION( newString);
	
	switch (ret) {
		case true:
			FAIL;
			break;
			
		case false:
			break;
	}
	
	ret = true;
	POSTCONDITION( ret);
	return ret;
	
EXCEPTION:
	return ret;
}

// =======================================================================


static bool PRECONDITION_Fail( const char* const qString) {
	bool ret = false;
	
	PRECONDITION( NULL);
	
	ret = true;
	return ret;
	
EXCEPTION:
	return ret;
}

// =======================================================================


static bool MIDCONDITION_Fail( const char* const qString) {
	bool ret = false;
	
	MIDCONDITION( NULL);
	
	ret = true;
	return ret;
	
EXCEPTION:
	return ret;
}

// =======================================================================


static bool POSTCONDITION_Fail( const char* const qString) {
	bool ret = false;
	
	POSTCONDITION( false);
	
	ret = true;
	return ret;
	
EXCEPTION:
	return ret;
}

// =======================================================================


static bool FAIL_Fail( const char* const qString) {
	bool ret = false;
	
	switch (ret) {
		default:
			FAIL;
			break;
	}
	
	ret = true;
	return ret;
	
EXCEPTION:
	return ret;
}

// =======================================================================


int main( int argc, char* argv[]) {
	int ret = -1;
	bool status;
	const char* somePtr = kTestString;
	
	status = ALL_Success( somePtr);
	MIDCONDITION( status);
	
	status = PRECONDITION_Fail( somePtr);
	MIDCONDITION( !status);
	
	status = MIDCONDITION_Fail( somePtr);
	MIDCONDITION( !status);

	status = POSTCONDITION_Fail( somePtr);
	MIDCONDITION( !status);
	
	status = FAIL_Fail( somePtr);
	MIDCONDITION( !status);

	printf( "DBC module is working correctly\n");
	ret = 0;
	return ret;

EXCEPTION:
	printf( "DBC module is defective\n");
	return ret;
}
