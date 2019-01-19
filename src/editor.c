#include "editor.h"
#include <stdio.h>

#define RAYGUI_IMPLEMENTATION
#include "../external/raygui.h"
#include <math.h>

typedef enum {
	WIDGET_WindowBox=0,
	WIDGET_GroupBox,
	WIDGET_Line,
	WIDGET_Panel,
	WIDGET_ScrollPanel,
	WIDGET_Label,
	WIDGET_Button,
	WIDGET_LabelButton,
	WIDGET_ImageButton,
	WIDGET_Toggle,
	WIDGET_ToggleGroup,
	WIDGET_CheckBox,
	WIDGET_ComboBox,
	WIDGET_DropdownBox,
	WIDGET_Spinner,
	WIDGET_ValueBox,
	WIDGET_TextBox,
	WIDGET_TextBoxMulti,
	WIDGET_Slider,
	WIDGET_SliderBar,
	WIDGET_ProgressBar,
	WIDGET_StatusBar,
	WIDGET_Dummy,
	WIDGET_ListView,
	WIDGET_ColorPicker,
	WIDGET_MessageBox,
	WIDGET_ColorPanel,
	WIDGET_ColorBarAlpha,
	WIDGET_ColorBarHue,
	WIDGET_Grid,
	WIDGET_COUNT
} WidgetType;

char* WidgetName[] = {
	"WindowBox",
	"GroupBox",
	"Line",
	"Panel",
	"ScrollPanel",
	"Label",
	"Button",
	"LabelButton",
	"ImageButton",
	"Toggle",
	"ToggleGroup",
	"CheckBox",
	"ComboBox",
	"DropdownBox",
	"Spinner",
	"ValueBox",
	"TextBox",
	"TextBoxMulti",
	"Slider",
	"SliderBar",
	"ProgressBar",
	"StatusBar",
	"Dummy",
	"ListView",
	"ColorPicker",
	"MessageBox",
	//newer/experimental controls in raygui!?
	"ColorPanel",
	"ColorBarAlpha",
	"ColorBarHue",
	"Grid"
};


typedef enum {
	MODE_NORMAL = 0,
	MODE_RESIZE_WIDGET,
	MODE_MOVE_WIDGET,
	MODE_SHOW_MENU,
} EditorMode;

const char* EditorModeName[] = {
	"NORMAL", "RESIZE", "MOVE", "MENU"
};

typedef struct {
	WidgetType type;
	Rectangle bounds; 
} Widget;

typedef Array(Widget) ArrayWidget;


/* RESIZER POINTS ARE ARRANGED LIKE THIS
 *    7     0     1 
 *   NW     N     NE
 *     *----*----*
 *     |         |
 * 6 W *         * E 2
 *     |         |
 *     *----*----*
 *   SW     S     SE
 *   5      4     3
 **/

typedef enum {
	RESIZER_POINT_N = 0,
	RESIZER_POINT_NE,
	RESIZER_POINT_E,
	RESIZER_POINT_SE,
	RESIZER_POINT_S,
	RESIZER_POINT_SW,
	RESIZER_POINT_W,
	RESIZER_POINT_NW,
	RESIZER_POINT_COUNT
}ResizerPoint;
Rectangle resizerPoints[RESIZER_POINT_COUNT] = {0};
int resizerPointActive = -1;


EditorMode mode = MODE_NORMAL;
int snapDistance = 5;
bool snap = true;
int selectedWidget = -1;
int addWidget = -1;
Texture2D texture; //a dummy texture used as a placeholder (some widgets require a texture)

ArrayWidget widgets = {0};
Color resizerColor = {245,0,0,140};
const int resizerPointSize = 8;

Rectangle menu = {0,0,0,0};
int scrollIndex = 0;
Vector2 lastMousePosition = {0,0};

//colors for the the snap grid
const Color gridLineColor[2] = { 
	(Color){ 120, 120, 120, 25 }, 
	(Color){ 120, 120, 120, 50 } 
};
static inline void DrawSnapGrid(int size, Color c) {
	//calculate how many squares we can fit
	int sq = (screenWidth>screenHeight) ? screenWidth/size : screenHeight/size;
	//and draw them
	for(int i=0; i<sq; ++i) {
		DrawLine(0, i*size, screenWidth, i*size,  c);
		DrawLine(i*size, 0, i*size, screenHeight, c);
	}
}


static inline int CheckCollisionWithResizerPoints() {
	Vector2 mouse = GetMousePosition();
	for(int i=0; i<RESIZER_POINT_COUNT; ++i) {
		if(CheckCollisionPointRec(mouse, resizerPoints[i]))
			return i;
	}
	return -1;
}

static inline void RecalculateResizePoints() {
	Rectangle r = Array_at(&widgets, selectedWidget).bounds;
	r.x-=4; r.y-=4; r.width+=8; r.height+=8;
	
	const int hrp = resizerPointSize/2;
	resizerPoints[RESIZER_POINT_N] = (Rectangle) {r.x+(r.width-resizerPointSize)/2, r.y-hrp, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_NE] = (Rectangle) {r.x+r.width-hrp, r.y-hrp, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_E] = (Rectangle) {r.x+r.width-hrp, r.y+(r.height-resizerPointSize)/2, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_SE] = (Rectangle) {r.x+r.width-hrp, r.y+r.height-hrp, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_S] = (Rectangle) {r.x+(r.width-resizerPointSize)/2, r.y+r.height-hrp, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_SW] = (Rectangle) {r.x-hrp, r.y+r.height-hrp, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_W] = (Rectangle) {r.x-hrp, r.y+(r.height-resizerPointSize)/2, 
				resizerPointSize, resizerPointSize};
	resizerPoints[RESIZER_POINT_NW] = (Rectangle) {r.x-hrp, r.y-hrp, 
				resizerPointSize, resizerPointSize};
}

//Bring to front (increase widget depth by one)
//widgets are rendered from lowest depth to highest
//so widgets with high depth will be rendered above 
//widgets with lower depth
static inline void BringToFront() {
	if( selectedWidget+1 < Array_size(&widgets)) {
		Widget w = Array_at(&widgets, selectedWidget);
		Array_at(&widgets, selectedWidget) = Array_at(&widgets, selectedWidget+1);
		Array_at(&widgets, selectedWidget+1) = w;
		TraceLog(LOG_INFO, TextFormat("Changing depth %i -> %i", selectedWidget, selectedWidget+1));
		selectedWidget += 1;
	}
}

//Send to back (decrease widget depth by one)
//widgets are rendered from lowest depth to highest
//so widgets with lower depth will be rendered below 
//widgets with higher depth
static inline void SendToBack() {
	if(selectedWidget - 1 >= 0) {
		Widget w = Array_at(&widgets, selectedWidget);
		Array_at(&widgets, selectedWidget) = Array_at(&widgets, selectedWidget-1);
		Array_at(&widgets, selectedWidget-1) = w;
		TraceLog(LOG_INFO, TextFormat("Changing depth %i -> %i", selectedWidget, selectedWidget-1));
		selectedWidget -= 1;
	}
}

int SelectWidget() {
	Vector2 mouse = GetMousePosition();
	if(Array_size(&widgets) == 0) return -1;
	for(int i=Array_size(&widgets)-1; i>=0; --i) {
		Widget w = Array_at(&widgets, i);
		if(CheckCollisionPointRec(mouse, w.bounds)) {
			return i;
		}
	}
	return -1;
}

void SaveUI() {
	int count = Array_size(&widgets);
	if(count == 0) return;
	const char* file = "project.ui";
	remove(file);
	//WRITE THE BINARY `*.ui` FILE
	FILE* f = fopen(file, "wb");
	if(f == NULL) {
		TraceLog(LOG_WARNING,TextFormat("Failed to save UI to file `%s`", file));
		return;
	}
	
	const char* magic = "UIF"; //write magic
	fwrite(magic, sizeof(char), strlen(magic), f);
	//write widget count
	fwrite(&count, 1, sizeof(int), f);
	//dump all the widgets
	for(ArrayIt i=0; i<count; ++i) {
		Widget w = Array_at(&widgets, i);
		fwrite(&w, 1, sizeof(Widget), f); //FIXME: this is dangerous fix later
	}
	fclose(f);
	
	//WRITE THE C SOURCE FILE
	const char* cfile = TextFormat("%s.c", file);
	remove(cfile);
	f = fopen(cfile, "wb");
	if(f == NULL) {
		TraceLog(LOG_WARNING, TextFormat("Failed to save UI to file `%s`", cfile));
		return;
	}
	const char* header = "#include <raylib.h>\n"\
	"#define RAYGUI_IMPLEMENTATION\n"\
	"#include <raygui.h>\n\n"\
	"void DrawGUI() {\n";
	fwrite(header, 1, strlen(header), f);
	for(ArrayIt i = 0; i<Array_size(&widgets); ++i){
		Widget w = Array_at(&widgets, i);
		char text[1024] = {0};
		switch(w.type) {
			case WIDGET_WindowBox:
			case WIDGET_GroupBox:
			case WIDGET_Button:
			case WIDGET_LabelButton:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\");\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_Dummy:
				sprintf(text, "    Gui%sRec((Rectangle){%i,%i,%i,%i}, \"%s%i\");\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_Line:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, 1);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_Panel:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i});\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_ScrollPanel:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, (Rectangle){0,0,0,0}, (Vector2){0,0});\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_Label:
				sprintf(text, "    Gui%sEx((Rectangle){%i,%i,%i,%i}, \"%s%i\", 0, 4);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_ImageButton:
				sprintf(text, "    Gui%sEx((Rectangle){%i,%i,%i,%i}, (Texture){0}, (Rectangle){0,0,20,20}, \"%s%i\");\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height, 
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_Toggle:
			case WIDGET_CheckBox:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\", true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_ToggleGroup:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\", true, 4, 1);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_ComboBox:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\", 0);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_DropdownBox:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\", &(int){0}, false);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_Spinner:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i},&(int){0}, 0, 100, 20, true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_ValueBox:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i},&(int){0}, 0, 100, true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_TextBox:
			case WIDGET_TextBoxMulti:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, (char*)&(char[32]){\"%s%i\"}, 32, true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_Slider:
			case WIDGET_SliderBar:
				sprintf(text, "    Gui%sEx((Rectangle){%i,%i,%i,%i}, \"%s%i\", 0.f, 0.f, 100.f, true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_ProgressBar:
				sprintf(text, "    Gui%sEx((Rectangle){%i,%i,%i,%i}, 0.f, 0.f, 100.f, true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_StatusBar:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\", 4);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			
			case WIDGET_ListView:
				sprintf(text, "    Gui%sEx((Rectangle){%i,%i,%i,%i}, (const char**)&(char*[]){\"ItemA\", \"ItemB\"}, NULL,"\
					"2, &(int){0}, &(int){0}, NULL, true);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_ColorPicker:
			case WIDGET_ColorPanel:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, DARKBLUE);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_MessageBox:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, \"%s%i\", \"MESSAGE HERE\");\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height,
					WidgetName[w.type], (int)i);
			break;
			
			case WIDGET_ColorBarAlpha:
			case WIDGET_ColorBarHue:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, 0.5f);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			case WIDGET_Grid:
				sprintf(text, "    Gui%s((Rectangle){%i,%i,%i,%i}, 10, 1);\n",
					WidgetName[w.type], (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height);
			break;
			
			default:
			break;
		}
		
		if(text[0] != '\0') fwrite(text, 1, strlen(text), f);
	}
	const char* footer = "}\n"\
	"int main(int argc, char **argv) {\n"\
	"    InitWindow(800, 450, \"GUI Test\");\n"\
	"    SetTargetFPS(60);\n\n"\
	"    while(!WindowShouldClose())\n"\
	"    {\n"\
	"        BeginDrawing();\n"\
	"        DrawGUI();\n"\
	"        EndDrawing();\n"\
	"    }\n"\
	"    CloseWindow();\n"\
	"    return 0;\n"\
	"}\n\n";
	
	fwrite(footer,1,strlen(footer),f);
	fclose(f);
	TraceLog(LOG_INFO,TextFormat("UI saved to `%s` and `%s.c`", file, file));
}

void LoadUI() {
	int count = 0;
	char** files = GetDroppedFiles(&count);
	if(count == 0) return;
	
	FILE* f = fopen(files[0], "rb");
	ClearDroppedFiles();
	if(f == NULL) {
		TraceLog(LOG_WARNING,TextFormat("Failed to load UI from file `%s`", files[0]));
		return;
	}
	
	char magic[4] = {0};
	fread(&magic,1,3,f);
	if(strncmp((const char*)&magic, "UIF\0", sizeof(magic)) == 0) {
		fread(&count,1,sizeof(int), f);
		if(count < 1000) {
			void* tmp = malloc(count*sizeof(Widget));
			fread(tmp, sizeof(Widget), count, f);
			Array_insert(&widgets, 0, tmp, count);
			free(tmp);
		}
	}
	fclose(f);
}

static inline void ResizeWidget() {
	if(resizerPointActive != -1) //should not happen but still check to be safe
	{
		Vector2 mouse = GetMousePosition();
		if(snap) { //snap to grid if enabled
			mouse.x = ((int)(mouse.x/snapDistance))*snapDistance;
			mouse.y = ((int)(mouse.y/snapDistance))*snapDistance;
			lastMousePosition.x = ((int)(lastMousePosition.x/snapDistance))*snapDistance;
			lastMousePosition.y = ((int)(lastMousePosition.y/snapDistance))*snapDistance;
		}
		
		ResizerPoint p = resizerPointActive;
		Rectangle* r = &(Array_at(&widgets, selectedWidget).bounds);
		switch(p) {
			case RESIZER_POINT_N:
				r->height = r->height + r->y - mouse.y;
				r->y = mouse.y;
			break;
			case RESIZER_POINT_NE:
				r->height = r->height + r->y - mouse.y;
				r->y = mouse.y;
				r->width = mouse.x - r->x;
			break;
			case RESIZER_POINT_E: 
				r->width = mouse.x - r->x;
			break;
			case RESIZER_POINT_SE:
				r->width = mouse.x - r->x;
				r->height = mouse.y - r->y;
			break;
			case RESIZER_POINT_S:
				r->height = mouse.y - r->y;
			break;
			case RESIZER_POINT_SW:
				r->width =r->width + r->x - mouse.x;
				r->x = mouse.x;
				r->height = mouse.y - r->y;
			break;
			case RESIZER_POINT_W:
				r->width =r->width + r->x - mouse.x;
				r->x = mouse.x;
			break;
			case RESIZER_POINT_NW:
				r->width = r->width + r->x - mouse.x;
				r->x = mouse.x;
				r->height = r->height + r->y - mouse.y;
				r->y = mouse.y;
			break;
			
			default: break;
		}
		
		lastMousePosition = mouse;
	}
	
}

void UpdateEditor() {
	Vector2 mouse = GetMousePosition();
	if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
			mode = MODE_SHOW_MENU;
			selectedWidget = -1;
			addWidget = -1;
			menu = (Rectangle){mouse.x, mouse.y, 200, 320};
	}else{
		if(mode != MODE_SHOW_MENU) {
			if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				if(selectedWidget == -1) 
					selectedWidget = SelectWidget();
				else {
					resizerPointActive = CheckCollisionWithResizerPoints();
					if( resizerPointActive != -1) {
						mode = MODE_RESIZE_WIDGET;
					}
					else if(mode != MODE_RESIZE_WIDGET) {
						selectedWidget = SelectWidget();
						if(selectedWidget == -1) mode = MODE_NORMAL;
						else {
							if(snap) {
								//realign widget to snap grid 
								//happens when we added/moved a widget when snap was off
								Rectangle* r = &(Array_at(&widgets, selectedWidget).bounds);
								r->x = ((int)(r->x/snapDistance))*snapDistance;
								r->y = ((int)(r->y/snapDistance))*snapDistance;
								r->width = ((int)(r->width/snapDistance))*snapDistance;
								r->height = ((int)(r->height/snapDistance))*snapDistance;
							}
							mode = MODE_MOVE_WIDGET; 
							lastMousePosition = mouse;
						}
					}
				}
				
				if(selectedWidget != -1) RecalculateResizePoints();
			}
			
			if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
				if(mode == MODE_MOVE_WIDGET) {
					if(selectedWidget != -1) {
						Rectangle* r = &(Array_at(&widgets, selectedWidget).bounds);
						if(snap) {
							mouse.x = ((int)(mouse.x/snapDistance))*snapDistance;
							mouse.y = ((int)(mouse.y/snapDistance))*snapDistance;
							lastMousePosition.x = ((int)(lastMousePosition.x/snapDistance))*snapDistance;
							lastMousePosition.y = ((int)(lastMousePosition.y/snapDistance))*snapDistance;
						}
						r->x += mouse.x - lastMousePosition.x;
						r->y += mouse.y - lastMousePosition.y;
						RecalculateResizePoints();
						lastMousePosition = mouse;
					}
				} else if(mode == MODE_RESIZE_WIDGET) {
					if(selectedWidget != -1) {
						ResizeWidget();
						RecalculateResizePoints();
					}
				}
			} else if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) mode = MODE_NORMAL;
		}
	}
	
	//KEYS
	if(selectedWidget != -1){
		if(IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_UP)) BringToFront();
		else if(IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_DOWN)) SendToBack();
		else if(IsKeyPressed(KEY_DELETE)||IsKeyPressed(KEY_X)) {
			Array_remove(&widgets, selectedWidget, 1);
			selectedWidget = -1;
			mode = MODE_NORMAL;
		}
		else if(IsKeyPressed(KEY_D)) {
			//duplicate widget
			Widget w = Array_at(&widgets, selectedWidget);
			Array_push(&widgets, w);
		}
	}
	
	if(IsKeyPressed(KEY_SPACE)) {
		//toggle snap
		snap = !snap;
	}
	else if(IsKeyPressed(KEY_S)) {
		//save UI to file
		SaveUI();
	}
	else if(IsFileDropped()) {
		//load UI from file
		LoadUI();
	}
	
}

void InitializeEditor() {
	Array_create(&widgets, 2); //initialize the widget array
	
	//generate the dummy texture required by some widgets (image button)
	Image tmp = GenImageChecked(100,100,5,5, RAYWHITE, GRAY);
	texture = LoadTextureFromImage(tmp);
	UnloadImage(tmp);
}

void FinalizeEditor() {
	Array_destroy(&widgets);
	UnloadTexture(texture);
}


void AddWidget() 
{
	int x = menu.x, y = menu.y;
	if(snap) {
		x = ((int)(menu.x/snapDistance))*snapDistance;
		y = ((int)(menu.y/snapDistance))*snapDistance;
	}
	Widget w={0};
	w.type = addWidget;
	switch(addWidget) {
		case WIDGET_WindowBox:
		case WIDGET_GroupBox:
		case WIDGET_Panel:
		case WIDGET_ToggleGroup:
			w.bounds = (Rectangle){x, y, 140, 100};
		break;
		
		case WIDGET_Line:
			w.bounds = (Rectangle){x, y, 100, 10};
		break;
		
		case WIDGET_ScrollPanel:
		case WIDGET_TextBoxMulti:
		case WIDGET_Grid:
			w.bounds = (Rectangle){x,y,200,200};
		break;
		
		case WIDGET_Label:
		case WIDGET_Button:
		case WIDGET_LabelButton:
		case WIDGET_ImageButton:
		case WIDGET_Toggle:
		case WIDGET_ComboBox:
		case WIDGET_DropdownBox:
		case WIDGET_TextBox:
		case WIDGET_StatusBar:
		case WIDGET_Dummy:
			w.bounds = (Rectangle){x,y,140,40};
		break;
		
		case WIDGET_Spinner:
		case WIDGET_ValueBox:
		case WIDGET_Slider:
		case WIDGET_SliderBar:
		case WIDGET_ProgressBar:
		case WIDGET_ColorBarAlpha:
			w.bounds = (Rectangle){x,y,100,20};
		break;
		
		case WIDGET_CheckBox:
			w.bounds = (Rectangle) {x,y,20,20};
		break;
		
		case WIDGET_ListView:
		case WIDGET_MessageBox:
			w.bounds = (Rectangle) {x,y,200,120};
		break;
		
		case WIDGET_ColorPicker:
		case WIDGET_ColorPanel:
			w.bounds = (Rectangle){x, y, 80, 60};
		break;
		
		case WIDGET_ColorBarHue:
			w.bounds = (Rectangle){x, y, 20, 120};
		break;
		
		default:
			return;
	}
	Array_append(&widgets, w);
	addWidget = -1;
	selectedWidget = Array_size(&widgets)-1;
	RecalculateResizePoints();
}

void DrawMenu() {
	GuiListView(menu, (const char**)WidgetName, WIDGET_COUNT, &scrollIndex, &addWidget, true); 
	//hack to make the ListView behave like a menu
	if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
		if(addWidget >=0 && addWidget < WIDGET_COUNT) {
			TraceLog(LOG_INFO, TextFormat("ADDING:%s", WidgetName[addWidget]));
			AddWidget();
		}
		mode = MODE_NORMAL;
	}
}


void DrawResizePoints() {
	Rectangle r = Array_at(&widgets, selectedWidget).bounds;
	r.x-=resizerPointSize/2; r.y-=resizerPointSize/2; 
	r.width+=resizerPointSize; r.height+=resizerPointSize;
	DrawRectangleLinesEx(r, 1, resizerColor);
	for(int i=0; i<RESIZER_POINT_COUNT; ++i) {
		DrawRectangleRec(resizerPoints[i], resizerColor);
	}
}

void DrawEditor() {
	//DRAW GRID
	if(snap){
		DrawSnapGrid(snapDistance*4, gridLineColor[1]);
		DrawSnapGrid(snapDistance, gridLineColor[0]);
	}
	
	//DRAW WIDGETS
	GuiLock(); //lock so widgets won't get focused
	for(ArrayIt i = 0; i< Array_size(&widgets); ++i) {
		Widget w = Array_at(&widgets, i);
		switch(w.type) {
			case WIDGET_WindowBox:
				GuiWindowBox(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i));
			break;
			
			case WIDGET_GroupBox:
				GuiGroupBox(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i));
			break;
			
			case WIDGET_Line:
				GuiLine(w.bounds, 1);
			break;
			
			case WIDGET_Panel:
				GuiPanel(w.bounds);
			break;
			
			case WIDGET_ScrollPanel:
				GuiScrollPanel(w.bounds,(Rectangle){0,0,0,0},(Vector2){0,0});
			break;
			
			case WIDGET_Label:
				GuiLabelEx(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), 0, 4);
			break;
			
			case WIDGET_Button:
				GuiButton(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i));
			break;
			
			case WIDGET_LabelButton:
				GuiLabelButton(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i));
			break;
			
			case WIDGET_ImageButton:
				GuiImageButtonEx(w.bounds, texture, (Rectangle){0,0,20,20}, 
					TextFormat("  %s%03i", WidgetName[w.type], i));
			break;
			
			case WIDGET_Toggle:
				GuiToggle(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), true);
			break;
			
			case WIDGET_ToggleGroup:
				GuiToggleGroupEx(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i),true, 4, 1);
			break;
			
			case WIDGET_CheckBox:
				GuiCheckBox(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), true);
			break;
			
			case WIDGET_ComboBox:
				GuiComboBox(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), 0);
			break;
			
			case WIDGET_DropdownBox:{
				int active = 0;
				GuiDropdownBox(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), &active, false);
			}
			break;
			
			case WIDGET_Spinner:{
				int value = 30;
				GuiSpinner(w.bounds,&value,0,100,20,true);
			}
			break;
			
			case WIDGET_ValueBox:{
				int value = 80;
				GuiValueBox(w.bounds,&value,0,100,true);
			}
			break;
			
			case WIDGET_TextBox:
				GuiTextBox(w.bounds, (char*)TextFormat("%s%03i", WidgetName[w.type], i), 32, true);
			break;
			
			case WIDGET_TextBoxMulti:
				GuiTextBoxMulti(w.bounds, (char*)TextFormat("%s%03i", WidgetName[w.type], i), 32, true);
			break;
			
			case WIDGET_Slider:
				GuiSliderEx(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), 70.f, 0.f, 100.f, true);
			break;
			
			case WIDGET_SliderBar:
				GuiSliderBarEx(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), 70.f, 0.f, 100.f, true);
			break;
			
			case WIDGET_ProgressBar:
				GuiProgressBarEx(w.bounds, 40.f, 0.f, 100.f, true);
			break;
			
			case WIDGET_StatusBar:
				GuiStatusBar(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), 4);
			break;
			
			case WIDGET_Dummy:
				GuiDummyRec(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i));
			break;
			
			case WIDGET_ListView:{
				int scroll = 0, active = 2;
				static const char *entries[] = { "Cowboy Bebop", "Evangelion", "Slayers", "Trigun", "Dragon Ball" };
				GuiListViewEx(w.bounds, entries, NULL, sizeof(entries)/sizeof(entries[0]), &scroll, &active, NULL, true);
			}
			break;
			
			case WIDGET_ColorPicker:
				GuiColorPicker(w.bounds, DARKBLUE);
			break;
			
			case WIDGET_MessageBox:{
				const char* msg = "Hi, how are you today?";
				GuiMessageBox(w.bounds, TextFormat("%s%03i", WidgetName[w.type], i), msg);
			}
			break;
			
			//NEWER CONTROLS IN RAYGUI?
			case WIDGET_ColorPanel:
				GuiColorPanel(w.bounds, GOLD);
			break;
			
			case WIDGET_ColorBarAlpha:
				GuiColorBarAlpha(w.bounds, 0.3f);
			break;
			
			case WIDGET_ColorBarHue:
				GuiColorBarHue(w.bounds, 0.2f);
			break;
			
			case WIDGET_Grid:
				GuiGrid(w.bounds, 10, 1);
			break;
			
			default:
			break;
		}
	}
	GuiUnlock();
	
	
	
	//DRAW OWN UI ABOVE THE WIDGETS
	if(selectedWidget != -1 && mode != MODE_SHOW_MENU)
		DrawResizePoints();
		
	if(selectedWidget != -1) {
		Widget w = Array_at(&widgets, selectedWidget);
		char* const tsnap = snap?"ON":"OFF";
		DrawText(TextFormat("DEPTH:%03i | SNAP:%s | %i widgets | BOUNDS:[%i %i %i %i] | %s", selectedWidget, tsnap, 
			Array_size(&widgets), (int)w.bounds.x, (int)w.bounds.y, (int)w.bounds.width, (int)w.bounds.height, 
			EditorModeName[mode]), 4, 4, 10, BLACK);
	} else {
		char* const tsnap = snap?"ON":"OFF";
		DrawText(TextFormat("SNAP:%s | %s | %i widgets", tsnap, EditorModeName[mode], Array_size(&widgets)), 4, 4, 10, BLACK);
	}
	
	//SHOW MENU
	if(mode == MODE_SHOW_MENU) {
		DrawMenu();
	}
	
	//DRAW GRADIENTS
	DrawRectangleGradientV(0,0,screenWidth, 10, (Color){0,0,0,80}, (Color){0,0,0,0});
	DrawRectangleGradientV(0,screenHeight-10,screenWidth, 10, (Color){0,0,0,0}, (Color){0,0,0,80});
}

