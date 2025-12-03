//#include <pugixml.hpp>
#include <nlohmann\json.hpp>

#include "project.h"
#include "util.h"
#include "test.h"
#include "resource/script.h"
#include "resource/object.h"
#include "error.h"
#include "parser.h"

#include <iostream>
#include <fstream>

#include "yypreader.cpp"

const char* RESOURCE_TYPE_NAMES[] = {
  "sprite",
  "sound",
  "background",
  "path",
  "script",
  "shader",
  "font",
  "timeline",
  "object",
  "room",
  "constant",
  "tileset",
  "sequence",
  "audiogroup",
  "notes"
};

const char* RESOURCE_TREE_NAMES[] = {
  "sprites",
  "sounds",
  "backgrounds",
  "paths",
  "scripts",
  "shaders",
  "fonts",
  "timelines",
  "objects",
  "rooms",
  "constants",
  "tilesets",
  "sequences",
  "audiogroups",
  "notes"
};

ResourceTableEntry::ResourceTableEntry(ResourceType r, std::string path): type(r), path(path), ptr(nullptr)
{ }

ResourceTableEntry::ResourceTableEntry(ResourceType r, Resource* ptr): type(r), ptr(ptr)
{ }

ResourceTableEntry::ResourceTableEntry(const ResourceTableEntry& r): type(r.type), path(r.path), ptr(r.ptr)
{ }

Resource& ResourceTableEntry::get() {
  if (ptr)
    return *ptr;
  // if resource not realized, construct it:
  switch (type) {
    case SCRIPT:
      ptr = new ResScript(path);
      break;
    case OBJECT:
      ptr = new ResObject(path);
      break;
  }
  return *ptr;
}

Project::Project(std::string path): root(path_directory(path)), project_file(path_leaf(path))
{ }

void Project::read_project_file() {
  //pugi::xml_document doc;
  //pugi::xml_parse_result result = doc.load_file((root + project_file).c_str());
  
    GMProject gm_project = YYPReader::parseProject(root + project_file);

  std::cout<<"Reading project file " << root<<std::endl;
  std::cout << "Project: " << gm_project.projectName << std::endl;
  std::cout << "IDE Version: " << gm_project.ideVersion << std::endl;
  
  new (&resourceTree) ResourceTree();
  
  // Clear existing resources
  resourceTable.clear();
  resourceTree.list.clear();

  // Create root trees for each resource type
  for (int r = 0; r < NONE; r++) {
      resourceTree.list.push_back(ResourceTree());
      resourceTree.list.back().type = (ResourceType)r;
  }

  // Process resources from YYP
  for (const auto& resource : gm_project.resources) {
      ResourceType type = determine_resource_type(resource.path);

      if (type != NONE) {
          std::string path = root + resource.path;

          if (type == SCRIPT) {
			  path.replace(path.find(".yy"), 3, ".gml");
          }

          // Create resource table entry
          ResourceTableEntry rte(type, path);
          resourceTable.insert(std::make_pair(resource.name, rte));

          // Add to resource tree (simplified - you may want to preserve folder structure)
          resourceTree.list[type].list.push_back(ResourceTree());
          ResourceTree& leaf = resourceTree.list[type].list.back();
          leaf.type = type;
          leaf.rtkey = resource.name;
          leaf.is_leaf = true;
      }
  }

  // Process room order
  if (!gm_project.roomOrder.empty()) {
      std::cout << "Room order: ";
      for (const auto& room : gm_project.roomOrder) {
          std::cout << room.name << " ";
      }
      std::cout << std::endl;
  }

  /*pugi::xml_node assets = doc.child("assets");
  
  for (int r = 0; r < NONE; r++) {
    int prev_rte_size = resourceTable.size();
    ResourceType r_type = (ResourceType)r;
    read_resource_tree(resourceTree, &assets, r_type);
    bool add = false;
    if (resourceTree.list.empty()){
      add = true;
    }
    else if (resourceTree.list.back().type != r_type) {
      add = true;
    }
    if (add)
      resourceTree.list.push_back(ResourceTree());
    
    std::cout<<"Added "<<resourceTable.size() - prev_rte_size<<" "<<RESOURCE_TREE_NAMES[r_type]<<std::endl;
  }*/
}

ResourceType Project::determine_resource_type(const std::string& path) {
    if (path.find("sprites/") == 0) return SPRITE;
    if (path.find("sounds/") == 0) return SOUND;
    if (path.find("backgrounds/") == 0) return BACKGROUND;
    if (path.find("paths/") == 0) return PATH;
    if (path.find("scripts/") == 0) return SCRIPT;
    if (path.find("shaders/") == 0) return SHADER;
    if (path.find("fonts/") == 0) return FONT;
    if (path.find("timelines/") == 0) return TIMELINE;
    if (path.find("objects/") == 0) return OBJECT;
    if (path.find("rooms/") == 0) return ROOM;
	if (path.find("constants/") == 0) return CONSTANT;
    if (path.find("tilesets/") == 0) return TILESET;
    if (path.find("sequences/") == 0) return SEQUENCE;
    if (path.find("audiogroups/") == 0) return AUDIOGROUP;
    if (path.find("notes/") == 0) return NOTE;

    return NONE;
}

const char* RESOURCE_EXTENSION[] = {
  "", //sprites
  "", //sounds
  "", //backgrounds
  "", //paths
  ".gml", // scripts already have .gml listed in the project file
  "", //shaders
  "", //fonts
  "", //timelines
  ".gml", //objects
  "", //rooms
  "" // constants do not have file associations
  "",         // tileset
  "",         // sequence
  "",         // audiogroup
  ""          // notes
};

/*void Project::read_resource_tree(ResourceTree& root, void* xml_v, ResourceType t) {
  pugi::xml_document& xml = *(pugi::xml_document*)xml_v;

  for (pugi::xml_node node: xml.children()) {
    // subtree
    if (node.name() == std::string(RESOURCE_TREE_NAMES[t])) {
      root.list.push_back(ResourceTree());
      root.list.back().type = t;
      ResourceTree& rt = root.list.back();
      read_resource_tree(rt, &node, t);
    }
    
    // actual resource
    if (node.name() == std::string(RESOURCE_TYPE_NAMES[t])) {
      //! add resourceTable entry
      root.list.push_back(ResourceTree());
      root.list.back().type = t;
      std::string value = node.text().get();
      ResourceTableEntry rte(t,this->root + value + RESOURCE_EXTENSION[t]);
      std::string name;
      if (t == CONSTANT) {
        // constants are defined directly in the .project.gmx file; no additional cost
        // to realizing them immediately.
        ResConstant* res = new ResConstant();
        res->value = value;
        new(&rte) ResourceTableEntry(t,res);
        
        // determine resource name:
        name = node.attribute("name").value();
      } else {
        // determine resource name:
        name = path_leaf(value);
      }
      // insert resource table entry
      resourceTable.insert(std::make_pair(name, rte));
      root.list.back().rtkey = name;
      root.list.back().is_leaf = true;
    }
  }
}*/

void Project::beautify(BeautifulConfig bc, bool dry) {
  beautify_script_tree(bc, dry, resourceTree.list[SCRIPT]);
  beautify_object_tree(bc, dry, resourceTree.list[OBJECT]);
  
  if (dry) {
    std::cout<<"Dry run succeeded."<<std::endl;
  }
}

void Project::beautify_script_tree(BeautifulConfig bc, bool dry, ResourceTree& tree) {
  if (!tree.is_leaf) {
    for (auto iter : tree.list) {
      beautify_script_tree(bc, dry, iter);
    }    
  } else {
    resourceTable[tree.rtkey].get().beautify(bc, dry);
  }
}

void Project::beautify_object_tree(BeautifulConfig bc, bool dry, ResourceTree& tree) {
  if (!tree.is_leaf) {
    for (auto iter : tree.list) {
      beautify_object_tree(bc, dry, iter);
    }    
  } else {
    resourceTable[tree.rtkey].get().beautify(bc, dry);
  }
}