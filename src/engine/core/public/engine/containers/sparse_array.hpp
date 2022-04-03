#pragma once

#include "engine/debug/assertions.hpp"
#include <boost/dynamic_bitset.hpp>

namespace ze
{

template<typename T>
class SparseArray
{
public:
	template<typename U>
	class Iterator
	{
	public:
		using ArrayType = std::conditional_t<std::is_const_v<U>, const SparseArray<T>&, SparseArray<T>&>;
		using iterator_category = std::random_access_iterator_tag;
		using value_type = U;
		using difference_type = std::ptrdiff_t;
		using pointer = U*;
		using reference = U&;

		Iterator(ArrayType in_array,
			const size_t& in_current_idx) :
			current_idx(in_current_idx),
			array(in_array) {}

		U& operator*()
		{
			return array[current_idx];
		}

		const U& operator*() const
		{
			return array[current_idx];
		}

		Iterator& operator++()
		{
			while (++current_idx < array.get_capacity())
			{
				if (array.allocated_indices[current_idx])
					return *this;
			}

			return *this;
		}

		Iterator& operator=(const Iterator& other)
		{
			array = other.array;
			current_idx = other.current_idx;
			return *this;
		}

		Iterator& operator-(difference_type in_diff)
		{
			current_idx -= in_diff;
			ZE_CHECK(current_idx > 0 && current_idx <= array.get_capacity() - 1);
			return *this;
		}

		friend difference_type operator-(const Iterator& left, const Iterator& right)
		{
			return right - left;
		}

		friend bool operator==(const Iterator& left, const Iterator& right)
		{
			return left.current_idx == right.current_idx;
		}

		friend bool operator!=(const Iterator& left, const Iterator& right)
		{
			return left.current_idx != right.current_idx;
		}
	private:
		size_t current_idx;
		ArrayType array;
	};

	using ElementType = T;
	using IteratorType = Iterator<T>;
	using ConstIteratorType = const Iterator<const T>;

	SparseArray() : elements(nullptr), size(0), capacity(0) {}
	~SparseArray()
	{
		if (elements)
		{
			for (size_t i = 0; i < capacity; ++i)
			{
				if (is_valid(i))
					remove(i);
			}

			std::free(elements);
		}
	}

	template<typename... Args>
	[[nodiscard]] size_t emplace(Args&&... in_args)
	{
		size_t index = get_free_index();
		ZE_CHECKF(!is_valid(index), "Index {} already contains a element!", index);

		new (elements + index) T(std::forward<Args>(in_args)...);
		allocated_indices[index] = true;
		size++;

		return index;
	}

	void remove(size_t in_index)
	{
		elements[in_index].~T();
		allocated_indices[in_index] = false;
		size--;
	}

	IteratorType begin()
	{
		return IteratorType(*this, 0);
	}

	ConstIteratorType begin() const
	{
		return cbegin();
	}

	ConstIteratorType cbegin() const
	{
		return ConstIteratorType(*this, 0);
	}

	IteratorType end()
	{
		return IteratorType(*this, size);
	}

	ConstIteratorType end() const
	{
		return cend();
	}

	ConstIteratorType cend() const
	{
		return ConstIteratorType(*this, size);
	}

	ElementType& operator[](const size_t& in_index)
	{
		return at(in_index);
	}

	const ElementType& operator[](const size_t& in_index) const
	{
		return at(in_index);
	}

	[[nodiscard]] bool is_valid(const size_t& in_index) const
	{
		return in_index < capacity && allocated_indices[in_index];
	}

	[[nodiscard]] bool is_empty() const
	{
		return allocated_indices.none();
	}

	[[nodiscard]] auto get_size() const { return size;  }
	[[nodiscard]] auto get_capacity() const { return capacity;  }
private:
	[[nodiscard]] T& at(const size_t& in_index)
	{
		ZE_ASSERT(is_valid(in_index));
		return elements[in_index];
	}

	[[nodiscard]] const T& at(const size_t& in_index) const
	{
		ZE_ASSERT(is_valid(in_index));
		return elements[in_index];
	}

	[[nodiscard]] size_t get_free_index()
	{
		for (size_t i = 0; i < capacity; ++i)
			if (!allocated_indices[i])
				return i;

		realloc(capacity + 1);
		return capacity - 1;
	}

	void realloc(size_t new_capacity)
	{
		if (elements)
		{
			T* old_elements = elements;
			elements = static_cast<T*>(std::malloc(new_capacity * sizeof(T)));
			for (size_t i = 0; i < capacity; ++i)
			{
				if (is_valid(i))
				{
					if constexpr (std::is_move_constructible_v<T>)
						new (elements + i) T(std::move(old_elements[i]));
					else
						elements[i] = std::move(old_elements[i]);

					old_elements[i].~T();
				}
			}

			std::free(old_elements);
		}
		else
		{
			elements = static_cast<T*>(std::malloc(new_capacity * sizeof(T)));
		}

		ZE_CHECK(elements);
		capacity = new_capacity;
		allocated_indices.resize(new_capacity);
	}
private:
	T* elements;
	size_t size;
	size_t capacity;
	boost::dynamic_bitset<> allocated_indices;
};

}