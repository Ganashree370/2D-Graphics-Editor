#include <stdio.h>
#include <math.h>

#define ROWS 30
#define COLS 60

char canvas[ROWS][COLS];

// Initialize canvas
void clearCanvas() {
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            canvas[i][j] = '_';
        }
    }
}

// Display canvas
void displayCanvas() {
    for(int i = 0; i < ROWS; i++) {
        for(int j = 0; j < COLS; j++) {
            printf("%c", canvas[i][j]);
        }
        printf("\n");
    }
}

// Draw a point
void drawPoint(int x, int y, char ch) {
    if(x >= 0 && x < ROWS && y >= 0 && y < COLS)
        canvas[x][y] = ch;
}

// Draw rectangle
void drawRectangle(int x, int y, int width, int height) {
    for(int i = x; i < x + height; i++) {
        for(int j = y; j < y + width; j++) {
            drawPoint(i, j, '*');
        }
    }
}

// Draw line
void drawLine(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    float xInc = dx / (float)steps;
    float yInc = dy / (float)steps;

    float x = x1;
    float y = y1;

    for(int i = 0; i <= steps; i++) {
        drawPoint((int)round(x), (int)round(y), '*');
        x += xInc;
        y += yInc;
    }
}

// Draw triangle
void drawTriangle(int x, int y, int height) {
    for(int i = 0; i < height; i++) {
        for(int j = y - i; j <= y + i; j++) {
            drawPoint(x + i, j, '*');
        }
    }
}

// Draw circle
void drawCircle(int cx, int cy, int r) {
    for(int x = 0; x < ROWS; x++) {
        for(int y = 0; y < COLS; y++) {
            int dist = (x - cx)*(x - cx) + (y - cy)*(y - cy);
            if(abs(dist - r*r) <= r)
                drawPoint(x, y, '*');
        }
    }
}

// Delete rectangle area
void deleteArea(int x, int y, int width, int height) {
    for(int i = x; i < x + height && i < ROWS; i++) {
        for(int j = y; j < y + width && j < COLS; j++) {
            canvas[i][j] = '_';
        }
    }
}

int main() {
    int choice;

    clearCanvas();

    while(1) {
        printf("\n--- 2D Graphics Editor ---\n");
        printf("1. Add Rectangle\n");
        printf("2. Add Line\n");
        printf("3. Add Triangle\n");
        printf("4. Add Circle\n");
        printf("5. Delete Area\n");
        printf("6. Display Picture\n");
        printf("7. Clear Picture\n");
        printf("8. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        if(choice == 1) {
            int x,y,w,h;
            printf("Enter x y width height: ");
            scanf("%d%d%d%d",&x,&y,&w,&h);
            drawRectangle(x,y,w,h);
            displayCanvas();
        }
        else if(choice == 2) {
            int x1,y1,x2,y2;
            printf("Enter x1 y1 x2 y2: ");
            scanf("%d%d%d%d",&x1,&y1,&x2,&y2);
            drawLine(x1,y1,x2,y2);
            displayCanvas();
        }
        else if(choice == 3) {
            int x,y,h;
            printf("Enter x y height: ");
            scanf("%d%d%d",&x,&y,&h);
            drawTriangle(x,y,h);
            displayCanvas();
        }
        else if(choice == 4) {
            int cx,cy,r;
            printf("Enter center_x center_y radius: ");
            scanf("%d%d%d",&cx,&cy,&r);
            drawCircle(cx,cy,r);
            displayCanvas();
        }
        else if(choice == 5) {
            int x,y,w,h;
            printf("Enter x y width height to delete: ");
            scanf("%d%d%d%d",&x,&y,&w,&h);
            deleteArea(x,y,w,h);
            displayCanvas();
        }
        else if(choice == 6) {
            displayCanvas();
        }
        else if(choice == 7) {
            clearCanvas();
            printf("Picture cleared.\n");
        }
        else if(choice == 8) {
            break;
        }
        else {
            printf("Invalid choice!\n");
        }
    }

    return 0;
}