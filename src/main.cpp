#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <windows.h>

using namespace std;
using namespace glm;

#define INF 1.0e10
#define MAXPOINT 40000
#define MAXINDEX 70000

// 相机类（简化版）
class Camera {
public:
    vec3 Position;
    vec3 Front;
    vec3 Up;
    vec3 Right;
    vec3 WorldUp;
    float Yaw;
    float Pitch;
    float Speed;
    float Sensitivity;
    float Zoom;

    Camera(vec3 position = vec3(0.0f, 0.0f, 30.0f)) : Front(vec3(0.0f, 0.0f, -1.0f)) {
        Position = position;
        WorldUp = vec3(0.0f, 1.0f, 0.0f);
        Yaw = -90.0f;
        Pitch = 0.0f;
        Speed = 0.1f;
        Sensitivity = 0.1f;
        Zoom = 45.0f;
        updateCameraVectors();
    }

    mat4 GetViewMatrix() {
        return lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(int direction, float deltaTime) {
        float velocity = Speed * deltaTime;
        if (direction == 0) Position += Front * velocity;
        if (direction == 1) Position -= Front * velocity;
        if (direction == 2) Position -= Right * velocity;
        if (direction == 3) Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset) {
        xoffset *= Sensitivity;
        yoffset *= Sensitivity;
        Yaw += xoffset;
        Pitch += yoffset;
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

private:
    void updateCameraVectors() {
        vec3 front;
        front.x = cos(radians(Yaw)) * cos(radians(Pitch));
        front.y = sin(radians(Pitch));
        front.z = sin(radians(Yaw)) * cos(radians(Pitch));
        Front = normalize(front);
        Right = normalize(cross(Front, WorldUp));
        Up = normalize(cross(Right, Front));
    }
};

// 方向常量
const int FORWARD = 0;
const int BACKWARD = 1;
const int LEFT = 2;
const int RIGHT = 3;

// GLFW 键码常量
#define GLFW_KEY_W 87
#define GLFW_KEY_S 83
#define GLFW_KEY_A 65
#define GLFW_KEY_D 68
#define GLFW_KEY_LEFT 263
#define GLFW_KEY_RIGHT 262
#define GLFW_KEY_UP 265
#define GLFW_KEY_DOWN 264
#define GLFW_KEY_J 74
#define GLFW_KEY_K 75
#define GLFW_KEY_I 73
#define GLFW_KEY_L 76
#define GLFW_KEY_EQUAL 61
#define GLFW_KEY_ENTER 257
#define GLFW_KEY_ESCAPE 256
#define GLFW_KEY_TAB 258

// 函数声明
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void picking_callback(GLFWwindow *window, int button, int action, int mods);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void interactionFunctions();

// 窗口参数
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 768;

// 相机
Camera camera(vec3(0.0f, 0.0f, 30.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// 时间参数
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 兔子参数
mat4 modelBunny = mat4(1.0f);
GLuint vertices_size, indices_size;

// 鼠标参数
GLfloat mouseX = SCR_WIDTH / 2.0;
GLfloat mouseY = SCR_HEIGHT / 2.0;
GLuint selectedPointIndice = 0;
bool showPointInfo = true;  // 是否显示选取的点

const char* normalFile = "bunny_normal.ply2";
const char *SPLIT = "--------------------------------------------------------------";
GLfloat vertices[MAXPOINT*6];
GLuint indices[MAXINDEX*3];

bool keys[1024];
bool cursorDisabled = false;  // 默认允许鼠标移动

// PLY2 文件读取（参考 parser.cpp）
void parseNormal(const char* fileName, GLfloat* vertices, GLuint* indices, 
                 GLuint& vertices_size, GLuint& indices_size) {
    FILE* f = fopen(fileName, "r");
    if (!f) {
        cerr << "Failed to open file: " << fileName << endl;
        exit(EXIT_FAILURE);
    }
    
    fscanf(f, "%d%d", &vertices_size, &indices_size);
    cout << "Loaded " << vertices_size << " vertices, " << indices_size << " faces" << endl;
    
    for(int i = 0; i < vertices_size; i++) {
        fscanf(f, "%f%f%f%f%f%f", 
            &vertices[6*i], &vertices[6*i+1], &vertices[6*i+2],
            &vertices[6*i+3], &vertices[6*i+4], &vertices[6*i+5]);
    }
    
    for(int i = 0; i < indices_size; i++) {
        fscanf(f, "%d%d%d", &indices[3*i], &indices[3*i+1], &indices[3*i+2]);
    }
    
    fclose(f);
}

void description() {
    cout << SPLIT << endl;
    cout << "Starting GLFW context, OpenGL 3.3" << endl;
    cout << "=== 相机控制 ===" << endl;
    cout << "A: 相机向左" << endl;
    cout << "D: 相机向右" << endl;
    cout << "W: 相机向前" << endl;
    cout << "S: 相机向后" << endl;
    cout << "TAB: 切换光标控制模式（正常/拾取）" << endl;
    cout << "\n=== 模型变换 ===" << endl;
    cout << "方向键<-: 兔子左移" << endl;
    cout << "方向键->: 兔子右移" << endl;
    cout << "J: 兔子向左旋转" << endl;
    cout << "L: 兔子向右旋转" << endl;
    cout << "I: 兔子向前旋转" << endl;
    cout << "K: 兔子向后旋转" << endl;
    cout << "↑或鼠标向上滚动: 兔子放大" << endl;
    cout << "↓或鼠标向下滚动: 兔子缩小" << endl;
    cout << "\n=== 点选取 ===" << endl;
    cout << "鼠标左键: 选取模型中的点" << endl;
    cout << "P: 开关选取点显示" << endl;
    cout << "\nESC: 退出程序" << endl;
    cout << SPLIT << endl;
}

GLFWwindow* init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Stanford bunny", NULL, NULL);
    if (window == NULL) {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        exit(EXIT_SUCCESS);
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, picking_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // 默认允许鼠标移动
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        exit(EXIT_SUCCESS);
    }
    
    glEnable(GL_DEPTH_TEST);
    return window;
}

// 着色器类（简化版）- 内建着色器
class Shader {
public:
    unsigned int ID;
    
    Shader(const char* vertexPath, const char* fragmentPath) {
        const char* vertexShaderSource = "#version 330 core\n"
            "layout (location = 0) in vec3 aPos;\n"
            "layout (location = 1) in vec3 aNormal;\n"
            "out vec3 Normal;\n"
            "out vec3 FragPos;\n"
            "uniform mat4 model;\n"
            "uniform mat4 view;\n"
            "uniform mat4 projection;\n"
            "void main() {\n"
            "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
            "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
            "   gl_Position = projection * view * vec4(FragPos, 1.0);\n"
            "}\0";
        
        const char* fragmentShaderSource = "#version 330 core\n"
            "in vec3 Normal;\n"
            "in vec3 FragPos;\n"
            "out vec4 FragColor;\n"
            "uniform vec3 uColor;\n"
            "void main() {\n"
            "   vec3 norm = normalize(Normal);\n"
            "   vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));\n"
            "   float diff = max(dot(norm, lightDir), 0.3);\n"
            "   vec3 color = uColor * diff;\n"
            "   FragColor = vec4(color, 1.0);\n"
            "}\0";
        
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        ID = glCreateProgram();
        glAttachShader(ID, vertexShader);
        glAttachShader(ID, fragmentShader);
        glLinkProgram(ID);
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    
    void use() {
        glUseProgram(ID);
    }
    
    void setMat4(const string& name, const mat4& value) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &value[0][0]);
    }

    void setVec3(const string& name, const vec3& value) {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
};

// 回调函数实现
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
    
    // 只在禁用模式才旋转视角
    if(cursorDisabled) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }
        
        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;
        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // TAB 切换光标模式
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        cursorDisabled = !cursorDisabled;
        if (cursorDisabled) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    
    // P 切换是否显示选取的点
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        showPointInfo = !showPointInfo;
    }
    
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

void picking_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        GLfloat xpos = mouseX;
        GLfloat ypos = mouseY;
        cout << "\n========================================" << endl;
        cout << "选取点的屏幕坐标：" << xpos << ", " << ypos << endl;
        
        GLfloat minDistance = pow(10, 20);
        GLuint minIndice = 0;
        
        // 遍历所有顶点，找到距离鼠标最近的点
        for (int i = 0; i < vertices_size; i++) {
            // 只考虑朝向相机的点
            if (dot(mat3(transpose(inverse(modelBunny))) *
                    vec3(vertices[6 * i + 3], vertices[6 * i + 4], vertices[6 * i + 5]), 
                    camera.Front) < 0) {
                
                vec4 iPos;
                mat4 view = camera.GetViewMatrix();
                mat4 projection = perspective(radians(camera.Zoom), 
                    (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
                
                // 变换顶点到裁剪空间
                iPos = modelBunny * vec4(vertices[i * 6], vertices[i * 6 + 1], 
                                         vertices[i * 6 + 2], 1.0f);
                iPos = projection * view * iPos;
                
                // 透视除法转换为屏幕坐标
                GLfloat pointPosX = SCR_WIDTH / 2 * (iPos.x / iPos.w) + SCR_WIDTH / 2;
                GLfloat pointPosY = SCR_HEIGHT / 2 * (-iPos.y / iPos.w) + SCR_HEIGHT / 2;
                
                // 计算距离
                GLfloat distance = (pointPosX - xpos) * (pointPosX - xpos) + 
                                   (pointPosY - ypos) * (pointPosY - ypos);
                
                if (distance < minDistance) {
                    minDistance = distance;
                    minIndice = i;
                }
            }
        }
        
        // 如果距离足够近则选中该点
        if (minDistance < 1600) {  // 20 像素半径
            selectedPointIndice = minIndice;
            cout << "  [OK] 点索引号：" << minIndice << endl;
            cout << "  [OK] 点的 3D 坐标：(" 
                 << vertices[minIndice * 6] << ", " 
                 << vertices[minIndice * 6 + 1] << ", " 
                 << vertices[minIndice * 6 + 2] << ")" << endl;
            cout << "  [OK] 点的法线：(" 
                 << vertices[minIndice * 6 + 3] << ", " 
                 << vertices[minIndice * 6 + 4] << ", " 
                 << vertices[minIndice * 6 + 5] << ")" << endl;
            cout << "========================================" << endl;
        } else {
            cout << "  [X] 附近没有点" << endl;
            cout << "========================================" << endl;
        }
    }
}

void interactionFunctions() {
    GLfloat bunnySpeed = 30.0f * deltaTime;
    
    if (keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, bunnySpeed);
    if (keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, bunnySpeed);
    if (keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, bunnySpeed);
    if (keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, bunnySpeed);
    
    if (keys[GLFW_KEY_LEFT])
        modelBunny = translate(modelBunny, vec3(bunnySpeed, 0, 0));
    if (keys[GLFW_KEY_RIGHT])
        modelBunny = translate(modelBunny, vec3(-bunnySpeed, 0, 0));
    if (keys[GLFW_KEY_J])
        modelBunny = rotate(modelBunny, radians(bunnySpeed), vec3(0.f, 0.f, 1.f));
    if (keys[GLFW_KEY_L])
        modelBunny = rotate(modelBunny, radians(-bunnySpeed), vec3(0.f, 0.f, 1.f));
    if (keys[GLFW_KEY_I])
        modelBunny = rotate(modelBunny, radians(-bunnySpeed), vec3(1.f, 0.f, 0.f));
    if (keys[GLFW_KEY_K])
        modelBunny = rotate(modelBunny, radians(bunnySpeed), vec3(1.f, 0.f, 0.f));
    
    if (keys[GLFW_KEY_DOWN])
        modelBunny = scale(modelBunny, vec3(1.0f - 0.1f * bunnySpeed));
    if (keys[GLFW_KEY_UP])
        modelBunny = scale(modelBunny, vec3(1.0f + 0.1f * bunnySpeed));
}

int main() {
    SetConsoleOutputCP(65001);
    description();
    GLFWwindow* window = init();
    
    Shader shader("bunny.vs", "bunny.fs");
    
    parseNormal(normalFile, vertices, indices, vertices_size, indices_size);
    
    unsigned int bunnyVAO, bunnyVBO, bunnyEBO;
    glGenVertexArrays(1, &bunnyVAO);
    glGenBuffers(1, &bunnyVBO);
    glGenBuffers(1, &bunnyEBO);
    
    glBindVertexArray(bunnyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bunnyVBO);
    glBufferData(GL_ARRAY_BUFFER, 6*vertices_size*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bunnyEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * indices_size * sizeof(GLuint), 
                 indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 
                         (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    modelBunny = rotate(modelBunny, radians(135.0f), vec3(0.0f, 1.0f, 0.0f));
    
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        interactionFunctions();
        
        if(!cursorDisabled)
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.use();
        shader.setVec3("uColor", vec3(0.8f, 0.6f, 0.4f));
        
        mat4 projection = perspective(radians(camera.Zoom), 
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", modelBunny);
        
        glBindVertexArray(bunnyVAO);
        glDrawElements(GL_TRIANGLES, 3*indices_size, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        // 如果选中了点且开启显示，绘制高亮立方体
        if (showPointInfo && selectedPointIndice < vertices_size) {
            // 小立方体始终实体渲染
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            Shader selectShader("select.vs", "select.fs");
            vec4 now(modelBunny * vec4(vertices[selectedPointIndice * 6], 
                                       vertices[selectedPointIndice * 6 + 1], 
                                       vertices[selectedPointIndice * 6 + 2], 1.0f));
            
            mat4 model = translate(mat4(1.0f), vec3(now.x, now.y, now.z));
            model = scale(model, vec3(0.15f));
            view = camera.GetViewMatrix();

            selectShader.use();
            selectShader.setVec3("uColor", vec3(1.0f, 0.0f, 0.0f));
            selectShader.setMat4("projection", projection);
            selectShader.setMat4("view", view);
            selectShader.setMat4("model", model);
            
            unsigned int selectVAO, selectVBO;
            glGenVertexArrays(1, &selectVAO);
            glGenBuffers(1, &selectVBO);
            
            float cubeVertices[] = {
                -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
                 0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
                 0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,
                 0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
                -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,
                 0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,
                -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
                -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
                 0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
                -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f
            };
            
            glBindBuffer(GL_ARRAY_BUFFER, selectVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
            
            glBindVertexArray(selectVAO);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            glDrawArrays(GL_TRIANGLES, 0, 36);
            
            glDeleteVertexArrays(1, &selectVAO);
            glDeleteBuffers(1, &selectVBO);

            // 恢复线框/实体模式
            if (!cursorDisabled)
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &bunnyVAO);
    glDeleteBuffers(1, &bunnyVBO);
    
    glfwTerminate();
    return 0;
}
