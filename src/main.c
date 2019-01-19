#include <stdio.h>
#include <stdlib.h>
#include "editor.h"

int main(int argc, char **argv)
{
	InitWindow(screenWidth, screenHeight, "GUI Editor");
	SetTargetFPS(60);
	
	InitializeEditor();
	
	while(!WindowShouldClose()) 
	{
		UpdateEditor();
		
		BeginDrawing();
			ClearBackground(RAYWHITE);
			DrawEditor();
		
		EndDrawing();
	}
	
	FinalizeEditor();
	
	CloseWindow();
	return EXIT_SUCCESS;
}