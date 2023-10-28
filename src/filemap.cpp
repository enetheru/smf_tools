#include "filemap.h"

void FileMap::addBlock( uint32_t begin, uint32_t size, const std::string& name )
{
    SPDLOG_DEBUG( "adding: {}({})({}-{})", name, begin, begin + size-1);
    Block temp{ begin, begin + size-1, name };
    for( const auto& i : list ){
        if( (temp.begin >= i.begin && temp.begin <= i.end)
            || (temp.end >= i.begin && temp.end <= i.end)
            || (temp.begin <= i.begin && temp.end >= i.end) ){
            SPDLOG_ERROR( "'{}' clashes with existing block '{}'", temp.name, i.name );
        }
    }
    list.push_back( temp );
}