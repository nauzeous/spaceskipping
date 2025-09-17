
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <vector>


using namespace std;
using namespace glm;

#define ROOT_OCT_SIZE 1000.0f
#define LEAF_NODE_SIZE 2.0f

#define SCREEN_WIDTH = 1600.0f
#define SCREEN_HEIGHT = 900.0f

struct octtree_node{
	u16 parent_idx;
	u16 child_idx; //first child, next 7 are stored contiguously
};

struct node_data{
	vec3 centre;
	float half_length;
};

float sdf(const vec3& pos){
	return length(pos)-2.0f;
}

vec3 central_difference(const vec3& p) {;
	const float eps = 1e-4;
	vec3 diff;
	diff.x = sdf(p + vec3(eps,0,0)) - sdf(p - vec3(eps,0,0));
	diff.y = sdf(p + vec3(0,eps,0)) - sdf(p - vec3(0,eps,0));
	diff.z = sdf(p + vec3(0,0,eps)) - sdf(p - vec3(0,0,eps));
	return normalize(diff);
}

bool contains_volume(const node_data& node){
	float root3 = sqrt(3.0f);

	float dist = sdf(node.centre);
	
	if (dist > root3*node.half_length){ // circumscribed sphere around cube
		return false;
	}
	
	if (dist < node.half_length){ // inscribed sphere inside cube
		return true;
	}

	vec3 centre = node.centre;
	vec3 dir = central_difference(centre);
	float max_component = node.half_length / dist;
	return (abs(dir.x)<max_component) &&
		   (abs(dir.y)<max_component) &&
		   (abs(dir.z)<max_component);
}

class octtree{
public:
	vector<octtree_node> structure;
	vector<node_data> nodes;

	GLuint structure_ssbo;
	GLuint nodes_ssbo;

	void divide_tree(u16 idx){
		float new_half_length = nodes[idx].half_length / 2.0f;

		vec3 parent_centre = nodes[idx].centre;
		vec3 child_pos = parent_centre - new_half_length;
		
		structure[idx].child_idx = structure.size();
		
		
				
		for(u8 i = 0;i<8;i++){
			structure.push_back({idx,0});

			vec3 current_oct;
			// generate corners of cube
			current_oct.x = i&1;
			current_oct.y = (i>>1)&1;
			current_oct.z = (i>>2)&1;

			nodes.push_back({child_pos + current_oct*new_half_length, new_half_length});
		}
	}

	void upload_to_gpu(){
		// make structure buffer
		glGenBuffers(1, &structure_ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, structure_ssbo);
		
		int size = structure.size();
		int struc_size = size*sizeof(octtree_node);
		int nodes_size = size*sizeof(node_data);
		
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 struc_size,
					 structure.data(),
					 GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0, structure_ssbo);

		// make node data buffer
		glGenBuffers(1, &nodes_ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, nodes_ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER,
					 nodes_size,
					 nodes.data(),
					 GL_STATIC_DRAW);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nodes_ssbo);
	}


	void build_tree(){

		u16 parent_idx = 0;
		u16 child_idx = 0;
		structure[0] = {parent_idx,child_idx};
		
		vec3 root_centre = vec3(0.0f);
		nodes[0] = {root_centre,ROOT_OCT_SIZE};
			
		for(u16 i = 0;i<structure.size();i++){
			if (contains_volume(nodes[i]))
				divide_tree(i);
		}
	}
};

void init_opengl(){

	unsigned int vertex_shader;
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source,NULL);
	glCompileShader(vertex_shader);

	unsigned int fragment_shader;
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source);
	glCompileShader(fragment_shader);

	unsigned int shader_program;
	shader_program = glCreateProgram();

	glAttachShader(shader_program,vertex_shader);
	glAttachShader(shader_program,fragment_shader);
	
	glLinkProgram(shader_program);
	glUseProgram(shaderProgram);

	// delete since program is linked
	glDeleteShader(vertex_shader); 
	glDeleteShader(fragment_shader);
}

void init_vbo(){
	float vertices[] = {
		-1.0,-1.0,
		 3.0,-1.0
		-1.0, 3.0
	};

	GLuint vao,vbo;
	glGenVertexArrays(1,&vao);
	glGenBuffers(1,&vbo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER,vbo);
	glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);

	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,2*sizeof(float),(void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}


int main(){
	octtree tree;
	init_opengl()
	glUniform2f(u_resolution,SCREEN_WIDTH,SCREEN_HEIGHT);
	
	tree.build_tree();
	return 0;
}