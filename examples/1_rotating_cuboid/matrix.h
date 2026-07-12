#include <vector>
#define X_AXIS  0
#define Y_AXIS  1
#define Z_AXIS  2

struct Vertex {
    float x;
    float y;
    float z;

};


void printVertices(std::vector<Vertex> vts); 
Vertex rot(Vertex v, int angle, int axis);
Vertex mul(Vertex v, float mat[3][3]);
