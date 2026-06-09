#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define ROWS 30
#define COLS 60
#define MAX_SHAPES 100
// ----------------------------------------------------
// Structure Definitions
// ----------------------------------------------------
typedef enum {
    SHAPE_LINE = 1,
    SHAPE_RECTANGLE,
    SHAPE_CIRCLE,
    SHAPE_TRIANGLE
} ShapeType;
typedef struct {
    int x1, y1, x2, y2;
} LineParams;
typedef struct {
    int x, y, w, h;
    int fill;
} RectParams;
typedef struct {
    int cx, cy, r;
    int fill;
} CircleParams;
typedef struct {
    int x1, y1, x2, y2, x3, y3;
    int fill;
} TriangleParams;
typedef struct {
    int id; // Unique identifier for operations
    ShapeType type;
    union {
        LineParams line;
        RectParams rect;
        CircleParams circle;
        TriangleParams tri;
    } params;
} Shape;
// Global State
Shape shapes[MAX_SHAPES];
int num_shapes = 0;
int next_id = 1;
char grid[ROWS][COLS];
// ----------------------------------------------------
// Input Validation Helper Functions
// ----------------------------------------------------
/**
 * Robustly reads an integer from stdin, preventing infinite loops on invalid input.
 */
int readInt(const char* prompt, int min_val, int max_val) {
    int val;
    char buffer[128];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            continue;
        }
        // Remove trailing newline character
        buffer[strcspn(buffer, "\n")] = 0;
        
        char* endptr;
        long parsed = strtol(buffer, &endptr, 10);
        if (endptr == buffer || *endptr != '\0') {
            printf("Error: Invalid number format. Please enter an integer.\n");
            continue;
        }
        if (parsed < min_val || parsed > max_val) {
            printf("Error: Value out of range [%d, %d]. Please try again.\n", min_val, max_val);
            continue;
        }
        val = (int)parsed;
        break;
    }
    return val;
}
// ----------------------------------------------------
// Rasterization Drawing Code
// ----------------------------------------------------
void clearGrid(char g[ROWS][COLS]) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            g[r][c] = '_';
        }
    }
}
void setPixel(char g[ROWS][COLS], int x, int y) {
    if (x >= 0 && x < COLS && y >= 0 && y < ROWS) {
        g[y][x] = '*';
    }
}
void drawLine(char g[ROWS][COLS], int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    while (1) {
        setPixel(g, x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}
void drawRectangle(char g[ROWS][COLS], int x, int y, int w, int h, int fill) {
    if (fill) {
        for (int cy = y; cy < y + h; cy++) {
            for (int cx = x; cx < x + w; cx++) {
                setPixel(g, cx, cy);
            }
        }
    } else {
        int x2 = x + w - 1;
        int y2 = y + h - 1;
        // Top and Bottom sides
        for (int cx = x; cx <= x2; cx++) {
            setPixel(g, cx, y);
            setPixel(g, cx, y2);
        }
        // Left and Right sides
        for (int cy = y; cy <= y2; cy++) {
            setPixel(g, x, cy);
            setPixel(g, x2, cy);
        }
    }
}
void plotCirclePoints(char g[ROWS][COLS], int cx, int cy, int x, int y) {
    setPixel(g, cx + x, cy + y);
    setPixel(g, cx - x, cy + y);
    setPixel(g, cx + x, cy - y);
    setPixel(g, cx - x, cy - y);
    setPixel(g, cx + y, cy + x);
    setPixel(g, cx - y, cy + x);
    setPixel(g, cx + y, cy - x);
    setPixel(g, cx - y, cy - x);
}
void drawCircle(char g[ROWS][COLS], int cx, int cy, int r, int fill) {
    if (fill) {
        int minY = cy - r;
        int maxY = cy + r;
        for (int y = minY; y <= maxY; y++) {
            int dy = y - cy;
            double term = (double)(r * r - dy * dy);
            if (term >= 0) {
                int dx = (int)round(sqrt(term));
                for (int x = cx - dx; x <= cx + dx; x++) {
                    setPixel(g, x, y);
                }
            }
        }
    } else {
        int x = 0;
        int y = r;
        int d = 3 - 2 * r;
        
        plotCirclePoints(g, cx, cy, x, y);
        while (y >= x) {
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
            plotCirclePoints(g, cx, cy, x, y);
        }
    }
}
int isPointInTriangle(int px, int py, int x1, int y1, int x2, int y2, int x3, int y3) {
    int d1 = (px - x1) * (y2 - y1) - (py - y1) * (x2 - x1);
    int d2 = (px - x2) * (y3 - y2) - (py - y2) * (x3 - x2);
    int d3 = (px - x3) * (y1 - y3) - (py - y3) * (x1 - x3);
    int has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    int has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(has_neg && has_pos);
}
void drawTriangle(char g[ROWS][COLS], int x1, int y1, int x2, int y2, int x3, int y3, int fill) {
    if (fill) {
        // Find bounding box
        int minX = x1;
        if (x2 < minX) minX = x2;
        if (x3 < minX) minX = x3;
        
        int maxX = x1;
        if (x2 > maxX) maxX = x2;
        if (x3 > maxX) maxX = x3;
        
        int minY = y1;
        if (y2 < minY) minY = y2;
        if (y3 < minY) minY = y3;
        
        int maxY = y1;
        if (y2 > maxY) maxY = y2;
        if (y3 > maxY) maxY = y3;
        
        // Clamp bounds to screen boundaries
        if (minX < 0) minX = 0;
        if (maxX >= COLS) maxX = COLS - 1;
        if (minY < 0) minY = 0;
        if (maxY >= ROWS) maxY = ROWS - 1;
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                if (isPointInTriangle(x, y, x1, y1, x2, y2, x3, y3)) {
                    setPixel(g, x, y);
                }
            }
        }
    } else {
        drawLine(g, x1, y1, x2, y2);
        drawLine(g, x2, y2, x3, y3);
        drawLine(g, x3, y3, x1, y1);
    }
}
// ----------------------------------------------------
// Core Pipeline Methods
// ----------------------------------------------------
/**
 * Regenerates the 2D character grid by clear-loading and redrawing all active shapes
 */
void renderCanvas() {
    clearGrid(grid);
    for (int i = 0; i < num_shapes; i++) {
        Shape s = shapes[i];
        switch (s.type) {
            case SHAPE_LINE:
                drawLine(grid, s.params.line.x1, s.params.line.y1, s.params.line.x2, s.params.line.y2);
                break;
            case SHAPE_RECTANGLE:
                drawRectangle(grid, s.params.rect.x, s.params.rect.y, s.params.rect.w, s.params.rect.h, s.params.rect.fill);
                break;
            case SHAPE_CIRCLE:
                drawCircle(grid, s.params.circle.cx, s.params.circle.cy, s.params.circle.r, s.params.circle.fill);
                break;
            case SHAPE_TRIANGLE:
                drawTriangle(grid, s.params.tri.x1, s.params.tri.y1, s.params.tri.x2, s.params.tri.y2, s.params.tri.x3, s.params.tri.y3, s.params.tri.fill);
                break;
        }
    }
}
/**
 * Displays the 2D grid matrix with clean grid coordinates overlays
 */
void displayPicture(char g[ROWS][COLS]) {
    printf("\n=== RASTER SCREEN OUTPUT (Grid: %dx%d) ===\n", COLS, ROWS);
    
    // Print column coordinates headers (tens)
    printf("   ");
    for (int c = 0; c < COLS; c++) {
        printf("%d", (c / 10) % 10);
    }
    printf("\n");
    // Print column coordinates headers (units)
    printf("   ");
    for (int c = 0; c < COLS; c++) {
        printf("%d", c % 10);
    }
    printf("\n");
    // Print grid contents row by row, prefixed with row indices
    for (int r = 0; r < ROWS; r++) {
        printf("%02d ", r);
        for (int c = 0; c < COLS; c++) {
            putchar(g[r][c]);
        }
        printf("\n");
    }
    printf("==========================================\n\n");
}
/**
 * Prints description summary for a vector shape
 */
void printShapeDetail(Shape s, int listIndex) {
    const char* type_str = "Unknown";
    if (s.type == SHAPE_LINE) type_str = "Line";
    else if (s.type == SHAPE_RECTANGLE) type_str = "Rectangle";
    else if (s.type == SHAPE_CIRCLE) type_str = "Circle";
    else if (s.type == SHAPE_TRIANGLE) type_str = "Triangle";
    printf("[%d] ID: %d | Type: %s | ", listIndex, s.id, type_str);
    switch (s.type) {
        case SHAPE_LINE:
            printf("Points: (%d,%d) to (%d,%d)\n", s.params.line.x1, s.params.line.y1, s.params.line.x2, s.params.line.y2);
            break;
        case SHAPE_RECTANGLE:
            printf("Pos: (%d,%d), Width: %d, Height: %d, Filled: %s\n", s.params.rect.x, s.params.rect.y, s.params.rect.w, s.params.rect.h, s.params.rect.fill ? "Yes" : "No");
            break;
        case SHAPE_CIRCLE:
            printf("Center: (%d,%d), Radius: %d, Filled: %s\n", s.params.circle.cx, s.params.circle.cy, s.params.circle.r, s.params.circle.fill ? "Yes" : "No");
            break;
        case SHAPE_TRIANGLE:
            printf("P1: (%d,%d), P2: (%d,%d), P3: (%d,%d), Filled: %s\n", s.params.tri.x1, s.params.tri.y1, s.params.tri.x2, s.params.tri.y2, s.params.tri.x3, s.params.tri.y3, s.params.tri.fill ? "Yes" : "No");
            break;
    }
}
// ----------------------------------------------------
// UI Actions Handlers
// ----------------------------------------------------
void addShapeMenu() {
    if (num_shapes >= MAX_SHAPES) {
        printf("Error: Maximum shape capacity reached (%d shapes).\n", MAX_SHAPES);
        return;
    }
    printf("\n--- Add a New Shape ---\n");
    printf("1. Line\n");
    printf("2. Rectangle\n");
    printf("3. Circle\n");
    printf("4. Triangle\n");
    int choice = readInt("Select shape type (1-4): ", 1, 4);
    Shape s;
    s.id = next_id++;
    s.type = (ShapeType)choice;
    switch (s.type) {
        case SHAPE_LINE:
            s.params.line.x1 = readInt("Enter X1 (0-59): ", 0, COLS - 1);
            s.params.line.y1 = readInt("Enter Y1 (0-29): ", 0, ROWS - 1);
            s.params.line.x2 = readInt("Enter X2 (0-59): ", 0, COLS - 1);
            s.params.line.y2 = readInt("Enter Y2 (0-29): ", 0, ROWS - 1);
            break;
        case SHAPE_RECTANGLE:
            s.params.rect.x = readInt("Enter top-left X (0-59): ", 0, COLS - 1);
            s.params.rect.y = readInt("Enter top-left Y (0-29): ", 0, ROWS - 1);
            s.params.rect.w = readInt("Enter width (1-60): ", 1, COLS);
            s.params.rect.h = readInt("Enter height (1-30): ", 1, ROWS);
            s.params.rect.fill = readInt("Fill shape? (0=No, 1=Yes): ", 0, 1);
            break;
        case SHAPE_CIRCLE:
            s.params.circle.cx = readInt("Enter center X (0-59): ", 0, COLS - 1);
            s.params.circle.cy = readInt("Enter center Y (0-29): ", 0, ROWS - 1);
            s.params.circle.r = readInt("Enter radius (1-30): ", 1, 30);
            s.params.circle.fill = readInt("Fill shape? (0=No, 1=Yes): ", 0, 1);
            break;
        case SHAPE_TRIANGLE:
            s.params.tri.x1 = readInt("Enter X1 (0-59): ", 0, COLS - 1);
            s.params.tri.y1 = readInt("Enter Y1 (0-29): ", 0, ROWS - 1);
            s.params.tri.x2 = readInt("Enter X2 (0-59): ", 0, COLS - 1);
            s.params.tri.y2 = readInt("Enter Y2 (0-29): ", 0, ROWS - 1);
            s.params.tri.x3 = readInt("Enter X3 (0-59): ", 0, COLS - 1);
            s.params.tri.y3 = readInt("Enter Y3 (0-29): ", 0, ROWS - 1);
            s.params.tri.fill = readInt("Fill shape? (0=No, 1=Yes): ", 0, 1);
            break;
    }
    shapes[num_shapes++] = s;
    printf("Shape added successfully! (Assigned Vector ID: %d)\n", s.id);
}
void listShapes() {
    printf("\n--- Active Vector Layers (%d) ---\n", num_shapes);
    if (num_shapes == 0) {
        printf("No shapes exist on canvas.\n");
        return;
    }
    for (int i = 0; i < num_shapes; i++) {
        printShapeDetail(shapes[i], i + 1);
    }
}
void modifyShapeMenu() {
    if (num_shapes == 0) {
        printf("\nError: No shapes exist to modify.\n");
        return;
    }
    listShapes();
    int listIdx = readInt("Enter shape Index to modify (1 to active size): ", 1, num_shapes);
    int idx = listIdx - 1;
    Shape* s = &shapes[idx];
    printf("\n--- Modifying Shape index [%d] (ID: %d) ---\n", listIdx, s->id);
    switch (s->type) {
        case SHAPE_LINE:
            s->params.line.x1 = readInt("Enter new X1 (0-59): ", 0, COLS - 1);
            s->params.line.y1 = readInt("Enter new Y1 (0-29): ", 0, ROWS - 1);
            s->params.line.x2 = readInt("Enter new X2 (0-59): ", 0, COLS - 1);
            s->params.line.y2 = readInt("Enter new Y2 (0-29): ", 0, ROWS - 1);
            break;
        case SHAPE_RECTANGLE:
            s->params.rect.x = readInt("Enter new top-left X (0-59): ", 0, COLS - 1);
            s->params.rect.y = readInt("Enter new top-left Y (0-29): ", 0, ROWS - 1);
            s->params.rect.w = readInt("Enter new width (1-60): ", 1, COLS);
            s->params.rect.h = readInt("Enter new height (1-30): ", 1, ROWS);
            s->params.rect.fill = readInt("Fill shape? (0=No, 1=Yes): ", 0, 1);
            break;
        case SHAPE_CIRCLE:
            s->params.circle.cx = readInt("Enter new center X (0-59): ", 0, COLS - 1);
            s->params.circle.cy = readInt("Enter new center Y (0-29): ", 0, ROWS - 1);
            s->params.circle.r = readInt("Enter new radius (1-30): ", 1, 30);
            s->params.circle.fill = readInt("Fill shape? (0=No, 1=Yes): ", 0, 1);
            break;
        case SHAPE_TRIANGLE:
            s->params.tri.x1 = readInt("Enter new X1 (0-59): ", 0, COLS - 1);
            s->params.tri.y1 = readInt("Enter new Y1 (0-29): ", 0, ROWS - 1);
            s->params.tri.x2 = readInt("Enter new X2 (0-59): ", 0, COLS - 1);
            s->params.tri.y2 = readInt("Enter new Y2 (0-29): ", 0, ROWS - 1);
            s->params.tri.x3 = readInt("Enter new X3 (0-59): ", 0, COLS - 1);
            s->params.tri.y3 = readInt("Enter new Y3 (0-29): ", 0, ROWS - 1);
            s->params.tri.fill = readInt("Fill shape? (0=No, 1=Yes): ", 0, 1);
            break;
    }
    printf("Shape index [%d] modified successfully!\n", listIdx);
}
void deleteShapeMenu() {
    if (num_shapes == 0) {
        printf("\nError: No shapes exist to delete.\n");
        return;
    }
    listShapes();
    int listIdx = readInt("Enter shape Index to delete (1 to active size): ", 1, num_shapes);
    int idx = listIdx - 1;
    // Shift shapes to the left
    for (int i = idx; i < num_shapes - 1; i++) {
        shapes[i] = shapes[i + 1];
    }
    num_shapes--;
    printf("Shape index [%d] deleted successfully!\n", listIdx);
}
void loadDemoArt() {
    num_shapes = 0;
    
    // 1. Sun (Circle outline)
    Shape s1;
    s1.id = next_id++;
    s1.type = SHAPE_CIRCLE;
    s1.params.circle.cx = 30;
    s1.params.circle.cy = 11;
    s1.params.circle.r = 6;
    s1.params.circle.fill = 0;
    shapes[num_shapes++] = s1;
    // 2. Far Mountain Left (Triangle outline)
    Shape s2;
    s2.id = next_id++;
    s2.type = SHAPE_TRIANGLE;
    s2.params.tri.x1 = 5;  s2.params.tri.y1 = 22;
    s2.params.tri.x2 = 23; s2.params.tri.y2 = 8;
    s2.params.tri.x3 = 41; s2.params.tri.y3 = 22;
    s2.params.tri.fill = 0;
    shapes[num_shapes++] = s2;
    // 3. Far Mountain Right (Triangle outline)
    Shape s3;
    s3.id = next_id++;
    s3.type = SHAPE_TRIANGLE;
    s3.params.tri.x1 = 25; s3.params.tri.y1 = 22;
    s3.params.tri.x2 = 43; s3.params.tri.y2 = 6;
    s3.params.tri.x3 = 57; s3.params.tri.y3 = 22;
    s3.params.tri.fill = 0;
    shapes[num_shapes++] = s3;
    // 4. Ground (Line)
    Shape s4;
    s4.id = next_id++;
    s4.type = SHAPE_LINE;
    s4.params.line.x1 = 0;  s4.params.line.y1 = 22;
    s4.params.line.x2 = 59; s4.params.line.y2 = 22;
    shapes[num_shapes++] = s4;
    // 5. Cozy cabin base (Rectangle filled)
    Shape s5;
    s5.id = next_id++;
    s5.type = SHAPE_RECTANGLE;
    s5.params.rect.x = 17;
    s5.params.rect.y = 17;
    s5.params.rect.w = 10;
    s5.params.rect.h = 5;
    s5.params.rect.fill = 1;
    shapes[num_shapes++] = s5;
    // 6. Cabin roof (Triangle outline)
    Shape s6;
    s6.id = next_id++;
    s6.type = SHAPE_TRIANGLE;
    s6.params.tri.x1 = 15; s6.params.tri.y1 = 17;
    s6.params.tri.x2 = 22; s6.params.tri.y2 = 13;
    s6.params.tri.x3 = 29; s6.params.tri.y3 = 17;
    s6.params.tri.fill = 0;
    shapes[num_shapes++] = s6;
    // 7. Pond (Circle filled)
    Shape s7;
    s7.id = next_id++;
    s7.type = SHAPE_CIRCLE;
    s7.params.circle.cx = 42;
    s7.params.circle.cy = 26;
    s7.params.circle.r = 3;
    s7.params.circle.fill = 1;
    shapes[num_shapes++] = s7;
    printf("\nPreset Cyberpunk Sunrise demo landscape loaded successfully!\n");
}
// ----------------------------------------------------
// Main Loop
// ----------------------------------------------------
int main() {
    printf("==========================================\n");
    printf("  ASCII VECTOR-RASTER GRAPHICS EDITOR     \n");
    printf("==========================================\n");
    
    // Initial display
    renderCanvas();
    displayPicture(grid);
    while (1) {
        printf("--- Main Editor Menu ---\n");
        printf("1. Add Shape\n");
        printf("2. List Shapes\n");
        printf("3. Modify Shape\n");
        printf("4. Delete Shape\n");
        printf("5. Display Picture\n");
        printf("6. Load Demo Art\n");
        printf("7. Reset Canvas\n");
        printf("8. Exit\n");
        
        int choice = readInt("Select an option (1-8): ", 1, 8);
        
        switch (choice) {
            case 1:
                addShapeMenu();
                renderCanvas();
                displayPicture(grid);
                break;
            case 2:
                listShapes();
                break;
            case 3:
                modifyShapeMenu();
                renderCanvas();
                displayPicture(grid);
                break;
            case 4:
                deleteShapeMenu();
                renderCanvas();
                displayPicture(grid);
                break;
            case 5:
                renderCanvas();
                displayPicture(grid);
                break;
            case 6:
                loadDemoArt();
                renderCanvas();
                displayPicture(grid);
                break;
            case 7:
                num_shapes = 0;
                renderCanvas();
                displayPicture(grid);
                printf("Canvas reset. All vector layers cleared.\n");
                break;
            case 8:
                printf("\nExiting Editor. Goodbye!\n");
                return 0;
        }
    }
    return 0;
}
