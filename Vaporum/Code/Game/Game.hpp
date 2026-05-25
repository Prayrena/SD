#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/core/RaycastUtils.hpp"
#include "Game/GameCommon.hpp"
#include <vector>
#include <string>
#include <map>

class	App;
class	Entity;
class	UI;
class	RandomNumberGenerator;
class	Boid;
class	EnergyBar;
class	EnergySelectionRing;
class	Model;
struct	PhongLightingConstants;
class   Timer;
class   Map;
class	Player;
class	VertexBuffer;
class	IndexBuffer;
class   Widget;
class	Unit;

constexpr int INVALID_PLAYER_INDEX = 999;

enum TestingScene
{
	EMISSIVE_CUBES,
	TEXTURED_PROPS,
	TUTORIAL_BOX,
	HADRIAN_TANK,

	NUM_TESTSCENE
};

enum class TurnState
{
	WAITING_FOR_TURN,
	NO_SELECTION,
	UNIT_SELECTED_MOVE,
	UNIT_SELECTED_ATTACK,
	END_TURN,

	NUM_STATE
};

enum class GameState
{
	NONE,
	SPLASH_SCREEN,
	MAIN_MENU,
	GAME_START,			 // loading map and unit
	WAITING_FOR_PLAYERS, // done client and server connecting to each other, the local game mode will skip over this step?
	TURN_START,			 // enable widget show whose current turn is
	PLAYING,
	PAUSED,				 // can only enter from playing, and it only go back to playing state
	GAMEOVER,
	NUM_GAMESTATE
};

class Game
{
public:
	Game();
	~Game();
	void Startup();
	void Update();
	void Render();//mark for that the render is not going to change the variables
	void Shutdown();
	void ShutDownUIElementList(int arraySize, UI** m_entityArrayPointer);

	void RenderWorldInPlayerCamera();
	void RenderGroundPlane();
	void SpawnProps();

	// registered event
	static bool Event_LoadMapByInputName(EventArgs& args); // "LoadMap Name=name"

	//----------------------------------------------------------------------------------------------------------------------------------------------------
#pragma region Game state machine
	void ChangeGameState(GameState state);

	void EnterState(GameState state);
	void ExitState(GameState state);

	void EnterSplashScreen();
	void EnterMainMenu();
	void EnterGameStart();
	void EnterPlaying();
	void EnterTurnStart();
	void EnterGameOver();
	void EnterPauseMenu();
	void EnterWaiitingForPlayers();

	void ExitSplashScreen();
	void ExitMainMenu();
	void ExitPauseMenu();
	void ExitPlaying();
	void ExitGameOver();
	void ExitWaitingForPlayers();

	void UpdateSplashScreen();
	void UpdateMainMenu();
	void UpdateWaitingForPlayers();
	void UpdatePauseMenu();
	void UpdatePlaying();
	void UpdateTurnStart();
	void UpdateGameOver();

	void RenderSplashScreen() const;
	void RenderMainMenu() const;
	void RenderWaitingForPlayers() const;

	GameState m_currentGameState = GameState::SPLASH_SCREEN;
#pragma endregion

	
	//----------------------------------------------------------------------------------------------------------------------------------------------------
#pragma region Commands
	void		ScribeNetworkCommands();
	static bool Command_PlayerReady(EventArgs& args);
	static bool Command_PlayerQuit(EventArgs& args);
	static bool Command_StartTurn(EventArgs& args);
	static bool Command_EndTurn(EventArgs& args);
	static bool Command_SetFocusedHex(EventArgs& args);

	void PlayerReady();
	void PlayerQuit();

	bool m_theOtherPlayerIsReady = false;

	void StartTurn();
	void EndTurn();
	TurnState	m_currentTurnState = TurnState::WAITING_FOR_TURN;
	Player*		m_playerTakingCurrentTurn = nullptr;

	void SetFocusedHex(int tileIndex);

	// unit selection
	static bool Command_SelectFocusedUnit(EventArgs& args);
	static bool Command_SelectPreviousUnit(EventArgs& args);
	static bool Command_SelectNextUnit(EventArgs& args);

	void SelectFocusedUnit();
	void SelectPreviousUnit();
	void SelectNextUnit();

	Unit* m_selectedUnit = nullptr;
	Unit* m_defensingUnit = nullptr;

	// unit control
	static bool Command_Move(EventArgs& args);
	static bool Command_Stay(EventArgs& args);			// do nothing, do not move
	static bool Command_HoldFire(EventArgs& args);		// moved, but do not attack
	static bool Command_Attack(EventArgs& args);
	static bool Command_Cancel(EventArgs& args);
	
	void Move();
	void Stay();
	void Attack();
	void HoldFire();
	void Cancel();
#pragma endregion
	
	bool CheckIfSelectedHexIsInAttackTiles() const;
	bool CheckIfSelectedHexPointsToSelectedUnit() const;
	bool CheckIfPlayerHasNoUnitsToMove() const;
	bool CheckForWinner();

	void CreateWinnerWidget();
	Widget* m_winnerWidget_0 = nullptr;
	Widget* m_winnerWidget_1 = nullptr;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// viewer control
	void UpdatePlayingInput();
	void UpdatePauseMenuControl();
	void UseLeftAndRightArrowsToSelectUnit();
	bool LoadSelectedFile();
	void GetWindowsLastErrorAndDisplayOnScreen();
	void LoadNewModel();
	void ControlTextureMapDebug();

	std::vector<Model*> m_loadedModels;
	float m_debugMsgFontSize = 30.f;
	//----------------------------------------------------------------------------------------------------------------------------------------------------
#pragma region UI
	void	UpdateUnitInfoWidget(); // For Selected or current mouse Focused Unit
	Widget* CreateUnitInfoWidget(Player* unitOwner, Unit* unit); 
	Widget* m_unitWidget_player1 = nullptr;
	Widget* m_unitWidget_player2 = nullptr;

	void	CreatePlayingInstructionWidget();
	void	UpdatePlayingInstructionWidget();
	Widget* m_playInstructionWidget = nullptr;

	void CreateMainMenuWidgets();
	void CreateSplashScreenWidget();
	std::vector<Vertex_PCU> m_menuVerts;
	Widget* m_mainMenuWidget = nullptr;
	Widget* m_splashScreenWidget = nullptr;

	void	CreatePauseMenuWidget();
	Widget* m_pauseMenuWidget = nullptr;	
	
	void	UseArrowKeysToControlWidgetButtons(Widget* widget);

	void	CreateCenterAndEdgePlayerTurnWidgets();
	Widget* m_turnStartWidget = nullptr;
	Widget* m_edgeTurnWidget = nullptr;

	void	CreateEndTurnWidget();
	Widget* m_endTurnWidget = nullptr;	
	
	void	CreateWaitingForPlayersWidget();
	Widget* m_waitingWidget = nullptr;

	void	UpdatePlayerTurnIndex();
	int		m_currentTurnPlayerIndex = INVALID_PLAYER_INDEX;
#pragma endregion

	// Lighting control
	void ControlLightingSettings();
	PhongLightingConstants* m_phongLighinting = nullptr;

	Vec2 GetRandomPosInWorld(Vec2 worldSize);

	// camera functions
	void UpdateCameras(float deltaSeconds);

	Camera m_worldCamera;
	Camera m_screenCamera;

	bool  m_gameIsOver					 = false;
	float m_returnToStartTimer			 = 0.f;
	int	  m_playerLivesNum				 = PLAYER_LIVES_NUM;
	float m_introTimer = 0.f;

	bool IsNetworkGameMode() const;// if false, this is a local game

	// Entity children special power
	std::vector<Player*> m_players;
	std::map<Player*, Rgba8> m_playerUnitColorMap;
	std::map<Player*, Rgba8> m_playerSelectedUnitColorMap;
	std::map<Player*, Rgba8> m_playerHexColorMap;
	std::map<std::string, Entity*> m_entities;

	std::vector<Entity*> m_testSceneProps;

	std::string m_modelFilePath;
	std::string m_appFilePath;

	// scene management
	void ClearTestScene();
	void SwitchSceneBasedOnInput();
	void DeleteCurrentTestMode_And_CreateNewTestMode(TestingScene type);

	int	 m_testSceneIndex = 0;

	// debug
	void ToggleToShowDebugVertexes();
	void ToggleToRotateModelAndProp();

	bool m_debugMode = false;
	bool m_rotationMode = false;

#pragma region Ground
	// Ground plane vertex buffer
	void CreateVertexIndexBufferForGroundAndCopyFromCPUtoGPU();
	std::vector<Vertex_PCUTBN>		m_groundVertexs;
	std::vector<unsigned int>	m_groundIndexArray;

	VertexBuffer* m_groundVertexBuffer = nullptr;
	IndexBuffer* m_groundIndexBuffer = nullptr;
#pragma endregion

#pragma region map
	// map
	void GenerateAllMaps();
	Map* CheckIfTheMapIsLoadedBefore(std::string mapName);
	Map* CreateAndStartupNewMap(std::string mapName);

	std::vector<Map*>	m_allMaps;
	Map* m_currentMap = nullptr;
#pragma endregion
	// raycast
	RaycastResult3D RaycastFromCameraToMouseToMap(Map const* map);
	void AddToVertsForPlayerSelectedHexagons(int index);
	void RenderSelectedHex() const;

	RaycastResult3D				m_playerMouseRaycastResult;
	Vec3						m_rayForwardNormal;
	std::vector<Vertex_PCU>		m_selectedHexagonsVertexs;

	int		CheckMouseRaycastImpactIsInWhichHexagon(Map* map);

	int		m_selectedHexIndex = INVALID_HEX_INDEX;
	float	m_selectedHexInRadius = 0.4f;
	Rgba8	m_selectedHexColor = Rgba8::GREENISH_GRAY;

private:
	bool CheckUIEnabled(UI* const UI) const;
	bool m_OpenScene = true;

	// Collision Detecting
	bool DoEntitiesOverlap(Entity const& a, Entity const& b);

	void UpdateDebugRenderMessages();
	Timer* m_FPSTimer = nullptr;
	std::string m_FPSString;
};
