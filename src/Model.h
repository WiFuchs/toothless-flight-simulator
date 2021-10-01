#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>
#include <glm/gtx/rotate_vector.hpp>


#include <typeinfo>

#include "Mesh.h"
#include "Shader.h"
#include "Util.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

unsigned int TextureFromFile(const char *path, const string &directory, bool gamma = false);

class Model
{
public:
    /*  Model Data */
    vector<Texture> textures_loaded;    // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
    vector<Mesh> meshes;
    glm::mat4 inverseBindTransform;
    float debugTime = 0;
    string directory;
    bool gammaCorrection;
    shared_ptr<Bone> boneRoot;
    std::array<glm::mat4, 110> animationTransforms = {};
    glm::mat4 model = glm::mat4(1.0f);
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 moveDir = glm::vec3(0.0f, 0.0f, -1.0f);
    float speed = 5.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;
    bool isAnimated;
    Assimp::Importer importer;
    const aiScene* scene;

    /*  Functions   */
    // constructor, expects a filepath to a 3D model.
    Model(string const &path, bool gamma = false, bool animated = false) : gammaCorrection(gamma), isAnimated(animated)
    {
        loadModel(path);
        UpdateVectors();
    }
    
    void updatePosition(float deltaT) {
        // yaw change needs to accumulate so can fly in circles
        direction = glm::normalize(glm::rotate(direction, glm::radians(-yaw/90.0f), up));
    
        position += speed * deltaT * direction;
        
        right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        up    = glm::normalize(glm::cross(right, direction));
    }
    
    //TODO function to process keyboard input in a flight simulator-y way.
    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        // mouse sensitivity
        xoffset *= 0.1;
        yoffset *= 0.1;

        yaw   += xoffset;
        pitch -= yoffset;
        
        if (pitch > 89.0) {
            pitch = 89.0;
        }
        if (pitch < -89.0) {
            pitch = -89.0;
        }
        if (yaw > 89.0) {
            yaw = 89.0;
        }
        if (yaw < -89.0) {
            yaw = -89.0;
        }
        
        roll = sin(glm::radians(yaw)) * 89.0f;
        
        // Update direction, right and up Vectors using the updated Euler angles
        UpdateVectors();
    }
    
    void UpdateVectors() {
        glm::vec3 f;
        
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

       // direction = glm::normalize(glm::rotate(f, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        direction.y = sin(glm::radians(pitch));
        direction = glm::normalize(direction);
        
        // re-calculate the right and up vector
        right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        up    = glm::normalize(glm::cross(right, direction));
    }

    // draws the (animated) model, and thus all its meshes
    void Draw(Shader shader, double time)
    {
        
        // create rotation matrix
        glm::mat4 rotation = glm::mat4(direction.x, direction.y, direction.z, 0,
                                       right.x, right.y, right.z, 0,
                                       up.x, up.y, up.z, 0,
                                       0, 0, 0, 1);
        model = glm::translate(glm::mat4(1.0f), position);
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model *= glm::rotate(glm::mat4(1.0f), glm::radians(roll), -direction);
        model *= rotation;
        
        accumulateTransforms(boneRoot, glm::mat4(1.0f), inverseBindTransform, fmod(time * animTicks*10, animDuration));
        
        shader.setMat4("model", model);
        
        for(unsigned int i = 0; i < meshes.size(); i++){
            meshes[i].Draw(shader, &animationTransforms);
        }
    }
    
    // draw an unanimated model
    void DrawStill(Shader shader) {
        shader.setMat4("model", model);
        for(unsigned int i = 0; i < meshes.size(); i++){
           meshes[i].Draw(shader, nullptr);
        }
    }
    
    void setModel(glm::mat4 m) {
        model = m;
    }
    
private:
    int numBones = 0;
    float animDuration = 0;
    float animTicks = 25;
    std::map<std::string, int> boneIdMap;
    
    /*  Functions   */
    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const &path)
    {
        // read file via ASSIMP
        scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
        // check for errors
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }
        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene);
        
        if (isAnimated) {
            animDuration = scene->mAnimations[1]->mDuration;
            animTicks = scene->mAnimations[1]->mTicksPerSecond;
        }
        
    }
    
    void accumulateTransforms(shared_ptr<Bone> bone, glm::mat4 parentTransform, glm::mat4 invBind, double time) {
        glm::mat4 globalTrans = parentTransform * bone->getTransform(time);
        
        animationTransforms[bone->loc] = invBind * globalTrans * bone->boneOffset;
        
        for (int i=0; i < bone->children.size(); i++) {
            accumulateTransforms(bone->children[i], globalTrans, invBind, time);
        }
    }
    
    void setOffsetMatrix(shared_ptr<Bone> bone, int loc, glm::mat4 offset) {
        if (bone->loc == loc) {
            bone->boneOffset = offset;
        }
        else {
            for (int i=0; i < bone->children.size(); i++) {
                setOffsetMatrix(bone->children[i], loc, offset);
            }
        }
    }
    
    shared_ptr<Bone> processBoneNode(aiNode *node, const aiScene *scene) {
        // TODO change from hardcoded 1 to search for flying animation? or take it as a parameter?
        aiNodeAnim* boneChannel = nullptr;
        std::shared_ptr<Bone> bone = make_shared<Bone>();
        
        for (int i=0; i < scene->mAnimations[1]->mNumChannels; i++) {
            if (scene->mAnimations[1]->mChannels[i]->mNodeName == node->mName) {
                boneChannel = scene->mAnimations[1]->mChannels[i];
            }
        }
        
        // this node is not a bone if its name is not in the mChannels animation list
        // but we still have to include it
        if (boneChannel != nullptr) {
            bone->loc = numBones;
            boneIdMap.insert(make_pair((boneChannel->mNodeName.C_Str()), bone->loc));
            numBones++;
            bone->isBone = true;
            
            aiVectorKey tmpKey;
            aiQuatKey tmpKeyQ;
            glm::vec3 tmpVec;
            for (int i=0; i < boneChannel->mNumPositionKeys; i++) {
                tmpKey = boneChannel->mPositionKeys[i];
                tmpVec = ai_converters::vec3_cast(tmpKey.mValue);
                bone->positionKeys.insert(std::make_pair(tmpKey.mTime, tmpVec));
            }
            for (int i=0; i < boneChannel->mNumScalingKeys; i++) {
               tmpKey = boneChannel->mScalingKeys[i];
               tmpVec = ai_converters::vec3_cast(tmpKey.mValue);
               bone->scaleKeys.insert(std::make_pair(tmpKey.mTime, tmpVec));
            }
            for (int i=0; i < boneChannel->mNumRotationKeys; i++) {
                tmpKeyQ = boneChannel->mRotationKeys[i];
                bone->rotationKeys.insert(std::make_pair(tmpKeyQ.mTime, ai_converters::quat_cast(tmpKeyQ.mValue)));
            }
            
        }
        else {
            bone->isBone = false;
        }
        
        // add nodes children to our bone heirarchy
        for (int i=0; i<node->mNumChildren; i++) {
            std::shared_ptr<Bone> tmpBone = processBoneNode(node->mChildren[i], scene);
            if (tmpBone != nullptr) {
                bone->children.push_back(tmpBone);
            }
        }
        
        return bone;
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode *node, const aiScene *scene)
    {
        // process each mesh located at the current node
        for(unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for(unsigned int i = 0; i < node->mNumChildren; i++)
        {
            processNode(node->mChildren[i], scene);
        }

    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene)
    {
        // data to fill
        vector<Vertex> vertices;
        vector<unsigned int> indices;
        vector<Texture> textures;

        // Walk through each of the mesh's vertices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
            // positions
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;
            vertex.Position = vector;
            // normals
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
            // texture coordinates
            if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
            {
                glm::vec2 vec;
                // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            // tangent
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangent = vector;
            // bitangent
            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;
            vertices.push_back(vertex);
        }
        // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
        for(unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        // process materials
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        // 1. diffuse maps
        vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        // 4. PBR maps TODO fix
        std::vector<Texture> pbrMaps = loadMaterialTextures(material, aiTextureType_UNKNOWN, "texture_pbr");
        textures.insert(textures.end(), pbrMaps.begin(), pbrMaps.end());
        
        if (isAnimated) {
            // first few nodes are not animations, skip to the animations
            aiNode* animRoot = scene->mRootNode->mChildren[0]->mChildren[0]->mChildren[0]->mChildren[0]->mChildren[0]->mChildren[0]->mChildren[1];
            
            // calculate global inverse bind transform and store it
            inverseBindTransform = glm::mat4(1.0f);
            //glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            //
            
            //ai_converters::mat4_cast(animRoot->mTransformation);
            //glm::rotate(identity, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));//
            inverseBindTransform = glm::inverse(inverseBindTransform);
            
            // process bones
            boneRoot = processBoneNode(animRoot, scene);
            boneRoot->boneOffset = glm::mat4(1.0f);

            // walk through all bones in mesh and assign bone weights to vertices
            aiBone *tempBone;
            Vertex *tempVert;
            for (int i=0; i < mesh->mNumBones; i++) {
                tempBone = mesh->mBones[i];
                for (int j=0; j < tempBone->mNumWeights; j++) {
                    if (tempBone->mWeights[j].mWeight > 0){
                        tempVert = &vertices[tempBone->mWeights[j].mVertexId];
                        // add bone to vertex
                        tempVert->boneWeights[tempVert->numBones] = tempBone->mWeights[j].mWeight;
                        auto it = boneIdMap.find(string(tempBone->mName.C_Str()));
                        if (it == boneIdMap.end()) {
                            cout << "key not found: " << tempBone->mName.C_Str() << endl;
                        }
                        else {
                            tempVert->boneIds[tempVert->numBones] = it->second;
                            // set offset matrix
                            setOffsetMatrix(boneRoot, it->second, ai_converters::mat4_cast(tempBone->mOffsetMatrix));
                        }
                        tempVert->numBones += 1;
                    }
                }
            }
        }
        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }

    // checks all material textures of a given type and loads the textures if they're not loaded yet.
    // the required info is returned as a Texture struct.
    //TODO extend to PBR with #include <assimp/pbrmaterial.h>
    vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
    {
        vector<Texture> textures;
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
        {
            aiString str;
            mat->GetTexture(type, i, &str);
            // check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++)
            {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
                {
                    textures.push_back(textures_loaded[j]);
                    skip = true; // a texture with the same filepath has already been loaded, continue to next one. (optimization)
                    break;
                }
            }
            if(!skip)
            {   // if texture hasn't been loaded already, load it
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
};


unsigned int TextureFromFile(const char *path, const string &directory, bool gamma)
{
    string filename = string(path);
    filename = directory + '/' + filename;

    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
#endif
