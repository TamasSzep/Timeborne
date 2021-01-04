// Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GroundObjectTerrainTreeNodeMapping.cpp

#include <Timeborne/InGame/Model/GameObjects/ObjectToNodeMapping/GroundObjectTerrainTreeNodeMapping.h>

#include <Timeborne/InGame/Model/GameObjects/Prototype/GameObjectPrototype.h>
#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>

constexpr const double c_SizeReduction = 2e-5;

GroundObjectTerrainTreeNodeMapping::GroundObjectTerrainTreeNodeMapping(const TerrainTree& terrainTree)
	: ObjectToNodeMapping(terrainTree)
{
}

void GroundObjectTerrainTreeNodeMapping::_UpdateCurrentNodeIndices(GameObjectId objectId,
	GameObjectTypeIndex typeIndex, const GameObjectPose& pose)
{
	auto& prototype = GameObjectPrototype::GetPrototypes()[(uint32_t)typeIndex];
	auto& movementPrototype = prototype->GetMovement();

	m_CurrentNodeIndices.Clear();

	double circleRadius = (double)movementPrototype.PositionTerrainNodeMappingCircleRadius;
	if (circleRadius > 0.0)
	{
		PushForCirle(circleRadius, pose);
	}
	else
	{
		PushForRectangle(movementPrototype.GameLogicSize, pose);
	}
}

void GroundObjectTerrainTreeNodeMapping::PushForCirle(double circleRadius, const GameObjectPose& pose)
{
	auto position2d = pose.GetPosition2d();

	circleRadius -= c_SizeReduction * 0.5;
	double circleRadiusSqr = circleRadius * circleRadius;

	auto startPosition = position2d;
	auto startFieldIndex = GameObjectPose::GetTerrainFieldIndex(startPosition);

	auto PushNodeIfContained = [this, &startFieldIndex, &position2d, circleRadius, circleRadiusSqr]
	(int xOffset, int zOffset) {
		auto fieldIndex = startFieldIndex + glm::ivec2(xOffset, zOffset);
		auto fieldMiddle = GameObjectPose::GetMiddle2dFromTerrainFieldIndex(fieldIndex);

		auto diff = position2d - fieldMiddle;
		auto absDiff = glm::abs(diff);

		double absDiffLimit = circleRadius + 0.5;
		if (absDiff.x > absDiffLimit || absDiff.y > absDiffLimit) return;

		constexpr double neighborAbsDiffLimit = 0.5;
		bool intersecting = (absDiff.x <= neighborAbsDiffLimit || absDiff.y <= neighborAbsDiffLimit);

		if (!intersecting) // Diagonal sectors.
		{
			glm::dvec2 fieldCornerOffsets[] = {
				glm::dvec2(-0.5, -0.5), glm::dvec2(0.5, -0.5), glm::dvec2(-0.5, 0.5), glm::dvec2(0.5, 0.5) };
			auto diffBase = fieldMiddle - position2d;
			for (int i = 0; i < 4; i++)
			{
				if (glm::length2(diffBase + fieldCornerOffsets[i]) <= circleRadiusSqr) { intersecting = true; break; }
			}
		}

		if (intersecting)
		{
			auto nodeIndex = m_TerrainTree->GetNodeIndexForField(fieldIndex);
			if (nodeIndex != Core::c_InvalidIndexU) m_CurrentNodeIndices.UnsafePushBack(nodeIndex);
		}
	};

	if (circleRadius < 0.5)
	{
		m_CurrentNodeIndices.Reserve(4);
		int xOffset = std::round(startPosition.x) < startPosition.x ? -1 : 1;
		int zOffset = std::round(startPosition.y) < startPosition.y ? -1 : 1;
		PushNodeIfContained(0, 0);
		PushNodeIfContained(xOffset, 0);
		PushNodeIfContained(0, zOffset);
		PushNodeIfContained(xOffset, zOffset);
	}
	else
	{
		// Not yet implemented.
		assert(false);
	}
}

void GroundObjectTerrainTreeNodeMapping::PushForRectangle(const glm::vec3& sizeF, const GameObjectPose& pose)
{
	auto position2d = pose.GetPosition2d();

	// Sampling with the 1x1 pattern. This will miss some intersections, but find the most of them.

	auto orientation = pose.GetGameOrientation2d();
	auto dir = glm::dvec2(orientation.Direction);
	auto right = glm::dvec2(orientation.Right);
	
	auto size = glm::dvec2(sizeF.x, sizeF.z) - c_SizeReduction;
	auto halfSize = size * 0.5;

	auto countSamples = (glm::ivec2)glm::floor(size) + 2;
	auto sampleLength = size / glm::dvec2(countSamples - 1);
	auto incrementX = dir * sampleLength.x;
	auto incrementZ = right * sampleLength.y;

	auto startPosition = position2d
		- glm::dvec2(orientation.Direction) * halfSize.x
		- glm::dvec2(orientation.Right) * halfSize.y;
	auto endOffsetX = glm::dvec2(orientation.Direction) * size.x;
	auto endOffsetZ = glm::dvec2(orientation.Right) * size.y;

	for (int i = 0; i < countSamples.x; i++)
	{
		auto currentPositionBase = startPosition + incrementX * (double)i;
		for (int j = 0; j < countSamples.y; j++)
		{
			auto currentPosition = currentPositionBase + incrementZ * (double)j;
			auto fieldIndex = GameObjectPose::GetTerrainFieldIndex(currentPosition);
			auto nodeIndex = m_TerrainTree->GetNodeIndexForField(fieldIndex);
			if (nodeIndex != Core::c_InvalidIndexU) m_CurrentNodeIndices.PushBack(nodeIndex);
		}
	}

	// Removing duplicates.
	m_CurrentNodeIndices.SortAndRemoveDuplicates();

	// Adding missing nodes. Given the criteria in MatchesSamplingCriteria, after sampling the object, we only have to
	// sample the 4 corners and the middle point of the field.
	glm::dvec2 offsets[] = {
		glm::dvec2(0.0, 0.0), glm::dvec2(0.0, 1.0), glm::dvec2(1.0, 0.0), glm::dvec2(1.0, 1.0), glm::dvec2(0.5, 0.5) };
	for (unsigned i = 0; i < m_CurrentNodeIndices.GetSize(); i++)
	{
		auto& currentNode = m_TerrainTree->GetNode(m_CurrentNodeIndices[i]);
		auto neighbors = (const unsigned*)currentNode.Neighbors;

		for (unsigned j = 0; j < 8; j++)
		{
			auto neighborNodeIndex = neighbors[j];
			if (neighborNodeIndex == Core::c_InvalidIndexU || m_CurrentNodeIndices.Contains(neighborNodeIndex)) continue;
			auto& neighborNode = m_TerrainTree->GetNode(neighborNodeIndex);
			glm::dvec2 startPos(neighborNode.Start);
			
			for (unsigned k = 0; k < 5; k++)
			{
				glm::dvec2 samplePosition = startPos + offsets[k];
				glm::dvec2 offsetToStart = samplePosition - startPosition;
				auto dotX = glm::dot(offsetToStart, dir);
				auto dotZ = glm::dot(offsetToStart, right);
				if (dotX >= 0.0 && dotX <= size.x && dotZ >= 0.0 && dotZ <= size.y)
				{
					m_CurrentNodeIndices.PushBack(neighborNodeIndex);
					break;
				}
			}
		}
	}
}

bool GroundObjectTerrainTreeNodeMapping::MatchesSamplingCriteria(const GameObjectPrototype& prototype)
{
	auto& movementPrototype = prototype.GetMovement();

	auto circleRadius = movementPrototype.PositionTerrainNodeMappingCircleRadius;
	if (circleRadius > 0.0) return (circleRadius - c_SizeReduction * 0.5 < 0.5);

	auto size = movementPrototype.GameLogicSize;
	auto minSize = std::min(size.x, size.z);
	auto maxSize = std::max(size.x, size.z);
	return (minSize > std::sqrt(0.5)) || (maxSize / minSize < 2.0);
}
