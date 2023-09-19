#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

int VK_BEAM = 0x5A;
int MAX_CHARGING_TIME = 2500;

enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD,
	STATE_CHARGE,
};

struct GameState
{
	int score = 0;
	int hi_score = 0;
	int score_incr = 1;
	float score_multiplier = 1;
	Agent8State agentState = STATE_APPEAR;
};

GameState gameState;

enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COW,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_BEAM,
	TYPE_DESTROYED,
};

void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCows();
void UpdateCoinsAndStars();
void UpdateLasers();
void UpdateBeam();
void UpdateDestroyed();
void UpdateAgent8();

// The entry point for a PlayBuffer program
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");
	Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");
	Play::GetGameObject(id_fan).velocity = { 0, 3 };
	Play::GetGameObject(id_fan).animSpeed = 1.0f;
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::DrawBackground();
	UpdateAgent8();
	UpdateFan();
	UpdateTools();
	UpdateCows();
	UpdateCoinsAndStars();
	UpdateLasers();
	UpdateBeam();
	UpdateDestroyed();

	Play::DrawFontText("64px", "ARROW KEYS TO MOVE UP AND DOWN, SPACE TO FIRE, HOLD Z TO FIRE BEAM",
		{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);
	
	Play::DrawFontText("132px", "SCORE: " + std::to_string(gameState.score),
		{ DISPLAY_WIDTH / 4, 50 }, Play::CENTRE);

	Play::DrawFontText("132px", "HI-SCORE: " + std::to_string(gameState.hi_score),
		{ 3 * DISPLAY_WIDTH / 4, 50 }, Play::CENTRE);
	
	Play::DrawFontText("64px", std::to_string(std::ceil(gameState.score_multiplier * 100.0) / 100.0) + "x",
		{ DISPLAY_WIDTH / 4, 140 }, Play::CENTRE);

	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game 
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void HandlePlayerControls()
{
	static bool keyHeld = false;
	static DWORD keyPressStartTime = 0;

	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	if (Play::KeyDown(VK_UP))
	{
		obj_agent8.velocity = { 0, -4 };
		Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
	}
	else if (Play::KeyDown(VK_DOWN))
	{
		obj_agent8.acceleration = { 0, 1 };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
	}
	else
	{
		if (obj_agent8.velocity.y > 5)
		{
			gameState.agentState = STATE_HALT;
			Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
			obj_agent8.acceleration = { 0, 0 };
		}
		else
		{
			Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
			obj_agent8.velocity *= 0.5f;
			obj_agent8.acceleration = { 0, 0 };
		}
	}
	if (Play::KeyPressed(VK_SPACE))
	{
		Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);
		int id = Play::CreateGameObject(TYPE_LASER, firePos, 30, "laser");
		Play::GetGameObject(id).velocity = { 32, 0 };
		Play::PlayAudio("shoot");
	}
	if (Play::KeyDown(VK_BEAM))
	{
		gameState.agentState = STATE_CHARGE;
	}
	Play::UpdateGameObject(obj_agent8);
	
	if (Play::IsLeavingDisplayArea(obj_agent8))
		obj_agent8.pos = obj_agent8.oldPos;
	
	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}

void UpdateFan()
{
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);

	if (Play::RandomRoll(100) == 1)
	{
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 60, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);
		obj_tool.velocity = Point2f(-4, Play::RandomRollRange(-1, 1) * 6);
		
		if (Play::RandomRoll(2) == 1)
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;
			obj_tool.velocity.x = -2;
			obj_tool.rotSpeed = 0.1f;
		}
		Play::PlayAudio("tool");
	}
	if (Play::RandomRoll(300) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;
	}
	if (Play::RandomRoll(1000) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COW, obj_fan.pos, 120, "cow");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -1, 0 };
		obj_coin.rotSpeed = 0.03f;
		Play::PlayAudio("moo");
	}

	Play::UpdateGameObject(obj_fan);
	
	if (Play::IsLeavingDisplayArea(obj_fan))
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}
	Play::DrawObject(obj_fan);
}

void UpdateTools()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	
	for (int id : vTools)
	{
		GameObject& obj_tool = Play::GetGameObject(id);
		obj_tool.scale = { 0.8f };
		if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_tool, obj_agent8))
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			gameState.agentState = STATE_DEAD;
		}
		Play::UpdateGameObject(obj_tool);
		
		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}
		Play::DrawObjectRotated(obj_tool);
		
		if (!Play::IsVisible(obj_tool))
			Play::DestroyGameObject(id);
	}
}

void UpdateCows()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vCows = Play::CollectGameObjectIDsByType(TYPE_COW);

	for (int id : vCows)
	{
		GameObject& obj_cow = Play::GetGameObject(id);
		obj_cow.scale = { 0.6f };
		if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_cow, obj_agent8))
		{
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			gameState.agentState = STATE_DEAD;
		}
		Play::UpdateGameObject(obj_cow);

		if (Play::IsLeavingDisplayArea(obj_cow, Play::VERTICAL))
		{
			obj_cow.pos = obj_cow.oldPos;
			obj_cow.velocity.y *= -1;
		}
		Play::DrawObjectRotated(obj_cow);

		if (!Play::IsVisible(obj_cow))
			Play::DestroyGameObject(id);
	}
}

void UpdateCoinsAndStars()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_coin : vCoins)
	{
		GameObject& obj_coin = Play::GetGameObject(id_coin);
		bool hasCollided = false;

		if (Play::IsColliding(obj_coin, obj_agent8))
		{
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = { 0.0f, 0.5f };
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}

			hasCollided = true;
			gameState.score += 300;
			Play::PlayAudio("collect");
		}

		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		if (!Play::IsVisible(obj_coin) || hasCollided)
			Play::DestroyGameObject(id_coin);
	}

	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

	for (int id_star : vStars)
	{
		GameObject& obj_star = Play::GetGameObject(id_star);

		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);

		if (!Play::IsVisible(obj_star))
			Play::DestroyGameObject(id_star);
	}
}

void UpdateLasers()
{
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);
	
	for (int id_laser : vLasers)
	{
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		bool hasCollided = false;
		
		for (int id_tool : vTools)
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			if (Play::IsColliding(obj_laser, obj_tool))
			{
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score_incr++;
				gameState.score_multiplier = std::min(1.0 + 0.5 * (std::log(std::max(gameState.score_incr, 1) / 2.0)), 5.0);
				gameState.score += static_cast<int>(100 * gameState.score_multiplier);
			}
		}
	
		for (int id_coin : vCoins)
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			if (Play::IsColliding(obj_laser, obj_coin))
			{
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
				gameState.score_incr = 1;
				gameState.score_multiplier = 1;
			}
		}
		if (gameState.score < 0)
			gameState.score = 0;
	
		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);
		
		if (!Play::IsVisible(obj_laser) || hasCollided)
			Play::DestroyGameObject(id_laser);
	}
}

void UpdateCharging()
{
	static bool keyHeld = false;
	static DWORD keyPressStartTime = 0;
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	Play::SetSprite(obj_agent8, "agent8_charge", 0);
	obj_agent8.velocity = { 0,0 };
	obj_agent8.acceleration = { 0,0 };

	if (!keyHeld)
	{
		keyHeld = true;
		keyPressStartTime = timeGetTime();
		Play::PlayAudio("charging");
	}
	else
	{
		DWORD currentTime = timeGetTime();
		DWORD timeElapsed = currentTime - keyPressStartTime;
		if (timeElapsed >= MAX_CHARGING_TIME && Play::KeyUp(VK_BEAM))
		{
			Vector2D firePos = obj_agent8.pos + Vector2D(350, -75);
			int id = Play::CreateGameObject(TYPE_BEAM, firePos, 60, "beam");
			Play::GetGameObject(id).velocity = { 16, 0 };
			Play::PlayAudio("beam");

			keyHeld = false;
			keyPressStartTime = 0;
			Play::StopAudioLoop("charging");
			gameState.agentState = STATE_PLAY;
		}
		if (timeElapsed < MAX_CHARGING_TIME && Play::KeyUp(VK_BEAM))
		{
			keyHeld = false;
			keyPressStartTime = 0;
			Play::StopAudioLoop("charging");
			gameState.agentState = STATE_PLAY;
		}
	}

}

void UpdateBeam()
{
	std::vector<int> vBeam = Play::CollectGameObjectIDsByType(TYPE_BEAM);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCows = Play::CollectGameObjectIDsByType(TYPE_COW);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_beam : vBeam)
	{
		GameObject& obj_beam = Play::GetGameObject(id_beam);
		bool hasCollided = false;
		
		for (int id_tool : vTools)
		{
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			if (Play::IsColliding(obj_beam, obj_tool))
			{
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score_incr++;
				gameState.score_multiplier = std::min(1.0 + 0.5 * (std::log(std::max(gameState.score_incr, 1) / 2.0)), 5.0);
				gameState.score += static_cast<int>(100 * gameState.score_multiplier);
			}
		}

		for (int id_cow : vCows)
		{
			GameObject& obj_cow = Play::GetGameObject(id_cow);
			if (Play::IsColliding(obj_beam, obj_cow))
			{
				hasCollided = true;
				obj_cow.type = TYPE_DESTROYED;
				gameState.score_incr++;
				gameState.score_multiplier = std::min(1.0 + 0.5 * (std::log(std::max(gameState.score_incr, 1) / 2.0)), 5.0);
				gameState.score += static_cast<int>(500 * gameState.score_multiplier);
			}
		}

		for (int id_coin : vCoins)
		{
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			if (Play::IsColliding(obj_beam, obj_coin))
			{
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
				gameState.score_incr = 1;
				gameState.score_multiplier = 1;
			}
		}
		if (gameState.score < 0)
			gameState.score = 0;

		Play::UpdateGameObject(obj_beam);
		Play::DrawObject(obj_beam);

		if (!Play::IsVisible(obj_beam))
			Play::DestroyGameObject(id_beam);
	}
}

void UpdateDestroyed()
{
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	for (int id_dead : vDead)
	{
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);
		
		if (obj_dead.frame % 2)
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);
		
		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10)
			Play::DestroyGameObject(id_dead);
	}
}

void UpdateAgent8()
{
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);

	switch (gameState.agentState)
	{
	case STATE_APPEAR:
		obj_agent8.velocity = { 0, 12 };
		obj_agent8.acceleration = { 0, 0.5f };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
		obj_agent8.rotation = 0;
		if (obj_agent8.pos.y >= DISPLAY_HEIGHT / 3)
			gameState.agentState = STATE_PLAY;
		break;

	case STATE_HALT:
		obj_agent8.velocity *= 0.9f;
		if (Play::IsAnimationComplete(obj_agent8))
			gameState.agentState = STATE_PLAY;
		break;

	case STATE_PLAY:
		HandlePlayerControls();
		break;

	case STATE_CHARGE:
		UpdateCharging();
		break;
	
	case STATE_DEAD:
		obj_agent8.acceleration = { -0.3f , 0.5f };
		obj_agent8.rotation += 0.25f;
		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.agentState = STATE_APPEAR;
			obj_agent8.pos = { 115, 0 };
			obj_agent8.velocity = { 0, 0 };
			obj_agent8.frame = 0;
			Play::StartAudioLoop("music");
			
			if (gameState.score > gameState.hi_score)
			{
				gameState.hi_score = gameState.score;
			}

			gameState.score = 0;
			gameState.score_incr = 1;
			gameState.score_multiplier = 1;

			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_COIN))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_COW))
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
		}
		break;
	
	} // End of switch on Agent8State
	
	Play::UpdateGameObject(obj_agent8);
	
	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD)
		obj_agent8.pos = obj_agent8.oldPos;
	
	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
}