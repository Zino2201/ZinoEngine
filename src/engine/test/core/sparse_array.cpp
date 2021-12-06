#include "engine/core.hpp"
#include <gtest/gtest.h>
#include "engine/containers/sparse_array.hpp"
#include <robin_hood.h>

using namespace ze;

TEST(Core, SparseArray)
{
	/** POD tests */
	{
		SparseArray<int> pod_int;

		const size_t index_0 = pod_int.emplace(0);
		const size_t index_1 = pod_int.emplace(1);
		const size_t index_2 = pod_int.emplace(2);
		const size_t index_3 = pod_int.emplace(3);

		EXPECT_EQ(pod_int.is_valid(index_0), true);
		EXPECT_EQ(pod_int.is_valid(index_1), true);
		EXPECT_EQ(pod_int.is_valid(index_2), true);
		EXPECT_EQ(pod_int.is_valid(index_3), true);

		EXPECT_EQ(pod_int[index_0], 0);
		EXPECT_EQ(pod_int[index_1], 1);
		EXPECT_EQ(pod_int[index_2], 2);
		EXPECT_EQ(pod_int[index_3], 3);

		pod_int.remove(index_2);

		EXPECT_EQ(pod_int.is_valid(index_0), true);
		EXPECT_EQ(pod_int.is_valid(index_1), true);
		EXPECT_NE(pod_int.is_valid(index_2), true);
		EXPECT_EQ(pod_int.is_valid(index_3), true);

		const size_t index_2_new = pod_int.emplace(2);

		EXPECT_EQ(pod_int.is_valid(index_2_new), true);
		EXPECT_EQ(pod_int[index_2_new], 2);
	}
}
