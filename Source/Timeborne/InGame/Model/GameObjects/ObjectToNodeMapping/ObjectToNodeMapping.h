// Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/ObjectToNodeMapping.h

#pragma once

#include <Core/SingleElementPoolAllocator.hpp>
#include <Core/DataStructures/SimpleTypeVector.hpp>

#include <Timeborne/InGame/Model/GameObjects/GameObjectId.h>

#include <cassert>
#include <utility>
#include <vector>

class TerrainTree;

template <typename TKey, typename TData>
class ObjectToNodeMapping_IndexMapping
{
	std::vector<Core::SimpleTypeVectorU<TData>> m_Mappings;
	Core::IndexVectorU m_UnusedVectors;
	Core::FastStdMap<TKey, unsigned> m_VectorIndices;

public:

	void Add(TKey key, TData value)
	{
		auto sIt = m_VectorIndices.find(key);
		if (sIt == m_VectorIndices.end())
		{
			unsigned mappingIndex;
			if (m_UnusedVectors.IsEmpty())
			{
				mappingIndex = (unsigned)m_Mappings.size();
				m_Mappings.emplace_back();
			}
			else
			{
				mappingIndex = m_UnusedVectors.PopBackReturn();
			}
			sIt = m_VectorIndices.insert(std::make_pair(key, mappingIndex)).first;
		}
		m_Mappings[sIt->second].PushBack(value);
	}

	void Remove(TKey key, TData value)
	{
		auto mappingIndex = m_VectorIndices[key];
		auto& values = m_Mappings[mappingIndex];
		auto countValues = values.GetSize();
		for (unsigned i = 0; i < countValues; i++)
		{
			if (values[i] == value)
			{
				values.RemoveWithLastElementCopy(i);
				break;
			}
		}
		if (values.IsEmpty())
		{
			m_UnusedVectors.PushBack(mappingIndex);
			m_VectorIndices.erase(key);
		}
	}

	void Remove(TKey key)
	{
		auto mappingIndex = m_VectorIndices[key];
		m_Mappings[mappingIndex].Clear();
		m_UnusedVectors.PushBack(mappingIndex);
		m_VectorIndices.erase(key);
	}

	void RemoveMappings(TKey key)
	{
		auto mappingIndex = m_VectorIndices[key];
		m_Mappings[mappingIndex].Clear();
	}

	const Core::SimpleTypeVectorU<TData>* GetValues(TKey key) const
	{
		auto sIt = m_VectorIndices.find(key);
		if (sIt == m_VectorIndices.end()) return nullptr;
		return &m_Mappings[sIt->second];
	}
};

template <typename TDerived>
class ObjectToNodeMapping
{
	ObjectToNodeMapping_IndexMapping<unsigned, GameObjectId> m_NodeToObjectsMapping;
	ObjectToNodeMapping_IndexMapping<GameObjectId, unsigned> m_ObjectToNodesMapping;

	TDerived& Crtp()
	{
		return static_cast<TDerived&>(*this);
	}

	void AddMappings(GameObjectId objectId)
	{
		auto countNodeIndices = m_CurrentNodeIndices.GetSize();
		for (unsigned i = 0; i < countNodeIndices; i++)
		{
			auto nodeIndex = m_CurrentNodeIndices[i];
			m_NodeToObjectsMapping.Add(nodeIndex, objectId);
			m_ObjectToNodesMapping.Add(objectId, nodeIndex);
		}
	}

	void RemoveNodeToObjectMappings(GameObjectId objectId)
	{
		auto nodeIndices = m_ObjectToNodesMapping.GetValues(objectId);
		assert(nodeIndices != nullptr);
		auto countNodes = nodeIndices->GetSize();
		for (unsigned i = 0; i < countNodes; i++)
		{
			m_NodeToObjectsMapping.Remove((*nodeIndices)[i], objectId);
		}
	}

protected:

	Core::IndexVectorU m_CurrentNodeIndices;

	const TerrainTree* m_TerrainTree = nullptr;

public:

	explicit ObjectToNodeMapping(const TerrainTree& terrainTree)
		: m_TerrainTree(&terrainTree)
	{
	}

	void SetTerrainTree_KeepingMappings(const TerrainTree& terrainTree)
	{
		m_TerrainTree = &terrainTree;
	}

	template <typename... TArgs>
	void AddObject(GameObjectId objectId, TArgs&&... args)
	{
		Crtp()._UpdateCurrentNodeIndices(objectId, std::forward<TArgs>(args)...);
		AddMappings(objectId);
	}

	template <typename... TArgs>
	void SetObject(GameObjectId objectId, TArgs&&... args)
	{
		Crtp()._UpdateCurrentNodeIndices(objectId, std::forward<TArgs>(args)...);

		auto oldNodeIndices = m_ObjectToNodesMapping.GetValues(objectId);
		assert(oldNodeIndices != nullptr);

		if (*oldNodeIndices != m_CurrentNodeIndices)
		{
			RemoveNodeToObjectMappings(objectId);
			m_ObjectToNodesMapping.RemoveMappings(objectId);
			AddMappings(objectId);
		}
	}

	void RemoveObject(GameObjectId objectId)
	{
		RemoveNodeToObjectMappings(objectId);
		m_ObjectToNodesMapping.Remove(objectId);
	}

	const Core::SimpleTypeVectorU<GameObjectId>* GetObjectsForNode(unsigned nodeIndex) const
	{
		return m_NodeToObjectsMapping.GetValues(nodeIndex);
	}

	const Core::IndexVectorU* GetNodesForObject(GameObjectId objectId) const
	{
		return m_ObjectToNodesMapping.GetValues(objectId);
	}

	template <typename TFilter>
	bool IsObjectColliding(GameObjectId objectId, bool assertExistingMapping, TFilter&& objectFilter) const
	{
		auto nodesPtr = GetNodesForObject(objectId);
		assert(!assertExistingMapping || nodesPtr != nullptr);
		if (nodesPtr != nullptr)
		{
			auto& nodes = *nodesPtr;
			auto countNodes = nodes.GetSize();
			for (unsigned i = 0; i < countNodes; i++)
			{
				auto objectIdsPtr = GetObjectsForNode(nodes[i]);
				if (objectIdsPtr != nullptr)
				{
					auto& objectIds = *objectIdsPtr;
					auto countObjects = objectIds.GetSize();
					if (countObjects > 1) // The same object is always contained. Early-continue in most cases.
					{
						for (unsigned j = 0; j < countObjects; j++)
						{
							auto otherObjectId = objectIds[j];
							if (otherObjectId != objectId && objectFilter(otherObjectId))
							{
								return true;
							}
						}
					}
				}
			}
		}
		return false;
	}

	bool IsObjectColliding(GameObjectId objectId, bool assertExistingMapping) const
	{
		return IsObjectColliding(objectId, assertExistingMapping, [](GameObjectId otherObjectId) { return true; });
	}
};