#include <iostream>
#include "matrix.h"
#include <math.h>
#define PI 3.14159265

void printVertices(std::vector<Vertex> vts){
    std::cout << "------------" << '\n';
    for(Vertex v : vts){
        std::cout << "(" << v.x << ", " << v.y << ", " << v.z << ")" << '\n'; 
    }
}

// just a 3 x 3 rotation matrix
Vertex rot(Vertex v, int angle, int axis){
  float rad = (angle % 360) * PI / 180;
  
  // rotate around y-axis
  switch(axis){
    case X_AXIS:
       {
      float mat[3][3] = {
        {1, 0, 0},
        {0, std::cos(rad), -1*std::sin(rad)},
        {0,std::sin(rad),std::cos(rad)}
      }; 
      return mul(v, mat);
        break;
       }
    case Y_AXIS:
       {
      float mat[3][3] = {
        {std::cos(rad), 0, std::sin(rad)},
        {0, 1, 0},
        {-1*std::sin(rad),0,std::cos(rad)}
      }; 
      return mul(v, mat);
        break;
       }
    case Z_AXIS:
      {
      float mat[3][3] = {
        {std::cos(rad), -1*std::sin(rad), 0},
        {std::sin(rad), std::cos(rad), 0},
        {0,0,1}
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

Vertex mul(Vertex v, float mat[3][3]){
    Vertex v_t = {.x = v.x, .y = v.y, .z = v.z};
    //
    v_t.x = v.x * mat[0][0] + v.y * mat[0][1] + v.z * mat[0][2];   
    v_t.y = v.x * mat[1][0] + v.y * mat[1][1] + v.z * mat[1][2];   
    v_t.z = v.x * mat[2][0] + v.y * mat[2][1] + v.z * mat[2][2];   
    //
    return v_t;
}
