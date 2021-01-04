// Timeborne/InGame/Model/Terrain/FieldHeightQuadTree.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>

#include <Core/DataStructures/SimpleTypeUnorderedVector.hpp>
#include <EngineBuildingBlocks/Math/GLM.h>

class Terrain;

class FieldHeightQuadTree
{
public:

	struct Node
	{
		unsigned Parent;
		unsigned Children[4];
		float MinHeight, MaxHeight;
		bool IsDirty;

		void SerializeSB(Core::ByteVector& bytes) const;
		void DeserializeSB(const unsigned char*& bytes);
	};

private:

	const Terrain* m_Terrain;
	Core::SimpleTypeUnorderedVectorU<Node> m_Nodes;

	void CreateChildNodes(unsigned nodeIndex, glm::ivec2& start, glm::ivec2& end);
	unsigned PrepareLeaf(int x, int z);
	unsigned GetNode(int x, int z, glm::ivec2* pStart, glm::ivec2* pEnd);
	void ShrinkNodes();

public:

	FieldHeightQuadTree(const Terrain* terrain);

	Node& GetRootNode();

	void Update(const glm::ivec2& changeStart, const glm::ivec2& changeEnd);

	// Returns the smallest nonnegative ray parameter t, for which the ray is UNDER the surface.
	// Note that for rays which starts from below the surface the result is (approximately) zero.
	// Note that an intersection with the "outside terrain" is also considered.
	float IntersectRay(const glm::vec3& rayOrigin, const glm::vec3& rayDir) const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes);
};

struct TerrainBlockIntersection
{
	bool HasIntersection;
	glm::ivec2 Start, End;
	unsigned CornerIndex;
};

TerrainBlockIntersection GetTerrainBlockIntersection(
	EngineBuildingBlocks::Graphics::Camera* camera, const glm::uvec2& windowsSize,
	const glm::uvec2& contentSize,
	const Terrain& terrain, const FieldHeightQuadTree* quadTree, const glm::ivec2& blockSize,
	const glm::vec2& cursorPositionInScreen);