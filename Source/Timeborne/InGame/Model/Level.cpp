// Timeborne/InGame/Model/Level.cpp

#include <Timeborne/InGame/Model/Level.h>

#include <Timeborne/InGame/Model/Terrain/TerrainTree.h>

#include <Core/SimpleBinarySerialization.hpp>
#include <Core/System/SimpleIO.h>
#include <EngineBuildingBlocks/PathHandler.h>

using namespace EngineBuildingBlocks;

Level::Level()
	: m_FieldHeightQuadtree(&m_Terrain)
{
}

Level::Level(const LevelSetupData& data)
	: m_Name(data.Name)
	, m_Terrain(data.Size)
	, m_FieldHeightQuadtree(&m_Terrain)
{
}

Level::~Level()
{
}

const std::string& Level::GetName() const
{
	return m_Name;
}

const glm::uvec2 Level::GetCountFields() const
{
	return m_Terrain.GetCountFields();
}

const Terrain& Level::GetTerrain() const
{
	return m_Terrain;
}

Terrain& Level::GetTerrain()
{
	return m_Terrain;
}

FieldHeightQuadTree& Level::GetFieldHeightQuadtree()
{
	return m_FieldHeightQuadtree;
}

const FieldHeightQuadTree& Level::GetFieldHeightQuadtree() const
{
	return m_FieldHeightQuadtree;
}

TerrainTree* Level::GetTerrainTree()
{
	return m_TerrainTree.get();
}

const TerrainTree* Level::GetTerrainTree() const
{
	return m_TerrainTree.get();
}

Core::SimpleTypeUnorderedVectorU<GameObjectLevelData>& Level::GetGameObjects()
{
	return m_GameObjects;
}

const Core::SimpleTypeUnorderedVectorU<GameObjectLevelData>& Level::GetGameObjects() const
{
	return m_GameObjects;
}

std::string Level::GetPath(const PathHandler& pathHandler, const std::string& fileName)
{
	return pathHandler.GetPathFromResourcesDirectory("Levels/" + fileName + ".lvl");
}

void Level::Load(const PathHandler& pathHandler, const std::string& fileName, bool forceRecomputations)
{
	auto bytes = Core::ReadAllBytes(GetPath(pathHandler, fileName));
	auto byteArray = (const unsigned char*)bytes.GetArray();
	DeserializeSB(byteArray, forceRecomputations);
}

void Level::CreateTerrainTree(const MainApplication* application)
{
	m_TerrainTree = std::make_unique<TerrainTree>(m_Terrain, application);
}

void Level::Save(const PathHandler& pathHandler, const std::string& fileName) const
{
	Core::WriteAllBytes(GetPath(pathHandler, fileName), Core::StartSerializeSB(*this));
}

void Level::SerializeSB(Core::ByteVector& bytes) const
{
	// The terrain tree must be created.
	assert(m_TerrainTree != nullptr);

	Core::SerializeSB(bytes, m_Name);
	Core::SerializeSB(bytes, m_Terrain);
	Core::SerializeSB(bytes, m_FieldHeightQuadtree);
	Core::SerializeSB(bytes, *m_TerrainTree);

	// Converting to simple type vector.
	Core::SimpleTypeVectorU<GameObjectLevelData> gameObjects;
	m_GameObjects.ToSimpleTypeVector(gameObjects);
	Core::SerializeSB(bytes, gameObjects);
}

void Level::DeserializeSB(const unsigned char*& bytes, bool forceRecomputations)
{
	Core::DeserializeSB(bytes, m_Name);
	m_Terrain.DeserializeSB(bytes, forceRecomputations);

	// The field height quad tree must be deserialized, but we will overwrite it if the recomputations are forced.
	Core::DeserializeSB(bytes, m_FieldHeightQuadtree);

	if (forceRecomputations)
	{
		m_FieldHeightQuadtree = FieldHeightQuadTree(&m_Terrain);
	}

	auto terrainTree = new TerrainTree(m_Terrain);
	Core::DeserializeSB(bytes, *terrainTree);
	m_TerrainTree.reset(terrainTree);

	// Converting from simple type vector.
	Core::SimpleTypeVectorU<GameObjectLevelData> gameObjects;
	Core::DeserializeSB(bytes, gameObjects);
	m_GameObjects.Clear();
	for (unsigned i = 0; i < gameObjects.GetSize(); i++)
	{
		m_GameObjects.Add(gameObjects[i]);
	}
}
