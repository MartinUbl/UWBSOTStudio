#include "arena.h"

#include <functional>
#include <map>
#include <numbers>

#include <uwbsot/PluginInterface.h>

CArena::CArena(const TCLI_Args& args) : mArgs(args) {
	mNetwork = std::make_unique<CNetwork>(args);
}

bool CArena::Start() {

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = b2Vec2(0.0f, 0.0f); // No gravity in the arena
	mWorld_Id = b2CreateWorld(&worldDef);

	constexpr float wallThickness = 0.1f;
	const float halfWall = wallThickness * 0.5f;

	b2BodyDef bodyDef = b2DefaultBodyDef();
	bodyDef.type = b2_staticBody;

	b2BodyId arenaBody = b2CreateBody(mWorld_Id, &bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.material.friction = 0.3f;
	shapeDef.material.restitution = 0.2f;

	const b2Rot noRotation = b2MakeRot(0.0f);

	// Bottom wall: just below y = 0
	b2Polygon bottomWall = b2MakeOffsetBox(
		mArgs.arena_width * 0.5f + wallThickness,
		halfWall,
		b2Vec2(mArgs.arena_width * 0.5f, -halfWall),
		noRotation
	);
	b2CreatePolygonShape(arenaBody, &shapeDef, &bottomWall);

	// Top wall: just above y = mArena_Height
	b2Polygon topWall = b2MakeOffsetBox(
		mArgs.arena_width * 0.5f + wallThickness,
		halfWall,
		b2Vec2(mArgs.arena_width * 0.5f, mArgs.arena_height + halfWall),
		noRotation
	);
	b2CreatePolygonShape(arenaBody, &shapeDef, &topWall);

	// Left wall: just left of x = 0
	b2Polygon leftWall = b2MakeOffsetBox(
		halfWall,
		mArgs.arena_height * 0.5f,
		b2Vec2(-halfWall, mArgs.arena_height * 0.5f),
		noRotation
	);
	b2CreatePolygonShape(arenaBody, &shapeDef, &leftWall);

	// Right wall: just right of x = mArena_Width
	b2Polygon rightWall = b2MakeOffsetBox(
		halfWall,
		mArgs.arena_height * 0.5f,
		b2Vec2(mArgs.arena_width + halfWall, mArgs.arena_height * 0.5f),
		noRotation
	);
	b2CreatePolygonShape(arenaBody, &shapeDef, &rightWall);


	float cursor_x = 15.0f;
	float cursor_y = 0.0f;

	for (size_t i = 0; i < mArgs.teams; i++) {
		cursor_y = 15.0f;
		cursor_x += 15.0f;
		for (size_t j = 0; j < mArgs.robots_per_team; j++) {
			mRobots.emplace_back(static_cast<uint16_t>(j), static_cast<uint16_t>(i), cursor_x, cursor_y, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			cursor_y += 15.0f;
		}
	}

	for (TArena_Robot& robot : mRobots) {
		Create_Robot_Body(robot);
	}

	mBall.x = mArgs.arena_width * 0.5f;
	mBall.y = mArgs.arena_height * 0.5f;
	mBall.radius = 2.0f;
	mBall.state = NBall_State::Ground;

	Create_Ball();

	mNetwork->Set_Callbacks({
		std::bind(&CArena::On_Speed_Set, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
		std::bind(&CArena::On_Refresh_Positions_Request, this, std::placeholders::_1),
		std::bind(&CArena::On_Metadata_Request, this, std::placeholders::_1),
		std::bind(&CArena::On_Ball_Position_Request, this, std::placeholders::_1)
	});

	return mNetwork->Start();
}

void CArena::Create_Ball() {
	if (mBall.radius <= 0.0f)
		throw std::invalid_argument("Ball radius must be positive");

	b2BodyDef bodyDef = b2DefaultBodyDef();

	bodyDef.type = b2_dynamicBody;
	bodyDef.position = b2Vec2(mBall.x, mBall.y);

	// The ball should continue moving after an impact, but gradually slow down.
	bodyDef.linearDamping = 0.25f;
	bodyDef.angularDamping = 0.15f;

	// Helps prevent a small, fast ball from passing through thin objects.
	bodyDef.isBullet = true;

	// Circular objects may rotate quickly.
	bodyDef.allowFastRotation = true;

	// Keep it available for collisions immediately.
	bodyDef.enableSleep = true;

	mBall.body_id = b2CreateBody(mWorld_Id, &bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();

	/*
	 * Box2D density is kg/m² in a metrically scaled simulation.
	 *
	 * A table-tennis ball has a mass of roughly 0.0027 kg. Since Box2D is 2D,
	 * choose density so that:
	 *
	 *     mass = density * pi * radius²
	 */
	constexpr float desiredMass = 0.0027f;

	shapeDef.density =
		desiredMass /
		(std::numbers::pi * mBall.radius * mBall.radius);

	// Low surface friction: the ball should not stick to robots or walls.
	shapeDef.material.friction = 0.12f;

	// Moderately bouncy. Increase toward 1.0 for stronger bouncing.
	shapeDef.material.restitution = 0.65f;

	b2Circle circle{};
	circle.center = b2Vec2(0.0f, 0.0f);
	circle.radius = mBall.radius;

	b2CreateCircleShape(
		mBall.body_id,
		&shapeDef,
		&circle
	);
}

void CArena::Create_Robot_Body(TArena_Robot& robot) {

	constexpr float Robot_Size = 10.0f;

	b2BodyDef bodyDef = b2DefaultBodyDef();

	bodyDef.type = b2_dynamicBody;
	bodyDef.position = b2Vec2(robot.x, robot.y);
	bodyDef.rotation = b2MakeRot(robot.orientation);

	// The robot is actively controlled, so sleeping is usually undesirable.
	bodyDef.enableSleep = false;

	// Optional protection if robots can move very quickly.
	bodyDef.isBullet = false;

	// Only safe when the robot object's address remains stable.
	bodyDef.userData = &robot;

	robot.body_id = b2CreateBody(mWorld_Id, &bodyDef);

	b2ShapeDef shapeDef = b2DefaultShapeDef();

	// A non-zero density gives the dynamic body mass and inertia.
	shapeDef.density = 1.0f;

	shapeDef.material.friction = 0.5f;
	shapeDef.material.restitution = 0.1f;

	// b2MakeBox takes half-extents.
	//
	// Local X axis = forward direction.
	// Local Y axis = robot's lateral direction.
	const b2Polygon robotShape = b2MakeBox(
		Robot_Size * 0.5f,
		Robot_Size * 0.5f
	);

	b2CreatePolygonShape(
		robot.body_id,
		&shapeDef,
		&robotShape
	);

	Apply_Robot_Command(robot);
}

void CArena::Apply_Robot_Command(TArena_Robot& robot)
{
	if (!b2Body_IsValid(robot.body_id))
		return;

	constexpr const float Speed_Multiplier = 100.0f; // Adjust this multiplier to control the speed of the robots

	// Read the actual orientation from Box2D. It may have changed because
	// of a collision since the previous simulation step.
	const b2Rot rotation = b2Body_GetRotation(robot.body_id);
	const float angle = b2Rot_GetAngle(rotation);

	const b2Vec2 velocity = {
		std::cos(angle) * robot.speed_forward * Speed_Multiplier,
		std::sin(angle) * robot.speed_forward * Speed_Multiplier
	};

	b2Body_SetLinearVelocity(robot.body_id, velocity);
	b2Body_SetAngularVelocity(robot.body_id, robot.speed_angular);
}

void CArena::Synchronize_Ball() {
	if (!b2Body_IsValid(mBall.body_id))
		return;

	const b2Vec2 position = b2Body_GetPosition(mBall.body_id);

	mBall.x = position.x;
	mBall.y = position.y;
}

void CArena::Synchronize_Robot_State(TArena_Robot& robot)
{
	if (!b2Body_IsValid(robot.body_id))
		return;

	const b2Vec2 position =
		b2Body_GetPosition(robot.body_id);

	const b2Rot rotation =
		b2Body_GetRotation(robot.body_id);

	robot.x = position.x;
	robot.y = position.y;
	robot.orientation = b2Rot_GetAngle(rotation);
}

int CArena::Run() {
	// Main loop for the arena simulation
	while (true) {
		Update_Robot_Positions(0.01f);
		Synchronize_Ball();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return 0;
}

void CArena::Update_Robot_Positions(float delta_time) {

	for (TArena_Robot& robot : mRobots)
		Apply_Robot_Command(robot);

	constexpr int subStepCount = 4;

	b2World_Step(
		mWorld_Id,
		delta_time,
		subStepCount
	);

	// Read the collision-resolved positions back from Box2D.
	for (TArena_Robot& robot : mRobots)
		Synchronize_Robot_State(robot);
}

void CArena::On_Speed_Set(uint16_t robot_idx, uint16_t team_idx, float left, float right) {
	for (auto& robot : mRobots) {
		if (robot.robot_idx == robot_idx && robot.team_idx == team_idx) {
			// Update the robot's wheel speeds
			robot.left_wheel_speed = left;
			robot.right_wheel_speed = right;

			// assume a simple differential drive model to update speed_x and speed_y based on wheel speeds
			robot.speed_forward = (left + right) / 2.0f; // average of left and right wheel speeds
			robot.speed_angular = (right - left) / 0.5f; // simple differential drive model, wheel_base = 0.5

			Apply_Robot_Command(robot);

			break; // Exit loop after updating the correct robot
		}
	}
}

void CArena::On_Refresh_Positions_Request(const TWaiting_For_Positions& request) {
	std::vector<vsot::TNet_Robot_Position> positions;
	for (const auto& robot : mRobots) {
		if (robot.team_idx == request.request.team_idx || request.request.team_idx == Team_Idx_Any) {
			if (request.request.robot_idx == robot.robot_idx || request.request.robot_idx == Robot_Idx_Any) {
				vsot::TNet_Robot_Position pos;
				pos.robot_idx = robot.robot_idx;
				pos.team_idx = robot.team_idx;
				pos.x = robot.x;
				pos.y = robot.y;
				pos.orientation = robot.orientation;
				positions.push_back(pos);
			}
		}
	}

	mNetwork->Send_Refresh_Positions_Response(request, positions);
}

void CArena::On_Metadata_Request(const TWaiting_For_Any& request) {
	std::vector<uint16_t> teamCounts;

	std::map<uint16_t, uint16_t> teamCountMap;
	for (const auto& robot : mRobots) {
		teamCountMap[robot.team_idx]++;
	}
	for (const auto& [team_idx, count] : teamCountMap) {
		teamCounts.push_back(count);
	}

	mNetwork->Send_Metadata_Response(request, teamCounts, mArgs.arena_width, mArgs.arena_height);
}

void CArena::On_Ball_Position_Request(const TWaiting_For_Any& request) {
	mNetwork->Send_Ball_Position_Response(request, static_cast<uint16_t>(mBall.state), mBall.x, mBall.y, mBall.radius);
}
