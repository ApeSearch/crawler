#include <iostream>
#include "../libraries/AS/include/AS/unique_mmap.h"


int main( int argc, char **argv )
   {
   if ( argc != 2 )
      {
      std::cerr << "Auguments aren't two" << std::endl;
      return 1;
      } // end if
   APESEARCH::File file( argv[ 1 ], O_RDWR, (mode_t)0600  );
   unique_mmap map( file.fileSize( ), PROT_READ, MAP_SHARED, file.getFD( ), 0 );

   char const * ptr = ( char * ) map.get( );
   char const * const end = ptr + file.fileSize( );

   while( ptr != end )
      {
      if ( !*ptr++ ) 
          {
          std::cerr << "Found null-character" << std::endl;
	  return 2;
          } // end if
      } // end while 
   } // end main( )
