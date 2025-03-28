#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define global static


#include "../types.h"
#include "editor.c"

void DisplayTarget(RenderTexture2D target);

int main(int argc, char** argv) {

	EditorStart(argc, argv);

	while(!WindowShouldClose()) {
		EditorMouseUpdate();
		EditorKeyboardUpdate();

		CanvasRender();

		BeginDrawing();
		ClearBackground(GRAY);
		DrawTexturePro(ATLAS_PAGES[ATLAS_PAGE], PALETTE_SRC, PALETTE_DST, 
			(Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

		DrawTexturePro(ATLAS_PAGES[ATLAS_PAGE], SELECTED_TILE, 
			(Rectangle){ 
				ATLAS_PAGES[ATLAS_PAGE].width * PALETTE_SCALE, 0, 
				32, 32 
			}, (Vector2){0.0f, 0.0f},
			0.0f, WHITE);

		CanvasDisplay();

		EndDrawing();
	}

	CloseWindow();
}

/*
void DisplayTarget(RenderTexture2D target) {
	// NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
	DrawTexturePro(target.texture, 
		(Rectangle) { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },
		//(Rectangle){ 0.0f, 0.0f, (float)Editor.width, (float)Editor.height },
		(Rectangle) { Canvas.dest.x, Canvas.dest.y, 
			(Canvas.dest.width * Canvas.zoom), 
			(Canvas.dest.height * Canvas.zoom) },
		(Vector2) { 0.0f, 0.0f }, 0.0f, WHITE
	);
}*/
