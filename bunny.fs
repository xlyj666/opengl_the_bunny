// Shader function executes once per fragment
// Outputs color of surface at the current fragment's screen sample position

#version 330 core   // 版本号
out vec4 FragColor; // 输出变量

///兔子面元着色器

// 定义材质
struct Material {
    vec3 ambient;   // 环境光
    vec3 diffuse;   // 漫反射
    vec3 specular;  // 镜面反射
    float shininess;    // 反光度
}; 

// 定义点光源
struct PointLight {
    vec3 position;
    
    float constant;
    float linear;
    float quadratic;
	
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// 定义聚光灯光源
struct SpotLight {
    vec3 position;  // 光源顶点位置
    vec3 direction; // 母线方向
    float cutOff;   // 切光角
    float outerCutOff;
  
    // 实现衰减的二次项的系数
    float constant; // 常量
    float linear;   // 一次项系数
    float quadratic;    // 二次项系数
  
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;       
};

in vec3 FragPos;    
in vec3 Normal; // 方向量

// 全局共享变量
uniform vec3 viewPos;   // 摄像机位置(观察点)
uniform PointLight pointLights; // 点光源
uniform SpotLight spotLight;    // 聚光灯光源
uniform Material material;  // 材质

// function prototypes
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light , vec3 normal , vec3 fragPos, vec3 viewDir);

void main(){    
    // 属性(需要标准化)
    vec3 norm = normalize(Normal);  // 法向
    vec3 viewDir = normalize(viewPos - FragPos);    // 最终方向向量
    
    // == =====================================================
    // Our lighting is set up in 2 phases: a point light and a flashlight
    // For each phase, a calculate function is defined that calculates the corresponding color per lamp. In the main() function we take all the calculated colors and sum them up for this fragment's final color.
    // == =====================================================
    
    // phase 1: point lights
    vec3 result = CalcPointLight(pointLights, norm, FragPos, viewDir);
    
    // phase 2: spot light
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);    
    
    // 设置片元颜色,采用RGBA
    FragColor = vec4(result, 1.0);
}

// 计算使用点光源时的光照
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射光照
    // 计算光源对当前片段实际的漫发射影响(两个向量之间的角度大于90度，点乘的结果就会变成负数，为此，我们使用max函数返回两个参数之间较大的参数，从而保证漫反射分量不会变成负数)
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面反射光照
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // 光线衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // combine results
    vec3 ambient = light.ambient * material.ambient;
    vec3 diffuse = light.diffuse * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);            //点光源颜色
}

// 计算使用聚光灯的颜色
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir){
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射着色
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 镜面光着色
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // 光照衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // 综合光照
    vec3 ambient = light.ambient * material.diffuse;
    vec3 diffuse = light.diffuse * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;
    return (ambient + diffuse + specular);    //聚光灯颜色
}
