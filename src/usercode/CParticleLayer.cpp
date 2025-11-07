
#include "CScene.hpp"
#include <random>

int randomInt(int min, int max) {
    static thread_local std::minstd_rand gen(std::random_device{}()); // fast LCG
    uint32_t u = gen();
    int64_t range = int64_t(max) - int64_t(min) + 1; range = range == 0 ? 1 : range;
    return min + int(u % range);
}

void ParticleLayer::update(const glm::vec3& parentPos) {
    if (insts.size() >= maxCount) {
        insts.erase(insts.begin(), insts.begin() + (insts.size() - maxCount));
    }
    
    for (int i = 0; i < rate+1; i++) {
        
        ParticleInstance inst = {};
        inst.pos = parentPos + glm::vec3(randomInt(distanceMinX, distanceMaxX),randomInt(distanceMinY, distanceMaxY),0);
        inst.velocity = glm::vec3(randomInt(velocityMinX, velocityMaxX), randomInt(velocityMinY, velocityMaxY)-5, 0);
        inst.size = randomInt(sizeMin, sizeMax);
        insts.push_back(inst);
    }
    
    for (auto& inst : insts) {
        inst.pos += inst.velocity;
    }
}
