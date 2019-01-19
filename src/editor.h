#ifndef GE_EDITOR_H
#define GE_EDITOR_H

#include <raylib.h>
#include "../external/array.h"

static const int screenWidth = 800;
static const int screenHeight = 450;

extern void InitializeEditor();
extern void DrawEditor();
extern void UpdateEditor();
extern void FinalizeEditor();

#endif