/******************************************************************************/
/**
 * @file        FontObj.h
 * @project     Pulse Protocol
 * @author      Leu Jun Yong
 * @brief       Declares a simple font resource class for storing font
				identifiers and file paths.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/
#pragma once
#include "pch/pch_temp.h"

namespace Framework
{
	class Font {
		std::string FontID;
		std::string FontPath;

		virtual void Initialize() {
		}

		void SetFont(std::string t) {
			FontID = t;
		}
		void SetFontPath(std::string t) {
			FontPath = t;
		}
		std::string GetFontID() const
		{
			return FontID;
		}
		std::string GetFontPath() const
		{
			return FontPath;
		}

		//void Serialize(Value& data, Document::AllocatorType& alloc) override
		//{
		//	data.SetObject();
		//	data.AddMember("FontID", StringRef(FontID.c_str()), alloc);
		//}

		//void Deserialize(Value& data)
		//{
		//	SetFont(data["FontID"].GetString());
		//}
	};
}