#pragma once
#include "Engine/core/RaycastUtils.hpp"
#include "Engine/Math/Convex2.hpp"
#include "Engine/core/FileUtils.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameMode.hpp"

class VertexBuffer;

constexpr float MAX_RADIUS = WORLD_SIZE_X * 0.1f;
constexpr float MIN_RADIUS = WORLD_SIZE_X * 0.02f;

constexpr int NUM_TOTALCONVEX = 2048;
constexpr int NUM_MAXNUMRAYS = 65536;

constexpr float MAX_ANGLESTEP = 100.f;
constexpr float MIN_ANGLESTEP = 30.f;

struct ChunkSaveData
{
	uint8_t m_chunkType = 0;
	uint32_t m_chunkHeaderStart = 0;
	uint32_t m_chunkTotalSize = 0;
};

enum class CursorState_ConvexScene {
	RESEASED,
	DRAGGING,
	NUM_STATE
};

// Array of Structs(AOS)
// Each element is a struct containing multiple fields.
// The entire object is stored together in memory, leading to better spatial locality
// good for cache when accessing complete objects frequently
struct ConvexObj
{
	ConvexObj(Disc2& disc, int convexIndex);
	ConvexObj(int polyIndex, int hullIndex, int discIndex);

	int m_convexPolyIndex = 0;
	int m_convexHullIndex = 0;
	int m_discIndex = 0;

	uint32_t m_bitBucketMask = 0;

	// we can not use pointer
	// because std::vector dynamically manages memory, When a vector grows beyond its current capacity
	// it will allocate a new larger memory block and move all elements there. 
	// This process is called reallocation.
	// The old memory is freed.
	// The old addresses(pointers, references, or iterators) become invalid because they no longer point to valid memory.
	// ConvexPoly2*		m_convexPoly = nullptr;
	// ConvexHull2*		m_convexHull = nullptr;
	// Disc2*				m_disc = nullptr;

	// Less efficient when only specific fields of the struct are accessed in a loop
	// ConvexPoly2 m_convexPoly;
	// ConvexHull2 m_convexHull;
	// Disc2 m_disc;
};

struct RayWithBitBucketMask
{
	RayWithBitBucketMask(Raycast2D& ray, uint32_t& mask)
		: m_ray(ray)
		, m_bitBucketMask(mask)
	{}
	RayWithBitBucketMask() = default;
	Raycast2D m_ray;
	uint32_t m_bitBucketMask = 0;
};

class RaycastVSConvex2Mode : public GameMode
{
public:
	RaycastVSConvex2Mode();
	~RaycastVSConvex2Mode();
	void Startup() override;
	void Update(float deltaSeconds) override;
	void Render() const override;//mark for that the render is not going to change the variables
	void Shutdown() override;

	virtual void CreateRandomShapes() override;

	virtual void UpdateModeInfo() override;

	// control
	void ControlConvexNumsOnScreen();

	// verts management
	void AddVertsForRay();

	void AddVertsForAllConvexAndCreateVertexBuffer();
	void AddVertsForConvexHullBoundingLines();
	void RenderBoundingLineAndImpactPos() const;
	std::vector<Vertex_PCU> m_boundingLinesVerts;
	float	m_convexEdgeThickness = 0.5f; // todo: have not use it yet
	float m_boundingLineThickness = 0.3f;
	float m_impactPtRadius = WORLD_SIZE_Y * 0.006f;
	Rgba8 m_convexEdgeColor = Rgba8::DEEP_ORANGE;
	Rgba8 m_boundingLine_sameDir = Rgba8::GREENISH_GRAY; 
	Rgba8 m_boundingLine_oppDir = Rgba8::BRIGHT_ORANGE; 
	Rgba8 m_boundingLine_hit = Rgba8::MAGENTA; 

	// line and raycast
	float		shortestImpactDist = 0.f;
	ConvexObj*	m_hitConvex = nullptr;
	void UpdateRaycastResultsForAllConvex();

	// space partition testing
	void	TestGitBucketSpacePartition();
	RaycastResult2D CheckRaycastVSConvexByBitBucket(RayWithBitBucketMask& ray, ConvexObj& convex);
	void	AssignBitBucketIndexForConvex(ConvexObj& convex);
	int		m_numRow = 4;
	int		m_numColumn = 8;
	int		m_numRays = 8192;

	double	m_bitBucketAverageImpactDist = 0.f;
	double	m_bitMaskDuration = 0.f;
	double	m_discCheckAverageImpactDist = 0.f;
	double	m_discCheckDuration = 0.f;

	// ray properties
	void				UpdateRaycast2D();
	Raycast2D			m_ray;
	void				ResetGlobalRaycastTobeMissingResult();
	bool				IsSceneHasAHitConvex();
	RaycastResult2D		m_raycastResult;	// we just have one globally and passing reference to get the closest result
	float				m_arrowSize = WORLD_SIZE_Y * 0.02f;
	float				m_arrowLineThickness = 0.5f;

	// Accessing all Disc2 elements requires iterating over ConvexObj, following pointers, 
	// which could lead to poor cache locality
	std::vector<ConvexObj>	m_convexObjs;	// Array of structs
	std::vector<Vertex_PCU> m_convexVerts;	// contain all the shapes verts
	VertexBuffer*			m_convexVertexBuffer = nullptr;	

	int m_numConvexVertices[NUM_TOTALCONVEX] = {};
	int m_numLineSegmentsVertices[NUM_TOTALCONVEX] = {};
	int m_numDiscVertices[NUM_TOTALCONVEX] = {};
	int m_discNumSegments = 36;
	
	std::vector<Vertex_PCU> m_lineSegmentVerts;	// contain all the shapes verts
	VertexBuffer*			m_lineSegmentVertexBuffer = nullptr;	
	
	std::vector<Vertex_PCU> m_discVerts;	// contain all the shapes verts
	VertexBuffer*			m_discVertexBuffer = nullptr;

	bool m_opaqueMode = true;
	bool m_showDiscs = true;
	void RenderAllConvex() const;
	void RenderRaycastResults() const;

	// cursor management
	void UpdateCursorState();
	bool UpdateConvexTouchByMouseCursor();

	void MoveDraggingConvex();

	ConvexObj*	m_touchingConvex = nullptr;
	CursorState_ConvexScene m_cursorState = CursorState_ConvexScene::RESEASED;

	void RotateAndScaleTouchingConvex();

	float m_rotatingSpeed = 1.f;
	float m_scaleSpeed = 0.003f;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// to avoid cache misses, we have those structure for raycast query
	int	m_numConvexShownOnScreen = 16;


	// Structs of Arrays(SOA)
	// Each field of the struct is stored separately in a different array.
	// Improves cache efficiency when accessing a single field across multiple objects(e.g., iterating over only positions).
	// Often better for SIMD optimizations and large - scale computations.
	std::vector<Disc2>			m_discs;
	std::vector<ConvexPoly2>	m_convexPolys;
	std::vector<ConvexHull2>	m_convexHulls;

	void TestBufferWriterAndParser();
	void AppendTestFileBufferData(BufferWriter& bufWrite, eBufferEndian endianMode = eBufferEndian::NATIVE);
	void ParseTestFileBufferData(BufferParser& bufParse, eBufferEndian endianMode);
	void TestLoadingExampleGHCSSaveFile();
	void TestSavingExampleGHCSSaveFile();
};