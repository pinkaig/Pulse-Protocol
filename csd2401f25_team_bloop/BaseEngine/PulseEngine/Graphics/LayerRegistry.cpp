/******************************************************************************/
/**
 * @file        LayerRegistry.cpp
 * @project     Pulse Protocol
 * @author      Chloe Lau Rey En (primary) - 89%
 * @author      Ban Kai Wei Benjamin (secondary) - 10%
 * @author      Goh Pin Kai (secondary) - 1%
 * 
 * @brief       Implementation of centralized layer registry
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "pch/pch_temp.h"
#include "LayerRegistry.h"
#include "CoreEngine/Core/CoreEngine.h" // for FilePathToGame

// -------------------------------------------------------------------------
// Singleton Access
// -------------------------------------------------------------------------
LayerRegistry& LayerRegistry::Get()
{
    static LayerRegistry instance;
    return instance;
}

// -------------------------------------------------------------------------
// Helper: Get config file path (same location as scene JSONs)
// -------------------------------------------------------------------------
static std::string GetLayerConfigPath()
{
    return (FilePathToGame / "JSON" / "layers.json").string();
}

// -------------------------------------------------------------------------
// Initialization
// -------------------------------------------------------------------------
void LayerRegistry::Initialize()
{
    if (m_initialized) return;

    // Try to load from config file first
    std::string configPath = GetLayerConfigPath();
    if (LoadConfig(configPath))
    {
        //std::cout << "[LayerRegistry] Loaded layer config from: " << configPath << "\n";
        return; // Successfully loaded, don't use defaults
    }

    // No config file exists - use defaults
    //std::cout << "[LayerRegistry] No config file found at: " << configPath << ", using defaults\n";
    m_layers.clear();

    // Background layer
    LayerInfo bgLayer;
    bgLayer.name = "Background";
    bgLayer.mask = LAYER_BACKGROUND;
    bgLayer.visible = true;
    bgLayer.locked = false;
    bgLayer.isDefault = true;
    bgLayer.colorR = 0.6f;
    bgLayer.colorG = 0.8f;
    bgLayer.colorB = 0.6f;
    bgLayer.colorA = 1.0f;
    m_layers.push_back(bgLayer);

    // World layer
    LayerInfo worldLayer;
    worldLayer.name = "World";
    worldLayer.mask = LAYER_WORLD;
    worldLayer.visible = true;
    worldLayer.locked = false;
    worldLayer.isDefault = true;
    worldLayer.colorR = 0.8f;
    worldLayer.colorG = 0.8f;
    worldLayer.colorB = 1.0f;
    worldLayer.colorA = 1.0f;
    m_layers.push_back(worldLayer);

    // UI layer
    LayerInfo uiLayer;
    uiLayer.name = "UI";
    uiLayer.mask = LAYER_UI;
    uiLayer.visible = true;
    uiLayer.locked = false;
    uiLayer.isDefault = true;
    uiLayer.colorR = 1.0f;
    uiLayer.colorG = 0.8f;
    uiLayer.colorB = 0.6f;
    uiLayer.colorA = 1.0f;
    m_layers.push_back(uiLayer);

    LayerInfo textLayer;
    textLayer.name = "Text";
    textLayer.mask = LAYER_TEXT;
    textLayer.visible = true;
    textLayer.locked = false;
    textLayer.isDefault = true;
    textLayer.colorR = 1.0f;
    textLayer.colorG = 1.0f;
    textLayer.colorB = 1.0f;
    textLayer.colorA = 1.0f;
    m_layers.push_back(textLayer);

    m_initialized = true;
    //std::cout << "[LayerRegistry] Initialized with " << m_layers.size() << " default layers\n";
}

// -------------------------------------------------------------------------
// Layer Query API
// -------------------------------------------------------------------------
const std::vector<LayerInfo>& LayerRegistry::GetAllLayers() const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }
    return m_layers;
}

std::vector<LayerInfo>& LayerRegistry::GetAllLayersMutable()
{
    // Auto-initialize if needed
    if (!m_initialized) {
        Initialize();
    }
    return m_layers;
}

const LayerInfo* LayerRegistry::GetLayerByMask(LayerMask mask) const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    for (const auto& layer : m_layers)
    {
        if (layer.mask == mask)
            return &layer;
    }
    return nullptr;
}

LayerInfo* LayerRegistry::GetLayerByMaskMutable(LayerMask mask)
{
    // Auto-initialize if needed
    if (!m_initialized) {
        Initialize();
    }

    for (auto& layer : m_layers)
    {
        if (layer.mask == mask)
            return &layer;
    }
    return nullptr;
}

const LayerInfo* LayerRegistry::GetLayerByName(const std::string& name) const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    for (const auto& layer : m_layers)
    {
        if (layer.name == name)
            return &layer;
    }
    return nullptr;
}

std::string LayerRegistry::GetLayerName(LayerMask mask) const
{
    // Handle special case
    if (mask == LAYER_ALL)
        return "All";

    const LayerInfo* layer = GetLayerByMask(mask);
    if (layer)
        return layer->name;

    return "Unknown";
}

LayerMask LayerRegistry::GetLayerMask(const std::string& name) const
{
    // Handle special case
    if (name == "All")
        return LAYER_ALL;

    const LayerInfo* layer = GetLayerByName(name);
    if (layer)
        return layer->mask;

    // Default to World if not found
    return LAYER_WORLD;
}

int LayerRegistry::GetLayerIndex(LayerMask mask) const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        if (m_layers[i].mask == mask)
            return static_cast<int>(i);
    }
    return 1; // Default to World (index 1)
}

LayerMask LayerRegistry::GetLayerMaskFromIndex(int index) const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    if (index >= 0 && index < static_cast<int>(m_layers.size()))
        return m_layers[index].mask;
    return LAYER_WORLD;
}

size_t LayerRegistry::GetLayerCount() const
{
    return m_layers.size();
}

// -------------------------------------------------------------------------
// Layer Modification API
// -------------------------------------------------------------------------
void LayerRegistry::SetLayerName(LayerMask mask, const std::string& newName)
{
    LayerInfo* layer = GetLayerByMaskMutable(mask);
    if (layer && !newName.empty())
    {
        std::string oldName = layer->name;
        layer->name = newName;
        //std::cout << "[LayerRegistry] Renamed layer '" << oldName << "' to '" << newName << "'\n";

        // Auto-save to persist changes
        SaveConfig(GetLayerConfigPath());
    }
}

void LayerRegistry::SetLayerVisibility(LayerMask mask, bool visible)
{
    // Auto-initialize if needed
    if (!m_initialized) {
        Initialize();
    }

    LayerInfo* layer = GetLayerByMaskMutable(mask);
    if (layer)
    {
        layer->visible = visible;
    }
}

void LayerRegistry::SetLayerLocked(LayerMask mask, bool locked)
{
    LayerInfo* layer = GetLayerByMaskMutable(mask);
    if (layer)
    {
        layer->locked = locked;
    }
}

void LayerRegistry::SetLayerColor(LayerMask mask, float r, float g, float b, float a)
{
    LayerInfo* layer = GetLayerByMaskMutable(mask);
    if (layer)
    {
        layer->colorR = r;
        layer->colorG = g;
        layer->colorB = b;
        layer->colorA = a;
    }
}

LayerMask LayerRegistry::AddCustomLayer(const std::string& name)
{
    // Auto-initialize if needed
    if (!m_initialized) {
        Initialize();
    }

    if (name.empty())
        return 0;

    // Check if name already exists
    if (GetLayerByName(name) != nullptr)
    {
        //std::cerr << "[LayerRegistry] Layer name '" << name << "' already exists\n";
        return 0;
    }

    LayerMask newMask = FindNextAvailableMask();
    if (newMask == 0)
    {
        //std::cerr << "[LayerRegistry] No more layer slots available\n";
        return 0;
    }

    LayerInfo newLayer;
    newLayer.name = name;
    newLayer.mask = newMask;
    newLayer.visible = true;
    newLayer.locked = false;
    newLayer.isDefault = false;

    // Generate a color based on mask (for visual variety)
    float hue = static_cast<float>(newMask % 7) / 7.0f;
    newLayer.colorR = 0.5f + hue * 0.5f;
    newLayer.colorG = 0.7f;
    newLayer.colorB = 1.0f - hue * 0.3f;
    newLayer.colorA = 1.0f;

    m_layers.push_back(newLayer);
    //std::cout << "[LayerRegistry] Added custom layer '" << name << "' with mask " << newMask << "\n";

    // Print all current layers for verification
    //std::cout << "[LayerRegistry] Current layers (" << m_layers.size() << " total):\n";
    for (const auto& layer : m_layers)
    {
        std::cout << "  [" << layer.mask << "] " << layer.name << (layer.isDefault ? " (default)" : " (custom)") << "\n";
    }

    // Auto-save to persist changes
    SaveConfig(GetLayerConfigPath());

    return newMask;
}

bool LayerRegistry::RemoveCustomLayer(LayerMask mask)
{
    if (!CanRemoveLayer(mask))
        return false;

    auto it = std::find_if(m_layers.begin(), m_layers.end(),
        [mask](const LayerInfo& layer) { return layer.mask == mask; });

    if (it != m_layers.end())
    {
        //std::cout << "[LayerRegistry] Removed layer '" << it->name << "'\n";
        m_layers.erase(it);

        // Auto-save to persist changes
        SaveConfig(GetLayerConfigPath());

        return true;
    }
    return false;
}

bool LayerRegistry::CanRemoveLayer(LayerMask mask) const
{
    const LayerInfo* layer = GetLayerByMask(mask);
    if (!layer)
        return false;

    // Cannot remove default layers
    return !layer->isDefault;
}

LayerMask LayerRegistry::FindNextAvailableMask() const
{
    // Find all used masks
    LayerMask usedMasks = 0;
    for (const auto& layer : m_layers)
    {
        usedMasks |= layer.mask;
    }

    // Find first unused bit starting from bit 3 (custom layers)
    // Bits 0-2 are reserved for Background, World, UI
    LayerMask candidate = 1u << 3; // Start at bit 3 (value 8)

    while (candidate != 0 && candidate < 0x80000000u)
    {
        if ((usedMasks & candidate) == 0)
            return candidate;
        candidate <<= 1;
    }

    return 0; // No available slots
}

// -------------------------------------------------------------------------
// Visibility API
// -------------------------------------------------------------------------
bool LayerRegistry::IsLayerVisible(LayerMask mask) const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    const LayerInfo* layer = GetLayerByMask(mask);
    return layer ? layer->visible : true;
}

bool LayerRegistry::IsLayerLocked(LayerMask mask) const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    const LayerInfo* layer = GetLayerByMask(mask);
    return layer ? layer->locked : false;
}

LayerMask LayerRegistry::ComputeVisibleLayerMask() const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    LayerMask result = 0;
    for (const auto& layer : m_layers)
    {
        if (layer.visible)
            result |= layer.mask;
    }
    return result;
}

void LayerRegistry::SetAllLayersVisible(bool visible)
{
    // Auto-initialize if needed
    if (!m_initialized) {
        Initialize();
    }

    for (auto& layer : m_layers)
    {
        layer.visible = visible;
    }
}

void LayerRegistry::ShowAllLayers()
{
    SetAllLayersVisible(true);
}

void LayerRegistry::HideAllLayers()
{
    SetAllLayersVisible(false);
}

void LayerRegistry::UnlockAllLayers()
{
    // Auto-initialize if needed
    if (!m_initialized) {
        Initialize();
    }

    for (auto& layer : m_layers)
    {
        layer.locked = false;
    }
}

// -------------------------------------------------------------------------
// Serialization
// -------------------------------------------------------------------------
void LayerRegistry::SaveConfig(const std::string& filepath) const
{
    using namespace rapidjson;

    // Create directory if it doesn't exist
    std::filesystem::path filePath(filepath);
    std::filesystem::path dirPath = filePath.parent_path();

    if (!dirPath.empty() && !std::filesystem::exists(dirPath))
    {
        std::error_code ec;
        std::filesystem::create_directories(dirPath, ec);
        if (ec)
        {
            std::cerr << "[LayerRegistry] Failed to create directory: " << dirPath << " - " << ec.message() << "\n";
            return;
        }
        //std::cout << "[LayerRegistry] Created directory: " << dirPath << "\n";
    }

    Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    // Add version for future compatibility
    doc.AddMember("version", 1, alloc);

    // Create layers array
    Value layersArray(kArrayType);

    for (const auto& layer : m_layers)
    {
        Value layerObj(kObjectType);

        Value nameVal;
        nameVal.SetString(layer.name.c_str(), static_cast<SizeType>(layer.name.length()), alloc);
        layerObj.AddMember("name", nameVal, alloc);

        layerObj.AddMember("mask", layer.mask, alloc);
        layerObj.AddMember("visible", layer.visible, alloc);
        layerObj.AddMember("locked", layer.locked, alloc);
        layerObj.AddMember("isDefault", layer.isDefault, alloc);

        // Color
        Value colorObj(kObjectType);
        colorObj.AddMember("r", layer.colorR, alloc);
        colorObj.AddMember("g", layer.colorG, alloc);
        colorObj.AddMember("b", layer.colorB, alloc);
        colorObj.AddMember("a", layer.colorA, alloc);
        layerObj.AddMember("color", colorObj, alloc);

        layersArray.PushBack(layerObj, alloc);
    }

    doc.AddMember("layers", layersArray, alloc);

    // Write to file
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::ofstream file(filepath);
    if (file.is_open())
    {
        file << buffer.GetString();
        file.close();
        //std::cout << "[LayerRegistry] Saved config to " << filepath << "\n";

        // Print what was saved for verification
        //std::cout << "[LayerRegistry] Saved " << m_layers.size() << " layers:\n";
        //for (const auto& layer : m_layers)
        //{
        //    std::cout << "  - " << layer.name << " (mask=" << layer.mask << ", default=" << (layer.isDefault ? "yes" : "no") << ")\n";
        //}
    }
    else
    {
        std::cerr << "[LayerRegistry] Failed to save config to " << filepath << "\n";
    }
}

bool LayerRegistry::LoadConfig(const std::string& filepath)
{
    using namespace rapidjson;

    std::ifstream file(filepath);
    if (!file.is_open())
    {
        //std::cout << "[LayerRegistry] Config file not found: " << filepath << ", using defaults\n";
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    Document doc;
    if (doc.Parse(content.c_str()).HasParseError())
    {
        std::cerr << "[LayerRegistry] Failed to parse config file\n";
        return false;
    }

    if (!doc.HasMember("layers") || !doc["layers"].IsArray())
    {
        std::cerr << "[LayerRegistry] Invalid config format\n";
        return false;
    }

    m_layers.clear();

    const auto& layersArray = doc["layers"];
    for (SizeType i = 0; i < layersArray.Size(); ++i)
    {
        const auto& layerObj = layersArray[i];
        LayerInfo layer;

        if (layerObj.HasMember("name") && layerObj["name"].IsString())
            layer.name = layerObj["name"].GetString();

        if (layerObj.HasMember("mask") && layerObj["mask"].IsUint())
            layer.mask = layerObj["mask"].GetUint();

        if (layerObj.HasMember("visible") && layerObj["visible"].IsBool())
            layer.visible = layerObj["visible"].GetBool();

        if (layerObj.HasMember("locked") && layerObj["locked"].IsBool())
            layer.locked = layerObj["locked"].GetBool();

        if (layerObj.HasMember("isDefault") && layerObj["isDefault"].IsBool())
            layer.isDefault = layerObj["isDefault"].GetBool();

        if (layerObj.HasMember("color") && layerObj["color"].IsObject())
        {
            const auto& colorObj = layerObj["color"];
            if (colorObj.HasMember("r")) layer.colorR = colorObj["r"].GetFloat();
            if (colorObj.HasMember("g")) layer.colorG = colorObj["g"].GetFloat();
            if (colorObj.HasMember("b")) layer.colorB = colorObj["b"].GetFloat();
            if (colorObj.HasMember("a")) layer.colorA = colorObj["a"].GetFloat();
        }

        m_layers.push_back(layer);
    }

    m_initialized = true;
    //std::cout << "[LayerRegistry] Loaded " << m_layers.size() << " layers from config\n";
    return true;
}

std::vector<const char*> LayerRegistry::GetLayerNamesForCombo() const
{
    // Auto-initialize if needed
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    std::vector<const char*> names;
    names.reserve(m_layers.size());

    for (const auto& layer : m_layers)
    {
        names.push_back(layer.name.c_str());
    }

    return names;
}

//Undo Layer
LayerRegistrySnapshot LayerRegistry::MakeSnapshot() const
{
    if (!m_initialized) {
        const_cast<LayerRegistry*>(this)->Initialize();
    }

    LayerRegistrySnapshot snap;
    snap.layers = m_layers;
    return snap;
}
void LayerRegistry::RestoreSnapshot(LayerRegistrySnapshot const& snap)
{
    if (!m_initialized) {
        Initialize();
    }

    m_layers = snap.layers;

    SaveConfig(GetLayerConfigPath());
}
