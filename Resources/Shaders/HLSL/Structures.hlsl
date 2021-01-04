// Structures.hlsl

#ifndef _STRUCTURES_HLSL_
#define _STRUCTURES_HLSL_

///////////////////////////////////////////////////
////////////////////////////////////////// Position 
///////////////////////////////////////////////////

struct VertexPos3
{
	float3 Position : POSITION;
};

struct VertexPos4
{
	float4 Position : SV_POSITION;
};

//////////////////////////////////////////////////////////////
////////////////////////////////////////// Position + TexCoord
//////////////////////////////////////////////////////////////

struct VertexPos3TextCoord
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
};

struct VertexPos4TextCoord
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Position + TexCoord + Normal
///////////////////////////////////////////////////////////////////////

struct VertexPos3TextCoordNormal
{
	float3 Position : POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal   : NORMAL;
};

struct VertexPos4TextCoordNormal
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD0;
	float3 Normal   : NORMAL;
};

///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Position + TexCoord + Normal + Color
///////////////////////////////////////////////////////////////////////////////

struct VertexPos3TextCoordNormalColor
{
	float3 Position		: POSITION;
	float2 TexCoord		: TEXCOORD0;
	float3 Normal		: NORMAL;
	float4 VertexColor	: COLOR;
};

///////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Position + TexCoord + Normal + WorldPosition
///////////////////////////////////////////////////////////////////////////////////////

struct VertexPos4TextCoordNormalWPos
{
	float4 Position			: SV_POSITION;
	float2 TexCoord			: TEXCOORD0;
	float3 Normal			: NORMAL;
	float3 WorldPosition	: TEXCOORD1;
};

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Position + TexCoord + Normal + WorldPosition + Color
///////////////////////////////////////////////////////////////////////////////////////////////

struct VertexPos4TextCoordNormalWPosColor
{
	float4 Position			: SV_POSITION;
	float2 TexCoord			: TEXCOORD0;
	float3 Normal			: NORMAL;
	float3 WorldPosition	: TEXCOORD1;
	float4 VertexColor		: COLOR;
};

///////////////////////////////////////////////////////////
////////////////////////////////////////// Position + Color
///////////////////////////////////////////////////////////

struct VertexPos3Color3
{
	float3 Position		: POSITION;
	float3 VertexColor	: COLOR;
};

struct VertexPos3Color4
{
	float3 Position		: POSITION;
	float4 VertexColor	: COLOR;
};

struct VertexPos4Color3
{
	float4 Position		: SV_POSITION;
	float3 VertexColor	: COLOR;
};

struct VertexPos4Color4
{
	float4 Position		: SV_POSITION;
	float4 VertexColor	: COLOR;
};

///////////////////////////////////////////////
////////////////////////////////////////// Misc 
///////////////////////////////////////////////

struct VertexPositions
{
	float4 Position			: SV_POSITION;
	float3 WorldPosition	: TEXCOORD1;
};

struct VertexPos4Direction
{
	float4 Position			: SV_POSITION;
	float3 Direction		: TEXCOORD0;
};

#endif