//
//  Terrain.h
//  openGLDemo
//
//  Created by Will Fuchs on 3/11/20.
//  Copyright © 2020 wfuchs. All rights reserved.
//

#ifndef Terrain_h
#define Terrain_h

struct TerrainVertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 normal;
};

class Terrain {
public:
    Terrain(std::string const &path);
    void Draw(Shader* shader) const;
    void init();
    float getHeight(int x, int z);
private:
    std::vector<unsigned int> eleBuf;
    std::vector<float> posBuf;
    std::vector<float> norBuf;
    std::vector<std::vector<float>> heightMap;
    glm::mat4 model;
    unsigned int eleBufID = 0;
    unsigned int posBufID = 0;
    unsigned int norBufID = 0;
    unsigned int vaoID = 0;
    std::vector<TerrainVertex> vertices;
};

#endif /* Terrain_h */
