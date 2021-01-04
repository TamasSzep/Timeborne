// Timeborne/InGame/Model/Terrain/Terrain.cpp

#include <Timeborne/InGame/Model/Terrain/Terrain.h>

#include <Timeborne/InGame/Model/Terrain/TerrainCommon.h>

#include <Core/SimpleBinarySerialization.hpp>

Terrain::Terrain()
	: m_CountFields(0)
{
}

Terrain::Terrain(const glm::uvec2& countFields)
	: m_CountFields(countFields)
{
	assert(countFields.x > 0 && countFields.y > 0);

	unsigned fieldValueCount = countFields.x * countFields.y;

	m_Fields.PushBack(FieldData{}, fieldValueCount);

	CreateSurfaceData();
}

void Terrain::CreateSurfaceData()
{
	auto countFields1d = m_CountFields.x * m_CountFields.y;
	m_SurfaceCoefficients.Resize(countFields1d);
	m_SurfaceCoefficients.SetByte(0);
	m_DX.Resize(countFields1d);
	m_DY.Resize(countFields1d);
	m_DXY.Resize(countFields1d);
	m_DX.SetByte(0);
	m_DY.SetByte(0);
	m_DXY.SetByte(0);
}

const glm::uvec2 Terrain::GetCountFields() const
{
	return m_CountFields;
}

const glm::uvec2 Terrain::GetCountCornerHeights() const
{
	return m_CountFields + glm::uvec2(1);
}

glm::vec2 Terrain::GetWorldSizeXZ() const
{
	return glm::vec2(m_CountFields);
}

FieldData* Terrain::GetFields()
{
	return m_Fields.GetArray();
}

const FieldData* Terrain::GetFields() const
{
	return m_Fields.GetArray();
}

float Terrain::GetHeight(const glm::ivec2& fieldIndex, float xInField, float zInField) const
{
	return EvaluateBicubicFunctionValue(
		m_SurfaceCoefficients[fieldIndex.y * m_CountFields.x + fieldIndex.x], xInField, zInField);
}

float Terrain::GetMiddleHeight(const glm::ivec2& fieldIndex) const
{
	return GetHeight(fieldIndex, 0.5f, 0.5f);
}

glm::vec3 Terrain::GetPosition(const glm::ivec2& fieldIndex, float xInField, float zInField) const
{
	return { (fieldIndex.x + xInField), GetHeight(fieldIndex, xInField, zInField), (fieldIndex.y + zInField) };
}

glm::vec3 Terrain::GetMiddlePosition(const glm::ivec2& fieldIndex) const
{
	return { (fieldIndex.x + 0.5f), GetMiddleHeight(fieldIndex), (fieldIndex.y + 0.5f) };
}

glm::vec3 Terrain::GetNormal(const glm::ivec2& fieldIndex, float xInField, float zInField) const
{
	auto& coeffs = m_SurfaceCoefficients[fieldIndex.y * m_CountFields.x + fieldIndex.x];
	auto dxm = GetBicubicFunctionDXMatrix(coeffs);
	auto dzm = GetBicubicFunctionDZMatrix(coeffs);
	return glm::normalize(glm::vec3(-EvaluateBicubicFunctionDerivativeX(dxm, xInField, zInField), 1.0f,
		-EvaluateBicubicFunctionDerivativeZ(dzm, xInField, zInField)));
}

const Core::SimpleTypeVectorU<glm::mat4>& Terrain::GetSurfaceCoefficients() const
{
	return m_SurfaceCoefficients;
}

const float* Terrain::GetFieldHeights(const FieldData* fields, const glm::uvec2& countFields, int x, int z)
{
	return glm::value_ptr(fields[z * countFields.x + x].Heights);
}

glm::vec2 Terrain::GetFieldHeightMinMax(const FieldData* fields, const glm::uvec2& countFields, int x, int z)
{
	float heights[] = {
		GetFieldHeight(fields, countFields, x, z, 0),
		GetFieldHeight(fields, countFields, x, z, 1),
		GetFieldHeight(fields, countFields, x, z, 2),
		GetFieldHeight(fields, countFields, x, z, 3)
	};
	std::sort(heights, heights + 4);
	return { heights[0], heights[3] };
}

float Terrain::GetFieldHeight(const FieldData* fields, const glm::uvec2& countFields, int x, int z, int fId)
{
	return fields[z * countFields.x + x].Heights[fId];
}

void Terrain::SerializeSB(Core::ByteVector& bytes) const
{
	Core::SerializeSB(bytes, Core::ToPlaceHolder(m_CountFields));
	Core::SerializeSB(bytes, m_Fields);
	
	// Optionally read data has to be serialized at the end.
	Core::SerializeSB(bytes, m_DX);
	Core::SerializeSB(bytes, m_DY);
	Core::SerializeSB(bytes, m_DXY);
	Core::SerializeSB(bytes, m_SurfaceCoefficients);
}

void Terrain::DeserializeSB(const unsigned char*& bytes, bool forceRecomputations)
{
	Core::DeserializeSB(bytes, Core::ToPlaceHolder(m_CountFields));
	Core::DeserializeSB(bytes, m_Fields);

	// All data must be deserialized, but they will be overwritten if the recomputations are forced.
	Core::DeserializeSB(bytes, m_DX);
	Core::DeserializeSB(bytes, m_DY);
	Core::DeserializeSB(bytes, m_DXY);
	Core::DeserializeSB(bytes, m_SurfaceCoefficients);

	if (forceRecomputations)
	{
		CreateSurfaceData();
		UpdateSurfaceData(glm::ivec2(0, 0), glm::ivec2(m_CountFields.x - 1, m_CountFields.y - 1));
	}
}

void Terrain::GetFlippedLimits(const glm::ivec2& start, const glm::ivec2& end, glm::ivec2* pStart, glm::ivec2* pEnd)
{
	if (start.x <= end.x) { pStart->x = start.x; pEnd->x = end.x; }
	else { pStart->x = end.x; pEnd->x = start.x; }
	if (start.y <= end.y) { pStart->y = start.y; pEnd->y = end.y; }
	else { pStart->y = end.y; pEnd->y = start.y; }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float GetFieldHeight(const FieldData* fields, const glm::uvec2& countFields, int x, int z, int fId)
{
	if (x < 0 || x >= (int)countFields.x || z < 0 || z >= (int)countFields.y) return 0.0f;
	return Terrain::GetFieldHeight(fields, countFields, x, z, fId);
}

inline void GetFieldHeights(const FieldData* fields, const glm::uvec2& countFields, int x, int z, float* pY)
{
	for (int c = 0; c < 4; c++) pY[c] = GetFieldHeight(fields, countFields, x, z, c);
}

inline void UpdateDerivatives(FieldData* fields, const glm::uvec2& countFields, int x, int z,
	glm::vec4* dxs, glm::vec4* dys, glm::vec4* dxys)
{
	float f[5][4];
	auto index = z * countFields.x + x;
	auto& dx = dxs[index];
	auto& dy = dys[index];
	auto& dxy = dxys[index];

	float derivativeScalerL = 0.5f;

	GetFieldHeights(fields, countFields, x, z, f[0]);
	GetFieldHeights(fields, countFields, x, z + 1, f[1]);
	GetFieldHeights(fields, countFields, x + 1, z, f[2]);
	GetFieldHeights(fields, countFields, x, z - 1, f[3]);
	GetFieldHeights(fields, countFields, x - 1, z, f[4]);

	if (f[0][0] == f[4][1]) dx[0] = (f[0][1] - f[4][0]) * derivativeScalerL;
	else dx[0] = (f[0][1] - f[0][0]);
	if (f[0][0] == f[1][3]) dy[0] = (f[1][0] - f[0][3]) * derivativeScalerL;
	else dy[0] = (f[0][0] - f[0][3]);

	if (f[0][1] == f[2][0]) dx[1] = (f[2][1] - f[0][0]) * derivativeScalerL;
	else dx[1] = (f[0][1] - f[0][0]);
	if (f[0][1] == f[1][2]) dy[1] = (f[1][1] - f[0][2]) * derivativeScalerL;
	else dy[1] = (f[0][1] - f[0][2]);

	if (f[0][2] == f[2][3]) dx[2] = (f[2][2] - f[0][3]) * derivativeScalerL;
	else dx[2] = (f[0][2] - f[0][3]);
	if (f[0][2] == f[3][1]) dy[2] = (f[0][1] - f[3][2]) * derivativeScalerL;
	else dy[2] = (f[0][1] - f[0][2]);

	if (f[0][3] == f[4][2]) dx[3] = (f[0][2] - f[4][3]) * derivativeScalerL;
	else dx[3] = (f[0][2] - f[0][3]);
	if (f[0][3] == f[3][0]) dy[3] = (f[0][0] - f[3][3]) * derivativeScalerL;
	else dy[3] = (f[0][0] - f[0][3]);

	dxy[0] = (dx[0] - dx[3] + dy[1] - dy[0]) * derivativeScalerL;
	dxy[1] = (dx[1] - dx[2] + dy[1] - dy[0]) * derivativeScalerL;
	dxy[2] = (dx[1] - dx[2] + dy[2] - dy[3]) * derivativeScalerL;
	dxy[3] = (dx[0] - dx[3] + dy[2] - dy[3]) * derivativeScalerL;
}

inline float GetDv(const glm::vec4* dvs, const glm::uvec2& countFields, int x, int z, int fId)
{
	if (x < 0 || x >= (int)countFields.x || z < 0 || z >= (int)countFields.y) return 0.0f;
	return dvs[z * countFields.x + x][fId];
}

inline void GetFieldDvs(const glm::vec4* dvs, const glm::uvec2& countFields, int x, int z, float* pDv)
{
	for (int c = 0; c < 4; c++) pDv[c] = GetDv(dvs, countFields, x, z, c);
}

inline glm::mat4 GetCoeffMatrix(FieldData* fields, const glm::uvec2& countFields, int x, int z,
	const glm::vec4* dxs, const glm::vec4* dys, const glm::vec4* dxys)
{
	float f[9][4], dxv[9][4], dyv[9][4], dxyv[9][4];

	GetFieldHeights(fields, countFields, x, z, f[0]);
	GetFieldHeights(fields, countFields, x, z + 1, f[1]);
	GetFieldHeights(fields, countFields, x + 1, z, f[2]);
	GetFieldHeights(fields, countFields, x, z - 1, f[3]);
	GetFieldHeights(fields, countFields, x - 1, z, f[4]);
	GetFieldHeights(fields, countFields, x - 1, z + 1, f[5]);
	GetFieldHeights(fields, countFields, x + 1, z + 1, f[6]);
	GetFieldHeights(fields, countFields, x + 1, z - 1, f[7]);
	GetFieldHeights(fields, countFields, x - 1, z - 1, f[8]);

	GetFieldDvs(dxs, countFields, x, z, dxv[0]);
	GetFieldDvs(dxs, countFields, x, z + 1, dxv[1]);
	GetFieldDvs(dxs, countFields, x + 1, z, dxv[2]);
	GetFieldDvs(dxs, countFields, x, z - 1, dxv[3]);
	GetFieldDvs(dxs, countFields, x - 1, z, dxv[4]);
	GetFieldDvs(dxs, countFields, x - 1, z + 1, dxv[5]);
	GetFieldDvs(dxs, countFields, x + 1, z + 1, dxv[6]);
	GetFieldDvs(dxs, countFields, x + 1, z - 1, dxv[7]);
	GetFieldDvs(dxs, countFields, x - 1, z - 1, dxv[8]);

	GetFieldDvs(dys, countFields, x, z, dyv[0]);
	GetFieldDvs(dys, countFields, x, z + 1, dyv[1]);
	GetFieldDvs(dys, countFields, x + 1, z, dyv[2]);
	GetFieldDvs(dys, countFields, x, z - 1, dyv[3]);
	GetFieldDvs(dys, countFields, x - 1, z, dyv[4]);
	GetFieldDvs(dys, countFields, x - 1, z + 1, dyv[5]);
	GetFieldDvs(dys, countFields, x + 1, z + 1, dyv[6]);
	GetFieldDvs(dys, countFields, x + 1, z - 1, dyv[7]);
	GetFieldDvs(dys, countFields, x - 1, z - 1, dyv[8]);

	GetFieldDvs(dxys, countFields, x, z, dxyv[0]);
	GetFieldDvs(dxys, countFields, x, z + 1, dxyv[1]);
	GetFieldDvs(dxys, countFields, x + 1, z, dxyv[2]);
	GetFieldDvs(dxys, countFields, x, z - 1, dxyv[3]);
	GetFieldDvs(dxys, countFields, x - 1, z, dxyv[4]);
	GetFieldDvs(dxys, countFields, x - 1, z + 1, dxyv[5]);
	GetFieldDvs(dxys, countFields, x + 1, z + 1, dxyv[6]);
	GetFieldDvs(dxys, countFields, x + 1, z - 1, dxyv[7]);
	GetFieldDvs(dxys, countFields, x - 1, z - 1, dxyv[8]);

	// Averaging dx, dy and dxy values on same-height corners.
	glm::vec4 dx, dy, dxy;
	int countValues = 1;
	dx[0] = dxv[0][0];
	dy[0] = dyv[0][0];
	dxy[0] = dxyv[0][0];
	if (f[0][0] == f[1][3]) { dx[0] += dxv[1][3]; dy[0] += dyv[1][3]; dxy[0] += dxyv[1][3]; ++countValues; }
	if (f[0][0] == f[4][1]) { dx[0] += dxv[4][1]; dy[0] += dyv[4][1]; dxy[0] += dxyv[4][1]; ++countValues; }
	if (f[0][0] == f[5][2]) { dx[0] += dxv[5][2]; dy[0] += dyv[5][2]; dxy[0] += dxyv[5][2]; ++countValues; }
	dx[0] /= (float)countValues;
	dy[0] /= (float)countValues;
	dxy[0] /= (float)countValues;

	countValues = 1;
	dx[1] = dxv[0][1];
	dy[1] = dyv[0][1];
	dxy[1] = dxyv[0][1];
	if (f[0][1] == f[1][2]) { dx[1] += dxv[1][2]; dy[1] += dyv[1][2]; dxy[1] += dxyv[1][2]; ++countValues; }
	if (f[0][1] == f[2][0]) { dx[1] += dxv[2][0]; dy[1] += dyv[2][0]; dxy[1] += dxyv[2][0]; ++countValues; }
	if (f[0][1] == f[6][3]) { dx[1] += dxv[6][3]; dy[1] += dyv[6][3]; dxy[1] += dxyv[6][3]; ++countValues; }
	dx[1] /= (float)countValues;
	dy[1] /= (float)countValues;
	dxy[1] /= (float)countValues;

	countValues = 1;
	dx[2] = dxv[0][2];
	dy[2] = dyv[0][2];
	dxy[2] = dxyv[0][2];
	if (f[0][2] == f[2][3]) { dx[2] += dxv[2][3]; dy[2] += dyv[2][3]; dxy[2] += dxyv[2][3]; ++countValues; }
	if (f[0][2] == f[3][1]) { dx[2] += dxv[3][1]; dy[2] += dyv[3][1]; dxy[2] += dxyv[3][1]; ++countValues; }
	if (f[0][2] == f[7][0]) { dx[2] += dxv[7][0]; dy[2] += dyv[7][0]; dxy[2] += dxyv[7][0]; ++countValues; }
	dx[2] /= (float)countValues;
	dy[2] /= (float)countValues;
	dxy[2] /= (float)countValues;

	countValues = 1;
	dx[3] = dxv[0][3];
	dy[3] = dyv[0][3];
	dxy[3] = dxyv[0][3];
	if (f[0][3] == f[4][2]) { dx[3] += dxv[4][2]; dy[3] += dyv[4][2]; dxy[3] += dxyv[4][2]; ++countValues; }
	if (f[0][3] == f[3][0]) { dx[3] += dxv[3][0]; dy[3] += dyv[3][0]; dxy[3] += dxyv[3][0]; ++countValues; }
	if (f[0][3] == f[8][1]) { dx[3] += dxv[8][1]; dy[3] += dyv[8][1]; dxy[3] += dxyv[8][1]; ++countValues; }
	dx[3] /= (float)countValues;
	dy[3] /= (float)countValues;
	dxy[3] /= (float)countValues;

	glm::mat4 mFValues(f[0][3], f[0][2], dx[3], dx[2], f[0][0], f[0][1], dx[0], dx[1], dy[3], dy[2], dxy[3], dxy[2], dy[0], dy[1], dxy[0], dxy[1]);

	// Using formulas from: https://en.wikipedia.org/wiki/Bicubic_interpolation
	glm::mat4 mLeft(1.0f, 0.0f, -3.0f, 2.0f, 0.0f, 0.0f, 3.0f, -2.0f, 0.0f, 1.0f, -2.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1.0f);
	glm::mat4 mRight(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -3.0f, 3.0f, -2.0f, -1.0f, 2.0f, -2.0f, 1.0f, 1.0f);

	return mLeft * mFValues * mRight;
}

void Terrain::UpdateSurfaceData(const glm::ivec2& changeStart, const glm::ivec2& changeEnd)
{
	auto fields = m_Fields.GetArray();

	int sX1 = std::max(changeStart.x - 1, 0);
	int sY1 = std::max(changeStart.y - 1, 0);
	int eX1 = std::min(changeEnd.x + 1, (int)m_CountFields.x - 1);
	int eY1 = std::min(changeEnd.y + 1, (int)m_CountFields.y - 1);
	int sX2 = std::max(changeStart.x - 2, 0);
	int sY2 = std::max(changeStart.y - 2, 0);
	int eX2 = std::min(changeEnd.x + 2, (int)m_CountFields.x - 1);
	int eY2 = std::min(changeEnd.y + 2, (int)m_CountFields.y - 1);
	auto dxs = m_DX.GetArray();
	auto dys = m_DY.GetArray();
	auto dxys = m_DXY.GetArray();
	for (int y = sY1; y <= eY1; y++)
	{
		for (int x = sX1; x <= eX1; x++)
		{
			UpdateDerivatives(fields, m_CountFields, x, y, dxs, dys, dxys);
		}
	}
	for (int y = sY2; y <= eY2; y++)
	{
		for (int x = sX2; x <= eX2; x++)
		{
			auto fieldIndex = y * m_CountFields.x + x;
			m_SurfaceCoefficients[fieldIndex] = GetCoeffMatrix(fields, m_CountFields, x, y, dxs, dys, dxys);
		}
	}
}
