	
// tpbds -- System font (needs ResourceHub).

#ifndef _SYSTEM_FONT_H_
#define _SYSTEM_FONT_H_

void SystemFont_RequestTexture(ResourceHub &resHub);

// Draws a single line of text.
// xPos, yPos - Position in character space (40 on Y, 40 multiplied by aspect ratio on X).
void SystemFont_Draw(Renderer &renderer, const std::string &text, unsigned int xPos, unsigned int yPos);

#endif // _SYSTEM_FONT_H_
