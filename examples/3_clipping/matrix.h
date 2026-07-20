#include <vector>
#include <math.h>
#define X_AXIS  0
#define Y_AXIS  1
#define Z_AXIS  2
#define PI 3.14159265

struct Vertex {
    // location
    float x;
    float y;
    float z;
    float w;

    // color
    float r; //= 255;
    float g; //= 255;
    float b; //= 255;
    float a;

    // member methods
    Vertex copy();
    void printProperties();
};

struct Color {
    int r; //= 255;
    int g; //= 255;
    int b; //= 255;
};

float norm(Vertex v);
void restoreNorm(float desiredNorm);

void printVertices(std::vector<Vertex> vts); 
Vertex rot(Vertex v, int angle, int axis);
Vertex translate(Vertex v, Vertex offsets);
Vertex mul(Vertex v, float mat[4][4]);
