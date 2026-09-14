/******************************************************************************/
/**
 * @file        Serialization.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Implements JSON serialization and deserialization for engine
 *              configuration and entities (e.g., Config, NPC). Provides
 *              save/load utilities for writing and reading game data files.
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without
 *              the prior written consent of DigiPen Institute of Technology is
 *              prohibited.
 */
 /******************************************************************************/

#include "Serialization.h"
#include "pch/pch_temp.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

int Config::GetWindowWidth() const
{
    return WindowWidth;
}
int Config::GetWindowHeight() const
{
    return WindowHeight;
}
std::string Config::GetConfigPath() const
{
    return ConfigPath;
}
std::string Config::GetFirstScenePath() const
{
    return FirstScene;
}


void Config::SetWindowWidth(int w) { WindowWidth = w; }
void Config::SetWindowHeight(int h) { WindowHeight = h; }
void Config::SetConfigPath(std::string path) { ConfigPath = std::move(path); }
void Config::SetFirstScenePath(std::string path) { FirstScene = std::move(path); }

void Config::Serialize(rapidjson::Value &out, rapidjson::Document::AllocatorType &alloc) const
{
    using namespace rapidjson;
    out.SetObject();

    out.AddMember("WindowWidth", GetWindowWidth(), alloc);
    out.AddMember("WindowHeight", GetWindowHeight(), alloc);
    Value pathVal;
    pathVal.SetString(GetConfigPath().c_str(),
                      static_cast<SizeType>(GetConfigPath().length()),
                      alloc);
    out.AddMember("ConfigPath", pathVal, alloc);

    pathVal.SetString(GetConfigPath().c_str(),
        static_cast<SizeType>(GetConfigPath().length()),
        alloc);
    out.AddMember("FirstScene", pathVal, alloc);
}
void Config::Deserialize(const rapidjson::Value &in)
{
    if (!in.IsObject())
    {
        std::cerr << "[Config] Invalid JSON format\n";
        return;
    }

    if (in.HasMember("WindowWidth") && in["WindowWidth"].IsNumber())
    {
        SetWindowWidth(in["WindowWidth"].GetInt());
    }
    if (in.HasMember("WindowHeight") && in["WindowHeight"].IsNumber())
    {
        SetWindowHeight(in["WindowHeight"].GetInt());
    }
    if (in.HasMember("ConfigPath") && in["ConfigPath"].IsString())
    {
        SetConfigPath(in["ConfigPath"].GetString());
    }
    if (in.HasMember("FirstScene") && in["FirstScene"].IsString())
    {
        SetConfigPath(in["FirstScene"].GetString());
    }
}

bool SaveConfig(Config const &cfg, std::string const &path, bool pretty)
{
    // {"config": { ... }}
   //rapidjson::Document d = JSONUtils::MakeObjDocument(cfg, "config");
   //return JSONUtils::WriteJSONToFile(d, path, pretty);
   return JSONUtils::SaveObject(cfg, path, "config", pretty);
}


bool LoadConfig(Config &cfg, std::string const &path)
{
    //std::ifstream ifs(path);
    //if (!ifs.is_open())
    //{
    //    std::cerr << "Error: Cannot open file for reading: " << path << "\n";
    //    return false;
    //}
    //rapidjson::IStreamWrapper isw(ifs);
    //rapidjson::Document d;
    //d.ParseStream(isw);

    //if (d.HasParseError())
    //{
    //    std::cerr << "Error: Failed to parse JSON in file: " << path << "\n";
    //    return false;
    //}

    //if (!d.HasMember("config"))
    //{
    //    std::cerr << "Error: No -config- field in file : " << path << "\n";
    //    return false;
    //}

    //cfg.Deserialize(d["config"]);
    //return true;

    return JSONUtils::LoadObject(cfg, path, "config");
}

namespace JSONUtils {
    using namespace rapidjson;
    //bool pretty means use PrettyWriter or just Writer
    //PrettyWriter takes more file space but more readable
    //Writer makes everything one liner
    std::string JSONToString(Document const& d, bool pretty) {
        StringBuffer buffer;
        if (pretty) {
            PrettyWriter<StringBuffer> w(buffer);
            w.SetIndent(' ', 2); //space after : for Key :"indent size" Value
            d.Accept(w);
        }
        else {
            Writer<StringBuffer> w(buffer);
            d.Accept(w);
        }
        return buffer.GetString();
    }

    //bool pretty means use PrettyWriter or just Writer
    //PrettyWriter takes more file space but more readable
    //Writer makes everything one liner
    bool WriteJSONToFile(Document const& d, std::filesystem::path const& path, bool pretty) {
        namespace fs = std::filesystem;
        std::error_code ec;
        //fs::create_directories("../../PulseEngine/JSON", ec);
        //path.parent_path() gets ../../PulseEngine/JSON
        //error code so dont throw exception
        fs::create_directories(path.parent_path(), ec);

        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) {
            std::cerr << "Error: Cannot open file for writing: " << path << "\n";
            return false;
        }

        //rapidjson::StringBuffer buffer;
        //rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        //d.Accept(writer);
        //ostreamwrapper vs stringbuffer
        //ostreamwrapper use less memory not easily grab string after
        //stringbuffer use extra memory but can inspect reuse,log json
        //StringBuffer sbuffer;
        //bool writter_success = false;
        //if (pretty) {

        //   PrettyWriter<StringBuffer> pw(sbuffer);
        //   //adds 2 spaces at \n, before {} or kv pairs
        //   pw.SetIndent(' ', 2);
        //   writter_success = d.Accept(pw);
        //}
        //else {
        //    Writer<StringBuffer> w(sbuffer);
        //    writter_success = d.Accept(w);
        //}

        //if (!writter_success) {
        //    std::cerr << "Error: RapidJSON Accept() failed\n";
        //    return false;
        //}


        std::string json = JSONToString(d, pretty);
        //ofs << sbuffer.GetString();

        //low-level way to write bytes
        ofs.write(json.data(), static_cast<std::streamsize>(json.size()));
        ofs.flush(); //write data in buffer now
        if (!ofs.good())
        {
            std::cerr << "Error: Failed to write JSON to file: " << path << "\n";
            return false;
        }
        return true;
    }

    /*loading/parsing*/
    bool ReadJSONFromFile(Document& outDoc,
        std::filesystem::path const& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
        {
            std::cerr << "Error: Cannot open file for reading: " << path << "\n";
            return false;
        }

        IStreamWrapper isw(ifs);
        //outDoc.ParseStream(isw);
        //IMPORTANT ALLOW COMMENTS
        outDoc.ParseStream<rapidjson::kParseCommentsFlag>(isw);

        if (outDoc.HasParseError())
        {
            std::cerr << "Error: JSON parse error in " << path
                << " : " << GetParseError_En(outDoc.GetParseError())
                << " (offset " << outDoc.GetErrorOffset() << ")\n";
            return false;
        }
        return true;
    }

}//end of namespace JSONUtils
