#include "Model3D.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace gps {

    void Model3D::LoadModel(std::string fileName) {
        std::string extension = fileName.substr(fileName.find_last_of('.') + 1);

        if (extension == "obj" || extension == "OBJ") {
            std::string basePath = fileName.substr(0, fileName.find_last_of('/')) + "/";
            ReadOBJ(fileName, basePath);
        }
        else if (extension == "fbx" || extension == "FBX") {
            ReadFBX(fileName);
        }
        else {
            std::cerr << "Unsupported file format: " << extension << std::endl;
            return;
        }

        calculateBoundingBox();
    }



    std::vector<glm::vec3> Model3D::GetTriangles() {
        std::vector<glm::vec3> triangles;
        for (auto& mesh : meshes) {
            for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                glm::vec3 v0 = mesh.vertices[mesh.indices[i]].Position;
                glm::vec3 v1 = mesh.vertices[mesh.indices[i + 1]].Position;
                glm::vec3 v2 = mesh.vertices[mesh.indices[i + 2]].Position;
                triangles.push_back(v0);
                triangles.push_back(v1);
                triangles.push_back(v2);
            }
        }
        return triangles;
    }
    std::vector<gps::Texture> Model3D::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
        std::vector<gps::Texture> textures;

        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            bool skip = false;
            for (size_t j = 0; j < loadedTextures.size(); j++) {
                if (std::strcmp(loadedTextures[j].path.c_str(), str.C_Str()) == 0) {
                    textures.push_back(loadedTextures[j]);
                    skip = true;
                    break;
                }
            }

            if (!skip) {
                gps::Texture texture;
                texture.id = ReadTextureFromFile(str.C_Str());
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                loadedTextures.push_back(texture);
            }
        }

        return textures;
    }


    void Model3D::Draw(gps::Shader shaderProgram) {
        for (int i = 0; i < meshes.size(); i++) {
            meshes[i].Draw(shaderProgram);
        }
    }

    glm::vec3 minPoint;
    glm::vec3 maxPoint;

    void Model3D::calculateBoundingBox() {
        minPoint = glm::vec3(FLT_MAX);
        maxPoint = glm::vec3(-FLT_MAX);
        for (auto& mesh : meshes) {
            for (auto& vertex : mesh.vertices) {
                minPoint = glm::min(minPoint, vertex.Position);
                maxPoint = glm::max(maxPoint, vertex.Position);
            }
        }
    }

    void Model3D::ReadOBJ(std::string fileName, std::string basePath) {
        std::cout << "Loading : " << fileName << std::endl;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        int materialId;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fileName.c_str(), basePath.c_str(), GL_TRUE);
        if (!err.empty()) {
            std::cerr << err << std::endl;
        }
        if (!ret) {
            exit(1);
        }
        std::cout << "# of shapes    : " << shapes.size() << std::endl;
        std::cout << "# of materials : " << materials.size() << std::endl;
        for (size_t s = 0; s < shapes.size(); s++) {
            std::vector<gps::Vertex> vertices;
            std::vector<GLuint> indices;
            std::vector<gps::Texture> textures;
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                int fv = shapes[s].mesh.num_face_vertices[f];
                for (size_t v = 0; v < fv; v++) {
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    float vx = attrib.vertices[3 * idx.vertex_index + 0];
                    float vy = attrib.vertices[3 * idx.vertex_index + 1];
                    float vz = attrib.vertices[3 * idx.vertex_index + 2];
                    float nx = attrib.normals[3 * idx.normal_index + 0];
                    float ny = attrib.normals[3 * idx.normal_index + 1];
                    float nz = attrib.normals[3 * idx.normal_index + 2];
                    float tx = 0.0f;
                    float ty = 0.0f;
                    if (idx.texcoord_index != -1) {
                        tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                        ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    glm::vec3 vertexPosition(vx, vy, vz);
                    glm::vec3 vertexNormal(nx, ny, nz);
                    glm::vec2 vertexTexCoords(tx, ty);
                    gps::Vertex currentVertex;
                    currentVertex.Position = vertexPosition;
                    currentVertex.Normal = vertexNormal;
                    currentVertex.TexCoords = vertexTexCoords;
                    vertices.push_back(currentVertex);
                    indices.push_back((GLuint)(index_offset + v));
                }
                index_offset += fv;
            }
            size_t a = shapes[s].mesh.material_ids.size();
            if (a > 0 && materials.size() > 0) {
                materialId = shapes[s].mesh.material_ids[0];
                if (materialId != -1) {
                    gps::Material currentMaterial;
                    currentMaterial.ambient = glm::vec3(materials[materialId].ambient[0], materials[materialId].ambient[1], materials[materialId].ambient[2]);
                    currentMaterial.diffuse = glm::vec3(materials[materialId].diffuse[0], materials[materialId].diffuse[1], materials[materialId].diffuse[2]);
                    currentMaterial.specular = glm::vec3(materials[materialId].specular[0], materials[materialId].specular[1], materials[materialId].specular[2]);
                    std::string ambientTexturePath = materials[materialId].ambient_texname;
                    if (!ambientTexturePath.empty()) {
                        gps::Texture currentTexture;
                        currentTexture = LoadTexture(basePath + ambientTexturePath, "ambientTexture");
                        textures.push_back(currentTexture);
                    }
                    std::string diffuseTexturePath = materials[materialId].diffuse_texname;
                    if (!diffuseTexturePath.empty()) {
                        gps::Texture currentTexture;
                        currentTexture = LoadTexture(basePath + diffuseTexturePath, "diffuseTexture");
                        textures.push_back(currentTexture);
                    }
                    std::string specularTexturePath = materials[materialId].specular_texname;
                    if (!specularTexturePath.empty()) {
                        gps::Texture currentTexture;
                        currentTexture = LoadTexture(basePath + specularTexturePath, "specularTexture");
                        textures.push_back(currentTexture);
                    }
                }
            }
            meshes.push_back(gps::Mesh(vertices, indices, textures));
        }
    }
    void Model3D::ReadFBX(std::string fileName) {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Error loading FBX file: " << importer.GetErrorString() << std::endl;
            return;
        }

        processNode(scene->mRootNode, scene);
    }
    void Model3D::processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    gps::Mesh Model3D::processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<gps::Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<gps::Texture> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            gps::Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            vertex.Normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);

            vertex.TexCoords = mesh->mTextureCoords[0] ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        if (mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            std::vector<gps::Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            std::vector<gps::Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        }

        return gps::Mesh(vertices, indices, textures);
    }


    gps::Texture Model3D::LoadTexture(std::string path, std::string type) {
        for (int i = 0; i < loadedTextures.size(); i++) {
            if (loadedTextures[i].path == path) {
                return loadedTextures[i];
            }
        }
        gps::Texture currentTexture;
        currentTexture.id = ReadTextureFromFile(path.c_str());
        currentTexture.type = type;
        currentTexture.path = path;
        loadedTextures.push_back(currentTexture);
        return currentTexture;
    }

    GLuint Model3D::ReadTextureFromFile(const char* file_name) {
        int x, y, n;
        int force_channels = 4;
        unsigned char* image_data = stbi_load(file_name, &x, &y, &n, force_channels);
        if (!image_data) {
            fprintf(stderr, "ERROR: could not load %s\n", file_name);
            return 0;
        }
        if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
            fprintf(stderr, "WARNING: texture %s is not power-of-2 dimensions\n", file_name);
        }
        int width_in_bytes = x * 4;
        unsigned char* top = nullptr;
        unsigned char* bottom = nullptr;
        unsigned char temp = 0;
        int half_height = y / 2;
        for (int row = 0; row < half_height; row++) {
            top = image_data + row * width_in_bytes;
            bottom = image_data + (y - row - 1) * width_in_bytes;
            for (int col = 0; col < width_in_bytes; col++) {
                temp = *top;
                *top = *bottom;
                *bottom = temp;
                top++;
                bottom++;
            }
        }
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return textureID;
    }

    Model3D::~Model3D() {
        for (size_t i = 0; i < loadedTextures.size(); i++) {
            glDeleteTextures(1, &loadedTextures[i].id);
        }
        for (size_t i = 0; i < meshes.size(); i++) {
            GLuint VBO = meshes[i].getBuffers().VBO;
            GLuint EBO = meshes[i].getBuffers().EBO;
            GLuint VAO = meshes[i].getBuffers().VAO;
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteVertexArrays(1, &VAO);
        }
    }
}
