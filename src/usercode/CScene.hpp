#pragma once

#include "stdafx.hpp"
#include "PAK.h"
#include "Material.hpp"

#define LAYER_TYPE_IMAGE 0
#define LAYER_TYPE_PARTICLE 1
#define LAYER_TYPE_VIDEO 2
#define LAYER_TYPE_NODRAW -1

class Layer {
public:
    virtual void update() {};
    virtual void destroy() {};
    virtual void draw() {};
    virtual ~Layer() {}
    Material material;
    std::string name;
    
    int type;
};

class ImageLayer : public Layer {
public:
    glm::vec2 cropOffset;
};

class ParticleLayer : public Layer  {
public:
    struct ParticleInstance {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 velocity;
        float size;
    };
    std::vector<ParticleInstance> insts;
    
    int maxCount, rate;
    int signX, signY;
    int distanceMinX, distanceMaxX;
    int distanceMinY, distanceMaxY;
    
    int velocityMinX, velocityMaxX;
    int velocityMinY, velocityMaxY;
    int sizeMin, sizeMax;
    glm::vec3 origin;
    
    void update(const glm::vec3& parentPos);
};

struct Model
{
    
};

struct Node {
    uint64_t id;
    uint64_t parentId;
    Node* parent;
    glm::vec3 size;
    glm::vec3 scale;
    glm::vec3 origin;
    int xOffset, yOffset;
    int zOrder;
    bool inheritParent;
    
    Layer* layer;
    
    glm::vec2 recursedOrigin, recursedScale;
    
    std::string name;
};


class Scene {
public:
    struct Desc {
        int type = 0;
        std::string file;
        std::string folderPath;
        
        std::string title;
        uint64_t id;
    };
    
    void init(const Desc& desc);
    void initForPkg(const std::string& path);
    void initForVideo(const std::string& path);
    void initForPicture(const std::string& path);
    void initForWeb(const std::string& path);
    void update();
    void destroy();

    void parseMaterial(Material&, const std::string& path);
    void parseImage(ImageLayer* model, const std::string& path);
    
    void parseModel(Model& model, const std::string& path);
    void parseParticle(ParticleLayer* model, const std::string& path);
    void parseEffects(Model* model, const std::string& path);

    std::unordered_map<uint64_t, MTLTexture*> textures;
    std::unordered_map<uint64_t, Material*> materials;
    
    
    std::unordered_map<uint64_t, Node*> nodesById;
    std::vector<Node*> nodes;

    std::string sceneRootPath;

};
