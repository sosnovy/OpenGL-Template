#include "playground.h"

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Include GLM
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Include chrono for sleep command
#include <chrono>
#include <thread>

#include <common/shader.hpp>

int main(void)
{
    //Initialize window
    bool windowInitialized = initializeWindow();
    if (!windowInitialized) return -1;

    //Initialize vertex buffer
    bool vertexbufferInitialized = initializeVertexbuffer();
    if (!vertexbufferInitialized) return -1;

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");

    // Get a handle for our "MVP" and "MV" uniforms and update them for initalization 
    MatrixIDMV = glGetUniformLocation(programID, "MV");
    MatrixID = glGetUniformLocation(programID, "MVP");
    updateMVPTransformation();

    //initialize pose variables
    curr_x = 0;
    curr_y = 0;
    curr_angle = 0;

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    //start animation loop until escape key is pressed
    do {

        updateAnimationLoop();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    //Cleanup and close window
    cleanupVertexbuffer();
    glDeleteProgram(programID);
    closeWindow();

    return 0;
}



float zoom = 5;

void updateAnimationLoop()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // Update the variables for movement / rotation if a key was pressed
    if (glfwGetKey(window, GLFW_KEY_W)) zoom -= 0.5;
    else if (glfwGetKey(window, GLFW_KEY_S)) zoom += 0.5;
    else if (glfwGetKey(window, GLFW_KEY_A)) curr_angle -= 0.02;
    else if (glfwGetKey(window, GLFW_KEY_D)) curr_angle += 0.02;
    else if (glfwGetKey(window, GLFW_KEY_R)) toggleRot = !toggleRot;

    if (toggleRot) curr_angle +=0.02;

    // Update the MVP transformation with the new values
    updateMVPTransformation();

    // Send our transformation to the currently bound shader, 
    // in the "MVP" uniform and also the "MV" uniform
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(MatrixIDMV, 1, GL_FALSE, &MV[0][0]);
    
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    // Set our "myTextureSampler" sampler to use Texture Unit 0
    glUniform1i(texID, 0);


    sphere.DrawObject();

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool initializeWindow()
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Example: 3D Model with basic shading", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return false;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    return true;
}



bool updateMVPTransformation()
{
    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(30.0f), 4.0f / 3.0f, 0.1f, 10000.0f);

    // Camera matrix (modify this to let the camera move)
    glm::mat4 View = glm::lookAt(
        glm::vec3(0, 0, zoom), // Camera is at (0,0,0), in World Space
        glm::vec3(0, 0, 0), // and looks at the origin
        glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix, example with 3 parameters (curr_angle, curr_x, curr_y)
    glm::mat4 Model = glm::mat4(1.0f); //start with identity matrix
    //initial sphere rotations 
    Model = glm::rotate(Model, -1.5708f, glm::vec3(1.0f, 0.0f, 0.0f)); //apply orientation (last parameter: axis)
    Model = glm::rotate(Model, 0.401426f, glm::vec3(0.0f, 1.0f, 0.0f)); //apply orientation (last parameter: axis)

    //earth rotation
    Model = glm::rotate(Model, curr_angle, glm::vec3(0.0f, 0.0f, 1.0f)); //apply orientation (last parameter: axis)


    Model = glm::translate(Model, glm::vec3(curr_x, curr_y, 0.0f)); //apply translation

    // Our ModelViewProjection : multiplication of our 3 matrices
    MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
    MV = View * Model; //We also need MV in the shader to transform the light position

    return true;
}

bool initializeVertexbuffer()
{
    textureSamplerID = glGetUniformLocation(programID, "myTextureSampler");
    //this is only used for loading the uvs
    bool res = loadOBJ("earth.obj", vertices, uvs, normals);

    sphere = RenderingObject();
    sphere.InitializeVAO();
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //create vertex and normal data
    std::vector< glm::vec3 > vertices = std::vector< glm::vec3 >();
    std::vector< glm::vec3 > normals = std::vector< glm::vec3 >();
    loadSTLFile(vertices, normals, "newsphere.stl");

    vertexbuffer_size = vertices.size() * sizeof(glm::vec3);

    sphere.SetVertices(vertices);
    sphere.computeVertexNormalsOfTriangles(vertices, normals);
    sphere.SetNormals(normals);

    sphere.textureSamplerID = glGetUniformLocation(programID, "myTextureSampler");
    float scaling = 1.0f;
    std::vector< glm::vec2 > uvbufferdata = uvs;
    sphere.SetTexture(uvbufferdata, "earthtexture.bmp");
    

    glGenBuffers(2, vertexbuffer); //generate two buffers, one for the vertices, one for the normals

    ////fill first buffer (vertices)
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    ////fill second buffer (normals)
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    return true;
}


bool cleanupVertexbuffer()
{
    // Cleanup VBO
    glDeleteBuffers(2, vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);
    glDeleteTextures(1, &Texture);
    glDeleteVertexArrays(1, &VertexArrayID);
    return true;
}

bool closeWindow()
{
    glfwTerminate();
    return true;
}

void loadSTLFile(std::vector< glm::vec3 >& vertices,
    std::vector< glm::vec3 >& normals,
    std::string stl_file_name)
{
    stl::stl_data info = stl::parse_stl(stl_file_name);
    std::vector<stl::triangle> triangles = info.triangles;
    for (int i = 0; i < info.triangles.size(); i++) {
        stl::triangle t = info.triangles.at(i);
        glm::vec3 triangleNormal = glm::vec3(t.normal.x,
            t.normal.y,
            t.normal.z);
        //add vertex and normal for point 1:
        vertices.push_back(glm::vec3(t.v1.x, t.v1.y, t.v1.z));
        normals.push_back(triangleNormal);
        //add vertex and normal for point 2:
        vertices.push_back(glm::vec3(t.v2.x, t.v2.y, t.v2.z));
        normals.push_back(triangleNormal);
        //add vertex and normal for point 3:
        vertices.push_back(glm::vec3(t.v3.x, t.v3.y, t.v3.z));
        normals.push_back(triangleNormal);
    }
}
