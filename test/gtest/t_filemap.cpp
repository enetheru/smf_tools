#include <gtest/gtest.h>
#include "smflib/filemap.h"

TEST( fileemap, add_block ) {
    FileMap fileMap;

    EXPECT_EQ( fileMap.addBlock( 0, 5, "first" ), false );
    EXPECT_EQ( fileMap.addBlock( 6, 10, "second" ), false );
    EXPECT_EQ( fileMap.addBlock( 3, 7, "third" ), true );
}