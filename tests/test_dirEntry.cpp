#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../libraries/AS/include/AS/algorithms.h"
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h> // for linkat
#include <unistd.h> // for getcwd
#include <iostream>
#include <sys/stat.h> // for mkdir
#include "../libraries/AS/include/AS/File.h"
#include <ftw.h> // for nftw()
#include <stdio.h> // for perror
using std::cout;
using std::endl;
#include <chrono>
#include <ctime>

static char cwd[PATH_MAX];
static size_t length;

static int unlink_cb( const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf )
   {
    int rv = remove( fpath );
    
    if ( rv )
       perror( fpath );
    
    return rv;
   }

static int rmrf( char *path )
   {
    return nftw( path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS );
   }



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
    return !rmrf( buf );
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

// Implies that DIR *dir needs to be closed and reopened when
// a new file is created...
TEST( test_sCreateFile )
   {
   char filename[] = "temp.XXXXXX"; // For testing...
   APESEARCH::File file( mkstemp( filename ), O_EXCL, (mode_t)0600 );
   write( file.getFD(), "Ay ello\n", 8 );
   char dirPath[PATH_MAX];
   char buf[PATH_MAX];
   snprintf( dirPath, sizeof( dirPath ), "%s%s", cwd, "/tests/dirTest" );

   DIR *dir = opendir( dirPath );
   ASSERT_NOT_EQUAL( dir, nullptr );

   // Ensure that readdir reurns null
   struct dirent *dp;
   do 
    {
    dp = readdir( dir );
    }
   while( dp != NULL && ( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) );
   ASSERT_EQUAL( dp, nullptr );

   snprintf( buf, sizeof( buf ), "%s%s", dirPath, "/testing.txt" );
   ASSERT_EQUAL( link( filename, buf ), 0 );

   closedir( dir );
   dir = opendir( dirPath );

   do 
    {
    dp = readdir( dir );
    }
   while( dp != NULL && ( !strcmp( dp->d_name, "." ) || !strcmp( dp->d_name, ".." ) ) );

   ASSERT_NOT_EQUAL( dp, nullptr );
   ASSERT_EQUAL( strcmp( dp->d_name, "testing.txt" ), 0 );

   file = APESEARCH::File( mkstemp( filename ), O_EXCL, (mode_t)0600 );

   auto timeNow = std::chrono::system_clock::now();
   std::time_t nowLol = std::chrono::system_clock::to_time_t( timeNow );
   char timedName[1024];
   snprintf( timedName, sizeof( timedName ), "%s%s", "/testing_" ,std::ctime( &nowLol ) );
   
   APESEARCH::replace( timedName, timedName + strlen( timedName ), ' ', '-' );

   snprintf( buf, sizeof( buf ), "%s%s", dirPath, timedName );
   ASSERT_EQUAL( link( filename, buf ), 0 );

   closedir( dir );
   }




TEST_MAIN()