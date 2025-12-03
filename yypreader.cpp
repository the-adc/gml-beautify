#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <regex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct GMResourceId {
    std::string name;
    std::string path;
};

struct GMFolder {
    std::string name;
    std::string folderPath;
    std::string resourceType;
    std::string resourceVersion;
};

struct GMAudioGroup {
    std::string name;
    std::string resourceType;
    std::string resourceVersion;
};

struct GMTextureGroup {
    std::string name;
    std::string compressFormat;
    std::string resourceType;
    std::string resourceVersion;
};

struct GMProject {
    std::string projectName;
    std::string ideVersion;
    bool isEcma;
    int defaultScriptType;

    std::vector<GMFolder> folders;
    std::vector<GMResourceId> resources;
    std::vector<GMResourceId> roomOrder;
    std::vector<GMAudioGroup> audioGroups;
    std::vector<GMTextureGroup> textureGroups;

    // Statistics
    std::map<std::string, int> resourceCountByType;
};

std::string cleanGameMakerJSON(const std::string& input) {
    // Remove trailing commas before closing braces and brackets
    std::string result = input;

    // Remove trailing commas before }
    std::regex pattern1(",\\s*\\}");
    result = std::regex_replace(result, pattern1, "}");

    // Remove trailing commas before ]
    std::regex pattern2(",\\s*\\]");
    result = std::regex_replace(result, pattern2, "]");

    return result;
}

class YYPReader {
public:
    static GMProject parseProject(const std::string& filepath) {
        GMProject project;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }

        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());

        std::string cleaned = cleanGameMakerJSON(content);

        json data = json::parse(cleaned);

        // Parse basic project info
        project.projectName = data.value("name", "");
        project.isEcma = data.value("isEcma", false);
        project.defaultScriptType = data.value("defaultScriptType", 1);

        // Parse IDE version from metadata
        if (data.contains("MetaData") && data["MetaData"].contains("IDEVersion")) {
            project.ideVersion = data["MetaData"]["IDEVersion"];
        }

        // Parse folders
        if (data.contains("Folders")) {
            for (const auto& folder : data["Folders"]) {
                GMFolder f;
                f.name = folder.value("name", "");
                f.folderPath = folder.value("folderPath", "");
                f.resourceType = folder.value("resourceType", "");
                f.resourceVersion = folder.value("resourceVersion", "");
                project.folders.push_back(f);
            }
        }

        // Parse resources
        if (data.contains("resources")) {
            for (const auto& resource : data["resources"]) {
                if (resource.contains("id")) {
                    GMResourceId rid;
                    rid.name = resource["id"].value("name", "");
                    rid.path = resource["id"].value("path", "");
                    project.resources.push_back(rid);

                    // Count by file extension/type
                    std::string ext = getResourceTypeFromPath(rid.path);
                    project.resourceCountByType[ext]++;
                }
            }
        }

        // Parse room order
        if (data.contains("RoomOrderNodes")) {
            for (const auto& roomNode : data["RoomOrderNodes"]) {
                if (roomNode.contains("roomId")) {
                    GMResourceId rid;
                    rid.name = roomNode["roomId"].value("name", "");
                    rid.path = roomNode["roomId"].value("path", "");
                    project.roomOrder.push_back(rid);
                }
            }
        }

        // Parse audio groups
        if (data.contains("AudioGroups")) {
            for (const auto& audioGroup : data["AudioGroups"]) {
                GMAudioGroup ag;
                ag.name = audioGroup.value("name", "");
                ag.resourceType = audioGroup.value("resourceType", "");
                ag.resourceVersion = audioGroup.value("resourceVersion", "");
                project.audioGroups.push_back(ag);
            }
        }

        // Parse texture groups
        if (data.contains("TextureGroups")) {
            for (const auto& textureGroup : data["TextureGroups"]) {
                GMTextureGroup tg;
                tg.name = textureGroup.value("name", "");
                tg.compressFormat = textureGroup.value("compressFormat", "");
                tg.resourceType = textureGroup.value("resourceType", "");
                tg.resourceVersion = textureGroup.value("resourceVersion", "");
                project.textureGroups.push_back(tg);
            }
        }

        std::cout << "Loaded successfully." << std::endl;
        return project;
    }

    static std::string getResourceTypeFromPath(const std::string& path) {
        // Extract folder name from path to determine type
        size_t firstSlash = path.find('/');
        if (firstSlash != std::string::npos) {
            return path.substr(0, firstSlash);
        }
        return "unknown";
    }

    static void printProjectSummary(const GMProject& project) {
        std::cout << "===================================\n";
        std::cout << "GameMaker Studio 2 Project Summary\n";
        std::cout << "===================================\n\n";

        std::cout << "Project: " << project.projectName << "\n";
        std::cout << "IDE Version: " << project.ideVersion << "\n";
        std::cout << "ECMAScript: " << (project.isEcma ? "Yes" : "No") << "\n";
        std::cout << "Default Script Type: " << project.defaultScriptType << "\n\n";

        std::cout << "=== Folder Structure ===\n";
        for (const auto& folder : project.folders) {
            std::cout << "  " << folder.name
                << " (" << folder.resourceType << ")\n";
        }
        std::cout << "\n";

        std::cout << "=== Resource Counts ===\n";
        for (const auto& [type, count] : project.resourceCountByType) {
            std::cout << "  " << type << ": " << count << "\n";
        }
        std::cout << "Total Resources: " << project.resources.size() << "\n\n";

        std::cout << "=== Room Order ===\n";
        for (size_t i = 0; i < project.roomOrder.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << project.roomOrder[i].name << "\n";
        }
        std::cout << "\n";

        std::cout << "=== Audio Groups ===\n";
        for (const auto& ag : project.audioGroups) {
            std::cout << "  " << ag.name << "\n";
        }
        std::cout << "\n";

        std::cout << "=== Texture Groups ===\n";
        for (const auto& tg : project.textureGroups) {
            std::cout << "  " << tg.name
                << " [Compression: " << tg.compressFormat << "]\n";
        }
    }

    static void exportResourceList(const GMProject& project, const std::string& outputFile) {
        std::ofstream out(outputFile);

        out << "Resource List for: " << project.projectName << "\n";
        out << "Generated from .yyp file\n\n";

        // Group resources by type
        std::map<std::string, std::vector<GMResourceId>> groupedResources;
        for (const auto& resource : project.resources) {
            std::string type = getResourceTypeFromPath(resource.path);
            groupedResources[type].push_back(resource);
        }

        // Write grouped resources
        for (const auto& [type, resources] : groupedResources) {
            out << "=== " << type << " (" << resources.size() << ") ===\n";
            for (const auto& resource : resources) {
                out << "  " << resource.name << "\n";
                out << "    Path: " << resource.path << "\n";
            }
            out << "\n";
        }

        out.close();
        std::cout << "Resource list exported to: " << outputFile << "\n";
    }
};