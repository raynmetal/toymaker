#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <filesystem>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>

#include "toymaker/engine/shader_program.hpp"

using namespace ToyMaker;

GLint loadAndCompileShader(const std::vector<std::string>& shaderPaths, GLuint& shaderID, GLuint shaderType);
void freeProgram(GLuint programID);
GLuint buildProgram(const std::vector<std::string>& vertexPaths, const std::vector<std::string>& fragmentPaths);
GLuint buildProgram(const std::vector<std::string>& vertexPaths, const std::vector<std::string>& fragmentPaths, const std::vector<std::string>& geometryPaths);

ShaderProgram::ShaderProgram(GLuint program):
Resource<ShaderProgram>{0},
mID { program }
{
    assert(mID && "Must be the mID of a shader program tracked by OpenGL");
}


GLint loadAndCompileShader(const std::vector<std::string>& shaderPaths, GLuint& shaderID, GLuint shaderType) {
    //Read shader text
    const char** shaderSource { new const char* [shaderPaths.size()] };
    std::vector<std::string> shaderStrings(shaderPaths.size());
    for(std::size_t i{0}; i < shaderPaths.size(); ++i) {
        //enable file operation exceptions
        std::ifstream shaderFile;
        shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        bool readSuccess {true};
        try {
            shaderFile.open(shaderPaths[i]);
            // Read file buffer's contents into string stream
            std::stringstream shaderStream {};
            shaderStream << shaderFile.rdbuf();
            //Close file handles
            shaderFile.close();
            //Convert stream into string
            shaderStrings[i] = shaderStream.str();
            shaderSource[i] = shaderStrings[i].c_str();
        } catch(std::ifstream::failure& e) {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << shaderPaths[i] << std::endl;
            readSuccess = false;
        }
        if(!readSuccess) { 
            delete[] shaderSource;
            return GL_FALSE;
        }
    }

    //Variables for storing compilation success
    GLint success;
    char infoLog[512];

    //create vertex shader
    shaderID = glCreateShader(shaderType);
    glShaderSource(
        shaderID,
        shaderPaths.size(),
        shaderSource,
        NULL
    );
    glCompileShader(shaderID);
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    //Source no longer required, so remove it
    delete[] shaderSource;
    if(success != GL_TRUE) {
        glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
        std::string typeString {};
        switch(shaderType) {
            case GL_VERTEX_SHADER: typeString = "VERTEX"; break;
            case GL_GEOMETRY_SHADER: typeString = "GEOMETRY"; break;
            case GL_FRAGMENT_SHADER: typeString = "FRAGMENT"; break;
            default: typeString = "UNKNOWN"; break;
        }
        std::cout << "ERROR::SHADER::" << typeString << "::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(shaderID);
        shaderID = 0;
    }

    return success;
}

ShaderProgram::~ShaderProgram() {
    freeProgram(mID);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept :
Resource<ShaderProgram>{0},
mID {other.mID}
{
    // Prevent other from destroying resource 
    // when its destructor is called
    other.releaseResource();
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept {
    // Do nothing on self assignment
    if(&other == this) return *this;

    // Destroy whatever we've currently got to make room
    destroyResource();

    //Copy other
    mID = other.mID;

    //Prevent other from destroying moved resource when its
    //destructor is called
    other.releaseResource();

    return *this;
}

GLuint ShaderProgram::getProgramID() const { return mID; }
void ShaderProgram::use() const { glUseProgram(mID); }

GLint ShaderProgram::getLocationAttribArray(const std::string& name) const {
    return glGetAttribLocation(mID, name.c_str());
}
GLint ShaderProgram::getLocationUniform(const std::string& name) const {
    return glGetUniformLocation(mID, name.c_str());
}
GLint ShaderProgram::getLocationUniformBlock(const std::string& name) const {
    return glGetUniformBlockIndex(mID, name.c_str());
}

void ShaderProgram::enableAttribArray(const std::string& name) const {
    GLint loc {getLocationAttribArray(name)};
    if(loc<0) return;
    glEnableVertexAttribArray(loc);
}
void ShaderProgram::enableAttribArray(GLint locationAttrib) const {
    if(locationAttrib<0) return;
    glEnableVertexAttribArray(locationAttrib);
}
void ShaderProgram::disableAttribArray(const std::string& name) const {
    GLint loc {getLocationAttribArray(name)};
    if(loc<0) return;
    glDisableVertexAttribArray(loc);
}
void ShaderProgram::disableAttribArray(GLint locationAttrib) const {
    if(locationAttrib<0) return;
    glEnableVertexAttribArray(locationAttrib);
}

void ShaderProgram::setUBool(const std::string& name, bool value) const {
    GLint loc {getLocationUniform(name)};
    if(loc<0) return;
    glUniform1i(
        loc,
        static_cast<GLint>(value)
    );
}
void ShaderProgram::setUInt(const std::string& name, int value) const {
    GLint loc {getLocationUniform(name)};
    if(loc<0) return;
    glUniform1i(
        loc,
        static_cast<GLint>(value)
    );
}
void ShaderProgram::setUFloat(const std::string& name, float value) const {
    GLint loc {getLocationUniform(name)};
    if(loc<0) return;
    glUniform1f(
        loc,
        static_cast<GLfloat>(value)
    );
}
void ShaderProgram::setUVec2(const std::string& name, const glm::vec2& value) const {
    GLint loc { getLocationUniform(name) };
    if(loc<0) return;
    glUniform2fv(
        loc,
        1,
        glm::value_ptr(value)
    );
}

void ShaderProgram::setUVec3(const std::string& name, const glm::vec3& value) const {
    GLint loc {getLocationUniform(name)};
    if(loc<0) return;
    glUniform3fv(
        loc,
        1,
        glm::value_ptr(value)
    );
}
void ShaderProgram::setUVec4(const std::string& name, const glm::vec4& value) const {
    GLint loc {getLocationUniform(name)};
    if(loc<0) return;
    glUniform4fv(
        loc,
        1,
        glm::value_ptr(value)
    );
}
void ShaderProgram::setUMat4(const std::string& name, const glm::mat4& value) const {
    GLint loc {getLocationUniform(name)};
    if(loc<0) return;
    glUniformMatrix4fv(
        loc,
        1,
        GL_FALSE,
        glm::value_ptr(value)
    );
}
void ShaderProgram::setUniformBlock(const std::string& name, GLuint bindingPoint) const {
    GLint loc {getLocationUniformBlock(name)};
    if(loc<0) return;
    glUniformBlockBinding(
        mID,
        loc,
        bindingPoint
    );
}

void freeProgram(GLuint programID) {
    if(programID)
        glDeleteProgram(programID);
}

void ShaderProgram::destroyResource() {
    freeProgram(mID);
    mID = 0;
}

void ShaderProgram::releaseResource() {
    mID = 0;
}

std::shared_ptr<IResource> ShaderProgramFromFile::createResource(const nlohmann::json& methodParameters) {
    GLuint shaderProgram { 0 };
    std::ifstream jsonFileStream;
    std::string programJSONPath { methodParameters.at("path").get<std::string>() };

    // Load parent shader program definition
    jsonFileStream.open(programJSONPath);
    nlohmann::json programJSON{ nlohmann::json::parse(jsonFileStream) };
    jsonFileStream.close();
    nlohmann::json::iterator endIter { programJSON[0].end() };
    nlohmann::json::iterator typeIter { programJSON[0].find("type") };
    if(typeIter == endIter || *typeIter != "shader/program") {
        throw std::invalid_argument("Shader program JSON file " + programJSONPath + " is not of type \"shader/program\".");
    }
    nlohmann::json::iterator vertexIter { programJSON[0].find("vertexShader") };
    nlohmann::json::iterator fragmentIter { programJSON[0].find("fragmentShader") };
    if(vertexIter == endIter || fragmentIter == endIter) {
        throw std::invalid_argument("Shader program JSON file " + programJSONPath + " does not contain appropriate fragment or vertex shader definitions.");
    }
    std::filesystem::path programPath { programJSONPath };
    std::filesystem::path parentDirectory { programPath.parent_path() };
    std::filesystem::path vertexJSONPath { parentDirectory / std::string(*vertexIter) };
    std::filesystem::path fragmentJSONPath { parentDirectory / std::string(*fragmentIter) };

    // load vertex shader definition
    std::vector<std::string> vertexSources {};
    jsonFileStream.open(vertexJSONPath.string());
    nlohmann::json vertexJSON { nlohmann::json::parse(jsonFileStream) };
    jsonFileStream.close();
    typeIter = vertexJSON[0].find("type");
    if(typeIter == vertexJSON[0].end() || *typeIter != "shader/vertex") {
        throw std::invalid_argument("Vertex shader JSON file " + vertexJSONPath.string() + " is not of type \"shader/vertex\".");
    }
    std::filesystem::path vertexJSONDirectory { vertexJSONPath.parent_path() };
    for(std::string source : vertexJSON[0]["sources"]) {
        vertexSources.push_back((vertexJSONDirectory / source).string());
    }

    // load fragment shader definition
    std::vector<std::string> fragmentSources {};
    jsonFileStream.open(fragmentJSONPath.string());
    nlohmann::json fragmentJSON { nlohmann::json::parse(jsonFileStream) };
    jsonFileStream.close();
    typeIter = fragmentJSON[0].find("type");
    if(typeIter == fragmentJSON[0].end() || *typeIter != "shader/fragment") {
        throw std::invalid_argument("Fragment shader JSON file " + fragmentJSONPath.string() + " is not of type \"shader/fragment\".");
    }
    std::filesystem::path fragmentJSONDirectory { fragmentJSONPath.parent_path() };
    for(std::string source : fragmentJSON[0]["sources"]) {
        fragmentSources.push_back((fragmentJSONDirectory / source).string());
    }

    shaderProgram = buildProgram(vertexSources, fragmentSources);
    assert(shaderProgram && "failed to load or compile the shader program");
    return std::make_shared<ShaderProgram>(shaderProgram);
}

GLuint buildProgram(const std::vector<std::string>& vertexPaths, const std::vector<std::string>& fragmentPaths, const std::vector<std::string>& geometryPaths) {
    GLuint shaderProgram {};
    GLuint vertexShader {};
    GLuint fragmentShader {};
    GLuint geometryShader {};

    bool vShaderCompiled { GL_TRUE ==loadAndCompileShader(vertexPaths, vertexShader, GL_VERTEX_SHADER) };
    assert(vShaderCompiled && "Could not compile vertex shader");
    bool fShaderCompiled { GL_TRUE == loadAndCompileShader(fragmentPaths, fragmentShader, GL_FRAGMENT_SHADER) };
    if(!fShaderCompiled ) {
        glDeleteShader(vertexShader);
    }
    assert(fShaderCompiled && "Could not compile fragment shader");
    bool gShaderCompiled { GL_TRUE == loadAndCompileShader(geometryPaths, geometryShader, GL_GEOMETRY_SHADER) };
    if(!gShaderCompiled) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    assert(gShaderCompiled && "Could not compile geometry shader");

    //Create a shader program
    GLint success {};
    char infoLog[512];   
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glAttachShader(shaderProgram, geometryShader);
    glLinkProgram(shaderProgram);

    //Clean up (now unnecessary) shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    //Report failure, if it occurred
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(success != GL_TRUE) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
            << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    assert(success == GL_TRUE && "Failed to link shader program");

    return shaderProgram;
}

GLuint buildProgram(const std::vector<std::string>& vertexPaths, const std::vector<std::string>& fragmentPaths){
    GLuint shaderProgram {};
    GLuint vertexShader;
    GLuint fragmentShader;

    bool vShaderCompiled { GL_TRUE == loadAndCompileShader(vertexPaths, vertexShader, GL_VERTEX_SHADER) };
    assert(vShaderCompiled && "Could not compile vertex shader");
    bool fShaderCompiled { GL_TRUE == loadAndCompileShader(fragmentPaths, fragmentShader, GL_FRAGMENT_SHADER) };
    if(!fShaderCompiled) {
        glDeleteShader(vertexShader);
    }
    assert(fShaderCompiled && "Could not compile fragment shader");

    //Create a shader program
    GLint success {};
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    //Clean up (now unnecessary) shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //Report failure, if it occurred
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(success != GL_TRUE) {
        char infoLog[4096];
        glGetProgramInfoLog(shaderProgram, 4096, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
            << infoLog << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    assert(success == GL_TRUE && "Failed to link vertex and fragment shaders");

    // Store build success
    return shaderProgram;
}
