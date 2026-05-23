#ifndef PLY2_READER_H
#define PLY2_READER_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

struct Vertex {
    float x, y, z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

inline bool loadPLY2(const std::string& filename, Mesh& mesh) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;

    auto getLine = [&](std::string& out) -> bool {
        if (!std::getline(file, out)) return false;
        if (!out.empty() && out.back() == '\r') out.pop_back();
        return true;
    };

    int vertexCount, faceCount;

    if (!getLine(line)) {
        std::cerr << "Unexpected EOF: missing vertex count" << std::endl;
        return false;
    }
    vertexCount = std::stoi(line);

    if (!getLine(line)) {
        std::cerr << "Unexpected EOF: missing face count" << std::endl;
        return false;
    }
    faceCount = std::stoi(line);

    std::cout << "Reading " << vertexCount << " vertices, "
              << faceCount << " faces" << std::endl;

    mesh.vertices.clear();
    mesh.vertices.reserve(vertexCount);
    for (int i = 0; i < vertexCount; ++i) {
        if (!getLine(line)) {
            std::cerr << "Unexpected EOF at vertex " << i << std::endl;
            return false;
        }
        std::istringstream iss(line);
        Vertex v;
        if (!(iss >> v.x >> v.y >> v.z)) {
            std::cerr << "Failed to parse vertex " << i << ": " << line << std::endl;
            return false;
        }
        mesh.vertices.push_back(v);
    }

    mesh.indices.clear();
    mesh.indices.reserve(faceCount * 3);
    for (int i = 0; i < faceCount; ++i) {
        if (!getLine(line)) {
            std::cerr << "Unexpected EOF at face " << i << std::endl;
            return false;
        }
        std::istringstream iss(line);
        int vertsPerFace;
        if (!(iss >> vertsPerFace)) {
            std::cerr << "Failed to parse face " << i << ": " << line << std::endl;
            return false;
        }

        std::vector<unsigned int> faceIndices;
        unsigned int idx;
        while (iss >> idx) {
            if (idx >= (unsigned int)vertexCount) {
                std::cerr << "Face " << i << ": index " << idx
                          << " is out of range (max " << (vertexCount - 1) << ")" << std::endl;
                return false;
            }
            faceIndices.push_back(idx);
        }

        if ((int)faceIndices.size() != vertsPerFace) {
            std::cerr << "Face " << i << ": expected " << vertsPerFace
                      << " indices, got " << faceIndices.size() << std::endl;
            return false;
        }

        if (vertsPerFace == 3) {
            mesh.indices.push_back(faceIndices[0]);
            mesh.indices.push_back(faceIndices[1]);
            mesh.indices.push_back(faceIndices[2]);
        } else if (vertsPerFace == 4) {
            mesh.indices.push_back(faceIndices[0]);
            mesh.indices.push_back(faceIndices[1]);
            mesh.indices.push_back(faceIndices[2]);
            mesh.indices.push_back(faceIndices[0]);
            mesh.indices.push_back(faceIndices[2]);
            mesh.indices.push_back(faceIndices[3]);
        } else {
            for (unsigned int j = 0; j + 2 < faceIndices.size(); ++j) {
                mesh.indices.push_back(faceIndices[0]);
                mesh.indices.push_back(faceIndices[j + 1]);
                mesh.indices.push_back(faceIndices[j + 2]);
            }
        }
    }

    file.close();
    return true;
}

#endif
