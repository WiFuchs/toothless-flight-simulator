#ifndef MESH_H
#define MESH_H

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Shader.h"

#include <string>
#include <array>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
using namespace std;

const int MAX_BONES_VERTEX = 4;
// max bones that can be read - must match vertex shader
const int MAX_BONES = 110;

//TODO add skinned mesh data to vertices (bone weights)
struct Vertex {
    // position
    glm::vec3 Position;
    // normal
    glm::vec3 Normal;
    // texCoords
    glm::vec2 TexCoords;
    // tangent
    glm::vec3 Tangent;
    // bitangent
    glm::vec3 Bitangent;
    // weights of bones affecting this vertex
    float boneWeights[MAX_BONES_VERTEX] = {0};
    // id's of said bones, needed to index into the transformations array in vert shader
    int boneIds[MAX_BONES_VERTEX] = {0};
    //num bones affecting this vertex
    unsigned int numBones = 0;
};

struct Texture {
    unsigned int id;
    string type;
    string path;
};

struct Bone {
    // location in final array
    int loc;
    // position transformations keyframes
    std::map<double, glm::vec3> positionKeys;
    // scale transformations keyframes
    std::map<double, glm::vec3> scaleKeys;
    // rotation transformations keyframes
    std::map<double, glm::quat> rotationKeys;
    // vector of children Bones
    std::vector<std::shared_ptr<Bone>> children;
    // distinguish non bone nodes, such as root
    bool isBone;
    
    // offset from parent in bind pose, may be unnecessary depending on assimp
    glm::mat4 boneOffset;
    
    //TODO only works when animation always has keyframe at 0
    glm::mat4 getTransform(double time) {
        
        //  return identity if no keyframes (root node)
        if (positionKeys.size() <= 1) {
            return glm::mat4(1.0f);
        }
        
        glm::vec3 interpolatedPos;
        glm::vec3 interpolatedScale;
        glm::quat interpolatedRot;
        // find position keyframes and interpolate between them
        for (auto it = positionKeys.begin(); it != positionKeys.end(); it++) {
            if (it->first > time) {
                auto prev = (it == positionKeys.begin()) ? std::prev(positionKeys.end()) : it--;
                double deltaT = (time - prev->first) / (it->first - prev->first);
                interpolatedPos = glm::mix(prev->second, it->second, deltaT);
                break;
            }
        }
        
        // find scale keyframes and interpolate between them
        for (auto it = scaleKeys.begin(); it != scaleKeys.end(); it++) {
            if (it->first > time) {
                auto prev = (it == scaleKeys.begin()) ? std::prev(scaleKeys.end()) : it--;
                double deltaT = (time - prev->first) / (it->first - prev->first);
                interpolatedScale = glm::mix(prev->second, it->second, deltaT);
                break;
            }
        }
        
        // find rotation keyframes and interpolate between them
        for (auto it = rotationKeys.begin(); it != rotationKeys.end(); it++) {
            if (it->first > time) {
                auto prev = (it == rotationKeys.begin()) ? std::prev(rotationKeys.end()) : it--;
                float deltaT = (time - prev->first) / (it->first - prev->first);
                glm::quat previous = prev->second;
                glm::quat next = it->second;
                
                if (glm::dot(previous, next) < 0) {
                    previous = -previous;
                }
                
                interpolatedRot = glm::mix(previous, next, deltaT);
                break;
            }
        }
        
        // construct transformation matrix
        glm::mat4 identity = glm::mat4(1.0f);
        return glm::translate(identity, interpolatedPos) * glm::scale(identity, interpolatedScale) * glm::toMat4(interpolatedRot);
    }
};

class Mesh {
public:
    /*  Mesh Data  */
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;
    unsigned int VAO;

    /*  Functions  */
    // constructor
    Mesh(vector<Vertex> vertices, vector<unsigned int> indices, vector<Texture> textures)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }

    // render the mesh
    void Draw(Shader shader, std::array<glm::mat4, 110> *animationTransforms)
    {
        // bind appropriate textures
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;
        for(unsigned int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
            // retrieve texture number (the N in diffuse_textureN)
            string number;
            string name = textures[i].type;
            if(name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if(name == "texture_specular")
                number = std::to_string(specularNr++); // transfer unsigned int to stream
            else if(name == "texture_normal")
                number = std::to_string(normalNr++); // transfer unsigned int to stream
             else if(name == "texture_height")
                number = std::to_string(heightNr++); // transfer unsigned int to stream

            // now set the sampler to the correct texture unit
            glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
            // and finally bind the texture
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
            
        }
        
        // draw mesh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        if (animationTransforms) {
            // bind animation buffer
            glUniformBlockBinding(shader.ID, glGetUniformBlockIndex(shader.ID, "AnimationBlock"), 0);
            glBindBuffer(GL_UNIFORM_BUFFER, ABO);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, MAX_BONES * sizeof(glm::mat4), animationTransforms);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
        }

        // always good practice to set everything back to defaults once configured.
        glActiveTexture(GL_TEXTURE0);
    }

private:
    /*  Render data  */
    unsigned int VBO, EBO, ABO;

    /*  Functions    */
    // initializes all the buffer objects/arrays
    void setupMesh()
    {
        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        
        // set up UBO for animation buffer object
        glGenBuffers(1, &ABO);
        glBindBuffer(GL_UNIFORM_BUFFER, ABO);
        glBufferData(GL_UNIFORM_BUFFER, MAX_BONES * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        // define the range of the buffer that links to a uniform binding point
        glBindBufferRange(GL_UNIFORM_BUFFER, 0, ABO, 0, MAX_BONES * sizeof(glm::mat4));
        
        

        glBindVertexArray(VAO);
        // load data into vertex buffers
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        // A great thing about structs is that their memory layout is sequential for all its items.
        // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
        // again translates to 3/2 floats which translates to a byte array.
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // set the vertex attribute pointers
        // vertex Positions
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // vertex normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        // vertex texture coords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
        // vertex tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        // vertex bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        // bone ids
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
        // bone weights
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));

        glBindVertexArray(0);
    }
};
#endif
