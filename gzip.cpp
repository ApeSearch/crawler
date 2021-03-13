#include <zlib.h>
#include <array>
#include <vector>
#include <string.h>

// Perform the decompression
void DecompressResponse( std::vector < char > data_ )
    {
    z_stream zs;                        // z_stream is zlib's control structure
    memset( &zs, 0, sizeof( zs ) );

    std::vector < char > decompressed;
    std::array < char, 32768 > outBuffer;

    if ( inflateInit2( &zs, MAX_WBITS + 16 ) != Z_OK )
    throw std::runtime_error( "inflateInit2 fail" );

    zs.next_in =
        const_cast < Bytef * >( reinterpret_cast < const unsigned char * >( data_.data( ) ) );
    zs.avail_in = static_cast < unsigned int >( data_.size( ) );

    int ret;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do
        {
        zs.next_out = reinterpret_cast < Bytef* >( outBuffer.data( ) );
        zs.avail_out = sizeof( outBuffer );

        ret = inflate( &zs, 0 );

        if ( decompressed.size( ) < zs.total_out )
            {
            decompressed.insert( decompressed.end( ), outBuffer.data( ),
                    outBuffer.data( ) + zs.total_out - decompressed.size( ) );
            }
        }
    while ( ret == Z_OK );

    inflateEnd( &zs );

    if ( ret != Z_STREAM_END )
        {          // an error occurred that was not EOF
        throw std::runtime_error( "Non-EOF occurred while decompressing" );
        }

    std::swap( decompressed, data_ );
    }