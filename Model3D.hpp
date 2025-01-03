#ifndef Model3D_hpp
#define Model3D_hpp

#include "Mesh.hpp"
#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>

namespace gps {

    class Model3D {

    public:
        ~Model3D();

        void LoadModel(std::string fileName);
        void Draw(gps::Shader shaderProgram);
        std::vector<glm::vec3> GetTriangles();
        std::vector<gps::Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
        void calculateBoundingBox();

    private:
        std::vector<gps::Mesh> meshes;
        glm::vec3 minPoint;
        glm::vec3 maxPoint;

        void ReadOBJ(std::string fileName, std::string basePath);
        void ReadFBX(std::string fileName);
        void processNode(aiNode* node, const aiScene* scene);
        gps::Mesh processMesh(aiMesh* mesh, const aiScene* scene);

        gps::Texture LoadTexture(std::string path, std::string type);
        GLuint ReadTextureFromFile(const char* file_name);

        std::vector<gps::Texture> loadedTextures;
    };

}

#endif
