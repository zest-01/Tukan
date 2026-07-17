#include <vector>
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
    float r;
    float g;
    float b;
    float a;
};


void printVertices(std::vector<Vertex> vts); 
Vertex rot(Vertex v, int angle, int axis);
Vertex translate(Vertex v, Vertex offsets);
Vertex mul(Vertex v, float mat[4][4]);
