#ifndef PLY_READER_H
#define PLY_READER_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <cstdio>

struct Vertex {
    float x, y, z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Vertex> normals;
    std::vector<Vertex> colors;
    std::vector<unsigned int> indices;
};

inline void normalize(Vertex& v) {
    double length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length != 0) {
        v.x = float(v.x / length);
        v.y = float(v.y / length);
        v.z = float(v.z / length);
    }
}

inline bool loadPLY(const std::string& filename, Mesh& mesh) {
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

    getLine(line);
    if (line != "ply") {
        std::cerr << "Not a PLY file (missing 'ply' header): " << line << std::endl;
        return false;
    }

    getLine(line);
    if (line.rfind("format ascii", 0) != 0) {
        std::cerr << "Only ASCII PLY format supported, got: " << line << std::endl;
        return false;
    }

    int g_nVerticesNumber = 0;
    int g_nFacesNumber = 0;
    int propertyCount = 0;
    bool hasNormals = false;
    bool hasColors = false;
    bool inVertexElement = false;

    while (getLine(line)) {
        if (line.rfind("comment", 0) == 0) {
            continue;
        }

        if (line.rfind("element vertex", 0) == 0) {
            std::sscanf(line.c_str(), "element vertex %d", &g_nVerticesNumber);
            inVertexElement = true;
            continue;
        }

        if (line.rfind("element face", 0) == 0) {
            std::sscanf(line.c_str(), "element face %d", &g_nFacesNumber);
            inVertexElement = false;
            continue;
        }

        if (line == "end_header") {
            break;
        }

        if (inVertexElement && line.rfind("property", 0) == 0) {
            if (line.find("nx") != std::string::npos) hasNormals = true;
            if (line.find("red") != std::string::npos || line.find("green") != std::string::npos ||
                line.find("blue") != std::string::npos || line.find("diffuse_red") != std::string::npos) hasColors = true;
            propertyCount++;
        }
    }

    std::cout << "Vertices: " << g_nVerticesNumber << ", Faces: " << g_nFacesNumber
              << ", Properties: " << propertyCount
              << ", Normals: " << (hasNormals ? "yes" : "no")
              << ", Colors: " << (hasColors ? "yes" : "no") << std::endl;

    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.colors.clear();
    mesh.vertices.reserve(g_nVerticesNumber);

    for (int i = 0; i < g_nVerticesNumber; i++) {
        if (!getLine(line)) {
            std::cerr << "Unexpected EOF at vertex " << i << std::endl;
            return false;
        }

        std::istringstream iss(line);
        Vertex v;
        Vertex n;
        Vertex c;
        float alpha = 1.0f;

        if (propertyCount == 3) {
            iss >> v.x >> v.y >> v.z;
        } else if (propertyCount == 6) {
            iss >> v.x >> v.y >> v.z >> n.x >> n.y >> n.z;
            normalize(n);
            mesh.normals.push_back(n);
        } else if (propertyCount == 7) {
            iss >> v.x >> v.y >> v.z >> c.x >> c.y >> c.z >> alpha;
            c.x /= 255.0f; c.y /= 255.0f; c.z /= 255.0f;
            mesh.colors.push_back(c);
        } else if (propertyCount == 10) {
            iss >> v.x >> v.y >> v.z >> n.x >> n.y >> n.z >> c.x >> c.y >> c.z >> alpha;
            normalize(n);
            mesh.normals.push_back(n);
            c.x /= 255.0f; c.y /= 255.0f; c.z /= 255.0f;
            mesh.colors.push_back(c);
        } else {
            iss >> v.x >> v.y >> v.z;
            if (hasNormals && propertyCount >= 6) {
                iss >> n.x >> n.y >> n.z;
                normalize(n);
                mesh.normals.push_back(n);
            }
            if (hasColors) {
                int skipCount = propertyCount - 3;
                if (hasNormals) skipCount -= 3;
                for (int s = 0; s < skipCount - 3; s++) iss >> alpha;
                iss >> c.x >> c.y >> c.z;
                if (c.x > 1.0f || c.y > 1.0f || c.z > 1.0f) {
                    c.x /= 255.0f; c.y /= 255.0f; c.z /= 255.0f;
                }
                mesh.colors.push_back(c);
            }
        }

        mesh.vertices.push_back(v);

        if (!iss) {
            std::cerr << "Failed to parse vertex " << i << ": " << line << std::endl;
            return false;
        }
    }

    mesh.indices.clear();
    mesh.indices.reserve(g_nFacesNumber * 3);

    for (int i = 0; i < g_nFacesNumber; i++) {
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
            if (idx >= (unsigned int)g_nVerticesNumber) {
                std::cerr << "Face " << i << ": index " << idx
                          << " out of range (max " << (g_nVerticesNumber - 1) << ")" << std::endl;
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

    if (!getLine(line)) return false;
    vertexCount = std::stoi(line);
    if (!getLine(line)) return false;
    faceCount = std::stoi(line);

    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.colors.clear();
    mesh.vertices.reserve(vertexCount);

    for (int i = 0; i < vertexCount; ++i) {
        if (!getLine(line)) return false;
        std::istringstream iss(line);
        Vertex v;
        iss >> v.x >> v.y >> v.z;
        mesh.vertices.push_back(v);
    }

    mesh.indices.clear();
    mesh.indices.reserve(faceCount * 3);

    for (int i = 0; i < faceCount; ++i) {
        if (!getLine(line)) return false;
        std::istringstream iss(line);
        int vertsPerFace;
        iss >> vertsPerFace;

        std::vector<unsigned int> faceIndices;
        unsigned int idx;
        while (iss >> idx) faceIndices.push_back(idx);

        if (vertsPerFace == 3) {
            mesh.indices.insert(mesh.indices.end(), faceIndices.begin(), faceIndices.end());
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

inline bool loadModel(const std::string& filename, Mesh& mesh) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    std::string firstLine;
    std::getline(file, firstLine);
    if (!firstLine.empty() && firstLine.back() == '\r') firstLine.pop_back();
    file.close();

    if (firstLine == "ply") {
        return loadPLY(filename, mesh);
    } else {
        return loadPLY2(filename, mesh);
    }
}

#endif
