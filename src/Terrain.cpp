//
//  Terrain.cpp
//  openGLDemo
//
//  Created by Will Fuchs on 3/11/20.
//  Copyright © 2020 wfuchs. All rights reserved.
//

#include <stdio.h>

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <array>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include "GLSL.h"
#include "Shader.h"
#include "Terrain.h"
#include "stb_image.h"


using namespace std;

Terrain::Terrain(const string &heightmap) {
    int width, height, nChannels;
    int resolution = 1;
    int heightScale = 10;
    
    unsigned char* data = stbi_load(heightmap.c_str(), &width, &height, &nChannels, 0);
    
    
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-300.0f, -30.0f, -300.0f));
    model = glm::scale(model, glm::vec3(3.0f, 3.0f, 3.0f));
    
    heightMap = vector<vector<float>>(width, vector<float>(height, 0));
    
    // init posBuf to num pixels * 3, b/c (x, y, z) for every pixel
    posBuf = vector<float>(width*height*3, 0);
    //init eleBuf to num triangular faces = (width-1) * (height-1) * 2, times 3 b/c 3 vertices per face
    eleBuf = vector<unsigned int>((width-1) * (height-1) * 6, 0);
    for (int r = 0; r < width; r++) {
        for (int c = 0; c < height; c++) {
            //x coordinate
            posBuf[3*(r*height+c) + 0] = r*resolution;
            //y coordinate
            posBuf[3*(r*height+c) + 1] = (data[r + height * c] / 255.0f) * heightScale;
            //z coordinate
            posBuf[3*(r*height+c) + 2] = c*resolution;
            
            // initialize height map
            heightMap[r][c] = posBuf[3*(r*height+c) + 1];
        }
    }

    //create faces
    for (int r = 0; r < width-1; r++) {
        for (int c = 0; c < height-1; c++) {
            //face 1 of rectangle
            eleBuf[6*(r*(height-1) + c) + 0] = (r*height+c);
            eleBuf[6*(r*(height-1) + c) + 1] = (r*height+(c+1));
            eleBuf[6*(r*(height-1) + c) + 2] = ((r+1)*height+(c+1));
            //face 2 of rectangle
            eleBuf[6*(r*(height-1) + c) + 3] = ((r+1)*height+(c+1));
            eleBuf[6*(r*(height-1) + c) + 4] = ((r+1)*height+c);
            eleBuf[6*(r*(height-1) + c) + 5] = (r*height+c);
        }
    }

    init();
}

float Terrain::getHeight(int x, int z) {
    return heightMap[x][z];
}

void Terrain::Draw(Shader* prog) const {
    int h_pos = -1, h_nor = -1;
    
    prog->setMat4("model", model);

    CHECKED_GL_CALL(glBindVertexArray(vaoID));
    h_pos = glGetAttribLocation(prog->ID, "vertPos");
    h_nor = glGetAttribLocation(prog->ID, "vertNor");
    
    // Bind position buffer
    GLSL::enableVertexAttribArray(h_pos);
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
    CHECKED_GL_CALL(glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0));

    // Bind normal buffer
    GLSL::enableVertexAttribArray(h_nor);
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
    CHECKED_GL_CALL(glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0));


    // Bind element buffer
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));

    // Draw
    CHECKED_GL_CALL(glDrawElements(GL_TRIANGLES, (int)eleBuf.size(), GL_UNSIGNED_INT, (const void *)0));

    GLSL::disableVertexAttribArray(h_nor);
    GLSL::disableVertexAttribArray(h_pos);
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}


void Terrain::init() {
    CHECKED_GL_CALL(glGenVertexArrays(1, &vaoID));
    CHECKED_GL_CALL(glBindVertexArray(vaoID));
    cout << "Terrain VAOID: " << vaoID << endl;

    // Send the position array to the GPU
    CHECKED_GL_CALL(glGenBuffers(1, &posBufID));
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, posBufID));
    CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, posBuf.size()*sizeof(float), &posBuf[0], GL_STATIC_DRAW));
    cout << "Terrain posBufID: " << posBufID << endl;
    
    // Send the normal array to the GPU
    if (norBuf.empty())
    {
        // allocate space for 1 normal per vertex
        norBuf.resize(posBuf.size(), 0);
        // compute normal for every face and add it to the vertices that define the face
        for (int i = 0; i < eleBuf.size()/3; i++) {
            int aInd = 3*eleBuf[i*3+0];
            int bInd = 3*eleBuf[i*3+1];
            int cInd = 3*eleBuf[i*3+2];

            glm::vec3 a = glm::vec3(posBuf[aInd+0], posBuf[aInd+1], posBuf[aInd+2]);
            glm::vec3 b = glm::vec3(posBuf[bInd+0], posBuf[bInd+1], posBuf[bInd+2]);
            glm::vec3 c = glm::vec3(posBuf[cInd+0], posBuf[cInd+1], posBuf[cInd+2]);
            glm::vec3 nor = glm::cross(b-a, c-a);
            
            //add normal to norBuf[a], [b], [c]
            norBuf[aInd+0] += nor.x;
            norBuf[aInd+1] += nor.y;
            norBuf[aInd+2] += nor.z;

            norBuf[bInd+0] += nor.x;
            norBuf[bInd+1] += nor.y;
            norBuf[bInd+2] += nor.z;

            norBuf[cInd+0] += nor.x;
            norBuf[cInd+1] += nor.y;
            norBuf[cInd+2] += nor.z;
        }
        // loop through all normals and normalize them
        for (int i=0; i < norBuf.size()/3; i++) {
            glm::vec3 nor = glm::normalize(glm::vec3(norBuf[i*3+0], norBuf[i*3+1], norBuf[i*3+2]));
            norBuf[i*3+0] = nor.x;
            norBuf[i*3+1] = nor.y;
            norBuf[i*3+2] = nor.z;
        }
    }
    CHECKED_GL_CALL(glGenBuffers(1, &norBufID));
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, norBufID));
    CHECKED_GL_CALL(glBufferData(GL_ARRAY_BUFFER, norBuf.size()*sizeof(float), &norBuf[0], GL_STATIC_DRAW));
    cout << "Terrain norBufID: " << norBufID << endl;
    
    // Send the element array to the GPU
    CHECKED_GL_CALL(glGenBuffers(1, &eleBufID));
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID));
    CHECKED_GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size()*sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW));
    cout << "Terrain eleBufID: " << eleBufID << endl;

    // Unbind the arrays
    CHECKED_GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
    CHECKED_GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
