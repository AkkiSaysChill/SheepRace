#pragma once
#include "game/Aircraft.h"
#include "tinyxml2.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

class AircraftDatabase {
public:
    static AircraftDatabase& GetInstance() {
        static AircraftDatabase instance;
        return instance;
    }

    bool LoadFromMemory(const void* buffer, size_t size) {
        if (!buffer || size == 0) return false;
        tinyxml2::XMLDocument doc;
        if (doc.Parse(static_cast<const char*>(buffer), size) != tinyxml2::XML_SUCCESS) {
            std::cerr << "Failed to parse aircraft XML from packed memory!\n";
            return false;
        }
        return ParseDocument(doc);
    }

    bool LoadFromFile(const std::string& filepath) {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(filepath.c_str()) != tinyxml2::XML_SUCCESS) {
            std::cerr << "Failed to load aircraft data from: " << filepath << "\n";
            return false;
        }
        return ParseDocument(doc);
    }

    bool GetStats(const std::string& planeId, AircraftStats& outStats) {
        auto it = m_planes.find(planeId);
        if (it != m_planes.end()) {
            outStats = it->second;
            return true;
        }
        return false;
    }

    const std::vector<std::string>& GetAllPlaneIds() const { return m_planeIds; }

private:
    AircraftDatabase() = default;

    static float ParseFloatElement(tinyxml2::XMLElement* parent, const char* name, float defaultVal) {
        if (!parent) return defaultVal;
        auto elem = parent->FirstChildElement(name);
        return elem ? elem->FloatText(defaultVal) : defaultVal;
    }

    static bool ParseBoolElement(tinyxml2::XMLElement* parent, const char* name, bool defaultVal) {
        if (!parent) return defaultVal;
        auto elem = parent->FirstChildElement(name);
        if (!elem || !elem->GetText()) return defaultVal;
        std::string val = elem->GetText();
        return (val == "true" || val == "1" || val == "TRUE");
    }

    static int ParseIntElement(tinyxml2::XMLElement* parent, const char* name, int defaultVal) {
        if (!parent) return defaultVal;
        auto elem = parent->FirstChildElement(name);
        return elem ? elem->IntText(defaultVal) : defaultVal;
    }

    bool ParseDocument(tinyxml2::XMLDocument& doc) {
        tinyxml2::XMLElement* root = doc.FirstChildElement("AircraftDatabase");
        if (!root) return false;

        m_planes.clear();
        m_planeIds.clear();

        for (tinyxml2::XMLElement* planeElem = root->FirstChildElement("Plane");
             planeElem != nullptr;
             planeElem = planeElem->NextSiblingElement("Plane")) {
            
            const char* idAttr = planeElem->Attribute("id");
            if (!idAttr) continue;

            AircraftStats stats{};
            stats.id = idAttr;

            auto nameElem = planeElem->FirstChildElement("Name");
            stats.name = (nameElem && nameElem->GetText()) ? nameElem->GetText() : "Unknown Aircraft";

            auto modelElem = planeElem->FirstChildElement("ModelFile");
            stats.modelFile = (modelElem && modelElem->GetText()) ? modelElem->GetText() : "plane.glb";

            // Flight dynamics parameters
            stats.maxThrustSpeed      = ParseFloatElement(planeElem, "MaxThrustSpeed", 95.0f);
            stats.stallSpeedThreshold = ParseFloatElement(planeElem, "StallSpeedThreshold", 20.0f);
            stats.throttleResponse    = ParseFloatElement(planeElem, "ThrottleResponse", 8.0f);
            stats.brakeDecel          = ParseFloatElement(planeElem, "BrakeDecel", 14.0f);
            stats.baseDragFactor      = ParseFloatElement(planeElem, "BaseDragFactor", 1.2f);
            stats.climbDragMultiplier = ParseFloatElement(planeElem, "ClimbDragMultiplier", 38.0f);
            stats.glideSinkRate       = ParseFloatElement(planeElem, "GlideSinkRate", 1.2f);
            stats.bankSinkStrength    = ParseFloatElement(planeElem, "BankSinkStrength", 3.2f);
            stats.pitchRate           = ParseFloatElement(planeElem, "PitchRate", 1.05f);
            stats.rollRate            = ParseFloatElement(planeElem, "RollRate", 1.10f);
            stats.yawRate             = ParseFloatElement(planeElem, "YawRate", 0.55f);
            stats.pitchInertia        = ParseFloatElement(planeElem, "PitchInertia", 1.8f);
            stats.bankNoseSlip        = ParseFloatElement(planeElem, "BankNoseSlip", 0.35f);

            // Combat & Weapon parameters
            bool hasWeapons = ParseBoolElement(planeElem, "HasWeapons", false);
            if (stats.id == "jet_mk2") hasWeapons = true; // Fail-safe default for combat airframe
            stats.hasWeapons = hasWeapons;

            stats.minigunFireRate     = ParseFloatElement(planeElem, "MinigunFireRate", 0.06f);
            stats.bulletSpeed         = ParseFloatElement(planeElem, "BulletSpeed", 1100.0f);
            stats.bulletDamage        = ParseFloatElement(planeElem, "BulletDamage", 18.0f);
            stats.missileCooldownTime = ParseFloatElement(planeElem, "MissileCooldownTime", 2.5f);
            stats.maxMissiles         = ParseIntElement(planeElem, "MaxMissiles", 6);

            m_planes[idAttr] = stats;
            m_planeIds.push_back(idAttr);
        }
        return !m_planes.empty();
    }

    std::unordered_map<std::string, AircraftStats> m_planes;
    std::vector<std::string> m_planeIds;
};