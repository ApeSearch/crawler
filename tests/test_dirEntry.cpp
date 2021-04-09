#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h> // for getcwd
#include <iostream>
#include <sys/stat.h> // for mkdir
using std::cout;
using std::endl;

static char cwd[PATH_MAX];
static size_t length;


static void initDirectory( const char *directory )
    {
    DIR *dir;
    getcwd( cwd, PATH_MAX );
    char buf[PATH_MAX];
    snprintf( buf, sizeof( buf ), "%s%s", cwd, directory );
    if ( ( dir = opendir( buf ) ) == NULL )
        ASSERT_EQUAL( mkdir( buf, 0700 ), 0 );
    } 

static bool removeDirectory( const char *directory )
   {
    char buf[PATH_MAX];
    snprintf( buf, sizeof( buf ), "%s%s", cwd, directory );
    return !remove( buf );
   }

TEST( test_initDelDir )
    {
    initDirectory( "/tests/dirTest" );
    length = strlen( cwd );
    ASSERT_TRUE( removeDirectory( "/tests/dirTest" ) ); 
    }

TEST( test_justInitDir )
   {
     initDirectory( "/tests/dirTest" );
   }

TEST( test_sCreateFile )
   {
   cout << "Ay ello" << endl;
   }

TEST_MAIN()