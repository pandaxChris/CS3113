/*
 
 Chris Li
 cl3250
 Intro to Game Dev
 Final Project
 
 Sounds taken from freelcloud.org:
    https://freesound.org/people/Greek555/sounds/413196/
    https://freesound.org/people/GokhanBiyik/sounds/413093/
    https://freesound.org/people/Streetpoptunez/sounds/413076/
    https://freesound.org/people/tec%20studios/sounds/413234/
 https://freesound.org/people/sandyrb/sounds/86290/
*/
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "SheetSprite.h"
#include <vector>
#include <cmath>
#define FIXED_TIMESTEP .01f


enum GameMode{STATE_MAIN_MENU, STATE_LEVEL_SELECT,STATE_GAME_LEVEL,STATE_GAME_OVER};
enum GameType{TYPE_SOLO, TYPE_OBS_BOTHSIDES, TYPE_OBS};
enum EntityType{PLAYER,OBVERT,OBHORIZ};
GameType myGameType;
using namespace std;
int minIndex = 0;
SDL_Window* displayWindow;
void DrawText(ShaderProgram *program, int fontTexture, string text, float size, float spacing) {
    float texture_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        });
    }
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glUseProgram(program->programID);
    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program->positionAttribute);
    
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program->texCoordAttribute);
    
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}
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

float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

class Vector3{
public:
    Vector3():x(0.0), y(0.0), z(0.0){}
    Vector3(float x, float y, float z): x(x), y(y), z(z){}
    float x;
    float y;
    float z;
};

class Entity{
public:
    Entity(SheetSprite s, float x, float y, float z, float dx, float dy, float dz, float ax, float ay, float az, EntityType t):sprite(s), position(x,y,z), velocity(dx,dy,dz), acceleration(ax,ay,az), friction(.1, .1, 0.0), score(0), myType(t), collidedTop(false), collidedLeft(false), collidedRight(false), collidedBottom(false), health(100){}
    void Draw(ShaderProgram *p){
        sprite.Draw(p);
    }

    void move(float elapsed){
        velocity.x = lerp(velocity.x, 0.0f, elapsed * friction.x);
        velocity.y = lerp(velocity.y, 0.0f, elapsed * friction.y);

        
        velocity.x += acceleration.x * elapsed;
        velocity.y += acceleration.y * elapsed;
        
        position.x += velocity.x * elapsed;
        position.y += velocity.y * elapsed;
    }
    EntityType myType;
    SheetSprite sprite;
    int score;
    int health;
    Vector3 position;
    Vector3 velocity;
    Vector3 acceleration;
    Vector3 friction;
    
    bool collidedTop;
    bool collidedLeft;
    bool collidedRight;
    bool collidedBottom;
};

//Function to detect top bottom collision between entities
bool detectCollision(Entity * a, Entity * b){
    //Top of A less than bottom of B
    if((a->position.y + a->sprite.size/2) < (b->position.y - b->sprite.size/2) ){
        return false;
    }else{
        a->collidedTop = true;
        b->collidedBottom = true;
    }
    
    //Bottom of A higher than top of B
    if( (a->position.y - a->sprite.size/2) > (b->position.y + b->sprite.size/2) ){
        return false;
    }
    else{
        a->collidedBottom = true;
        b->collidedTop = true;
    }
    
    //Left of A greater than right of B
    if ( (a->position.x - a->sprite.size/2) > (b->position.x + b->sprite.size/2) ){
        return false;
    }else{
        a->collidedLeft = true;
        b->collidedRight = true;
    }
    //Right of A smaller than left of B
    if( (a->position.x + a->sprite.size/2) < (b->position.x - b->sprite.size/2) ){
        return false;
    }else{
        a->collidedRight = true;
        b->collidedLeft = true;
    }
    
    // collision
    return true;
}

void handleCollisions(Entity * a, Entity * b){
    if(a->myType == PLAYER && b->myType == PLAYER){
        if(a->collidedTop){
            while(detectCollision(a, b)){
                a->position.y -= .1;
                b->position.y += .1;
            }
            a->collidedTop = false;
            b->collidedBottom = false;
        }else if(a->collidedBottom){
            while(detectCollision(a, b)){
                a->position.y += .1;
                b->position.y -= .1;
            }
            a->collidedBottom = false;
            b->collidedTop = false;
        }else if(a->collidedLeft){
            while(detectCollision(a, b)){
                a->position.x += .1;
                b->position.x -= .1;
            }
            a->collidedLeft = false;
            b->collidedRight = false;
        }else if(a->collidedRight){
            while(detectCollision(a, b)){
                a->position.x -= .1;
                b->position.x += .1;
            }
            a->collidedRight = false;
            a->collidedLeft = false;
        }
    }else if(a->myType == PLAYER && (b->myType == OBVERT || b->myType == OBHORIZ)){
        a->health -= 20;
        a->collidedTop = false;
        a->collidedLeft = false;
        a->collidedBottom = false;
        a->collidedRight = false;
        b->position.x = 20.0;
        b->position.y = 20.0;
        b->velocity.x = 0.0;
        b->velocity.y = 0.0;
        b->acceleration.x = 0.0;
        b->acceleration.y= 0.0;
    }
}


class GameState{
public:
    GameState():playerOne(nullptr),playerTwo(nullptr){};
    
    GameState(Entity * p, Entity *p2){
        playerOne = p;
        playerTwo = p2;
    }

    Entity *playerOne;
    Entity *playerTwo;
    
    vector<Entity *> obstacles;
};



GameMode updateMenu(SDL_Event* event, ShaderProgram* program, Matrix& modelViewMatrix, Matrix& projectionMatrix, bool done, GLuint fonts, GameMode &mode){
    GLuint background = LoadTexture(RESOURCE_FOLDER"final_map.png");
    float vertices[] = {-10.0f, 5.0f, 10.0f, 5.0f, 10.0f, -5.0f, -10.0f, 5.0f, 10.0f, -5.0f, -10.0f, -5.0f};
    float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    while (!done) {
        while (SDL_PollEvent(event)) {
            if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
        if(mode == STATE_MAIN_MENU){
            glUseProgram(program->programID);
            modelViewMatrix.Identity();
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            glClearColor(1.0f,1.0f,1.0f,1.0f);   //Change to white background
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, background);
            glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            glEnableVertexAttribArray(program->positionAttribute);
            glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program->texCoordAttribute);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisableVertexAttribArray(program->positionAttribute);
            glDisableVertexAttribArray(program->texCoordAttribute);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-5.0f, 2.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            DrawText(program, fonts, "Bumper Survivor", 0.7f, 0.0f);
        
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-4.0f, -1.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            DrawText(program, fonts, "Press Space to start", 0.4f, 0.0f);
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            if(keys[SDL_SCANCODE_SPACE]){
                return STATE_LEVEL_SELECT;
            }
        }else if(mode == STATE_LEVEL_SELECT){
            glUseProgram(program->programID);
            modelViewMatrix.Identity();
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            glClearColor(1.0f,1.0f,1.0f,1.0f);   //Change to white background
            glClear(GL_COLOR_BUFFER_BIT);
            glBindTexture(GL_TEXTURE_2D, background);
            glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
            glEnableVertexAttribArray(program->positionAttribute);
            glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
            glEnableVertexAttribArray(program->texCoordAttribute);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisableVertexAttribArray(program->positionAttribute);
            glDisableVertexAttribArray(program->texCoordAttribute);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-5.0f, 2.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            DrawText(program, fonts, "Choose your level", 0.5f, 0.0f);
            
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-5.0f, 1.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            if(myGameType == TYPE_SOLO){
                DrawText(program,fonts, "Solo Game" , .6f, 0.0f);
            }else{
                DrawText(program, fonts, "Solo Game", 0.4f, 0.0f);
            }
            
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-5.0f, 0.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            if(myGameType == TYPE_OBS_BOTHSIDES){
                DrawText(program,fonts, "Co-op with obstacles(two side)",.5f, 0.0f);
            }else{
                DrawText(program, fonts, "Co-op with obstacles(two sides)", 0.3f, 0.0f);
            }
            
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-5.0f, -1.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            if(myGameType == TYPE_OBS){
                DrawText(program, fonts, "Co-op with obstacles(one side)", 0.5f,0.0f);
            }else{
                DrawText(program, fonts, "Co-op with obstacles(one side)", 0.3f, 0.0f);
            }
            const Uint8 *keys = SDL_GetKeyboardState(NULL);
            if(keys[SDL_SCANCODE_UP]){
                if(myGameType == TYPE_SOLO || myGameType == TYPE_OBS_BOTHSIDES){
                    myGameType = TYPE_SOLO;
                }else if(myGameType == TYPE_OBS){
                    myGameType = TYPE_OBS_BOTHSIDES;
                }
            }else if(keys[SDL_SCANCODE_DOWN]){
                if(myGameType == TYPE_OBS || myGameType == TYPE_OBS_BOTHSIDES){
                    myGameType = TYPE_OBS;
                }else if(myGameType == TYPE_SOLO){
                    myGameType = TYPE_OBS_BOTHSIDES;
                }
            }else if(keys[SDL_SCANCODE_0]){
                return STATE_GAME_LEVEL;
            }
            modelViewMatrix.Identity();
            modelViewMatrix.Translate(-5.0f, -2.0f, 0.0f);
            program->SetProjectionMatrix(projectionMatrix);
            program->SetModelviewMatrix(modelViewMatrix);
            DrawText(program, fonts, "Press 0 to continue", 0.4f, 0.0f);
        }
        SDL_GL_SwapWindow(displayWindow);
        
    }
    return STATE_MAIN_MENU;
}


void RenderGameState(ShaderProgram *p, const GameState &state, Matrix &projectionMatrix, Matrix &modelViewMatrix){
    glUseProgram(p->programID);
    modelViewMatrix.Identity();
    modelViewMatrix.Translate(state.playerOne->position.x, state.playerOne->position.y, state.playerOne->position.z);
    p->SetProjectionMatrix(projectionMatrix);
    p->SetModelviewMatrix(modelViewMatrix);
    state.playerOne->Draw(p);
    
    if(state.playerTwo != nullptr){
        modelViewMatrix.Identity();
        modelViewMatrix.Translate(state.playerTwo->position.x, state.playerTwo->position.y, state.playerTwo->position.z);
        p->SetProjectionMatrix(projectionMatrix);
        p->SetModelviewMatrix(modelViewMatrix);
        state.playerTwo->Draw(p);
    }
    
    for(size_t i = 0; i < state.obstacles.size(); i++){
        modelViewMatrix.Identity();
        modelViewMatrix.Translate(state.obstacles[i]->position.x, state.obstacles[i]->position.y, state.obstacles[i]->position.z);
        p->SetProjectionMatrix(projectionMatrix);
        p->SetModelviewMatrix(modelViewMatrix);
        state.obstacles[i]->Draw(p);
    }
}
void UpdateGameState(GameState & state, float elapsed){
    for(size_t i = 0; i < state.obstacles.size(); i++){
        state.obstacles[i]->move(elapsed);
    }
}

void handleOutOfBounds(Entity * a){
    if(a->myType == PLAYER){
        if(a->position.x < -6.0 || a->position.x > 6.0){
            if(a->sprite.size > 0.0)
                a->sprite.size -=.025;
        }
        if(a->position.x > -6.0 && a->position.x < 6.0){
            if(a->sprite.size < 1.0){
                a->sprite.size += .025;
            }
        }
    }
}

bool checkGameOver(GameState & state){
    return state.playerOne->health <= 0 || state.playerTwo->health <= 0 || state.playerOne->sprite.size < .05 || state.playerTwo->sprite.size < .05;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 435, 865, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    Matrix projectionMatrix, modelViewMatrix;
    projectionMatrix.SetOrthoProjection(-10.0f, 10.0f, -5.0f, 5.0f, -1.0f, 1.0f);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    //Image initialization
    GLuint fonts = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
    GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
    SheetSprite play = SheetSprite(spriteSheet, 224.0f/1024.0f, 832.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f, 1.0f);
    SheetSprite playTwo = SheetSprite(spriteSheet, 237.0f/1024.0f, 377.0/1024.0f, 99.0f/1024.0f, 75.0/1024.0f, 1.0f);
    SheetSprite obs = SheetSprite(spriteSheet, 224.0/1024.0f, 664.0f/1024.0f, 101.0/1024.0f, 84.0/1024.0, 1.0f);
    Entity * player = new Entity(play, -1.0f, -2.5f, 0.0f, 0.0f, 0.0f, 0.0f,0.0,0.0,0.0, PLAYER);
    Entity * playerTwo = new Entity(playTwo, 1.0f, -2.5,0.0f, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, PLAYER);
    GLuint background = LoadTexture(RESOURCE_FOLDER"final_map.png");
    float vertices[] = {-10.0f, 5.0f, 10.0f, 5.0f, 10.0f, -5.0f, -10.0f, 5.0f, 10.0f, -5.0f, -10.0f, -5.0f};
    float texCoords[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
    
    //Other variable intialization
    GameMode mode = STATE_MAIN_MENU;
    GameState state;
    SDL_Event event;
    bool setState = false;
    float frameTicks = 0.0f;
    float accumulator = 0.0f;
    myGameType = TYPE_SOLO;
    
    Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 4096 );
    //background song
    Mix_Music *bgSound;
    bgSound = Mix_LoadMUS(RESOURCE_FOLDER"413196__greek555__sample-100-bpm.mp3");
    //Asteroid hit sound
    Mix_Chunk *hitSound;
    hitSound = Mix_LoadWAV(RESOURCE_FOLDER"413076__streetpoptunez__spook-bass.wav");
    Mix_Chunk *ending;
    ending = Mix_LoadWAV(RESOURCE_FOLDER"135831__davidbain__end-game-fail.wav");
    bool done = false;
    Mix_PlayMusic(bgSound, -1);
    while(!done){
        switch(mode){
            case STATE_MAIN_MENU:
                mode = updateMenu(&event,&program,modelViewMatrix,projectionMatrix, done, fonts, mode);
                break;
            case STATE_LEVEL_SELECT:
                mode = updateMenu(&event,&program,modelViewMatrix,projectionMatrix, done, fonts, mode);
                break;
            case STATE_GAME_LEVEL:
                if(myGameType == TYPE_SOLO){
                    if(!setState){
                        state = GameState(player, nullptr);
                        setState = true;
                    }
                    int counter = 1;
                    while (!done) {
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                                done = true;
                            }
                        }

                        glClear(GL_COLOR_BUFFER_BIT);
                        glUseProgram(program.programID);

                        modelViewMatrix.Identity();
                        program.SetProjectionMatrix(projectionMatrix);
                        program.SetModelviewMatrix(modelViewMatrix);
                        //glClearColor(1.0f,1.0f,1.0f,1.0f);   //Change to white background
                        glClear(GL_COLOR_BUFFER_BIT);
                        glBindTexture(GL_TEXTURE_2D, background);
                        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
                        glEnableVertexAttribArray(program.positionAttribute);
                        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
                        glEnableVertexAttribArray(program.texCoordAttribute);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                        glDisableVertexAttribArray(program.positionAttribute);
                        glDisableVertexAttribArray(program.texCoordAttribute);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        
                        
                        if(counter % 100 == 0){
                            Entity * newObstacle = new Entity(obs, rand()%6-3, 5.0, 1.0, 0.0, -.5, 0.0,0.0, -.5, 0.0, OBVERT);
                            state.obstacles.push_back(newObstacle);
                            state.playerOne->score++;
                        }

                        //Handle user key presses
                        const Uint8 *keys = SDL_GetKeyboardState(NULL);
                        if(keys[SDL_SCANCODE_LEFT]){
                            if(state.playerOne->position.x > -10.0){
                                state.playerOne->position.x -= .1;
                            }else{
                                state.playerOne->position.x = -9.9;
                            }
                        }else if(keys[SDL_SCANCODE_RIGHT]){
                            if(state.playerOne->position.x < 10.0){
                                state.playerOne->position.x+= .1;
                            }else{
                                state.playerOne->position.x = 9.9;
                            }
                        }else if(keys[SDL_SCANCODE_UP]){
                            if(state.playerOne->position.y < 5.0){
                                state.playerOne->position.y += .1;
                            }else{
                                state.playerOne->position.y = 4.9;
                            }
                        }else if(keys[SDL_SCANCODE_DOWN]){
                            if(state.playerOne->position.y > -5.0){
                                state.playerOne->position.y -= .1;
                            }else{
                                state.playerOne->position.y = -4.9;
                            }
                        }else if(keys[SDL_SCANCODE_ESCAPE]){
                            mode = STATE_LEVEL_SELECT;
                            setState = false;
                            break;
                        }
                        
                        for(size_t i = 0; i < state.obstacles.size(); i++){
                            if(detectCollision(state.playerOne,state.obstacles[i])){
                                handleCollisions(state.playerOne, state.obstacles[i]);
                                Mix_PlayChannel(-1, hitSound, 0);
                            }
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
                            UpdateGameState(state, elapsed);
                            handleOutOfBounds(state.playerOne);
                            elapsed -= FIXED_TIMESTEP;
                        }
                        accumulator = elapsed;

                        //Update the obstacles on the map
                        UpdateGameState(state, elapsed);
                        //Display the entities
                        RenderGameState(&program, state, projectionMatrix, modelViewMatrix);
                        counter++;

                        if(state.playerOne->health <= 0 || state.playerOne->sprite.size < .05){
                            mode = STATE_GAME_OVER;
                            break;
                        }
                        SDL_GL_SwapWindow(displayWindow);
                    }
                    
                }else if(myGameType == TYPE_OBS_BOTHSIDES){
                    bool side = false;
                    if(!setState){
                        state = GameState(player, playerTwo);
                        setState = true;
                    }
                    int counter = 1;
                    while (!done) {
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                                done = true;
                            }
                        }
                        
                        glClear(GL_COLOR_BUFFER_BIT);
                        glUseProgram(program.programID);
                        
                        modelViewMatrix.Identity();
                        program.SetProjectionMatrix(projectionMatrix);
                        program.SetModelviewMatrix(modelViewMatrix);
                        glClearColor(1.0f,1.0f,1.0f,1.0f);   //Change to white background
                        glClear(GL_COLOR_BUFFER_BIT);
                        glBindTexture(GL_TEXTURE_2D, background);
                        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
                        glEnableVertexAttribArray(program.positionAttribute);
                        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
                        glEnableVertexAttribArray(program.texCoordAttribute);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                        glDisableVertexAttribArray(program.positionAttribute);
                        glDisableVertexAttribArray(program.texCoordAttribute);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        
                        
                        if(counter % 100 == 0){
                            Entity * newObstacle;
                            if(!side){
                                newObstacle = new Entity(obs, rand()%6-3, 5.0, 1.0, 0.0, -.5, 0.0,0.0, -.5, 0.0, OBVERT);
                            }else{
                                newObstacle = new Entity(obs, -10.0, rand()%5, 1.0, .5, 0.0, 0.0,0.5, 0.0, 0.0, OBHORIZ);
                            }
                            state.obstacles.push_back(newObstacle);
                            state.playerOne->score++;
                            state.playerTwo->score++;
                            side = !side;
                        }
                        
                        //Handle user key presses
                        const Uint8 *keys = SDL_GetKeyboardState(NULL);
                        if(keys[SDL_SCANCODE_LEFT] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.x > -10.0){
                                state.playerOne->position.x -= .1;
                            }else{
                                state.playerOne->position.x = -9.9;
                            }
                        }else if(keys[SDL_SCANCODE_RIGHT] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.x < 10.0){
                                state.playerOne->position.x+= .1;
                            }else{
                                state.playerOne->position.x = 9.9;
                            }
                        }else if(keys[SDL_SCANCODE_UP] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y < 5.0){
                                state.playerOne->position.y += .1;
                            }else{
                                state.playerOne->position.y = 4.9;
                            }
                        }else if(keys[SDL_SCANCODE_DOWN] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y > -5.0){
                                state.playerOne->position.y -= .1;
                            }else{
                                state.playerOne->position.y = -4.9;
                            }
                        }else if(keys[SDL_SCANCODE_ESCAPE]){
                            mode = STATE_LEVEL_SELECT;
                            setState = false;
                            break;
                        }
                        if(keys[SDL_SCANCODE_A] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerTwo->position.x > -10.0){
                                state.playerTwo->position.x -= .1;
                            }else{
                                state.playerTwo->position.x = -9.9;
                            }
                        }else if(keys[SDL_SCANCODE_D] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.x < 10.0){
                                state.playerTwo->position.x+= .1;
                            }else{
                                state.playerTwo->position.x = 9.9;
                            }
                        }else if(keys[SDL_SCANCODE_W] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y < 5.0){
                                state.playerTwo->position.y += .1;
                            }else{
                                state.playerTwo->position.y = 4.9;
                            }
                        }else if(keys[SDL_SCANCODE_S] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y > -5.0){
                                state.playerTwo->position.y -= .1;
                            }else{
                                state.playerTwo->position.y = -4.9;
                            }
                        }
                        if(detectCollision(state.playerOne, state.playerTwo)){
                            handleCollisions(state.playerOne, state.playerTwo);
                            Mix_PlayChannel(-1, hitSound, 0);
                        }
                        
                        for(size_t i = 0; i < state.obstacles.size(); i++){
                            if(detectCollision(state.playerOne,state.obstacles[i])){
                                handleCollisions(state.playerOne, state.obstacles[i]);
                                Mix_PlayChannel(-1, hitSound, 0);
                            }
                            if(detectCollision(state.playerTwo,state.obstacles[i])){
                                handleCollisions(state.playerTwo, state.obstacles[i]);
                                Mix_PlayChannel(-1, hitSound, 0);
                            }
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
                            UpdateGameState(state, elapsed);
                            handleOutOfBounds(state.playerOne);
                            handleOutOfBounds(state.playerTwo);
                            elapsed -= FIXED_TIMESTEP;
                        }
                        
                        accumulator = elapsed;
                        
                        //Update the obstacles on the map
                        UpdateGameState(state, elapsed);
                        //Display the entities
                        RenderGameState(&program, state, projectionMatrix, modelViewMatrix);
                        counter++;
                        if(checkGameOver(state)){
                            mode = STATE_GAME_OVER;
                            break;
                        }
                        SDL_GL_SwapWindow(displayWindow);
                    }
                }else if(myGameType == TYPE_OBS){
                    if(!setState){
                        state = GameState(player, playerTwo);
                        setState = true;
                    }
                    int counter = 1;
                    while (!done) {
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                                done = true;
                            }
                        }
                        
                        glClear(GL_COLOR_BUFFER_BIT);
                        glUseProgram(program.programID);
                        
                        modelViewMatrix.Identity();
                        program.SetProjectionMatrix(projectionMatrix);
                        program.SetModelviewMatrix(modelViewMatrix);
                        glClearColor(1.0f,1.0f,1.0f,1.0f);   //Change to white background
                        glClear(GL_COLOR_BUFFER_BIT);
                        glBindTexture(GL_TEXTURE_2D, background);
                        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
                        glEnableVertexAttribArray(program.positionAttribute);
                        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
                        glEnableVertexAttribArray(program.texCoordAttribute);
                        glDrawArrays(GL_TRIANGLES, 0, 6);
                        glDisableVertexAttribArray(program.positionAttribute);
                        glDisableVertexAttribArray(program.texCoordAttribute);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        
                        
                        if(counter % 100 == 0){
                            Entity * newObstacle = new Entity(obs, rand()%6-3, 5.0, 1.0, 0.0, -.5, 0.0,0.0, -.5, 0.0, OBVERT);
                            state.obstacles.push_back(newObstacle);
                            state.playerOne->score++;
                            state.playerTwo->score++;
                        }
                        
                        //Handle user key presses
                        const Uint8 *keys = SDL_GetKeyboardState(NULL);
                        if(keys[SDL_SCANCODE_LEFT] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.x > -10.0){
                                state.playerOne->position.x -= .1;
                            }else{
                                state.playerOne->position.x = -9.9;
                            }
                        }else if(keys[SDL_SCANCODE_RIGHT] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.x < 10.0){
                                state.playerOne->position.x+= .1;
                            }else{
                                state.playerOne->position.x = 9.9;
                            }
                        }else if(keys[SDL_SCANCODE_UP] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y < 5.0){
                                state.playerOne->position.y += .1;
                            }else{
                                state.playerOne->position.y = 4.9;
                            }
                        }else if(keys[SDL_SCANCODE_DOWN] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y > -5.0){
                                state.playerOne->position.y -= .1;
                            }else{
                                state.playerOne->position.y = -4.9;
                            }
                        }else if(keys[SDL_SCANCODE_ESCAPE]){
                            mode = STATE_LEVEL_SELECT;
                            setState=false;
                            break;
                        }
                        if(keys[SDL_SCANCODE_A] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerTwo->position.x > -10.0){
                                state.playerTwo->position.x -= .1;
                            }else{
                                state.playerTwo->position.x = -9.9;
                            }
                        }else if(keys[SDL_SCANCODE_D] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.x < 10.0){
                                state.playerTwo->position.x+= .1;
                            }else{
                                state.playerTwo->position.x = 9.9;
                            }
                        }else if(keys[SDL_SCANCODE_W] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y < 5.0){
                                state.playerTwo->position.y += .1;
                            }else{
                                state.playerTwo->position.y = 4.9;
                            }
                        }else if(keys[SDL_SCANCODE_S] && !detectCollision(state.playerOne, state.playerTwo)){
                            if(state.playerOne->position.y > -5.0){
                                state.playerTwo->position.y -= .1;
                            }else{
                                state.playerTwo->position.y = -4.9;
                            }
                        }
                        if(detectCollision(state.playerOne, state.playerTwo)){
                            handleCollisions(state.playerOne, state.playerTwo);
                            Mix_PlayChannel(-1, hitSound, 0);
                        }
                        
                        for(size_t i = 0; i < state.obstacles.size(); i++){
                            if(detectCollision(state.playerOne,state.obstacles[i])){
                                handleCollisions(state.playerOne, state.obstacles[i]);
                                Mix_PlayChannel(-1, hitSound, 0);
                            }
                            if(detectCollision(state.playerTwo,state.obstacles[i])){
                                handleCollisions(state.playerTwo, state.obstacles[i]);
                                Mix_PlayChannel(-1, hitSound, 0);
                            }
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
                            UpdateGameState(state, elapsed);
                            handleOutOfBounds(state.playerOne);
                            handleOutOfBounds(state.playerTwo);
                            elapsed -= FIXED_TIMESTEP;
                        }
                        
                        accumulator = elapsed;
                        
                        //Update the obstacles on the map
                        UpdateGameState(state, elapsed);
                        //Display the entities
                        RenderGameState(&program, state, projectionMatrix, modelViewMatrix);
                        counter++;
                        if(checkGameOver(state)){
                            mode = STATE_GAME_OVER;
                            break;
                        }
                        SDL_GL_SwapWindow(displayWindow);
                    }
                }
            case STATE_GAME_OVER:
                Mix_PlayChannel(-1, ending, 0);
                modelViewMatrix.Identity();
                program.SetProjectionMatrix(projectionMatrix);
                program.SetModelviewMatrix(modelViewMatrix);
                glClearColor(1.0f,1.0f,1.0f,1.0f);   //Change to white background
                glClear(GL_COLOR_BUFFER_BIT);
                glBindTexture(GL_TEXTURE_2D, background);
                glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
                glEnableVertexAttribArray(program.positionAttribute);
                glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
                glEnableVertexAttribArray(program.texCoordAttribute);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                glDisableVertexAttribArray(program.positionAttribute);
                glDisableVertexAttribArray(program.texCoordAttribute);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                
                modelViewMatrix.Identity();
                modelViewMatrix.Translate(-5.0f, 2.0f, 0.0f);
                program.SetProjectionMatrix(projectionMatrix);
                program.SetModelviewMatrix(modelViewMatrix);
                DrawText(&program, fonts, "Game Over", 0.5f, 0.0f);
                if(myGameType == TYPE_OBS || myGameType == TYPE_OBS_BOTHSIDES){
                    if(state.playerOne->health <= 0 || state.playerOne->sprite.size < .05){
                        modelViewMatrix.Identity();
                        modelViewMatrix.Translate(-5.0f, 0.0f, 0.0f);
                        program.SetProjectionMatrix(projectionMatrix);
                        program.SetModelviewMatrix(modelViewMatrix);
                        DrawText(&program, fonts, "Player Two Wins With a score of "+ to_string(state.playerTwo->score), 0.3f, 0.0f);
                    }else if(state.playerTwo->health <= 0 || state.playerTwo->sprite.size < .05){
                        modelViewMatrix.Identity();
                        modelViewMatrix.Translate(-5.0f, 0.0f, 0.0f);
                        program.SetProjectionMatrix(projectionMatrix);
                        program.SetModelviewMatrix(modelViewMatrix);
                        DrawText(&program, fonts, "Player One Wins With a score of "+ to_string(state.playerOne->score), 0.3f, 0.0f);
                    }
                }else if(myGameType == TYPE_SOLO){
                    modelViewMatrix.Identity();
                    modelViewMatrix.Translate(-5.0f, 0.0f, 0.0f);
                    program.SetProjectionMatrix(projectionMatrix);
                    program.SetModelviewMatrix(modelViewMatrix);
                    DrawText(&program, fonts, "You had a score of "+ to_string(state.playerOne->score), 0.3f, 0.0f);
                }

        }
        SDL_GL_SwapWindow(displayWindow);
    }


    SDL_Quit();
    return 0;
}
