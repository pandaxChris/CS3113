/*
 Chris Li cl3250
 Assignment 2: Pong
*/

#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#include <math.h>
#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

//constants for game setup
SDL_Window* displayWindow;

//Modifies the array of floats using x,y; Boolean indicates if direction needs to be changed
void moveItem(float x, float y, float *matrix){
    for(int i = 0; i < 12; i++){
        if(i%2 == 1){   //modify y values
            matrix[i] += y;
        }else{ //modify x values
            matrix[i] += x;
        }
    }
}
//Functions to check if a matrix is in boundaries
bool inBoundsTop(float* matrix){ return matrix[1] < 3.95f; }
bool inBoundsBot(float* matrix){ return matrix[9] > -3.95f; }

//Check collison between two matrices
bool checkCollision(float* m1, float* m2){

    //Check m1 left side touching m2 right side
    if( m1[0] == trunc(m2[2]*100/100) && ((m1[1] >= m2[3] && m1[11] <= m2[3]) || (m1[1] >= m2[5] && m1[11] <= m2[5]) ))
        return true;
    //Check m1 right side touching m2 left side
    if(m1[2] == trunc(m2[0]*100/100) && ((m1[3] >= m2[1] && m1[5] <= m2[1]) || (m1[3] >= m2[11] && m1[5] <= m2[11]) ))
        return true;
    //Check m1 top side touching m2 bottom side
    if(m1[1] == m2[11] && ((m2[0] >= m1[0] && m2[0] <= m1[2]) || (m2[2] >= m1[0] && m2[2] <= m1[2])))
        return true;
    //Check m1 bottom side touching m2 top side
    if(m1[11] == m2[1] && ((m2[0] >= m1[0] && m2[0] <= m1[2]) || (m2[2] >= m1[0] && m2[2] <= m1[2])))
        return true;
    return false;

}



//Adding a set of vertices to a shaderprogram
void genShapes(ShaderProgram& program, float *texCoords, float* vertices){
    //Pass vertex attributes
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    //Pass texture coordinate attributes
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    //Draw the vertices onto shader program
    glDrawArrays(GL_TRIANGLES, 0, 6);
    //Disable attributes after usage
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

}
//Checks if game is over
bool checkEndGame(float *ball){ return ball[2] >= 5.9f || ball[0] <= -5.9f;}    //Left side ball is out or right side is out

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif

	SDL_Event event;
	bool done = false;
    Matrix projectionMatrix, modelViewMatrix;
    //Set the coordinate grid
    projectionMatrix.SetOrthoProjection(-6.0f, 6.0f, -4.0f, 4.0f, -1.0f, 1.0f);
    ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    // Initialize original positions for left and right paddle with W = 0.2f and H = 2.0f
    float leftPad[] = {-5.2f, 1.0f, -5.0f, 1.0f, -5.0f, -1.0f, -5.2f, 1.0f, -5.0f, -1.0f, -5.2f, -1.0f};
    float rightPad[] = {5.0f, 1.0f, 5.2f, 1.0f, 5.2f, -1.0f, 5.0f, 1.0f, 5.2f, -1.0f, 5.0f, -1.0f };
    // ball coords
    float ball[] = {0.0f, 0.15f, 0.15f, 0.15f, 0.15f, 0.0, 0.0, 0.15f, 0.15f, 0.0, 0.0, 0.0};
    // Texture coords
    float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    // Declare constants
    float frameTicks = 0.0f;
    //Ball constants -- will always start off with the ball going to the right side
    float ballXVelocity = 3.0f;
    float ballYVelocity = 1.0f;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - frameTicks;
        frameTicks = ticks;
        
        
        
        if(!inBoundsTop(ball) || !inBoundsBot(ball))    //Check to make sure it bounces off top/bottom boundaries
            ballYVelocity *= -1;
            
        if(checkCollision(rightPad,ball) || checkCollision(leftPad,ball))
            ballXVelocity *= -1;
        
        moveItem(cos(3.14/3) * elapsed * ballXVelocity, sin(3.14/3) * elapsed * ballYVelocity, ball);
        
        program.SetProjectionMatrix(projectionMatrix);
        program.SetModelviewMatrix(modelViewMatrix);

        glClearColor(.9f,0.0f,0.0f,0.0f); //Red in the case that no one has won yet
        glClear(GL_COLOR_BUFFER_BIT);

        genShapes(program, texCoords, leftPad);     //Draw left pad
        genShapes(program, texCoords, rightPad);    //Draw right pad
        genShapes(program,texCoords, ball);         //Draw ball
        
        //Paddle movement
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        // Will use UP/DOWN keys for right paddle, and W/S key for left paddle
        if(keys[SDL_SCANCODE_UP]){
            if(inBoundsTop(rightPad))
                moveItem(0.0f, 0.1f, rightPad);
        }else if(keys[SDL_SCANCODE_DOWN]){
            if(inBoundsBot(rightPad))
                moveItem(0.0f, -0.1f, rightPad);
        }else if(keys[SDL_SCANCODE_W]){
            if(inBoundsTop(leftPad))
                moveItem(0.0f, 0.1f, leftPad);
        }else if(keys[SDL_SCANCODE_S]){
            if(inBoundsBot(leftPad))
                moveItem(0.0f, -0.1f, leftPad);
        }
        if(checkEndGame(ball)){
            glClearColor(0.0f,0.9f,0.0f,0.0f); //Change background if someone won
            glClear(GL_COLOR_BUFFER_BIT);
        }
        SDL_GL_SwapWindow(displayWindow);
    
    }
	SDL_Quit();
	return 0;
}
