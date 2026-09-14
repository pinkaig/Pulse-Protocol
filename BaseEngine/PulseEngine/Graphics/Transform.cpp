/******************************************************************************/
/**
 * @file        Transform.cpp
 * @project     Pulse Protocol
 * @author      Goh Pin Kai (primary) - 90%
 * @author 		Ban Kai Wei Benjamin (secondary) - 10%
 * @brief       Implements the Transform component, providing position,
 *              scale, rotation, animation, and sprite management. Includes
 *              update logic for entity transforms, matrix composition,
 *              and JSON-based serialization/deserialization for saving
 *              and loading transform state.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
#include "Transform.h"
#include "Math/math.h"
#include "CoreEngine/Core/CoreEngine.h" // for FilePathToGame

namespace Framework
{
	//constexpr float VIRTUAL_W = 1600.0f;
	//constexpr float VIRTUAL_H = 900.0f;
	Transform::Transform()
	{
		mdl_to_ndc_xform.SetToIdentity();
	}
	Transform::~Transform() {}

	void Transform::Initialize() {}

	void Transform::update(GLdouble delta_time, const std::set<Entity> entity)
	{
		auto *g_coordinator = Coordinator::GetInstance();

		for (auto it : entity)
		{
			// IMPORTANT: reference, not copy
			auto &x = g_coordinator->GetComponent<Framework::Transform>(it);

			// normalize from pixels to your game space
			const float PnormX = x.Pos.x;
			const float PnormY = x.Pos.y;
			const float SnormX = x.Scale.x;
			const float SnormY = x.Scale.y;

			Matrix3x3 Scaling, Rotating, Translating;

			// Scale
			Scaling = Matrix3x3(
				SnormX, 0.0f, 0.0f,
				0.0f, SnormY, 0.0f,
				0.0f, 0.0f, 1.0f);

			// Rotate
			constexpr float DEG_TO_RAD = 3.14159265358979323846f / 180.0f;
			const float cosA = std::cos(x.Rotation.x * DEG_TO_RAD);
			const float sinA = std::sin(x.Rotation.x * DEG_TO_RAD);
			Rotating = Matrix3x3(
				cosA, -sinA, 0.0f,
				sinA, cosA, 0.0f,
				0.0f, 0.0f, 1.0f);

			// Translate (column-major layout for your Matrix3x3)
			Translating = Matrix3x3(
				1.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f,
				PnormX, PnormY, 1.0f);

			// M = T * R * S
			Matrix3x3 ScreenToNDC(
				2.0f / static_cast<float>(GLApp::VIRTUAL_W), 0.0f, 0.0f,
				0.0f, 2.0f / static_cast<float>(GLApp::VIRTUAL_H), 0.0f,
				0.0f, 0.0f, 1.0f);

			x.mdl_to_ndc_xform = ScreenToNDC * (Translating * Rotating * Scaling);

			x.Rotation.x += x.Rotation.y * static_cast<GLfloat>(delta_time);
			// std::cout << x.mdl_to_ndc_xform;
		}
	}

	void Transform::draw() const {}

	/* Operator overloads */
	//comparison function operator !=
	bool Transform::operator==(Transform const& other) const
	{
		return
			Pos == other.Pos &&
			Scale == other.Scale &&
			Rotation == other.Rotation &&
			isVisible == other.isVisible &&
			visibilityTimer == other.visibilityTimer &&
			autoHide == other.autoHide;
	}
	bool Transform::operator!=(Transform const& other) const
	{
		return !(*this == other);
	}

	/*Serialization && Deserialization */
	void Transform::Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
	{
		using namespace rapidjson;
		out.SetObject();

		// Vector2s
		Value posVal, scaleVal, rotVal, renderVal;
		Pos.Serialize(posVal, alloc);
		Scale.Serialize(scaleVal, alloc);
		Rotation.Serialize(rotVal, alloc);

		out.AddMember("position", posVal, alloc);
		out.AddMember("scale", scaleVal, alloc);
		out.AddMember("rotation", rotVal, alloc);

		// simple types
		// out.AddMember("angle_disp", angle_disp, alloc);
		// out.AddMember("angle_speed", angle_speed, alloc);

		out.AddMember("isVisible", isVisible, alloc);
		out.AddMember("visibilityTimer", visibilityTimer, alloc);
		out.AddMember("autoHide", autoHide, alloc);

		// mat3
		//  Matrix3x3
		// Value matVal;
		// mdl_to_ndc_xform.Serialize(matVal, alloc);
		// out.AddMember("mdl_to_ndc_xform", matVal, alloc);
	}

	void Transform::Deserialize(const rapidjson::Value &in)
	{
		if (!in.IsObject())
		{
			std::cerr << "[Transform] Invalid JSON format\n";
			return;
		}

		if (in.HasMember("position"))
			Pos.Deserialize(in["position"]);
		if (in.HasMember("scale"))
			Scale.Deserialize(in["scale"]);
		if (in.HasMember("rotation"))
			Rotation.Deserialize(in["rotation"]);

		// if (in.HasMember("angle_disp"))
		//	angle_disp = in["angle_disp"].GetFloat();
		// if (in.HasMember("angle_speed"))
		//	angle_speed = in["angle_speed"].GetFloat();

		if (in.HasMember("isVisible"))
			isVisible = in["isVisible"].GetBool();
		if (in.HasMember("visibilityTimer"))
			visibilityTimer = in["visibilityTimer"].GetFloat();
		if (in.HasMember("autoHide"))
			autoHide = in["autoHide"].GetBool();

		// if (in.HasMember("mdl_to_ndc_xform"))
		//   mdl_to_ndc_xform.Deserialize(in["mdl_to_ndc_xform"]);
	}

	bool SaveTransform(Transform const &t, std::string const &path)
	{
		rapidjson::Document d;
		d.SetObject();
		auto &alloc = d.GetAllocator();

		rapidjson::Value tVal;
		t.Serialize(tVal, alloc);
		d.AddMember("transform", tVal, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		d.Accept(writer);

		// Ensure directory exists
		namespace fs = std::filesystem;
		fs::create_directories(FilePathToGame / "JSON");

		std::ofstream ofs(path);
		if (!ofs.is_open())
		{
			std::cerr << "Error: Cannot open file for writing: " << path << "\n";
			return false;
		}
		ofs << buffer.GetString();
		if (!ofs.good())
		{
			std::cerr << "Error: Failed to write JSON to file: " << path << "\n";
			return false;
		}
		return true;
	}

	bool LoadTransform(Transform &t, std::string const &path)
	{
		std::ifstream ifs(path);
		if (!ifs.is_open())
		{
			std::cerr << "Error: Cannot open file for reading: " << path << "\n";
			return false;
		}
		rapidjson::IStreamWrapper isw(ifs); // must have
		rapidjson::Document d;				// must have
		d.ParseStream(isw);					// must have

		if (d.HasParseError())
		{
			std::cerr << "Error: Failed to parse JSON in file: " << path << "\n";
			return false;
		}

		if (!d.HasMember("transform"))
		{
			std::cerr << "Error: No 'transform' field in file: " << path << "\n";
			return false;
		}

		t.Deserialize(d["transform"]);
		return true;
	}


	std::ostream& operator<<(std::ostream& os, Transform const& t) {
		os << "[Transform Component] contains these values\n";
		os << "Transform pos:" << t.Pos << "\n";
		os << "Transform scale:" << t.Scale << "\n";
		os << "Transform rot:" << t.Rotation << "\n";

		return os;
	}


} // end of namespace
