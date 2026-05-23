#include <iostream>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <climits>
#include <cstring>
#include <vector>

struct Vertex {
    float x, y, z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<Vertex> normals;
    std::vector<Vertex> colors;
    std::vector<unsigned int> indices;
};

bool loadPLY(const std::string& filename, Mesh& mesh);
bool loadPLY2(const std::string& filename, Mesh& mesh);
bool loadModel(const std::string& filename, Mesh& mesh);

// ===== PLY Reader =====
bool loadPLY(const std::string& filename, Mesh& mesh) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    char line[4096];

    auto getLine = [&](char* out) -> bool {
        if (!fgets(out, 4096, file)) return false;
        int len = strlen(out);
        if (len > 0 && out[len-1] == '\n') out[len-1] = '\0';
        if (len > 1 && out[len-2] == '\r') out[len-2] = '\0';
        return true;
    };

    getLine(line);
    if (strcmp(line, "ply") != 0) {
        std::cerr << "Not a PLY file: " << line << std::endl;
        fclose(file);
        return false;
    }

    int nVertices = 0, nFaces = 0;
    int propertyCount = 0;
    bool hasNormals = false, hasColors = false, inVertex = false;

    while (getLine(line)) {
        if (strstr(line, "comment") == line) continue;

        if (strstr(line, "element vertex") == line) {
            sscanf(line, "element vertex %d", &nVertices);
            inVertex = true;
            continue;
        }

        if (strstr(line, "element face") == line) {
            sscanf(line, "element face %d", &nFaces);
            inVertex = false;
            continue;
        }

        if (strcmp(line, "end_header") == 0) break;

        if (inVertex && strstr(line, "property") == line) {
            if (strstr(line, "nx")) hasNormals = true;
            if (strstr(line, "red") || strstr(line, "diffuse_red")) hasColors = true;
            propertyCount++;
        }
    }

    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.colors.clear();
    mesh.vertices.reserve(nVertices);

    for (int i = 0; i < nVertices; i++) {
        if (!getLine(line)) {
            std::cerr << "Unexpected EOF at vertex " << i << std::endl;
            fclose(file);
            return false;
        }
        Vertex v = {0,0,0}, n = {0,0,0}, c = {0,0,0};
        float alpha = 1.0f;

        if (propertyCount == 3) {
            sscanf(line, "%f%f%f", &v.x, &v.y, &v.z);
        } else if (propertyCount == 6) {
            sscanf(line, "%f%f%f%f%f%f", &v.x, &v.y, &v.z, &n.x, &n.y, &n.z);
            mesh.normals.push_back(n);
        } else if (propertyCount == 7) {
            sscanf(line, "%f%f%f%f%f%f%f", &v.x, &v.y, &v.z, &c.x, &c.y, &c.z, &alpha);
            c.x /= 255.0f; c.y /= 255.0f; c.z /= 255.0f;
            mesh.colors.push_back(c);
        } else if (propertyCount == 10) {
            sscanf(line, "%f%f%f%f%f%f%f%f%f%f",
                   &v.x, &v.y, &v.z, &n.x, &n.y, &n.z, &c.x, &c.y, &c.z, &alpha);
            mesh.normals.push_back(n);
            c.x /= 255.0f; c.y /= 255.0f; c.z /= 255.0f;
            mesh.colors.push_back(c);
        }
        mesh.vertices.push_back(v);
    }

    mesh.indices.clear();
    mesh.indices.reserve(nFaces * 3);

    for (int i = 0; i < nFaces; i++) {
        if (!getLine(line)) {
            std::cerr << "Unexpected EOF at face " << i << std::endl;
            fclose(file);
            return false;
        }
        int vertsPerFace;
        char* p = line;
        int offset;
        sscanf(p, "%d%n", &vertsPerFace, &offset);
        p += offset;

        std::vector<unsigned int> faceIdx;
        unsigned int idx;
        while (sscanf(p, "%u%n", &idx, &offset) == 1) {
            if (idx >= (unsigned int)nVertices) {
                std::cerr << "Face " << i << ": index " << idx
                          << " out of range (max " << nVertices-1 << ")" << std::endl;
                fclose(file);
                return false;
            }
            faceIdx.push_back(idx);
            p += offset;
        }

        if ((int)faceIdx.size() != vertsPerFace) {
            std::cerr << "Face " << i << ": expected " << vertsPerFace
                      << " indices, got " << faceIdx.size() << std::endl;
            fclose(file);
            return false;
        }

        if (vertsPerFace == 3) {
            mesh.indices.push_back(faceIdx[0]);
            mesh.indices.push_back(faceIdx[1]);
            mesh.indices.push_back(faceIdx[2]);
        } else if (vertsPerFace == 4) {
            mesh.indices.push_back(faceIdx[0]); mesh.indices.push_back(faceIdx[1]); mesh.indices.push_back(faceIdx[2]);
            mesh.indices.push_back(faceIdx[0]); mesh.indices.push_back(faceIdx[2]); mesh.indices.push_back(faceIdx[3]);
        } else {
            for (size_t j = 0; j + 2 < faceIdx.size(); ++j) {
                mesh.indices.push_back(faceIdx[0]);
                mesh.indices.push_back(faceIdx[j+1]);
                mesh.indices.push_back(faceIdx[j+2]);
            }
        }
    }

    fclose(file);
    return true;
}

// ===== PLY2 Reader =====
bool loadPLY2(const std::string& filename, Mesh& mesh) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    int vertexCount = 0, faceCount = 0;
    char hdrline[4096];
    
    // Some PLY2 files have both counts on one line, others on two lines
    if (!fgets(hdrline, sizeof(hdrline), file)) {
        std::cerr << "Failed to read header line 1" << std::endl;
        fclose(file); return false;
    }
    int n = sscanf(hdrline, "%d%d", &vertexCount, &faceCount);
    if (n == 1) {
        // Second count is on next line
        if (!fgets(hdrline, sizeof(hdrline), file)) {
            std::cerr << "Failed to read header line 2" << std::endl;
            fclose(file); return false;
        }
        if (sscanf(hdrline, "%d", &faceCount) != 1) {
            std::cerr << "Failed to parse face count" << std::endl;
            fclose(file); return false;
        }
    } else if (n != 2) {
        std::cerr << "Failed to read header" << std::endl;
        fclose(file); return false;
    }

    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.colors.clear();
    mesh.vertices.reserve(vertexCount);

    for (int i = 0; i < vertexCount; ++i) {
        Vertex v;
        // Read entire line and parse first 3 floats (supports 3 or 6 float formats)
        char vline[4096];
        if (!fgets(vline, sizeof(vline), file)) {
            std::cerr << "Failed to read vertex " << i << std::endl;
            fclose(file);
            return false;
        }
        if (sscanf(vline, "%f%f%f", &v.x, &v.y, &v.z) != 3) {
            std::cerr << "Failed to parse vertex " << i << ": " << vline;
            fclose(file);
            return false;
        }
        mesh.vertices.push_back(v);

        // Optionally parse normals if line has >= 6 floats
        float nx, ny, nz;
        if (sscanf(vline, "%*f%*f%*f%f%f%f", &nx, &ny, &nz) == 3) {
            Vertex n = {nx, ny, nz};
            mesh.normals.push_back(n);
        }
    }

    mesh.indices.clear();
    mesh.indices.reserve(faceCount * 3);

    // Check remaining lines to determine face format
    // Format A: "3 i0 i1 i2" (with face vertex count prefix like bunny_iH.ply2)
    // Format B: "i0 i1 i2" (no prefix, like bunny_normal.ply2)
    long pos = ftell(file);
    char peek[4096];
    if (!fgets(peek, sizeof(peek), file)) {
        std::cerr << "Failed to read first face" << std::endl;
        fclose(file);
        return false;
    }
    fseek(file, pos, SEEK_SET);

    int firstVal;
    int peekOffset;
    sscanf(peek, "%d%n", &firstVal, &peekOffset);

    bool hasFaceCount = (firstVal == 3 || firstVal == 4);

    for (int i = 0; i < faceCount; ++i) {
        char fline[4096];
        if (!fgets(fline, sizeof(fline), file)) {
            std::cerr << "Failed to read face " << i << std::endl;
            fclose(file);
            return false;
        }

        std::vector<unsigned int> faceIdx;
        int offset = 0;

        if (hasFaceCount) {
            int vertsPerFace;
            if (sscanf(fline, "%d%n", &vertsPerFace, &offset) != 1) {
                std::cerr << "Failed to parse face " << i << ": " << fline;
                fclose(file);
                return false;
            }
            faceIdx.resize(vertsPerFace);
            for (int j = 0; j < vertsPerFace; ++j) {
                if (sscanf(fline + offset, "%u%n", &faceIdx[j], &peekOffset) != 1) {
                    std::cerr << "Failed to parse index " << j << " of face " << i << std::endl;
                    fclose(file);
                    return false;
                }
                offset += peekOffset;
            }
        } else {
            unsigned int idx;
            while (sscanf(fline + offset, "%u%n", &idx, &peekOffset) == 1) {
                faceIdx.push_back(idx);
                offset += peekOffset;
            }
        }

        for (size_t j = 0; j < faceIdx.size(); j++) {
            if (faceIdx[j] >= (unsigned int)vertexCount) {
                std::cerr << "Face " << i << ": index " << faceIdx[j]
                          << " out of range (max " << vertexCount-1 << ")" << std::endl;
                fclose(file);
                return false;
            }
        }

        if (faceIdx.size() == 3) {
            mesh.indices.push_back(faceIdx[0]);
            mesh.indices.push_back(faceIdx[1]);
            mesh.indices.push_back(faceIdx[2]);
        } else if (faceIdx.size() == 4) {
            mesh.indices.push_back(faceIdx[0]); mesh.indices.push_back(faceIdx[1]); mesh.indices.push_back(faceIdx[2]);
            mesh.indices.push_back(faceIdx[0]); mesh.indices.push_back(faceIdx[2]); mesh.indices.push_back(faceIdx[3]);
        } else if (faceIdx.size() > 4) {
            for (size_t j = 0; j + 2 < faceIdx.size(); ++j) {
                mesh.indices.push_back(faceIdx[0]);
                mesh.indices.push_back(faceIdx[j+1]);
                mesh.indices.push_back(faceIdx[j+2]);
            }
        }
    }

    fclose(file);
    return true;
}

bool loadModel(const std::string& filename, Mesh& mesh) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    char firstLine[8];
    bool ok = fgets(firstLine, sizeof(firstLine), file) != NULL;
    fclose(file);
    if (!ok) return false;

    if (strncmp(firstLine, "ply", 3) == 0)
        return loadPLY(filename, mesh);
    else
        return loadPLY2(filename, mesh);
}

// ===== Validation =====
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { std::cout << "\n[TEST] " << name << " ... "; } while(0)
#define PASS() do { std::cout << "PASS" << std::endl; tests_passed++; } while(0)
#define FAIL(msg) do { std::cout << "FAIL: " << msg << std::endl; tests_failed++; } while(0)

bool validateMesh(const Mesh& mesh, const char* name) {
    TEST(name);
    if (mesh.vertices.empty()) { FAIL("no vertices"); return false; }
    if (mesh.indices.empty()) { FAIL("no indices"); return false; }

    // Check vertex data for NaN/Inf
    int nan_count = 0, inf_count = 0;
    float min_x = FLT_MAX, min_y = FLT_MAX, min_z = FLT_MAX;
    float max_x = -FLT_MAX, max_y = -FLT_MAX, max_z = -FLT_MAX;

    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        float x = mesh.vertices[i].x, y = mesh.vertices[i].y, z = mesh.vertices[i].z;
        if (std::isnan(x) || std::isnan(y) || std::isnan(z)) nan_count++;
        if (std::isinf(x) || std::isinf(y) || std::isinf(z)) inf_count++;
        if (x < min_x) min_x = x; if (x > max_x) max_x = x;
        if (y < min_y) min_y = y; if (y > max_y) max_y = y;
        if (z < min_z) min_z = z; if (z > max_z) max_z = z;
    }

    if (nan_count > 0) { FAIL("found " << nan_count << " NaN values"); return false; }
    if (inf_count > 0) { FAIL("found " << inf_count << " Inf values"); return false; }

    // Check index validity
    int bad_idx = 0;
    for (size_t i = 0; i < mesh.indices.size(); i++) {
        if (mesh.indices[i] >= mesh.vertices.size()) bad_idx++;
    }
    if (bad_idx > 0) { FAIL("found " << bad_idx << " out-of-range indices"); return false; }

    // Check normals match vertex count (if present)
    if (!mesh.normals.empty() && mesh.normals.size() != mesh.vertices.size()) {
        FAIL("normal count (" << mesh.normals.size() << ") != vertex count (" << mesh.vertices.size() << ")");
        return false;
    }

    // Check colors match vertex count (if present)
    if (!mesh.colors.empty() && mesh.colors.size() != mesh.vertices.size()) {
        FAIL("color count (" << mesh.colors.size() << ") != vertex count (" << mesh.vertices.size() << ")");
        return false;
    }

    // Check face count is reasonable
    size_t faceCount = mesh.indices.size() / 3;
    if (faceCount < 1) { FAIL("no faces"); return false; }

    std::cout << "PASS (v=" << mesh.vertices.size() << " f=" << faceCount;
    std::cout << " bbox=[(" << min_x << "," << min_y << "," << min_z << ") - ("
              << max_x << "," << max_y << "," << max_z << ")]" << std::endl;
    std::cout << "       dims=[" << (max_x-min_x) << "," << (max_y-min_y) << "," << (max_z-min_z) << "]";
    if (!mesh.normals.empty()) std::cout << " normals=yes";
    if (!mesh.colors.empty()) std::cout << " colors=yes";
    std::cout << std::endl;
    tests_passed++;
    return true;
}

void testBrokenFiles() {
    std::cout << "\n=== Testing error handling ===" << std::endl;

    Mesh mesh;

    TEST("nonexistent file");
    if (!loadModel("nonexistent.ply2", mesh)) { PASS(); }
    else { FAIL("should have returned false"); }

    // Test invalid data - make a temp file
    {
        TEST("empty file");
        FILE* f = fopen("output/_test_empty.ply2", "w");
        fclose(f);
        if (!loadPLY2("output/_test_empty.ply2", mesh)) { PASS(); }
        else { FAIL("should have returned false"); }
        remove("output/_test_empty.ply2");
    }

    {
        TEST("file with only header");
        FILE* f = fopen("output/_test_hdr.ply2", "w");
        fprintf(f, "10 5\n");
        fclose(f);
        if (!loadPLY2("output/_test_hdr.ply2", mesh)) { PASS(); }
        else { FAIL("should have returned false"); }
        remove("output/_test_hdr.ply2");
    }

    {
        TEST("file with bad index");
        FILE* f = fopen("output/_test_badidx.ply2", "w");
        fprintf(f, "3 1\n");
        fprintf(f, "0 0 0\n1 1 1\n2 2 2\n");
        fprintf(f, "3 0 1 999\n");  // index 999 out of range
        fclose(f);
        if (!loadPLY2("output/_test_badidx.ply2", mesh)) { PASS(); }
        else { FAIL("should have returned false"); }
        remove("output/_test_badidx.ply2");
    }

    {
        TEST("file with NaN vertex");
        FILE* f = fopen("output/_test_nan.ply2", "w");
        fprintf(f, "3 1\n");
        fprintf(f, "0 0 0\n1 nan 1\n2 2 2\n");
        fprintf(f, "3 0 1 2\n");
        fclose(f);
        bool ok = loadPLY2("output/_test_nan.ply2", mesh);
        bool has_nan = false;
        if (ok) {
            for (auto& v : mesh.vertices) {
                if (std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z)) has_nan = true;
            }
        }
        if (!ok || has_nan) {
            PASS();  // correctly rejects or contains detectable NaN
        } else {
            FAIL("should have NaN or parsing error");
        }
        remove("output/_test_nan.ply2");
    }
}

int main() {
    std::cout << "=== OpenGL Bunny Viewer Tests ===" << std::endl;

    Mesh mesh1, mesh2, mesh3;

    // Test 1: bunny_iH.ply2 (no normals)
    std::cout << "\n--- Testing bunny_iH.ply2 ---" << std::endl;
    if (loadModel("bunny_iH.ply2", mesh1)) {
        validateMesh(mesh1, "bunny_iH.ply2 data");
    } else {
        tests_failed++;
    }

    // Test 2: bunny_normal.ply2 (with normals, 6 floats per vertex)
    std::cout << "\n--- Testing bunny_normal.ply2 ---" << std::endl;
    if (loadModel("bunny_normal.ply2", mesh2)) {
        validateMesh(mesh2, "bunny_normal.ply2 data");
    } else {
        tests_failed++;
    }

    // Test 3: bunny_iH.ply (PLY format)
    std::cout << "\n--- Testing bunny_iH.ply ---" << std::endl;
    if (loadModel("bunny_iH.ply", mesh3)) {
        validateMesh(mesh3, "bunny_iH.ply data");
    } else {
        tests_failed++;
    }

    // Test 4: bunny,vs / bunny.vs (vertex shader) 
    std::cout << "\n--- Testing shader file existence ---" << std::endl;
    const char* shaderFiles[] = {"bunny,vs", "bunny.fs", "marker.vs", "marker.fs"};
    for (int i = 0; i < 4; i++) {
        TEST(shaderFiles[i]);
        FILE* f = fopen(shaderFiles[i], "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            long sz = ftell(f);
            fclose(f);
            std::cout << "PASS (" << sz << " bytes)" << std::endl;
            tests_passed++;
        } else {
            FAIL("file not found");
        }
    }

    // Test 5: Error handling
    testBrokenFiles();

    // Test 6: Consistency - check that same model loaded from different files has same vertex count
    std::cout << "\n--- Testing cross-file consistency ---" << std::endl;
    TEST("vertex count consistent across files");
    // bunny_iH.ply2 and bunny_iH.ply should have same vertex count
    if (mesh1.vertices.size() == mesh3.vertices.size()) {
        PASS();
    } else {
        FAIL("bunny_iH.ply2=" << mesh1.vertices.size() << " vs bunny_iH.ply=" << mesh3.vertices.size());
    }

    // Test 7: Main executable exists and can run
    std::cout << "\n--- Testing executable ---" << std::endl;
    TEST("main.exe exists");
    FILE* exe = fopen("output/main.exe", "rb");
    if (exe) {
        fclose(exe);
        PASS();
    } else {
        FAIL("output/main.exe not found");
    }

    // Summary
    int total = tests_passed + tests_failed;
    std::cout << "\n========================================" << std::endl;
    std::cout << "Results: " << tests_passed << "/" << total << " tests passed";
    if (tests_failed > 0) {
        std::cout << " (" << tests_failed << " FAILED)";
    }
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
