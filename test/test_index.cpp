#include <iostream>

#include "index.h"
#include "decompress.h"
#include "gtest/gtest.h"

using namespace std;


TEST(FindQueryStartByte, Test1) {
    map<string, map<string, tuple<string, string>>> index_map = {
            {"1", {{"100", {"0", "0"}}}},
            {"2", {{"200", {"123", "1"}}}},
            {"3", {{"300", {"456", "2"}}}}
    };
    tuple<int, int> start_byte_block_1 = find_query_start_byte(1, 50, index_map);
    tuple<int, int> start_byte_block_2 = find_query_start_byte(1, 100, index_map);
    tuple<int, int> start_byte_block_3 = find_query_start_byte(2, 200, index_map);
    tuple<int, int> start_byte_block_4 = find_query_start_byte(2, 250, index_map);
    tuple<int, int> start_byte_block_5 = find_query_start_byte(3, 300, index_map);
    tuple<int, int> start_byte_block_6 = find_query_start_byte(3, 350, index_map);

    ASSERT_EQ(get<0>(start_byte_block_1), -1);
    ASSERT_EQ(get<1>(start_byte_block_1), -1);
    ASSERT_EQ(get<0>(start_byte_block_2), 0);
    ASSERT_EQ(get<1>(start_byte_block_2), 0);
    ASSERT_EQ(get<0>(start_byte_block_3), 123);
    ASSERT_EQ(get<1>(start_byte_block_3), 1);
    ASSERT_EQ(get<0>(start_byte_block_4), 123);
    ASSERT_EQ(get<1>(start_byte_block_4), 1);
    ASSERT_EQ(get<0>(start_byte_block_5), 456);
    ASSERT_EQ(get<1>(start_byte_block_5), 2);

}


