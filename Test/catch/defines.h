#ifndef __MACRO_H_
#define __MACRO_H_
#include "catch/catch.hpp"

#define EXPECT_EQ(A, B) REQUIRE((A) == (B))
#define EXPECT_NE(A, B) REQUIRE((A) != (B))
#define EXPECT_LT(A, B) REQUIRE((A) < (B))
#define EXPECT_GT(A, B) REQUIRE((A) > (B))
#define EXPECT_GE(A, B) REQUIRE((A) >= (B))
#define EXPECT_LE(A, B) REQUIRE((A) <= (B))
#define EXPECT_TRUE(A) REQUIRE((A) == true)
#define EXPECT_FALSE(A) REQUIRE((A) == false)

const bool DEBUG = true;



#endif