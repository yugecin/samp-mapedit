/* vim: set filetype=c ts=8 noexpandtab: */

#define STATIC_ASSERT(E) typedef char __static_assert_[(E)?1:-1]
#define EXPECT_SIZE(S,SIZE) STATIC_ASSERT(sizeof(S)==(SIZE))

EXPECT_SIZE(char, 1);
EXPECT_SIZE(short, 2);
EXPECT_SIZE(int, 4);
