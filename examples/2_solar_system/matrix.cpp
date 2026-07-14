#include <iostream>
#include "matrix.h"
#include <math.h>

// to deal with ordering of functions, all functions not in matrix.h are declared here
void printMatrix(float mat[4][4]);
void printVertices(std::vector<Vertex> vts);
//

void printVertices(std::vector<Vertex> vts){
    std::cout << "------------" << '\n';
    for(Vertex v : vts){
        std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")" << '\n'; 
    }
}

// just a 4 x 4 rotation matrix
// positive angle for CCW, negative angle for CCWi
Vertex rot(Vertex v, int angle, int axis){
  float rad = (angle % 360) * PI / 180;

  // rotate around y-axis
  switch(axis){
    case X_AXIS:
       {
      float mat[4][4] {
        {1,             0,                0, 0},
        {0, std::cos(rad), -1*std::sin(rad), 0},
        {0, std::sin(rad),    std::cos(rad), 0},
        {0,           0,                  0, 1}
      };
      return mul(v, mat);
        break;
       }
    case Y_AXIS:
       {
      float mat[4][4] {
        {   std::cos(rad), 0, std::sin(rad), 0},
        {               0, 1,             0, 0},
        {-1*std::sin(rad), 0, std::cos(rad), 0},
        {               0, 0,             0, 1}
      }; 
      return mul(v, mat);
        break;
       }
    case Z_AXIS:
      {
      float mat[4][4] {
        {std::cos(rad), -1*std::sin(rad), 0,0},
        {std::sin(rad),    std::cos(rad), 0,0},
        {            0,                0, 1,0},
        {            0,                0, 0,1}
      }; 
      return mul(v, mat);
      break;
      }
    default: {
        std:: cout << "INVALID AXIS" << '\n';
        return v;
     }
  }
  return v;
}

//
Vertex translate(Vertex v, Vertex offsets){
    // create a translation matrix from the offsets
    float mat[4][4] {
        {1,   0,   0,    offsets.x},
        {0,   1,   0,    offsets.y},
        {0,   0,   1,    offsets.z},
        {0,   0,   0,            1}
    };
   Vertex v_t = mul(v, mat);
   // 
   return v_t; 
}

//
void printMatrix(float mat[4][4]){
    std::cout << '\n';
    std::cout << "--------------" << '\n';
    for(int i = 0; i < 4; i++){
        for(int j = 0; j < 4; j++){
            std::cout << mat[i][j] << " ";
        }
        std::cout << '\n';
    }
}

//
Vertex mul(Vertex v, float mat[4][4]){
    // 
    Vertex v_t = {.x = v.x, .y = v.y, .z = v.z, .w = v.w};
    v_t.x = v.x * mat[0][0] + v.y * mat[0][1] + v.z * mat[0][2] + v.w * mat[0][3];   
    v_t.y = v.x * mat[1][0] + v.y * mat[1][1] + v.z * mat[1][2] + v.w * mat[1][3];   
    v_t.z = v.x * mat[2][0] + v.y * mat[2][1] + v.z * mat[2][2] + v.w * mat[2][3];   
    v_t.w = v.x * mat[3][0] + v.y * mat[3][1] + v.z * mat[3][2] + v.w * mat[3][3];   
    //
    return v_t;
}
