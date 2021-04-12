
#pragma once
#ifndef DYNAMICBUFF_H_AS
#define DYNAMICBUFF_H_AS

#include "../../libraries/AS/include/AS/circular_buffer.h"

namespace APESEARCH
   {
   
   template< typename T>
    class dynamicBuffer : public Buffer<T>
       {
       vector<T> buffer;
       typedef T value_type;
    public:
       // For default construct objects...
       dynamicBuffer( const size_t capacity ) : buffer( capacity )
          {
          assert( computeTwosPowCeiling( capacity ) == capacity );
          }

       dynamicBuffer( const size_t capacity, T&val ) : buffer( capacity, val )
          {
          assert( computeTwosPowCeiling( capacity ) == capacity );
          }
       inline value_type *getBuffer() noexcept
          {
          return &buffer.front();
          }
       inline void insert( const T& val, size_t index ) noexcept
          {
          buffer[ index ] = val;
          }
       inline void insert( size_t index, T&& val ) noexcept
          {
          buffer[ index ] = std::forward<T>( val );
          }
       inline virtual T& get( size_t index )
          {
          return buffer[ index ];
          }
       
       inline virtual size_t getCapacity() const
          {
          return buffer.size();
          }
       inline virtual T *begin() noexcept
          {
          return &buffer.front();
          }
       void print( std::ostream& os, const size_t , const size_t sizeOf ) const
          {
          return;
          }
       };
   } // end namespace
#endif