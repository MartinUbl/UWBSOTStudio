#pragma once

#include "args.h"
#include "network.h"

#include <vector>
#include <memory>
#include <cstdint>

#include <box2d/box2d.h>
#include <uwbsot/PluginInterface.h>

#include <pybind11/embed.h>
#include <uwbsot-python/Runtime.h>

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
	std::string scriptName;
};

struct TArena_Ball {
	NBall_State state = NBall_State::Not_Found;
	float x = 0.0f;
	float y = 0.0f;
	float radius = 1.0f;

	b2BodyId body_id = b2_nullBodyId;
	b2ShapeId shape_id = b2_nullShapeId;
};

struct TArena_Walls {
	b2BodyId body_id = b2_nullBodyId;

	b2ShapeId bottom = b2_nullShapeId;
	b2ShapeId top = b2_nullShapeId;
	b2ShapeId left = b2_nullShapeId;
	b2ShapeId right = b2_nullShapeId;
};

class CArena : public CPython_UWBSOT_API {
	private:
		TCLI_Args mArgs;

		std::unique_ptr<CNetwork> mNetwork;

		b2WorldId mWorld_Id = b2_nullWorldId;
		std::vector<TArena_Robot> mRobots;

		TArena_Walls mWalls;
		TArena_Ball mBall;

		long long mCurrent_Time_Ms = 0;

		std::unique_ptr<CPython_Runtime> mPython_Runtime;
		std::vector<TScript_Entry> mScript_List;

	protected:
		void Create_Ball();
		void Create_Robot_Body(TArena_Robot& robot);

		void Apply_Robot_Command(TArena_Robot& robot);

		void Synchronize_Ball();
		void Synchronize_Robot_State(TArena_Robot& robot);

		void Process_Ball_Contact_Events();

		bool Is_Arena_Wall(b2ShapeId shapeId);
		void On_Ball_Hit_Wall(b2ShapeId wallShape);

		void Reset_Ball_Position();

	protected:
		void Init_Simulation();

	public: // pybind API
		int Get_Robot_Count(size_t team_index) const override;
		void Set_Robot_Script(size_t team_index, size_t robot_index, const std::string& script_id) override;
		long long Get_Elapsed_Time_Milliseconds() const override;
		void Set_Robot_Speed(size_t team_index, size_t robot_index, float speed_left, float speed_right) override;
		py::tuple Get_Robot_Position(size_t team_index, size_t robot_index) const override;
		py::tuple Get_Ball_Position() const override;

	public:
		CArena(const TCLI_Args& args);

		bool Start();
		int Run();

		void Update_Robot_Positions(float delta_time);

		void On_Speed_Set(uint16_t robot_idx, uint16_t team_idx, float left, float right);
		void On_Refresh_Positions_Request(const TWaiting_For_Positions& request);
		void On_Metadata_Request(const TWaiting_For_Any& request);
		void On_Ball_Position_Request(const TWaiting_For_Any& request);
};
