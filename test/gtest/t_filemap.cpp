#include <gtest/gtest.h>
#include "filemap.h"

TEST( fileemap, add_block ) {
    FileMap fileMap;

    ASSERT_EQ( fileMap.addBlock( 0, 5, "first" ), false );
    ASSERT_EQ( fileMap.addBlock( 6, 10, "second" ), false );
    ASSERT_EQ( fileMap.addBlock( 3, 7, "third" ), true );
}