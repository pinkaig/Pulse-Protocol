/******************************************************************************/
/**
 * @file        Animation.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai
 * @brief       Implements the Animation component defined in Animation.h.
 *              Handles serialization and deserialization of animation state,
 *              including frame index, animation timing, and sprite sheet
 *              layout parameters for scene saving and loading.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#include "Animation.h"
#include "Math/math.h"


namespace Framework
{
	bool Animation::IsSingleImage(const Animation& a) {
		return (a.rows <= 1 && a.columns <= 1) || a.totalFrames <= 1;
	}

	void Animation::Tick(float dt) {
		if (!Framework::GameState::IsPlaying()) return;

		if (IsSingleImage(*this)) {
			currentFrame = 0;
			elapsedTime = 0.0f;
			return;
		}

		elapsedTime += dt;
		float spf = (animationSpeed > 0.0f) ? (1.0f / animationSpeed) : 1.0f;
		while (elapsedTime >= spf) {
			elapsedTime -= spf;
			currentFrame = (currentFrame + 1) % std::max(1, totalFrames);
		}
	}


	/*Serialization && Deserialization */
	void Animation::Serialize(rapidjson::Value& out, rapidjson::Document::AllocatorType& alloc) const
	{
		using namespace rapidjson;
		out.SetObject();

		out.AddMember("currentFrame", currentFrame, alloc);
		out.AddMember("elapsedTime", elapsedTime, alloc);
		out.AddMember("totalFrames", totalFrames, alloc);
		out.AddMember("columns", columns, alloc);
		out.AddMember("rows", rows, alloc);
		out.AddMember("animationSpeed", animationSpeed, alloc);

	}

	void Animation::Deserialize(const rapidjson::Value& in)
	{
		if (!in.IsObject())
		{
			throw std::runtime_error("Invalid JSON for Transform");
		}

		if (in.HasMember("currentFrame"))
			currentFrame = in["currentFrame"].GetInt();
		if (in.HasMember("elapsedTime"))
			elapsedTime = in["elapsedTime"].GetFloat();
		if (in.HasMember("totalFrames"))
			totalFrames = in["totalFrames"].GetInt();
		if (in.HasMember("columns"))
			columns = in["columns"].GetInt();
		if (in.HasMember("rows"))
			rows = in["rows"].GetInt();
		if (in.HasMember("animationSpeed"))
			animationSpeed = in["animationSpeed"].GetFloat();
	}

	//bool Animation::operator==(Animation const& other) const {
	//	return id == other.id &&
	//		path == other.path &&
	//		currentFrame == other.currentFrame &&
	//		elapsedTime == other.elapsedTime &&
	//		totalFrames == other.totalFrames &&
	//		columns == other.columns &&
	//		rows == other.rows &&
	//		animationSpeed == other.animationSpeed;
	//}
	//bool Animation::operator!=(Animation const& other) const
	//{
	//	return !(*this == other);
	//}

	//std::ostream& operator<<(std::ostream& os, Animation const& t) {
	//	os << "[Animation Component values] "
	//		<< "id: \"" << t.id << "\", "
	//		<< "path: \"" << t.path << "\", "
	//		<< "currentFrame: " << t.currentFrame << ", "
	//		<< "elapsedTime: " << t.elapsedTime << ", "
	//		<< "totalFrames: " << t.totalFrames << ", "
	//		<< "columns: " << t.columns << ", "
	//		<< "rows: " << t.rows << ", "
	//		<< "animationSpeed: " << t.animationSpeed
	//		<< "\n";
	//	return os;
	//}

} // end of namespace
