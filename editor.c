/*
	TODO:
		maybe make it so that the canvas scales uniformly on all sides
		rather than just on the right and bottom

		erasing *
			allow user to change the tile associated with eraser
		picker *
		buttons
			save, load
		highlight currently selected tile on palette
 * */

#define PALETTE_MAX 10
#define PALETTE_PATH_MAX 64

global char* BLISS_HEADER = "BLISS";
#define BLISS_HEADER_LEN 5

global int WINDOW_WIDTH = 1280;
global int WINDOW_HEIGHT = 720;

#define IO_FILENAME_MAX 64
global char OUTPUT_FILENAME[IO_FILENAME_MAX] = "default.bliss";
global char INPUT_FILENAME[IO_FILENAME_MAX]  = "default.bliss";
global int  EDITOR_LOAD_LEVEL;

global char ATLAS_PATHS[PALETTE_MAX][PALETTE_PATH_MAX+1];
global int ATLAS_PAGE_COUNT;
global Texture2D ATLAS_PAGES[PALETTE_MAX];
global int ATLAS_PAGE = 0;
global int ATLAS_WIDTH;
global int ATLAS_HEIGHT;

global int TILE_SIZE = 8;
#define    TILE_VOID -1 // index for lack of tile

global Rectangle SELECTED_TILE = { TILE_VOID, TILE_VOID, 8, 8 };
global int       SELECTED_TILE_INDEX = TILE_VOID;

global float     PALETTE_SCALE = 2.0f;
global Rectangle PALETTE_SRC;
global Rectangle PALETTE_DST;

global RenderTexture2D LEVEL_TARGET;
global float           LEVEL_TARGET_SCALE = 1.0f;
global float		   LEVEL_TARGET_SCALE_MAX = 10.0f;
global float		   LEVEL_TARGET_SCALE_MIN = 0.25f;
global Rectangle       LEVEL_TARGET_DST;
global int**		   LEVEL_DATA;
global int			   LEVEL_WIDTH    = 25;
global int			   LEVEL_HEIGHT   = 15;
global int			   LEVEL_WIDTH_PX;
global int			   LEVEL_HEIGHT_PX;

void ParseArgs(int argc, char** argv);
void EditorSaveLevel();
void EditorLoadLevel();

void UpdatePalette() {
	PALETTE_SRC = (Rectangle){ 
		0, 0, 
		ATLAS_PAGES[ATLAS_PAGE].width,
		ATLAS_PAGES[ATLAS_PAGE].height,
	};

	PALETTE_DST = (Rectangle){ 
		0, 0, 
		ATLAS_PAGES[ATLAS_PAGE].width * PALETTE_SCALE, 
		ATLAS_PAGES[ATLAS_PAGE].height * PALETTE_SCALE
	};

	SELECTED_TILE.width  = TILE_SIZE;
	SELECTED_TILE.height = TILE_SIZE;

	// NOTE: needs to be updated when pages are swapped
	ATLAS_WIDTH  = (ATLAS_PAGES[ATLAS_PAGE].width / TILE_SIZE);
	ATLAS_HEIGHT = (ATLAS_PAGES[ATLAS_PAGE].height / TILE_SIZE);
}

void EditorStart(int argc, char** argv) {
	ParseArgs(argc, argv);

	InitWindow(1280, 720, "");
	SetTargetFPS(60);

	printf("Loading Resources:\n");
	printf("\tAtlas pages: %d\n", ATLAS_PAGE_COUNT);
	for(int i = 0; i < ATLAS_PAGE_COUNT; i++) {
		ATLAS_PAGES[i] = LoadTexture(ATLAS_PATHS[i]);
		printf("\tLoaded page[%d]: %s\n", i, ATLAS_PATHS[i]);
	}

	printf("\tLevel dimensions: [%d %d]\n", LEVEL_WIDTH, LEVEL_HEIGHT);
	printf("\tTile size: %d\n", TILE_SIZE);
	printf("\tPalette scale: %.2f\n", PALETTE_SCALE);
	printf("\tSaving level as: %s\n", OUTPUT_FILENAME);

	// tmp
	UpdatePalette();

	if(EDITOR_LOAD_LEVEL) {
		EditorLoadLevel();
	} else {
		// Create level data
		LEVEL_DATA = (int**)malloc(sizeof(int*) * LEVEL_WIDTH);
		for(int i = 0; i < LEVEL_WIDTH; i++) {
			LEVEL_DATA[i] = (int*)malloc(sizeof(int) * LEVEL_HEIGHT);
			memset(LEVEL_DATA[i], 1, LEVEL_HEIGHT * sizeof(int));
		}
	}

	LEVEL_WIDTH_PX  = TILE_SIZE * LEVEL_WIDTH;
	LEVEL_HEIGHT_PX = TILE_SIZE * LEVEL_HEIGHT;
	LEVEL_TARGET_DST = (Rectangle) {
		(WINDOW_WIDTH / 2) - (LEVEL_WIDTH_PX / 2), 
		(WINDOW_HEIGHT / 2) - (LEVEL_HEIGHT_PX / 2), 
		LEVEL_WIDTH_PX, LEVEL_HEIGHT_PX
	};

	LEVEL_TARGET = LoadRenderTexture(
		TILE_SIZE * LEVEL_WIDTH,
		TILE_SIZE * LEVEL_HEIGHT
	);
}

size_t str_len(const char* str) {
	char c;
	int i = 0;
	size_t len = 0;

	while((c = str[i]) != 0) {
		len++;
		i++;
	}

	return len;
}

int str_compare(const char* a, const char* b) {
	size_t a_len = str_len(a);
	size_t b_len = str_len(b);
	if(a_len != b_len) {
		return 0;
	}

	int i = 0;
	while(i < a_len) {
		if(a[i] != b[i])
			return 0;
		i++;
	}

	return 1;
}

void str_copy(char* src, char* dst, size_t n) {
	for(size_t i = 0; i < n; i++) {
		dst[i] = src[i];
	}
}

int is_digit(char c) {
	if((c >= '0') && (c <= '9')) {
		return 1;
	}
	return 0;
}

int parse_int(const char* str) {
	int res = 0;
	size_t len = str_len(str);
	int negative = 1;

	// TODO: cleaner way of doing this?
	if(str[0] == '-' && len < 2) {
		printf("parse_int failed on provided string: %s\n", str);
		exit(-1);
	} else if(str[0] == '-') {
		negative = -1;
	}

	int place = 1;
	for(int i = len-1; i >= 0; i--) {
		if((i == 0) && (str[i] == '-')) {
			break;
		} else if(!is_digit(str[i])) {
			printf("parse_int failed on provided string: %s\n", str);
			exit(-1);
		}
		res += (int)(str[i] - 48) * place;
		place *= 10;
	}
	return res * negative;
}

int is_valid_path(char c) {
	if(((c >= '0') && (c <= '9')) ||
		((c >= 'a') && (c <= 'z')) ||
		((c >= 'A') && (c <= 'Z')) ||
		(c == '-') || (c == '/') || 
		(c == '_') || (c == '.') || 
		(c == ','))
	{
		return 1;
	}
	return 0;
}

const char* USAGE_STR = 
"Usage: lvl-edit [OPTION] [VALUE]...\n\
Options:\n\
\t--atlas, -a [VALUE],[VALUE]...\n\
\t--window-width [VALUE]\n\
\t--window-height [VALUE]\n\
\t--level-width [VALUE]\n\
\t--level-height [VALUE]\n\
\t--tile-width [VALUE]\n\
\t--tile-height [VALUE]\n\
\t--save-as [VALUE]\n";


void ParseArgs(int argc, char** argv) {
	SetTraceLogLevel(LOG_ERROR);
	if(argc < 2) {
		printf("%s", USAGE_STR);
		exit(-1);
	}

	size_t path_index = 0;
	for(int i = 1; i < argc; i += 2) {
		if(i+1 == argc) {
			printf("Option provided without value.\n");
			exit(-1);
		}

		if(str_compare("--atlas", argv[i]) || str_compare("-a", argv[i])) {
			path_index = (size_t)i+1;
			continue;
		} else if(str_compare("--window-width", argv[i]) || 
			str_compare("-ww", argv[i])) {
			WINDOW_WIDTH = parse_int(argv[i+1]);
		} else if(str_compare("--window-height", argv[i]) || 
			str_compare("-wh", argv[i])) {
			 WINDOW_HEIGHT = parse_int(argv[i+1]);
		} else if(str_compare("--level-width", argv[i]) ||
			str_compare("-lw", argv[i])) {
			LEVEL_WIDTH = parse_int(argv[i+1]);
		} else if(str_compare("--level-height", argv[i]) || 
			str_compare("-lh", argv[i])) {
			LEVEL_HEIGHT = parse_int(argv[i+1]);
		} else if(str_compare("--tile-size", argv[i]) || 
			str_compare("-ts", argv[i])) {
			TILE_SIZE = parse_int(argv[i+1]);
		}
		else if(str_compare("--palette-scale", argv[i]) ||
			str_compare("-ps", argv[i])) {
			PALETTE_SCALE = parse_int(argv[i+1]);
		}
		/*
		else if(str_compare("--palette-width", argv[i]) ||
			str_compare("-pw", argv[i])) {
			//palette->width = parse_int(argv[i+1]);
		}
		else if(str_compare("--palette-height", argv[i]) ||
			str_compare("-ph", argv[i])) {
			//palette->height = parse_int(argv[i+1]);
		}
		*/
		else if(str_compare("--level-width", argv[i]) ||
			str_compare("--lw", argv[i])) {
			LEVEL_WIDTH = parse_int(argv[i+1]);
		}
		else if(str_compare("--level-height", argv[i]) ||
			str_compare("--lh", argv[i])) {
			LEVEL_HEIGHT = parse_int(argv[i+1]);
		}
		else if(str_compare("--load", argv[i]) ||
			str_compare("-l", argv[i])) {
			memset(INPUT_FILENAME, 0, IO_FILENAME_MAX);
			str_copy(argv[i+1], INPUT_FILENAME, str_len(argv[i+1]));
			EDITOR_LOAD_LEVEL = 1;
		}
		else if(str_compare("--save-as", argv[i]) ||
			str_compare("-o", argv[i])) {
			memset(OUTPUT_FILENAME, 0, IO_FILENAME_MAX);
			str_copy(argv[i+1], OUTPUT_FILENAME, str_len(argv[i+1]));
		}

		else {
			printf("Invalid option provided: '%s'.\n", argv[i]);
			exit(-1);
		}
	}
	
	//char paths[PALETTE_MAX][PALETTE_PATH_MAX+1];
	for(int i = 0; i < PALETTE_MAX; i++) {
		memset(ATLAS_PATHS[i], 0, PALETTE_PATH_MAX+1);
	}

	char* path = argv[path_index];
	size_t page = 0;
	size_t start = 0;
	size_t path_len = str_len(path);
	for(int i = 0; i < path_len; i++) {
		if(!is_valid_path(path[i])) {
			printf("Invalid path provided: %s.\n", path);
			exit(-1);
		}
		if((path[i] == ',') && ((i - start) <= PALETTE_PATH_MAX)) {
			str_copy(path+start, ATLAS_PATHS[page], i - start);
			start = i + 1;
			page++;
		} else if(i+1 == path_len) {
			str_copy(path+start, ATLAS_PATHS[page], i - start + 1);
			page++;
		}
	}

	ATLAS_PAGE_COUNT = page;
}

int IsCursorInCanvas(int mx, int my) {
	if( 
		( // X
		 	(mx >= LEVEL_TARGET_DST.x) && 
			(mx <= LEVEL_TARGET_DST.x + 
				(LEVEL_TARGET_DST.width * LEVEL_TARGET_SCALE))
		) &&
		( // Y
			(my >= LEVEL_TARGET_DST.y) &&
			(my <= LEVEL_TARGET_DST.y + 
				(LEVEL_TARGET_DST.height * LEVEL_TARGET_SCALE))
		)
	  ) {
		return 1;
	}
	return 0;
}

void EditorKeyboardUpdate() {
	if(IsKeyPressed(KEY_S) && IsKeyDown(KEY_LEFT_CONTROL)) {
		printf("Level saved to %s\n", OUTPUT_FILENAME);
		EditorSaveLevel();
	}
}

void EditorMouseUpdate() {
	int mx = GetMouseX();
	int my = GetMouseY();

	Vector2 mouse_delta = GetMouseDelta();

	if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) { // Canvas Controls
		LEVEL_TARGET_DST.x += mouse_delta.x;
		LEVEL_TARGET_DST.y += mouse_delta.y;
		if(LEVEL_TARGET_DST.x < 0) {
			LEVEL_TARGET_DST.x = 0;
		} else if(
			(LEVEL_TARGET_DST.x + 
				(LEVEL_WIDTH_PX * LEVEL_TARGET_SCALE)) > WINDOW_WIDTH) 
		{
			LEVEL_TARGET_DST.x = WINDOW_WIDTH - 
				(LEVEL_WIDTH_PX * LEVEL_TARGET_SCALE);
		}

		if(LEVEL_TARGET_DST.y < 0) {
			LEVEL_TARGET_DST.y = 0;
		} else if(
			(LEVEL_TARGET_DST.y + 
				(LEVEL_HEIGHT_PX * LEVEL_TARGET_SCALE)) > WINDOW_HEIGHT) 
		{
			LEVEL_TARGET_DST.y = WINDOW_HEIGHT - 
				(LEVEL_HEIGHT_PX * LEVEL_TARGET_SCALE);
		}
	}

	//if(IsCursorInCanvas(mx, my)) {
	LEVEL_TARGET_SCALE += 0.25 * GetMouseWheelMove();
	if(LEVEL_TARGET_SCALE > LEVEL_TARGET_SCALE_MAX) {
		LEVEL_TARGET_SCALE = LEVEL_TARGET_SCALE_MAX;
	}
	if(LEVEL_TARGET_SCALE < LEVEL_TARGET_SCALE_MIN) {
		LEVEL_TARGET_SCALE = LEVEL_TARGET_SCALE_MIN;
	}
	//}

	if(mx < (PALETTE_DST.width) && my < (PALETTE_DST.height)) {
		if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
			SELECTED_TILE.x = (float)TILE_SIZE * 
				floorf( (float)(mx / PALETTE_SCALE) / (float)TILE_SIZE );
			SELECTED_TILE.y = (float)TILE_SIZE * 
				floorf( (float)(my / PALETTE_SCALE) / (float)TILE_SIZE);
			SELECTED_TILE_INDEX = 
				(SELECTED_TILE.y * 
					ATLAS_WIDTH + SELECTED_TILE.x) / TILE_SIZE;
			//int x = TILE_SIZE * (SELECTED_TILE_INDEX % ATLAS_WIDTH);
			//int y = TILE_SIZE * (SELECTED_TILE_INDEX / ATLAS_WIDTH);
			// y*w+x ser
			// (i % width, i / width) des
		}
	}

	if(IsCursorInCanvas(mx, my)) {

		int x = 
		(int)((mx - LEVEL_TARGET_DST.x) / (TILE_SIZE * LEVEL_TARGET_SCALE));
		int y = 
		(int)((my - LEVEL_TARGET_DST.y) / (TILE_SIZE * LEVEL_TARGET_SCALE));
		
		// do nothing if out of bounds for level
		if((x >= LEVEL_WIDTH || x < 0) || 
			(y >= LEVEL_HEIGHT || y < 0)) {
			return;
		}

		if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			if(IsKeyDown(KEY_LEFT_ALT)) {
				SELECTED_TILE_INDEX = LEVEL_DATA[x][y];
			} else {
				LEVEL_DATA[x][y] = SELECTED_TILE_INDEX;
			}
		} 
		if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
			LEVEL_DATA[x][y] = TILE_VOID;
		} 
	}
}

Rectangle CanvasIndexToRectangle(int i) {
	return (Rectangle) {
		(i % ATLAS_WIDTH) * TILE_SIZE,
		(i / ATLAS_WIDTH) * TILE_SIZE,
		TILE_SIZE, TILE_SIZE
	};
}

void CanvasRender() {
	BeginTextureMode(LEVEL_TARGET);
	ClearBackground(BLACK);

	for(int x = 0; x < LEVEL_WIDTH; x++) {
		for(int y = 0; y < LEVEL_HEIGHT; y++) {
			if(LEVEL_DATA[x][y] == TILE_VOID) {
				continue;
			}
			DrawTexturePro(
				ATLAS_PAGES[ATLAS_PAGE],
				CanvasIndexToRectangle(LEVEL_DATA[x][y]),
				(Rectangle) {
					x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE
				},
				(Vector2){ 0.0f, 0.0f },
				0.0f, WHITE
			);
		}
	}

	EndTextureMode();
}

void CanvasDisplay() {
	DrawTexturePro(LEVEL_TARGET.texture, 
		(Rectangle) { 0.0f, 0.0f, (float)LEVEL_TARGET.texture.width, (float)-LEVEL_TARGET.texture.height },
		//(Rectangle){ 0.0f, 0.0f, (float)Editor.width, (float)Editor.height },
		(Rectangle) {
			LEVEL_TARGET_DST.x, LEVEL_TARGET_DST.y, 
			LEVEL_TARGET_DST.width * LEVEL_TARGET_SCALE,
			LEVEL_TARGET_DST.height * LEVEL_TARGET_SCALE,
		},
		(Vector2) { 0.0f, 0.0f }, 0.0f, WHITE
	);
}

void EditorSaveLevel() {
	FILE* fp = fopen(OUTPUT_FILENAME, "w");

	// Write header
	// (BLISS)(CXCY)(TS)(DATA)

	fwrite(BLISS_HEADER, BLISS_HEADER_LEN, 1, fp);
	fwrite(&LEVEL_WIDTH, sizeof(int), 1, fp);
	fwrite(&LEVEL_HEIGHT, sizeof(int), 1, fp);
	fwrite(&TILE_SIZE, sizeof(int), 1, fp);

	for(int i = 0; i < LEVEL_WIDTH; i++) {
		fwrite(LEVEL_DATA[i], sizeof(int) * LEVEL_HEIGHT, 1, fp);
	}

	fclose(fp);
}

void EditorLoadLevel() {

	FILE* fp = fopen(INPUT_FILENAME, "r");
	
	// Verify header
	global char header[BLISS_HEADER_LEN];
	fread(header, BLISS_HEADER_LEN, 1, fp);
	if(0 != strncmp(BLISS_HEADER, header, BLISS_HEADER_LEN)) {
		fprintf(stderr, "Invalid header.\n");
		exit(-1);
	}

	fread(&LEVEL_WIDTH, sizeof(int), 1, fp);
	fread(&LEVEL_HEIGHT, sizeof(int), 1, fp);
	fread(&TILE_SIZE, sizeof(int), 1, fp);

	// Set up level data
	LEVEL_DATA = (int**)malloc(sizeof(int*) * LEVEL_WIDTH);
	for(int x = 0; x < LEVEL_WIDTH; x++) {
		LEVEL_DATA[x] = (int*)malloc(sizeof(int) * LEVEL_HEIGHT);
		memset(LEVEL_DATA[x], 0, LEVEL_HEIGHT * sizeof(int));
	}

	for(int i = 0; i < LEVEL_WIDTH; i++) {
		fread(LEVEL_DATA[i], sizeof(int) * LEVEL_HEIGHT, 1, fp);
	}

	fclose(fp);
}
