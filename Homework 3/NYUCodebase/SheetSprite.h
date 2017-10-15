
#ifndef SheetSprite_h
#define SheetSprite_h
#include "ShaderProgram.h"

#endif /* SheetSprite_h */


class SheetSprite {
public:
    SheetSprite();
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size);
    
    void Draw(ShaderProgram *program);
    
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};
