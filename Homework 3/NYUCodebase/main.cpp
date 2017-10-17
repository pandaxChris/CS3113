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
#include "ShaderProgram.h"
#include "SheetSprite.h"
#include <vector>
#include <cmath>
#define FIXED_TIMESTEP .01f

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
enum GameMode{STATE_MAIN_MENU, STATE_GAME_LEVEL};

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
    Entity(SheetSprite s, float x, float y, float z, float dx, float dy, float dz):sprite(s), position(x,y,z), velocity(dx,dy,dz){}
    void Draw(ShaderProgram *p){
        sprite.Draw(p);
    }
    SheetSprite sprite;
    Vector3 position;
    Vector3 velocity;
};
vector<Entity *>  getSprites(){
    vector<Entity *>  ships;   //Vector containing 32 ships that will be blackship1 from spritesheet
    GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
    //Load enemy ship from sprite sheet
    SheetSprite ship = SheetSprite(spriteSheet, 423.0f/1024.0f, 728.0f/1024.0f, 93.0f/1024.0f, 84.0f/1024.0f, .4f);
    float x_val = -2.0f;
    float y_val = 4.0f;
    
    while(y_val > 0.0){
        ships.push_back(new Entity(ship, x_val, y_val, 0.0, 0.5f, -0.1f, 0.0f));
        x_val += 1.0f;
        if(x_val == 3.0){
            x_val = -2.0;
            y_val -= 1.0f;
        }
    }
    return ships;
}
//Function to detect top bottom collision between entities
bool detectCollision(Entity * a, Entity * b){
    //Top of A less than bottom of B
    if((a->position.y + a->sprite.size/2) < (b->position.y - b->sprite.size/2) )
        return false;
    //Bottom of A higher than top of B
    else if( (a->position.y - a->sprite.size/2) > (b->position.y + b->sprite.size/2) )
        return false;
    //Left of A greater than right of B
    else if ( (a->position.x - a->sprite.size/2) > (b->position.x + b->sprite.size/2) )
        return false;
    //Right of A smaller than left of B
    else if( (a->position.x + a->sprite.size/2) < (b->position.x - b->sprite.size/2) ){
        return false;
    }
    
    // collision
    return true;
}
class GameState{
public:
    GameState(){player = nullptr;}
    
    GameState(Entity * p){
        player = p;
        enemies = getSprites();
    }
    
    void addBullet(Entity * bull){
        bullets.push_back(bull);
    }
    Entity *player;
    vector<Entity *> enemies;
    vector<Entity *> bullets;
};
//Function to check for the bullet ship collision in the gamestate and modifies accordingly
void modifyCollisions(GameState & state){

    for(int i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i] == nullptr){continue;}
        for(int j = 0; j < state.bullets.size(); j++){
            if(state.bullets[j] == nullptr){continue;}
            
            if(detectCollision(state.bullets[j], state.enemies[i])){
                state.bullets[j] = nullptr;
                state.enemies[i] = nullptr;
                return;
            }
        }

    }
}
//Updates gameState
void updateGame(GameState &state, float elapsed){
    
    //Move the state up one state
    for(size_t i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i]== nullptr){continue;}
        state.enemies[i]->position.x += elapsed * state.enemies[i]->velocity.x;
    }
    //Move the bullets up one state
    for(size_t i = 0; i < state.bullets.size(); i++){
        if(state.bullets[i] == nullptr){continue;}
        state.bullets[i]->position.y += elapsed * state.bullets[i]->velocity.y;
    }
    
    bool collides = false;
    //Check if collides with a border
    for(size_t i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i] == nullptr){continue;}
        if(state.enemies[i]->position.x  + .1 >= 4.9f || state.enemies[i]->position.x - .1 <= -4.9){
            collides = true;
            break;
        }
    }
    //Update the velocity if it does collide
    if(collides){
        for(size_t i = 0; i < state.enemies.size(); i++){
            if(state.enemies[i] == nullptr){continue;}
            state.enemies[i]->velocity.x *= -1;
        }
        collides = false;
    }
    modifyCollisions(state);
    
}
//Function to check if player collides with the border
bool collidesBorder(Entity * a){
    if( a->position.x >= 4.8f){
        a->position.x = 4.7f;
        return true;
    }else if(a->position.x <= -4.8f){
        a->position.x = -4.7f;
        return true;
    }
    return false;
}


GameMode updateMenu(SDL_Event* event, ShaderProgram* program, Matrix& modelViewMatrix, Matrix& projectionMatrix, bool done, GLuint fonts){
    while (!done) {
        while (SDL_PollEvent(event)) {
            if (event->type == SDL_QUIT || event->type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glUseProgram(program->programID);
        glClearColor(0.75f,0.86f,0.96f,1.0f);   //Change to light blue background
        glClear(GL_COLOR_BUFFER_BIT);
        program->SetProjectionMatrix(projectionMatrix);
        program->SetModelviewMatrix(modelViewMatrix);
        modelViewMatrix.Translate(-1.0f, -3.0f, 0.0f);
        DrawText(program, fonts, "Space Invaders", 0.2f, 0.0f);
        
        program->SetProjectionMatrix(projectionMatrix);
        program->SetModelviewMatrix(modelViewMatrix);
        modelViewMatrix.Identity();
        modelViewMatrix.Translate(0.0f, 1.0f, 0.0f);
        DrawText(program, fonts, "Press Q to start", 0.2f, 0.0f);
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if(keys[SDL_SCANCODE_Q]){
            return STATE_GAME_LEVEL;
        }
        SDL_GL_SwapWindow(displayWindow);
    }
    return STATE_MAIN_MENU;
}

void RenderGameState(ShaderProgram *p, const GameState &state, Matrix &projectionMatrix, Matrix &modelViewMatrix){
    glUseProgram(p->programID);
    //Draw player ship
    modelViewMatrix.Identity();
    modelViewMatrix.Translate(state.player->position.x, state.player->position.y, state.player->position.z);
    p->SetProjectionMatrix(projectionMatrix);
    p->SetModelviewMatrix(modelViewMatrix);
    state.player->Draw(p);
    
    //Draw enemy ships
    for(size_t i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i] == nullptr){
            continue;
        }
        modelViewMatrix.Identity();
        modelViewMatrix.Translate(state.enemies[i]->position.x, state.enemies[i]->position.y, 0.0f);
        p->SetProjectionMatrix(projectionMatrix);
        p->SetModelviewMatrix(modelViewMatrix);
        state.enemies[i]->Draw(p);
    }
    
    //Draw bullets
    for(size_t i =0; i < state.bullets.size();i++){
        if(state.bullets[i] == nullptr){continue;}
        modelViewMatrix.Identity();
        modelViewMatrix.Translate(state.bullets[i]->position.x, state.bullets[i]->position.y, 0.0f);
        p->SetProjectionMatrix(projectionMatrix);
        p->SetModelviewMatrix(modelViewMatrix);
        state.bullets[i]->Draw(p);
    }
}
//Checks if game over
bool checkGameOver(GameState &state){
    for(size_t i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i]!=nullptr)
            return false;
    }
    return true;
}
Entity* getDownwardBullet(const GameState & state){
    Entity *e = nullptr;
    for(int i = 0; i < state.bullets.size(); i++){
        if(state.bullets[i] == nullptr){continue;}
        if(state.bullets[i]->velocity.y < 0){
            e = state.bullets[i];
            break;
        }
    }
    return e;
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 640, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    Matrix projectionMatrix, modelViewMatrix;
    projectionMatrix.SetOrthoProjection(-6.0f, 6.0f, -5.0f, 5.0f, -1.0f, 1.0f);
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    GLuint fonts = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
    GameMode mode = STATE_MAIN_MENU;
    GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
    SheetSprite bull = SheetSprite(spriteSheet, 827.0f/1024.0f, 125.0f/1024.0f, 16.0f/1024.0f, 40.0f/1024.0f, .4f);
    SheetSprite play = SheetSprite(spriteSheet, 224.0f/1024.0f, 832.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f, .5f);
    Entity player = Entity(play, 0.0f, -3.5f, 0.0f, .1f, .1f, 0.0f);
    GameState state;
    Entity * eBullet;
    SDL_Event event;
    bool enemyBullet = false;
    float frameTicks = 0.0f;
    float accumulator = 0.0f;
    bool done = false;
    while(!done){
        switch(mode){
            case STATE_MAIN_MENU:
                mode = updateMenu(&event,&program,modelViewMatrix,projectionMatrix, done, fonts);
                break;
            case STATE_GAME_LEVEL:
                state = GameState(&player);
                
                while (!done) {
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                            done = true;
                        }
                    }
                    
                    glClearColor(0.75f,0.86f,0.96f,1.0f);   //Change to light blue background
                    glClear(GL_COLOR_BUFFER_BIT);
                    glUseProgram(program.programID);
                    
                    float ticks = (float)SDL_GetTicks()/1000.0f;
                    float elapsed = ticks - frameTicks;
                    frameTicks = ticks;
                    //Handle user key presses
                    const Uint8 *keys = SDL_GetKeyboardState(NULL);
                    if(keys[SDL_SCANCODE_LEFT] && !collidesBorder(state.player)){
                        state.player->position.x -= 1.5f * elapsed;
                    }else if(keys[SDL_SCANCODE_RIGHT] && !collidesBorder(state.player)){
                        state.player->position.x += 1.5f * elapsed;
                    }else if(keys[SDL_SCANCODE_SPACE]){
                        Entity * bullet = new Entity(bull, state.player->position.x, state.player->position.y + 1.0f, 0.0f, 0.0f, .5f, 0.0f);
                        state.bullets.push_back(bullet);
                        enemyBullet = true;
                    }
                    //Handle enemy shooting bullet
                    if(!enemyBullet){   //No enemy bullet
                        eBullet = new Entity(bull,  rand() % 11 - 4, .5f, 0.0f, 0.0f, -.5f, 0.0f);
                        state.bullets.push_back(eBullet);
                        enemyBullet = true;
                    }else{
                        Entity * bullet = getDownwardBullet(state);
                        //Check if hit player
                        if(detectCollision(state.player,bullet)){
                            //Close window upon win
                            done=true;
                        }
                        //Else check if it's below screen
                        else if(bullet->position.y < -4.8){
                            for(size_t i = 0; i < state.bullets.size(); i++){
                                if(state.bullets[i] == bullet){
                                    state.bullets.erase(state.bullets.begin()+i);
                                    break;
                                }
                            }
                            enemyBullet = false;
                        }
                    }
                    
                    elapsed += accumulator;
                    if(elapsed < FIXED_TIMESTEP){
                        accumulator = elapsed;
                        continue;
                    }
                    while(elapsed >= FIXED_TIMESTEP){
                        updateGame(state,elapsed);
                        elapsed -= FIXED_TIMESTEP;
                    }
                    accumulator = elapsed;
                    
                    //Display the entities
                    RenderGameState(&program, state, projectionMatrix, modelViewMatrix);
                    if(checkGameOver(state)){
                        done = true;
                    }
                    SDL_GL_SwapWindow(displayWindow);
                }
            }   //End switch
    }


    SDL_Quit();
    return 0;
}
