#version 330 core

// 输入变量也叫顶点属性(Vertex Attribute),能声明的顶点属性是有上限的
// 为了定义顶点数据该如何管理，使用location这一个元数据指定输入变量，这样才可以在CPU上配置顶点属性。
layout (location = 0) in vec3 aPos;	// 位置变量属性 = 0
layout (location = 1) in vec3 aNormal;	

out vec3 FragPos;	// 片元的位置
out vec3 Normal;    // 添加法向量

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
