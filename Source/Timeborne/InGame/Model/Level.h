// Timeborne/InGame/Model/Level.h

#pragma once

#include <Timeborne/Declarations/EngineBuildingBlocksDeclarations.h>
#include <Timeborne/InGame/Model/GameObjects/GameObject.h>
#include <Timeborne/InGame/Model/Terrain/Terrain.h>
#include <Timeborne/InGame/Model/Terrain/FieldHeightQuadTree.h>

#include <memory>
#include <string>

class TerrainTree;
class MainApplication;

struct LevelSetupData
{
	std::string Name;
	glm::uvec2 Size;
};

class Level
{
	std::string m_Name;

	Terrain m_Terrain;

	// @todo: this will be replaced with the terrain tree.
	FieldHeightQuadTree m_FieldHeightQuadtree;

	std::unique_ptr<TerrainTree> m_TerrainTree;

	Core::SimpleTypeUnorderedVectorU<GameObjectLevelData> m_GameObjects;

public:

	Level();
	explicit Level(const LevelSetupData& data);
	~Level();

	const std::string& GetName() const;
	const glm::uvec2 GetCountFields() const;

	const Terrain& GetTerrain() const;
	Terrain& GetTerrain();
	FieldHeightQuadTree& GetFieldHeightQuadtree();
	const FieldHeightQuadTree& GetFieldHeightQuadtree() const;
	TerrainTree* GetTerrainTree();
	const TerrainTree* GetTerrainTree() const;
	Core::SimpleTypeUnorderedVectorU<GameObjectLevelData>& GetGameObjects();
	const Core::SimpleTypeUnorderedVectorU<GameObjectLevelData>& GetGameObjects() const;

	void CreateTerrainTree(const MainApplication* application);

	void Load(const EngineBuildingBlocks::PathHandler& pathHandler, const std::string& fileName,
		bool forceRecomputations);
	void Save(const EngineBuildingBlocks::PathHandler& pathHandler, const std::string& fileName) const;

	void SerializeSB(Core::ByteVector& bytes) const;
	void DeserializeSB(const unsigned char*& bytes, bool forceRecomputations);

	static std::string GetPath(const EngineBuildingBlocks::PathHandler& pathHandler,
		const std::string& levelName);
};