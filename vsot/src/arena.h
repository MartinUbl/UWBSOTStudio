#pragma once

#include "args.h"
#include "network.h"

#include <vector>
#include <memory>
#include <cstdint>

#include <box2d/box2d.h>

struct TArena_Robot {
	uint16_t robot_idx;
	uint16_t team_idx;
	float x;
	float y;
	float orientation; // in radians, 0 = facing right, pi/2 = facing up
	float speed_forward;
	float speed_angular;

	float left_wheel_speed;  // cached
	float right_wheel_speed; // cached

	b2BodyId body_id = b2_nullBodyId;
};

class CArena {
	private:
		TCLI_Args mArgs;

		std::unique_ptr<CNetwork> mNetwork;

		b2WorldId mWorld_Id;
		std::vector<TArena_Robot> mRobots;

	protected:
		void Create_Robot_Body(TArena_Robot& robot);
		void Apply_Robot_Command(TArena_Robot& robot);
		void Synchronize_Robot_State(TArena_Robot& robot);

	public:
		CArena(const TCLI_Args& args);

		bool Start();
		int Run();

		void Update_Robot_Positions(float delta_time);

		void On_Speed_Set(uint16_t robot_idx, uint16_t team_idx, float left, float right);
		void On_Refresh_Positions_Request(const TWaiting_For_Positions& request);
		void On_Metadata_Request(const TWaiting_For_Metadata& request);
};
