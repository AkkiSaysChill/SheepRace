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
            stats.name = nameElem && nameElem->GetText() ? nameElem->GetText() : "Unknown Aircraft";

            auto modelElem = planeElem->FirstChildElement("ModelFile");
            stats.modelFile = modelElem && modelElem->GetText() ? modelElem->GetText() : "plane.glb";

            stats.maxThrustSpeed      = planeElem->FirstChildElement("MaxThrustSpeed")->FloatText(95.0f);
            stats.stallSpeedThreshold = planeElem->FirstChildElement("StallSpeedThreshold")->FloatText(20.0f);
            stats.throttleResponse    = planeElem->FirstChildElement("ThrottleResponse")->FloatText(8.0f);
            stats.brakeDecel          = planeElem->FirstChildElement("BrakeDecel")->FloatText(14.0f);
            stats.baseDragFactor      = planeElem->FirstChildElement("BaseDragFactor")->FloatText(1.2f);
            stats.climbDragMultiplier = planeElem->FirstChildElement("ClimbDragMultiplier")->FloatText(38.0f);
            stats.glideSinkRate       = planeElem->FirstChildElement("GlideSinkRate")->FloatText(1.2f);
            stats.bankSinkStrength    = planeElem->FirstChildElement("BankSinkStrength")->FloatText(3.2f);
            stats.pitchRate           = planeElem->FirstChildElement("PitchRate")->FloatText(1.05f);
            stats.rollRate            = planeElem->FirstChildElement("RollRate")->FloatText(1.10f);
            stats.yawRate             = planeElem->FirstChildElement("YawRate")->FloatText(0.55f);
            stats.pitchInertia        = planeElem->FirstChildElement("PitchInertia")->FloatText(1.8f);
            stats.bankNoseSlip        = planeElem->FirstChildElement("BankNoseSlip")->FloatText(0.35f);

            m_planes[idAttr] = stats;
            m_planeIds.push_back(idAttr);
        }
        return true;
    }

    std::unordered_map<std::string, AircraftStats> m_planes;
    std::vector<std::string> m_planeIds;
};