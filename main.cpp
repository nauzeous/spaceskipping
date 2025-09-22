#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "main.h"



using namespace std;
using namespace glm;




void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}  




f32 sdf(const vec3& p){
    vec3 w = p;

    f32 m = dot(w,w);

    vec4 trap = vec4(abs(w),m);
    f32 dz = 1.0f;

    for( int i=0; i<4; i++ ){
        dz = 8.0f*pow(m,3.5f)*dz + 1.0f;

        f32 r = length(w);
        f32 b = 8.0*acos(w.y/r);
        f32 a = 8.0*atan(w.x, w.z);

        w = p + (f32)pow(r,8.0f) * vec3( sin(b)*sin(a), cos(b), sin(b)*cos(a) );


        m = dot(w,w);
        if(m > 256.0f ) break;
    
    }

    return 0.25f*log(m)*sqrt(m)/dz;
}


void processInput(GLFWwindow *window, vec3& camera_pos){
    float step = 0.05*sdf(camera_pos);
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
    else if (glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS){
        camera_pos.x -= step;
    } 
    else if (glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS){
        camera_pos.z += step;
    } 
    else if (glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS){
        camera_pos.z -= step;
    } 
    else if (glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS){
        camera_pos.x += step;
    }
}


vec3 central_difference(const vec3& p) {;
	const f32 eps = 1e-4;
	vec3 diff;
	diff.x = sdf(p + vec3(eps,0,0)) - sdf(p - vec3(eps,0,0));
	diff.y = sdf(p + vec3(0,eps,0)) - sdf(p - vec3(0,eps,0));
	diff.z = sdf(p + vec3(0,0,eps)) - sdf(p - vec3(0,0,eps));
	return normalize(diff);
}

bool contains_volume(const node_data& node){
	f32 root3 = sqrt(3.0f);

	f32 dist = sdf(node.centre);
	
	if (dist > root3*node.half_length && dist > 0.0f){ // circumscribed sphere around cube
		return false;
	}
	
	if (dist < node.half_length){ // inscribed sphere inside cube
		return true;
	}

	vec3 centre = node.centre;
	vec3 dir = central_difference(centre);
	f32 max_component = node.half_length / dist;
	return (abs(dir.x)<max_component) &&
		   (abs(dir.y)<max_component) &&
		   (abs(dir.z)<max_component);
}

    
int main(){
	octtree tree;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    Shader ourShader("vertex.glsl", "shader.glsl"); // you can name your shader files however you like

    

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    f32 vertices[] = {
    -1.0f, -1.0f,
     3.0f, -1.0f,
    -1.0f,  3.0f
	};

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);

    //tree.upload_to_gpu();
    //printf("mem usage: %d KB \n", tree.nodes.size()*40 / 1000);


    f32 last_time = glfwGetTime();
    f32 last_update_time = last_time;
    f32 accumulated_time = 0.0f;

    vec3 camera_pos = vec3(0,0,-2);

    while (!glfwWindowShouldClose(window)){
        // input
        // -----
        f32 current_time = glfwGetTime();
        f32 dt = current_time - last_time;
        last_time = current_time;
        accumulated_time += dt;

        processInput(window,camera_pos);

        if (accumulated_time >= FRAME_TIME){

            float update_dt = current_time - last_update_time;
            last_update_time = current_time;
            accumulated_time = 0.f;

            // render
            // ------
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);


            // render the triangle
            ourShader.use();
            
            ourShader.setVec2("resolution",SCREEN_WIDTH,SCREEN_HEIGHT);
            ourShader.setFloat("min_leaf_size",MIN_LEAF_SIZE);
            ourShader.setVec3("camera_pos",camera_pos.x,camera_pos.y,camera_pos.z);
            //tree.rebind();
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            // -------------------------------------------------------------------------------
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();

	return 0;
}