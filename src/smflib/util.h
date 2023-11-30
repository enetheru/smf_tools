#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <iomanip>
#include <spdlog/spdlog.h>
#include <fmt/format.h>


/// Evaluates a string into two integers
/* The function takes a string in the form of IxK, and assigns
 * the values of I, K to x,y.
 */
std::pair< uint32_t, uint32_t > valxval( const std::string& s );

/// Expand a string sequence of integers to a vector
/*  The function takes a string in the form of comma ',' separated values.
 *  When two values are separated with a dash '-' then all the numbers
 *  inbetween are expanded.\n
 *  ie '1,2,3-7,2-5' = {1,2,3,4,5,6,7,2,3,4,5}
 */
std::vector< uint32_t > expandString( const std::string &source );

/// Convert integer to hex string.
/*  Takes an integer and outputs its hex value as a string with leading '0x'
 */
template< typename T >
std::string to_hex(T i)
{
  std::stringstream stream;
  stream << "0x"
         << std::setfill( '0' ) << std::setw( sizeof( T ) * 2 )
         << std::hex << i;
  return stream.str();
}



/// output a progress indicator
void progressBar( const std::string& message, float goal, float progress );

// Convert an image to ascii so we can get direct feedback from the app
/* FIXME does this need to use some sort of enumerator for  the type? probably
 * in fact homogenising the image types used in the lib is probably a good idea.
 *
 */
std::string
image_to_hex( const uint8_t *data, int width, int height, int type = 0 );

// Reworked from the example page on cppreference
// https://en.cppreference.com/w/cpp/filesystem/directory_entry/file_size
std::string humanise( std::uintmax_t size ){
    int i{};
    double mantissa = size;
    for (; mantissa >= 1024.0; mantissa /= 1024.0, ++i) { }
    std::string os = fmt::format( "{}{}", std::ceil(mantissa * 10.0) / 10.0, i["BKMGTPE"] );
    if( i ){
        fmt::format_to(std::back_inserter(os), "B ({})",size );
    }
    return os;
}

class Unimplemented : public std::logic_error {
    using std::logic_error::logic_error;
public:
    explicit Unimplemented(const char *msg) : std::logic_error(msg){}
};