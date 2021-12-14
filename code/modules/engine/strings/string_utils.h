#include <string>
#include <string_view>
#include <cstdint>

namespace rebel_road
{
    namespace util
    {
        // FNV-1a 32bit hashing algorithm.
        constexpr uint32_t fnv1a_32( char const* s, std::size_t count )
        {
            return ( ( count ? fnv1a_32( s, count - 1 ) : 2166136261u ) ^ s[count] ) * 16777619u;
        }

        constexpr size_t const_strlen( const char* s )
        {
            size_t size = 0;
            while ( s[size] ) { size++; };
            return size;
        }

        struct string_hash
        {
            uint32_t computed_hash;

            constexpr string_hash( uint32_t hash ) noexcept : computed_hash( hash ) {}

            constexpr string_hash( const char* s ) noexcept : computed_hash( 0 )
            {
                computed_hash = fnv1a_32( s, const_strlen( s ) );
            }
            constexpr string_hash( const char* s, std::size_t count ) noexcept : computed_hash( 0 )
            {
                computed_hash = fnv1a_32( s, count );
            }
            constexpr string_hash( std::string_view s ) noexcept : computed_hash( 0 )
            {
                computed_hash = fnv1a_32( s.data(), s.size() );
            }
            string_hash( const string_hash& other ) = default;

            constexpr operator uint32_t() noexcept { return computed_hash; }
        };
    }
}
