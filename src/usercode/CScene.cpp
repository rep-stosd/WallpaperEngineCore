#include "deps/simdjson.h"
#include "deps/xxhash.h"
#include "TextureLoader.hpp"
#include "CScene.hpp"

std::string GetBundleFilePath(const std::string& filename);

void Scene::parseEffects(Model* model, const std::string& path) {
    
}

void Scene::parseMaterial(Material& mat, const std::string& path) {
    if(!std::filesystem::exists(sceneRootPath + path ) )
        return;

    simdjson::dom::parser parser;
    simdjson::dom::element matRoot = parser.load(sceneRootPath + path); // todo: filesystem

    for (auto matPass : matRoot["passes"].get_array()) {

        if (matPass["textures"].is_array()) {
            for (auto texture : matPass["textures"].get_array()) {
                if (texture.is_string()) {
                    std::string path = texture.get_string().value().data();
                    path += ".tex";
                    mat.texture = new MTLTexture();
                    auto img = PAKImage_Alloc(sceneRootPath + "materials/" + path);
                    
                    if (!img.name.size())
                        img = PAKImage_Alloc(GetBundleFilePath("assets/materials/" + path));
                        
                    if (img.name.size()) {
                        mat.width = img.width;
                        mat.height = img.height;
                        mat.texWidth = img.texWidth;
                        mat.texHeight = img.texHeight;
                        
                        if (img.fiFormat == FIF_MP4) {
                            (new MatVideoComponent(mat))->init(img);
                        }
                        else
                            PAKImage_GLUpload(img, *mat.texture);
                        
                        PAKImage_Free(img);
                    }
                    else
                        *mat.texture = TextureLoader::createUncompressedTexture("7jx.png");
                    
                    materials[XXH3_64bits(img.name.data(), img.name.length())] = &mat;
                    
                }
            }

        }
    }

}


void Scene::parseImage(ImageLayer* render, const std::string& path) {
    if(!std::filesystem::exists(sceneRootPath + path))
        return;

    puts((sceneRootPath + path).data());
    simdjson::dom::parser parser;
    simdjson::dom::element root = parser.load(sceneRootPath + path); // todo: filesystem

    render->material = {};

    if (root["cropoffset"].is_string()) {
        auto originStr = root["cropoffset"].get_string().value().data();
        std::stringstream ss(originStr);
            
        float x, y, z;
        ss >> x >> y >> z;
        render->cropOffset.x = x;
        render->cropOffset.y = y;
    }

    if (root["material"].is_string()) {
        parseMaterial(render->material, root["material"].get_string().value().data() );
    }
    
}


void Scene::parseParticle(ParticleLayer* render, const std::string& path) {
    if(!std::filesystem::exists(sceneRootPath + path))
        return;
    
    render->rate = 1;

    puts((sceneRootPath + path).data());
    simdjson::dom::parser parser;
    simdjson::dom::element root = parser.load(sceneRootPath + path);
    
    if (root["material"].is_string()) {
        parseMaterial(render->material, root["material"].get_string().value().data() );
    }
    
    if (root["maxcount"].is_uint64()) {
        render->maxCount = (int)root["maxcount"].get_uint64().value();
    }
    else {
        render->maxCount = 8000;
    }
    
    
    if (root["emitter"].is_array())
        for (auto object : root["emitter"].get_array()) {
            if (object["name"].is_string()) {
                if (object["distancemin"].is_uint64())
                    render->distanceMinX = render->distanceMinY = (int)object["distancemin"].get_uint64().value();
                else if (object["distancemin"].is_string()) {
                    auto min = object["distancemin"].get_c_str().value();
                    char* end;
                    
                    render->distanceMinX = (int)std::strtol(min, &end, 10);
                    render->distanceMinY = (int)std::strtol(end, &end, 10);
                }
                else
                    render->distanceMinX = render->distanceMinY = 0;
                
                if (object["origin"].is_string()) {
                    auto min = object["origin"].get_c_str().value();
                    char* end;
                    
                    render->origin.x = (int)std::strtol(min, &end, 10);
                    render->origin.y = (int)std::strtol(end, &end, 10);
                }
                else
                    render->origin = {0,0,0};
                
                if (object["distancemax"].is_uint64())
                    render->distanceMaxX = render->distanceMaxY = (int)object["distancemax"].get_uint64().value();
                else if (object["distancemax"].is_string()) {
                    auto min = object["distancemax"].get_c_str().value();
                    char* end;
                    
                    render->distanceMaxX = (int)std::strtol(min, &end, 10);
                    render->distanceMaxY = (int)std::strtol(end, &end, 10);
                }
                else
                    render->distanceMaxX = render->distanceMaxY = 0;
                
                if (object["sign"].is_string()) {
                    auto min = object["sign"].get_c_str().value();
                    char* end;
                    
                    render->signX = (int)std::strtol(min, &end, 10);
                    render->signY = (int)std::strtol(end, &end, 10);
                }
                
                
                if (object["rate"].is_uint64())
                    render->rate = (int)object["rate"].get_uint64().value()/60;
                
                switch (render->signX) {
                    case 0:
                        render->distanceMinX = -render->distanceMaxX;
                        break;
                    case -1:
                        render->distanceMinX = -render->distanceMinX;
                        render->distanceMaxX = -render->distanceMaxX;
                        break;
                }
                
                switch (render->signY) {
                    case 0:
                        render->distanceMinY = -render->distanceMaxY;
                        break;
                    case -1:
                        render->distanceMinY = -render->distanceMinY;
                        render->distanceMaxY = -render->distanceMaxY;
                        break;
                }
                
            }
        }
    
    
    if (root["initializer"].is_array())
        for (auto object : root["initializer"].get_array()) {
            if (object["name"].is_string()) {
                std::string name = object["name"].get_c_str().value();
                if (name == "velocityrandom") {
                    auto sz = object["min"].is_string() ? object["min"].get_c_str().value() : "1.0 1.0";
                    char* end;
                    
                    render->velocityMinX = (int)std::strtol(sz, &end, 10);
                    render->velocityMinY = (int)std::strtol(end, &end, 10);
                  //  auto z = std::strtol(end, nullptr, 10);
                    
                    sz = object["max"].is_string() ? object["max"].get_c_str().value() : "1.0 1.0";
                    
                    render->velocityMaxX = (int)std::strtol(sz, &end, 10);
                    render->velocityMaxY = (int)std::strtol(end, &end, 10);
                  //  z = std::strtol(end, nullptr, 10);
                    
                }
                else if (name == "sizerandom") {
                    render->sizeMin = object["min"].is_uint64() ? (int)object["min"].get_uint64().value() : 1;
                    render->sizeMax = object["max"].is_uint64() ? (int)object["max"].get_uint64().value() : 1;
                }
            }
        }
    
}


glm::vec2 recurseModelPosition(Model* mdl) {
    auto translatedOrigin = glm::vec2(mdl->origin.x, mdl->origin.y);

    if (mdl->parent && mdl->parentId != -1) {
        return translatedOrigin+ recurseModelPosition(mdl->parent);
    }
    return translatedOrigin;
}


glm::vec2 recurseModelScale(Model* mdl) {
    auto scale = glm::vec2(mdl->scale.x, mdl->scale.y);
   if (mdl->parent && mdl->parentId != -1) {
        return scale * recurseModelScale(mdl->parent);
   }
    return scale;
}

void Scene::init(const Scene::Desc& desc) {
    destroy();
    
    switch(desc.type) {
        case 0:
            PAKFile_LoadAndDecompress( (std::string(desc.folderPath) + "/scene.pkg").data());
            initForPkg((Wallpaper64GetStorageDir() + "tmp_scene").data());
            break;
        case 1:
            initForVideo(desc.folderPath + "/" + desc.file);
            break;
    }
}

void Scene::initForPkg(const std::string& path) {

    sceneRootPath = path + "/";

    simdjson::dom::parser parser;
    simdjson::dom::element root = parser.load(sceneRootPath + "scene.json");
    for (auto& i : root.get_object()) {
        if (i.key == "objects")
            for (auto object : i.value.get_array()) {



                Model* model = new Model();

                if (object["scale"].is_string()) {
                    auto originStr = object["scale"].get_string().value().data();
                    std::stringstream ss(originStr);
                    
                    float x, y, z;
                    ss >> x >> y >> z;

                    model->scale = glm::vec3(x,y, 1.f);
                }
                else
                    model->scale = glm::vec3(1.f,1.f,1.f);

                //    "origin" : "1920.00000 1080.00000 0.00000",
                if (object["origin"].is_string()) {
                    auto originStr = object["origin"].get_string().value().data();
                    std::stringstream ss(originStr);
                    
                    float x, y, z;
                    ss >> x >> y >> z;


                    model->origin = glm::vec3(x,y,z);
                }

                //    "origin" : "1920.00000 1080.00000 0.00000",
                if (object["size"].is_string()) {
                    auto originStr = object["size"].get_string().value().data();
                    std::stringstream ss(originStr);
                    
                    float x, y;
                    ss >> x >> y;

                    model->size = glm::vec3(x,y,1);
                }
                else
                    model->size = glm::vec3(1.f, 1.f,1.f);

          //      model->cropOffset = glm::vec3(0,0,0);
                
                //"image" : "models/util/composelayer.json",
                if (object["image"].is_string()) {
                    auto imageStr = object["image"].get_string().value().data();
                    model->layer = new ImageLayer();
                    parseImage((ImageLayer*)model->layer, imageStr);
                    
                 //   auto& rend = ((ImageRender*)model->render)->material;
                   // model->size *= glm::vec3((float)rend.width/rend.texWidth, (float)rend.height/rend.texHeight, 0.f);
                   
                }
                
                if (object["particle"].is_string()) {
                    auto imageStr = object["particle"].get_string().value().data();
                    model->layer = new ParticleLayer();
                    parseParticle((ParticleLayer*)model->layer, imageStr);
                }

                // has parent
               if (object["parent"].is_int64()) {
                    auto id = object["parent"].get_int64().value();

                   model->parentId = id;
                }
                else
                  model->parentId = -1;

            
                model->xOffset = 0;
                model->yOffset = 0;

               if (object["id"].is_int64()) {
                    auto id = object["id"].get_int64().value();
                   // printf("INFO : add model %d \n", id);
                    model->id = id;
                    modelsById[id] = model;
                }

                model->xOffset = model->yOffset = 1;
                if (object["alignment"].is_string()) {
                    std::string alignment = object["alignment"].get_string().value().data();

                    if (alignment.find("top") != std::string::npos) {
                        model->yOffset = 0;
                    } else if (alignment.find("bottom") != std::string::npos) {
                        model->yOffset = 2;
                    }

                    if (alignment.find("left") != std::string::npos) {
                        model->xOffset = 0;
                    } else if (alignment.find("right") != std::string::npos) {
                        model->xOffset = 2;
                    }
                    printf("WARNING: alignment found! %d %d \n", model->xOffset, model->yOffset);
                    
                }
                
                if (object["name"].is_string()) {
                    
                    model->name = object["name"].get_c_str().value();
                }

                models.push_back(model);


            }
        else if (i.key == "general") {
            if (i.value["orthogonalprojection"].is_object()) {
       //         camSize.x = i.value["orthogonalprojection"].get_object()["width"].get_int64().value();
      //          camSize.y = i.value["orthogonalprojection"].get_object()["height"].get_int64().value();
            }
        }
        else if (i.key == "camera") {
            if (i.value["center"].is_string()) {
                auto originStr = i.value["center"].get_string().value().data();
                std::stringstream ss(originStr);
                
                float x, y, z;
                ss >> x >> y >> z;

           //     camPos = glm::vec3(x,y,z);
            }
        }
    }

    // resolve parent
    for (int i = 0; i < models.size(); i++) {
         models[models.size()-i-1]->zOrder = -i;
        if (models[i] && models[i]->parentId != -1) {
            models[i]->parent = modelsById[models[i]->parentId];
        }
    }

    for (auto& i : models) {
        i->recursedScale = recurseModelScale(i);
        i->recursedOrigin = recurseModelPosition(i);
    }
    
}


void Scene::initForVideo(const std::string& path) {
    assert(!("UNIMPLEMENTED AS OF YET!"));
    /*
        Model* model = new Model();
        model->render = new VideoRender();
        ((VideoRender*)model->render)->init(path);
        model->scale = {1,1, 0};
        model->size = {1920,1200, 0};
        models.push_back(model);
    */
}
    

// i have no idea who might use this (just use your own shell wallpaper) but its included for consistency purposes
void Scene::initForPicture(const std::string& path) {
    assert(!("UNIMPLEMENTED AS OF YET!"));
    /*
    Model* model = new Model();
    model->render = new ImageRender();
    *model->render->material.texture = TextureLoader::createUncompressedTexture(path);
    model->scale = {1,1, 0};
    model->size = {1920,1200, 0};
    models.push_back(model);
     */
}
    

void Scene::update() {
    for (auto& i : materials) {
        for (auto* j : i.second->components) {
            j->update();
        }
    }
    for (auto& i : models) {
        if (i->layer) {
            i->layer->update();
        }
    }
}

void Scene::destroy() {
    for (auto& i : models) {
        if (i->layer) {
            i->layer->destroy();
            if (i->layer->material.texture) {
                if (i->layer->material.texture->_pTexture)
                    i->layer->material.texture->destroy();
                delete i->layer->material.texture;
            }
            delete i->layer;
        }
        delete i;
    }
    models.clear();
    modelsById.clear();
    materials.clear();
}
