#include "parser.h"

// 读取原始的ply2文件,顶点为三个元素
void parseIndex(const char* fileName, GLfloat* vertices, GLuint* indices, GLuint& vertices_size, GLuint& indices_size){
    std::FILE* f = std::fopen(fileName, "r");
    
    // 先读取前两行,分别是顶点数和面元个数
    std::fscanf(f, "%d%d", &vertices_size, &indices_size);
    // cout << vertices_size << endl << indices_size << endl;
    
    for(int i = 0; i < vertices_size; i++){
        std::fscanf(f, "%f%f%f", &vertices[3*i], &vertices[3*i+1], &vertices[3*i+2]);
    }
    
    for(int i = 0; i < indices_size; i++){
        int _;
        std::fscanf(f, "%d%d%d%d", &_, &indices[3*i], &indices[3*i+1], &indices[3*i+2]);
    }
    std::fclose(f);
}

// 读取更新后的ply2_normal文件,顶点为六个元素,后面三个是法向量(?)
void parseNormal(const char* fileName, GLfloat* vertices, GLuint* indices, GLuint& vertices_size, GLuint& indices_size){
    std::FILE* f = std::fopen(fileName, "r");
    
    std::fscanf(f, "%d%d", &vertices_size, &indices_size);
    // cout << vertices_size << endl << indices_size << endl;
    
    for(int i = 0; i < vertices_size; i++){
        std::fscanf(f, "%f%f%f%f%f%f", &vertices[6*i], &vertices[6*i+1], &vertices[6*i+2], \
            &vertices[6*i+3], &vertices[6*i+4], &vertices[6*i+5]);
    }
    for(int i = 0; i < indices_size; i++){
        std::fscanf(f, "%d%d%d", &indices[3*i], &indices[3*i+1], &indices[3*i+2]);
    }
    std::fclose(f);
}
