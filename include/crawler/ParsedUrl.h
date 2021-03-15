
#ifndef PARSEDURL_H_APESEARCH
#define PARSEDURL_H_APESEARCH

#pragma once
#include <string>
#include "../../libraries/AS/include/AS/utility.h"

class ParsedUrl
   {
   public:
      const char *CompleteUrl;
      char *Service, *Host, *Port, *Path;
      bool protocolType; // true == http, false == https
      char *getRequest;
      int reqSize;

      APESEARCH::pair<const char *, size_t> getReqStr() 
      {
         return APESEARCH::pair<const char *, size_t>( getRequest, static_cast<size_t> ( reqSize ) );
      } // end formRequest()


      ParsedUrl( const char *url )
         {
         // Assumes url points to static text but
         // does not check.

         CompleteUrl = url;

         pathBuffer = new char[ strlen( url ) + 1 ];
         const char *f;
         char *t;
         for ( t = pathBuffer, f = url; *t++ = *f++; )
            ;

         Service = pathBuffer;

         const char Colon = ':', Slash = '/';
         char *p;
         for ( p = pathBuffer; *p && *p != Colon; p++ )
            ;

         if ( *p )
            {
            // Mark the end of the Service.
            *p++ = 0;
            protocolType = strcmp(Service, "https");

            if ( *p == Slash )
               p++;
            if ( *p == Slash )
               p++;

            Host = p;

            for ( ; *p && *p != Slash && *p != Colon; p++ )
               ;

            if ( *p == Colon )
               {
               // Port specified.  Skip over the colon and
               // the port number.
               *p++ = 0;
               Port = +p;
               for ( ; *p && *p != Slash; p++ )
                  ;
               }
            else
               Port = p;

            if ( *p )
               // Mark the end of the Host and Port.
               *p++ = 0;

            // Whatever remains is the Path.
            Path = p;
            }
         else
            Host = Path = p;
         
         if ( ( reqSize = asprintf( &getRequest, "GET /%s HTTP/1.1\r\nHost: %s\r\n", Path, Host ) ) == -1 )
            {
            throw std::runtime_error(" asprintf failed. Could be due to lack of memory. Please investiagate further" );
            } // end if
         
         } // end if
         

      ~ParsedUrl( )
         {
         delete[ ] pathBuffer;
         free ( getRequest );
         }

   private:
      char *pathBuffer;
   };

#endif