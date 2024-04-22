#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>



#include"shaderClass.h"
#include"VAO.h"
#include"VBO.h"
#include"EBO.h"

#define numParticlesx 10
#define numParticlesy 10
#define numParticlesz 100
#define numPlanes 4


int numFrames = 0;
bool simulating = false;

const int numParticles = numParticlesx * numParticlesy * numParticlesz;
const unsigned int width = 1920;
const unsigned int height = 1080;
const float g = -9.81;

const GLfloat gratio = ((1.0 + sqrt(5.0)) / 2.0);
const GLfloat scale = 1.0f;
const GLfloat minVelocity = 0.7f;

const GLfloat particleScale = 80.0f;
const float restitution = 0.3;
const float friction = 0.1;
const float radius = (0.809 * 4) / particleScale;
const float spacing = 1.9 * radius;
const float tableSpacing = 2 * radius;
const int tableSize = 2 * numParticles;
const float maxDist = 2 * radius;
int cellStart[tableSize + 1];
int cellEntries[numParticles];
int queryIDs[numParticles];
int querySize = 0;

glm::vec3 startingPoint = glm::vec3(0.5f, 1.0f, -25.0f);

glm::mat4 model;
glm::mat4 view;
glm::mat4 proj;



class particle
{
public:
    glm::vec3 position;
    glm::vec3 velocity;
    bool resting;
};

particle particles[numParticles];

class plane
{
    public:
        int numPoints;
        glm::vec3 points[3];
        glm::vec3 normal;
        glm::vec3 centre;
        glm::vec3 midPoints[3];
        glm::vec3 midNormals[3];

};

plane planes[numPlanes];

void printFPS(double fps) {
    std::cout << "FPS: " << round(fps) << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        std::cout << "Spacebar pressed. Starting simulation." << std::endl;
        simulating = true;
    }
}



int hashCoords(int x, int y, int z)
{
    int h = x * 92837111 + y * 689287499 + z * 283923481;
    return abs(h % tableSize);

}

int intCoord(float coord)
{
    return floor(coord / tableSpacing);

}

int hashPos(float x, float y, float z)
{

    return hashCoords(intCoord(x), intCoord(y), intCoord(z));

}

void create()
{
    std::fill(cellStart, cellStart + (tableSize + 1), 0);
    std::fill(cellEntries, cellEntries + numParticles, 0);

    for (int i = 0; i < numParticles; i ++) 
    {
        int h = hashPos(particles[i].position.x, particles[i].position.y, particles[i].position.z);
        cellStart[h]++;
    }

    int start = 0;

    for (int i = 0; i < tableSize; i++)
    {
        start += cellStart[i];
        cellStart[i] = start;
    }
    cellStart[tableSize] = start;

    for (int i = 0; i < numParticles; i++)
    {
        int h = hashPos(particles[i].position.x, particles[i].position.y, particles[i].position.z);
        cellStart[h]--;
        cellEntries[cellStart[h]] = i;
    }

}

void query(int nr)
{
    querySize = 0;
    for (int xi = intCoord(particles[nr].position.x - maxDist); xi < intCoord(particles[nr].position.x + maxDist); xi++)
    {
        for (int yi = intCoord(particles[nr].position.y - maxDist); yi < intCoord(particles[nr].position.y + maxDist); yi++)
        {
            for (int zi = intCoord(particles[nr].position.z - maxDist); zi < intCoord(particles[nr].position.z + maxDist); zi++)
            {
                int h = hashCoords(xi, yi, zi);
                int start = cellStart[h];
                int end = cellStart[h + 1];

                for (int i = start; i < end; i++)
                {
                   

                    queryIDs[querySize] = cellEntries[i];
                    querySize++;
                }
            }
        }
    }
}



GLfloat vertices[] =
{
    -1.0f / particleScale, gratio / particleScale,  0.0f / particleScale,     0.529f, 0.424f, 0.267f,
     1.0f / particleScale, gratio / particleScale, 0.0f / particleScale,     0.776f, 0.690f, 0.565f,
     -1.0f / particleScale, -gratio / particleScale,  0.0f / particleScale,     0.529f, 0.424f, 0.267f,
     1.0f / particleScale, -gratio / particleScale, 0.0f / particleScale,     0.776f, 0.690f, 0.565f,
     0.0f / particleScale, -1.0f / particleScale,  gratio / particleScale,     0.529f, 0.424f, 0.267f,
     0.0f / particleScale, 1.0f / particleScale, gratio / particleScale,    0.776f, 0.690f, 0.565f,
     0.0f / particleScale, -1.0f / particleScale,  -gratio / particleScale,     0.529f, 0.424f, 0.267f,
     0.0f / particleScale, 1.0 / particleScale, -gratio / particleScale,     0.776f, 0.690f, 0.565f,
     gratio / particleScale, 0.0f / particleScale,  -1.0f / particleScale,    0.529f, 0.424f, 0.267f,
     gratio / particleScale, 0.0f / particleScale, 1.0f / particleScale,     0.776f, 0.690f, 0.565f,
     -gratio / particleScale, 0.0f / particleScale,  -1.0f / particleScale,    0.529f, 0.424f, 0.267f,
     -gratio / particleScale, 0.0f / particleScale, 1.0f / particleScale,     0.776f, 0.690f, 0.565f,
};

GLuint indices[]
{
    0, 11, 5,
    0, 5, 1,
    0, 1, 7,
    0, 7, 10,
    0, 10, 11,
    1, 5, 9,
    5, 11, 4,
    11, 10, 2,
    10, 7, 6,
    7, 1, 8,
    3, 9, 4,
    3, 4, 2,
    3, 2, 6,
    3, 6, 8,
    3, 8, 9,
    4, 9, 5,
    2, 4, 11,
    6, 2, 10,
    8, 6, 7,
    9, 8, 1

};
GLfloat planeVertices[] =
{
   
   0.0f, -1.0f, -30.0f,  0.53f, 0.70f, 0.44f,
   8.0f, 0.0f, -15.0f,  0.13f, 0.10f, 0.6f,
   8.0f, 0.0f, -30.0f,  0.53f, 0.70f, 0.44f,

   0.0f, -1.0f, -15.0f,  0.13f, 0.10f, 0.6f,
   0.0f, -1.0f, -30.0f,  0.53f, 0.70f, 0.44f,
   8.0f, 0.0f, -15.0f,  0.13f, 0.10f, 0.6f,


   2.0f, -5.0f, -30.0f,  0.53f, 0.70f, 0.44f,
   -10.0f, -3.0f, -15.0f,  0.13f, 0.10f, 0.6f,
   -10.0f, -3.0f, -30.0f,  0.53f, 0.70f, 0.44f,

   2.0f, -5.0f, -15.0f,  0.13f, 0.10f, 0.6f,
   2.0f, -5.0f, -30.0f,  0.53f, 0.70f, 0.44f,
   -10.0f, -3.0f, -15.0f,  0.13f, 0.10f, 0.6f
    

    
};



GLuint planeIndices[] =
{
    0, 1, 2,
    3, 4, 5,
    6, 7, 8,
    9, 10, 11
    
};





void planeInit()
{
    for (int i = 0; i < numPlanes; i++)
    {
        int i0 = planeIndices[3 * i];
        int i1 = planeIndices[3 * i + 1];
        int i2 = planeIndices[3 * i + 2];
        planes[i].points[0].x = planeVertices[6 * i0];
        planes[i].points[0].y = planeVertices[6 * i0 + 1];
        planes[i].points[0].z = planeVertices[6 * i0 + 2];
        planes[i].points[1].x = planeVertices[6 * i1];
        planes[i].points[1].y = planeVertices[6 * i1 + 1];
        planes[i].points[1].z = planeVertices[6 * i1 + 2];
        planes[i].points[2].x = planeVertices[6 * i2];
        planes[i].points[2].y = planeVertices[6 * i2 + 1];
        planes[i].points[2].z = planeVertices[6 * i2 + 2];
        planes[i].normal = glm::cross(planes[i].points[1] - planes[i].points[0], planes[i].points[2] - planes[i].points[0]);
        planes[i].normal = glm::normalize(planes[i].normal);
        if (planes[i].normal.y < 0)
        {
            planes[i].normal *= -1;
        }
        planes[i].centre = (planes[i].points[0] + planes[i].points[1] + planes[i].points[2]) / 3.0f;
        planes[i].midPoints[0] = 0.5f * (planes[i].points[0] + planes[i].points[1]);
        planes[i].midPoints[1] = 0.5f * (planes[i].points[1] + planes[i].points[2]);
        planes[i].midPoints[2] = 0.5f * (planes[i].points[2] + planes[i].points[0]);
        planes[i].midNormals[0] = glm::cross((planes[i].points[1] - planes[i].points[0]), planes[i].normal);
        planes[i].midNormals[1] = glm::cross((planes[i].points[2] - planes[i].points[1]), planes[i].normal);
        planes[i].midNormals[2] = glm::cross((planes[i].points[0] - planes[i].points[2]), planes[i].normal);

        if (glm::dot(planes[i].midNormals[0], planes[i].centre - planes[i].midPoints[0]) < 0)
        {
            planes[i].midNormals[0] *= -1;
        }
        if (glm::dot(planes[i].midNormals[1], planes[i].centre - planes[i].midPoints[1]) < 0)
        {
            planes[i].midNormals[1] *= -1;
        }
        if (glm::dot(planes[i].midNormals[2], planes[i].centre - planes[i].midPoints[2]) < 0)
        {
            planes[i].midNormals[2] *= -1;
        }


        

       

        
    }

   


}
void checkPlaneCollision(int i)
{
    for (int j = 0; j < numPlanes; j++)
    {
        GLfloat distance = glm::dot(particles[i].position - planes[j].centre, planes[j].normal);

        if (
            (distance < 0.0) && (distance > -3) &&
            (glm::dot((planes[j].midNormals[0]), (particles[i].position - planes[j].midPoints[0])) > 0) &&
            (glm::dot((planes[j].midNormals[1]), (particles[i].position - planes[j].midPoints[1])) > 0) &&
            (glm::dot((planes[j].midNormals[2]), (particles[i].position - planes[j].midPoints[2])) > 0)

            )


        {
            glm::vec3 perp = particles[i].velocity - (glm::dot(particles[i].velocity, planes[j].normal) * planes[j].normal);
            particles[i].position = particles[i].position + abs(distance) * planes[j].normal;
            particles[i]. velocity = (1 - friction) * perp + restitution * abs(glm::dot(particles[i].velocity, planes[j].normal)) * planes[j].normal;
        }
    }

}





int main()
{
    
    glfwInit();
    planeInit();



    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, "Project", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }




    glfwMakeContextCurrent(window);
    gladLoadGL();


    glViewport(0, 0, width, height);



    Shader shaderProgram("Resource Files\\default.vert", "Resource Files\\default.frag");


    VAO VAO_plane;
    VAO_plane.Bind();
    VBO VBO_plane(planeVertices, sizeof(planeVertices));
    EBO EBO_plane(planeIndices, sizeof(planeIndices));
    VAO_plane.LinkAttrib(VBO_plane, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    VAO_plane.LinkAttrib(VBO_plane, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
   



    VAO VAO1;
    VAO1.Bind();

    VBO VBO1(vertices, sizeof(vertices));
    EBO EBO1(indices, sizeof(indices));

    VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 6 * sizeof(float), (void*)0);
    VAO1.LinkAttrib(VBO1, 1, 3, GL_FLOAT, 6 * sizeof(float), (void*)(3 * sizeof(float)));
   


    VAO1.Unbind();
    VBO1.Unbind();
    EBO1.Unbind();
    VAO_plane.Unbind();
    VBO_plane.Unbind();
    EBO_plane.Unbind();

    GLuint uniID = glGetUniformLocation(shaderProgram.ID, "scale");
  


    float rotation = 0.0f;
    double prevTime = glfwGetTime();

    glEnable(GL_DEPTH_TEST);



    int particlesInArray = 0;
    for (int i = 0; i < numParticlesx;i++) {

        for (int j = 0; j < numParticlesy;j++) {

            for (int k = 0; k < numParticlesz;k++) {

                particles[particlesInArray].position = glm::vec3(startingPoint.x + spacing * i, startingPoint.y + spacing * j, startingPoint.z + spacing * k);
                particles[particlesInArray].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                particles[particlesInArray].resting = false;
                particlesInArray++;
            }

        }

    }
    




    shaderProgram.Activate();
    model = glm::mat4(1.0f);
    view = glm::mat4(1.0f);
    proj = glm::mat4(1.0f);

    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));

    proj = glm::perspective(glm::radians(55.0f), (float)(width / height), 0.1f, 100.0f);

    int modelLoc = glGetUniformLocation(shaderProgram.ID, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    int viewLoc = glGetUniformLocation(shaderProgram.ID, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    int projLoc = glGetUniformLocation(shaderProgram.ID, "proj");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    glUniform1f(uniID, scale);


    


    double lastTime = glfwGetTime();

    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window))
    {
        
        
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        

        double crntTime = glfwGetTime();
       

        


        
        

     

        if (crntTime - prevTime >= (1.0 / 60.0))
        {
            create();

            for (int i = 0; i < numParticles; i++)
            {
                if (particles[i].resting == true)
                {
                    continue;
                }
                query(i);

                for (int nr = 0; nr < querySize; nr++)
                {
                    int j = queryIDs[nr];

                    glm::vec3 normal = glm::vec3(particles[i].position.x - particles[j].position.x, particles[i].position.y - particles[j].position.y, particles[i].position.z - particles[j].position.z);
                    if ((glm::length(normal) <= 2 * radius) && (i != j))
                    {

                        float length = glm::length(normal);

                      

                        normal = glm::normalize(normal);


                        particles[i].position = particles[i].position + normal * length / 2.0f;
                        particles[j].position = particles[j].position - normal * length / 2.0f;

                        GLfloat vi = glm::dot(particles[i].velocity, normal);
                        GLfloat vj = glm::dot(particles[j].velocity, normal);

                  

                        particles[i].velocity = particles[i].velocity + (((1 + restitution) * vj - (1 + restitution) * vi) / 2) * normal;
                       
                        
                        if ((glm::length(particles[i].velocity) >= (2.0f * minVelocity)) || (particles[j].resting == false))
                        {
                            particles[j].velocity = particles[j].velocity + (((1 + restitution) * vi - (1 + restitution) * vj) / 2) * normal;
                            
                            checkPlaneCollision(j);
                            particles[j].resting = false;
                        }

                  
                        

                    }

                }
                checkPlaneCollision(i);

                if (particles[i].position.y <= -6.0)
                {
                    particles[i].position.y = -6.0;
                    particles[i].velocity.y = restitution * -particles[i].velocity.y;

                    particles[i].velocity.x *= (1 - friction);
                    particles[i].velocity.z *= (1 - friction);
                }


                particles[i].velocity.y = particles[i].velocity.y + (crntTime - prevTime) * g;
                particles[i].position.x = particles[i].position.x + (crntTime - prevTime) * particles[i].velocity.x;
                particles[i].position.y = particles[i].position.y + (crntTime - prevTime) * particles[i].velocity.y;
                particles[i].position.z = particles[i].position.z + (crntTime - prevTime) * particles[i].velocity.z;

                if ((glm::length(particles[i].velocity) <= minVelocity) && particles[i].position.y <= -5.9)
                {
                    particles[i].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
                    particles[i].resting = true;
                }
          

            }
            prevTime = crntTime;
            numFrames++;
            if (crntTime - lastTime >= 1.0) {
                
                printFPS(numFrames / (crntTime - lastTime));
                numFrames = 0;
                lastTime += 1.0;
            }
        }
        
        

        VAO1.Bind();

        for (int i = 0; i < numParticles; i++)
        {
            
            view = glm::translate(view, particles[i].position);
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(int), GL_UNSIGNED_INT, 0);
            
            view = glm::translate(view, glm::vec3(-particles[i].position.x, -particles[i].position.y, -particles[i].position.z));

        }
        VAO1.Unbind();

        VAO_plane.Bind();

        
       glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1)));
        glDrawElements(GL_TRIANGLES, sizeof(planeIndices) / sizeof(GLuint), GL_UNSIGNED_INT, 0);
        

        VAO_plane.Unbind();
        
        
        


 



        glfwSwapBuffers(window);

        glfwPollEvents();
        
    }
    VAO1.Delete();
    VBO1.Delete();
    EBO1.Delete();
    VAO_plane.Delete();
    VBO_plane.Delete();
    EBO_plane.Delete();


    glfwDestroyWindow(window);

    glfwTerminate();
    return 0;
}