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
#define FIXED_TIMESTEP 1.0f

using namespace std;

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
bool detectCollision(Entity * a, Entity * b){
    //Top of a collides with bottom of b
    if((a->position.x+b->position.x/2<=.1) && (a->position.y + a->sprite.size/2) > (b->position.y - b->sprite.size/2))
        return true;
    //Bottom of a collides with top of b
    else if((a->position.x+b->position.x/2<=.1)&&(a->position.y - a->sprite.size/2) > (b->position.y + b->sprite.size/2))
        return true;
    
    //No collision
    return false;
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
//Updates gameState
void updateGame(GameState &state, float elapsed){
    bool collides = false;
    //Check if collides with a border
    for(size_t i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i]->position.x >= 4.8f || state.enemies[i]->position.x <= -4.8){
            collides = true;
            break;
        }
    }
    //Update the velocity if it does collide
    if(collides){
        for(size_t i = 0; i < state.enemies.size(); i++){
            state.enemies[i]->velocity.x *= -1;
        }
        collides = false;
    }
    bool hit = false;
    //check collisions between enemies and bullets
    for(size_t i = 0; i < state.bullets.size(); i++){
        for(size_t j = state.enemies.size()-1; j >= 1; j--){
            if(detectCollision(state.bullets[i],state.enemies[j])){
                hit = true;
                state.enemies.erase(state.enemies.begin()+j);
                break;
            }
        }
        if(hit){
            state.bullets.erase(state.bullets.begin()+i);
            break;
        }
        //Check first index of ships
        if(detectCollision(state.bullets[i],state.enemies[0])){
            state.enemies.erase(state.enemies.begin());
            state.bullets.erase(state.bullets.begin()+i);
            break;
        }
    }
    
    for(size_t i = 0; i < state.enemies.size(); i++){
        if(state.enemies[i] == nullptr){continue;}
        state.enemies[i]->position.x += elapsed * state.enemies[i]->velocity.x;
    }
    for(size_t i = 0; i < state.bullets.size(); i++){
        if(state.bullets[i] == nullptr){continue;}
        if(state.bullets[i]->position.y > 5.0f){
            state.bullets.erase(state.bullets.begin()+i);
            i -=  1;
        }else{
            state.bullets[i]->position.y += elapsed * state.bullets[i]->velocity.y;
        }
    }
}

Entity createBullet(Entity * player){
    GLuint spriteSheet = LoadTexture(RESOURCE_FOLDER"sheet.png");
    SheetSprite bull = SheetSprite(spriteSheet, 827.0f/1024.0f, 125.0f/1024.0f, 16.0f/1024.0f, 40.0f/1024.0f, .1f);
    Entity bullet = Entity(bull, player->position.x, player->position.y + 1.0f, 0.0f, 0.0f, .5f, 0.0f);
    return bullet;
}
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
        DrawText(program, fonts, "Press space to start", 0.2f, 0.0f);
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        if(keys[SDL_SCANCODE_SPACE]){
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
        modelViewMatrix.Identity();
        modelViewMatrix.Translate(state.bullets[i]->position.x, state.bullets[i]->position.y, 0.0f);
        p->SetProjectionMatrix(projectionMatrix);
        p->SetModelviewMatrix(modelViewMatrix);
        state.bullets[i]->Draw(p);
    }
}
bool checkAllDone(GameState &state){
    return state.enemies.size() == 0;
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
    SheetSprite play = SheetSprite(spriteSheet, 224.0f/1024.0f, 832.0f/1024.0f, 99.0f/1024.0f, 75.0f/1024.0f, .4f);
    Entity player = Entity(play, 0.0f, -2.7f, 0.0f, .1f, .1f, 0.0f);
    GameState state = GameState(&player);
    SDL_Event event;
    float frameTicks = 0.0f;
    bool done = false;
    while(!done)
        switch(mode){
            case STATE_MAIN_MENU:
                mode = updateMenu(&event,&program,modelViewMatrix,projectionMatrix, done, fonts);
                break;
            case STATE_GAME_LEVEL:
                while (!done) {
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                            done = true;
                        }
                        glClearColor(0.75f,0.86f,0.96f,1.0f);   //Change to light blue background
                        glClear(GL_COLOR_BUFFER_BIT);
                        glUseProgram(program.programID);
                        
                        float ticks = (float)SDL_GetTicks()/1000.0f;
                        float elapsed = ticks - frameTicks;
                        frameTicks = ticks;
                        const Uint8 *keys = SDL_GetKeyboardState(NULL);
                        if(keys[SDL_SCANCODE_LEFT] && !collidesBorder(state.player)){
                            state.player->position.x -= 1.5f * elapsed;
                        }else if(keys[SDL_SCANCODE_RIGHT] && !collidesBorder(state.player)){
                            state.player->position.x += 1.5f * elapsed;
                        }else if(keys[SDL_SCANCODE_SPACE]){
                            Entity bull = createBullet(state.player);
                            state.addBullet(&bull);
                        }
                        RenderGameState(&program, state, projectionMatrix, modelViewMatrix);
                        //Updates by one frame
                        updateGame(state,ticks);
                        if(checkAllDone(state)){
                            mode = STATE_MAIN_MENU;
                        }
                        SDL_GL_SwapWindow(displayWindow);
                    }
                }
                
        }
    
    SDL_Quit();
    return 0;
}
