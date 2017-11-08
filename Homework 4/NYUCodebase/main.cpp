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
#include "ShaderProgram.h"
#include "Matrix.h"
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FIXED_TIMESTEP .01f
using namespace std;
int mapWidth;
int mapHeight;
int SPRITE_COUNT_X = 8;
int SPRITE_COUNT_Y = 4;
int** levelData;
float TILE_SIZE = .5;
SDL_Window* displayWindow;


void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / TILE_SIZE);
    *gridY = (int)(-worldY / TILE_SIZE);
}

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

class Vector{
public:
    Vector():x(0.0),y(0.0){}
    
    Vector(float xval, float yval):x(xval), y(yval){}
    float x;
    float y;
};


class SheetSprite {
public:
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size):
    textureID(textureID), u(u), v(v), width(width), height(height), size(size){}
    
    void Draw(ShaderProgram *program){
        glBindTexture(GL_TEXTURE_2D, textureID);
        GLfloat texCoords[] = {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        float aspect = width / height;
        float vertices[] = {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size ,
            0.5f * size * aspect, -0.5f * size};
        // draw our arrays
        glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program->positionAttribute);
        glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program->texCoordAttribute);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(program->positionAttribute);
        glDisableVertexAttribArray(program->texCoordAttribute);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
    }
    
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};

enum EntityType{ENTITY_PLAYER, ENTITY_BALL};

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}
class Entity{
public:
    //Constructors
    Entity(EntityType type, float posx, float posy, float velx, float vely, float accx, float accy, float fx, float fy, float sx, float sy, float rot, bool stat, SheetSprite s):entType(type),position(posx,posy), velocity(velx,vely), acceleration(accx,accy), friction(fx,fx),size(sx,sy),rotation(rot), isStatic(stat), sprite(s), collidedTop(false), collidedLeft(false),collidedRight(false),collidedBottom(false){}
    
    void Draw(ShaderProgram &program){ sprite.Draw(&program); }
    
    void move(float elapsed){
        if(!isStatic){
            
            if(acceleration.x > 4.0){
                acceleration.x = 4.0;
            }else if(acceleration.x < -4.0){
                acceleration.x = -4.0;
            }
            velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
            velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);

            if(acceleration.x > 0.0){
                acceleration.x -= .05;
            }else if (acceleration.x < 0.0){
                acceleration.x += .05;
            }
            
            if(acceleration.y > 0.0){
                acceleration.y -= 1.1;
            }else if(acceleration.y < -5.0){
                acceleration.y = -5.0;
            }else if(acceleration.y < -0.0){
                acceleration.y -= 1.1;
            }
            velocity.x += acceleration.x * elapsed;
            velocity.y += acceleration.y * elapsed;
            
            position.x += velocity.x * elapsed;
            //In bounds
            if(position.y < -.81){
                position.y = -.8;
            }else{
                position.y += velocity.y * elapsed;
            }
        }
    }
    
    void handleCollisions(Entity * e){
        int gridX, gridY;
        
        if(collidedTop){
            worldToTileCoordinates(position.x, position.y, &gridX, &gridY);
            
        }
    }
    SheetSprite sprite;
    EntityType entType;
    Vector friction;
    Vector position;
    Vector velocity;
    Vector acceleration;
    Vector size;
    float rotation;
    
    bool isStatic;
    //Coliision flags
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
    
};
vector<Entity *> entities;
void placeEntity(string type, int posX, int posY){
    GLuint sprites = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    SheetSprite player = SheetSprite(sprites, 0.0f/128.0f, 64.0f/128.0f, 16.0f/256.0f, 32.0f/128.0f, 1.5f);
    if(type == "Character"){
        entities.push_back(new Entity(ENTITY_PLAYER, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 2.0,2.0, 1.0, 1.0, 0.0, false, player));
    }
    
}
bool readHeader(ifstream &stream) {
    string line;
    mapWidth = -1;
    mapHeight = -1;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "width") {
            mapWidth = atoi(value.c_str());
        } else if(key == "height"){
            mapHeight = atoi(value.c_str());
        }
    }
    if(mapWidth == -1 || mapHeight == -1) {
        return false;
    } else { // allocate our map data
        levelData = new int*[mapHeight];
        for(int i = 0; i < mapHeight; ++i) {
            levelData[i] = new int[mapWidth];
        }
        return true;
    }
}

bool readLayerData(std::ifstream &stream) {
    string line;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "data") {
            for(int y=0; y < mapHeight; y++) {
                getline(stream, line);
                istringstream lineStream(line);
                string tile;
                for(int x=0; x < mapWidth; x++) {
                    getline(lineStream, tile, ',');
                    int val =  atoi(tile.c_str());
                    if(val > 0) {
                        // be careful, the tiles in this format are indexed from 1 not 0
                        levelData[y][x] = val-1;
                    } else {
                        levelData[y][x] = 0;
                    }
                    //   cout << levelData[y][x];
                }
                // cout << "\n";
            }
        }
    }
    return true;
}

bool readEntityData(std::ifstream &stream) {
    string line;
    string type;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "type") {
            type = value;
        } else if(key == "location") {
            istringstream lineStream(value);
            string xPosition, yPosition;
            getline(lineStream, xPosition, ',');
            getline(lineStream, yPosition, ',');
            float placeX = atoi(xPosition.c_str())*TILE_SIZE;
            float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
            //cout << type << "," << placeX << ";\n";
            placeEntity(type, placeX, placeY);
        }
    }
    return true;
}
void drawTiles(ShaderProgram *program, GLuint spriteSheetTexture){
    vector<float> vertexData;
    vector<float> texCoordData;
    int spritesDrawn = 0;
    for(int y=0; y < mapHeight; y++) {
        for(int x=0; x < mapWidth; x++) {
            if(levelData[y][x] != 0){
                spritesDrawn++;
                float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_Y) / (float) SPRITE_COUNT_Y;
                float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
                float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
                vertexData.insert(vertexData.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
                });
                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+(spriteHeight),
                    u, v,
                    u+spriteWidth, v+(spriteHeight),
                    u+spriteWidth, v
                });
            }
            
        }
    }
    glUseProgram(program->programID);
    
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
    glDrawArrays(GL_TRIANGLES, 0, spritesDrawn*6);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

void drawEntities(ShaderProgram &p, Matrix &proj, Matrix & mv){
    for(int i = 0; i < entities.size(); i++){
        mv.Identity();
        mv.Translate(entities[i]->position.x, entities[i]->position.y, 0);
        p.SetProjectionMatrix(proj);
        p.SetModelviewMatrix(mv);
        entities[i]->Draw(p);
    }
}

//Function to check if player collides with the border
bool collidesBorder(Entity * a){
    if( a->position.x >= 15.0f){
        a->position.x = 14.9f;
        return true;
    }else if(a->position.x <= -30.0f){
        a->position.x = -29.9f;
        return true;
    }
    return false;
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

	SDL_Event event;
	bool done = false;
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    GLuint sprites = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    Matrix projectionMatrix, modelViewMatrix,viewMatrix;
    projectionMatrix.SetOrthoProjection(-7.1f, 7.1f, -4.0f, 4.0f, -1.0f, 1.0f);
    ifstream infile(RESOURCE_FOLDER"map.txt");
    string line;
    
    while (getline(infile, line)) {
        if(line == "[header]") {
            if(!readHeader(infile)) { assert(false); }
        } else if(line == "[layer]") { readLayerData(infile); }
        else if(line == "[Object Layer 1]") { readEntityData(infile); }
    }
    float frameTicks = 0.0f;
    float accumulator = 0.0f;
	while (!done) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
				done = true;
			}
		}
        program.SetProjectionMatrix(projectionMatrix);
        program.SetModelviewMatrix(modelViewMatrix);
        drawTiles(&program, sprites);
        glUseProgram(program.programID);
        glClearColor(0.75f,0.86f,0.96f,1.0f);   //Change to light blue background
        glClear(GL_COLOR_BUFFER_BIT);
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if(keys[SDL_SCANCODE_LEFT]){
            //entities[0]->position.x += entities[0]->velocity.x * elapsed;
            entities[0]->acceleration.x -= .2;
        }else if(keys[SDL_SCANCODE_RIGHT]){
            //entities[0]->position.x -= entities[0]->velocity.x * elapsed;
            entities[0]->acceleration.x += .2;
        }else if(keys[SDL_SCANCODE_UP]){
            entities[0]->acceleration.y = 5.0;
        }
        
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - frameTicks;
        frameTicks = ticks;

        elapsed += accumulator;
        if(elapsed < FIXED_TIMESTEP){
            accumulator = elapsed;
            continue;
        }

        while(elapsed >= FIXED_TIMESTEP){
            entities[0]->move(elapsed);
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(entities[0]->position.x, entities[0]->position.y, 0);
            program.SetModelviewMatrix(modelViewMatrix);
            
            entities[0]->Draw(program);

            elapsed -= FIXED_TIMESTEP;
        }
        accumulator = elapsed;
        modelViewMatrix.Translate(-entities[0]->position.x, -entities[0]->position.y, 0);
		SDL_GL_SwapWindow(displayWindow);
	}

	SDL_Quit();
	return 0;
}
