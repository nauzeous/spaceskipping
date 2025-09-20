#include <glm/glm.hpp>
#include <vector>
#include <stdint.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>


#define ROOT_OCT_SIZE 6.0f
#define MIN_LEAF_SIZE .02f

#define SCREEN_WIDTH 1600.0f
#define SCREEN_HEIGHT 900.0f

#define MAX_FPS 60.0f
#define FRAME_TIME 1.f/MAX_FPS

#define PAD4B 0.0f
#define PAD12B {0.0f,0.0f,0.0f}

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef float f32;
typedef double f64;

using namespace glm;

// boring boilerplate, copy pasted from 
// https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/shader_s.h
class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try 
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode   = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() 
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, f32 value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }

    void setVec2(const std::string &name, f32 v0 , f32 v1) const
    {
    	glUniform2f(glGetUniformLocation(ID,name.c_str()),v0,v1);
    }

    void setVec3(const std::string &name, f32 v0, f32 v1, f32 v2){
        glUniform3f(glGetUniformLocation(ID,name.c_str()),v0,v1,v2);
    }

private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

struct octtree_node{
	u32 parent_idx;
	u32 child_idx; //first child, next 7 are stored contiguously
};

struct node_data{
    // apparently glm vec3 is 12 bytes, but in glsl its 16?!!? need padding
	vec3 centre;
    f32 pad0;
	f32 half_length;
    f32 pad1[3];
};

bool contains_volume(const node_data& node);

class octtree{
public:
	std::vector<octtree_node> structure;
	std::vector<node_data> nodes;

    GLuint ssbo0;
    GLuint ssbo1;


	void divide_tree(u32 idx){
		f32 new_half_length = nodes[idx].half_length / 2.0f;

		vec3 parent_centre = nodes[idx].centre;
		vec3 child_pos = parent_centre - new_half_length;
		
		structure[idx].child_idx = structure.size();
				
		for(u32 i = 0;i<8;i++){
			structure.push_back({idx,0});

			vec3 current_oct;
			// generate corners of cube
			current_oct.x = i&1;
			current_oct.y = (i>>1)&1;
			current_oct.z = (i>>2)&1;

			nodes.push_back({child_pos + 2.0f*current_oct*new_half_length, PAD4B, new_half_length, PAD12B});
		}
	}

	void upload_to_gpu(){
		// make structure buffer

		int size = structure.size();
		int struc_size = size*sizeof(octtree_node);
		int nodes_size = size*sizeof(node_data);
		

        glGenBuffers(1, &ssbo0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo0);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 struc_size,
					 structure.data(),
					 GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo0);

		// make node data buffer
		glGenBuffers(1, &ssbo1);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo1);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 nodes_size,
					 nodes.data(),
					 GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo1);
	}



	octtree(){


		structure.push_back({0,0}); // root node has no parent, and no children yet
		
		vec3 root_centre = vec3(2.0f);
		nodes.push_back({root_centre,PAD4B,ROOT_OCT_SIZE,PAD12B});
			
		for(u32 i = 0;i<nodes.size();i++){
			if ( contains_volume(nodes[i]) && (nodes[i].half_length > MIN_LEAF_SIZE) ){
				divide_tree(i);
			}
		}
	}


    void print(){
        for(int i = 0;i<nodes.size();i++){
            std::cout << nodes[i].half_length << std::endl;
        }
    }

    void rebind(){
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,ssbo0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER,1,ssbo1);
    }
};

