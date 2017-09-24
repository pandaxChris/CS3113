/*
Chris Li cl3250
Intro to Game Programming Assignment 1
Draws a scene containing three images blended into a light blue background
*/
#ifdef _WINDOWS
	#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
	#define RESOURCE_FOLDER ""
#else
	#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

SDL_Window* displayWindow;
#include "ShaderProgram.h"
//Code taken from slides to load an image
GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    //Attempt to load image
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    //Check load success
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    GLuint retTexture;
    glGenTextures(1, &retTexture);      //Create new texture id
    glBindTexture(GL_TEXTURE_2D, retTexture); //Bind type to id
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(image);
    return retTexture;
}

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
	SDL_GL_MakeCurrent(displayWindow, context);
	#ifdef _WINDOWS
		glewInit();
	#endif
    
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    Matrix projectionMatrix;
    projectionMatrix.SetOrthoProjection(-5.0f, 5.0f, -3.0f, 3.0f, -1.0f, 1.0f);
    Matrix modelviewMatrix;
    
    //Load images taken from the assets folder
    GLuint person = LoadTexture(RESOURCE_FOLDER"pieceBlack_border00.png");
    GLuint car = LoadTexture(RESOURCE_FOLDER"pieceBlack_border13.png");
    GLuint house = LoadTexture(RESOURCE_FOLDER"pieceBlack_border07.png");
    
    SDL_Event event;
	bool done = false;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
        glClearColor(0.75f,0.86f,0.96f,1.0f);   //Change to light blue background
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program.programID);
        program.SetModelviewMatrix(modelviewMatrix);
        program.SetProjectionMatrix(projectionMatrix);
        float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
        
        //Create matrix for each texture
        float firstTex[] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
        float secondTex[] = {1.0f, -0.5f, 2.0f, -0.5f, 2.0f, 0.5f, 1.0f, -0.5f, 2.0f, 0.5f, 1.0f, 0.5f};
        float thirdTex[] = {-4.0f, -0.5f, -3.0f, -0.5f, -3.0f, 0.5f, -4.0f, -0.5f, -3.0f, 0.5f, -4.0f, 0.5f};

        //Draw first object(Person)
        glBindTexture(GL_TEXTURE_2D, person);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, firstTex);
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
        //Draw second object(car)
        glBindTexture(GL_TEXTURE_2D, car);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, secondTex);
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //Draw third object(house)
        glBindTexture(GL_TEXTURE_2D, house);
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, thirdTex);
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
