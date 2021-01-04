// Timeborne/LevelEditor/Terrain/TerrainFieldBlockHeightIterator.h

#pragma once

#include <Timeborne/InGame/Model/Terrain/Terrain.h>

class TerrainFieldBlockHeightIterator
{
	const Core::SimpleTypeVectorU<glm::ivec2>* m_FieldIndices;
	glm::uvec2 m_CountFields;
	FieldData* m_Fields;

	glm::uvec2 m_Current;

public:

	TerrainFieldBlockHeightIterator(const Core::SimpleTypeVectorU<glm::ivec2>* fieldIndices, const glm::uvec2& countFields,
		FieldData* fields)
		: m_FieldIndices(fieldIndices)
		, m_CountFields(countFields)
		, m_Fields(fields)
		, m_Current(0, 0)
	{
	}

	TerrainFieldBlockHeightIterator& operator++()
	{
		if (m_Current.y < 3) ++m_Current.y;
		else
		{
			++m_Current.x;
			m_Current.y = 0;
		}
		return *this;
	}

	operator bool() const
	{
		return (m_Current.x < m_FieldIndices->GetSize());
	}

	float& operator*()
	{
		auto& fieldIndex = (*m_FieldIndices)[m_Current.x];
		return m_Fields[fieldIndex.y * m_CountFields.x + fieldIndex.x].Heights[m_Current.y];
	}
};