// Timeborne/DataStructures/FastReusableResourceMap.h

#pragma once

#include <Core/SingleElementPoolAllocator.hpp>
#include <Core/DataStructures/ResourceUnorderedVector.hpp>

#include <cassert>

template <typename TKey, typename TData>
class FastReusableResourceMap
{
public:

	struct Element
	{
		TKey Key;
		TData Data;
	};

	using Container = Core::ResourceUnorderedVectorU<Element>;

private:

	Core::FastStdMap<TKey, unsigned> m_IndexMap;
	Container m_Elements;

	template <typename U, typename TElements>
	static inline U* _Get(const TKey& key, const Core::FastStdMap<TKey, unsigned>& indexMap, TElements& elements)
	{
		auto iIt = indexMap.find(key);
		return iIt != indexMap.end() ? &elements[iIt->second].Data : nullptr;
	}

public:

	TData& Add(const TKey& key)
	{
		unsigned index = m_Elements.AddNoReinit();
		m_IndexMap[key] = index;
		auto& element = m_Elements[index];
		element.Key = key;
		return element.Data;
	}

	void Remove(const TKey& key)
	{
		auto iIt = m_IndexMap.find(key);
		assert(iIt != m_IndexMap.end());
		unsigned index = iIt->second;
		m_Elements.RemoveNoReinit(index);
		m_IndexMap.erase(iIt);
	}

	void Clear()
	{
		auto eEnd = m_Elements.GetEndIterator();
		for (auto eIt = m_Elements.GetBeginIterator(); eIt != eEnd; ++eIt)
		{
			m_Elements.RemoveNoReinit(m_Elements.ToIndex(eIt));
		}
		m_IndexMap.clear();
	}

	bool HasKey(const TKey& key)
	{
		return m_IndexMap.find(key) != m_IndexMap.end();
	}

	TData* Get(const TKey& key)
	{
		return _Get<TData>(key, m_IndexMap, m_Elements);
	}

	const TData* Get(const TKey& key) const
	{
		return _Get<const TData>(key, m_IndexMap, m_Elements);
	}

	bool IsEmpty() const
	{
		return m_Elements.IsEmpty();
	}

	unsigned GetSize() const
	{
		return m_Elements.GetSize();
	}

	const Container& GetElements() const
	{
		return m_Elements;
	}
};