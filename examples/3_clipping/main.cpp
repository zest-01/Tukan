#include <SDL3/SDL_init.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_log.h>
#include "matrix.h"
#include <iostream>
#include <vector>
#include <math.h>
#include <utility>
#include <algorithm>
#include <cmath>
#include <memory>

//
#define WIREFRAME_MODE  0
#define LINE_MODE       1
#define FILL_MODE       2

//
const Vertex CAMERA_POS = Vertex {.x = 0, .y = 0, .z = 0, .w = 1}; // camera placed at origin of world coord system for convenience
const int W = 800;
const int H = 800;
const float FOV = 90; // in degrees
Color COLOR_BUFF[H][W]{};
float Z_BUFF[H][W]{};


std::vector<std::pair<int, int>> bresenham(SDL_Renderer* renderer, Vertex p, Vertex q);
void printColor(int row, int col);

// only works if the elements are printable


bool inScreen(int x, int y){
    return 0 <= x && x < W && 0 <= y && y < H;
}

void reset_Z_BUFF(){
    for(int row = 0 ; row < H ; row++){
        for(int col = 0; col < H ; col++){
            Z_BUFF[row][col] = 255;
        }
    }
}
void reset_COLOR_BUFF(){
    for(int row = 0 ; row < H ; row++){
        for(int col = 0; col < H ; col++){
            COLOR_BUFF[row][col] = Color{};
        }
    }
}

void printVector(std::vector<int> vctr){
    std::cout << "<";
    for(const auto& v : vctr){
        std::cout << v <<  ",";
    }
    std::cout << ">" << '\n';
};


// prints color from color buffer
void printColor(int row, int col){
    std::cout << "(" << row << ", " << col << ") = ("
        << (COLOR_BUFF[row][col]).r << ", "
        << (COLOR_BUFF[row][col]).g << ", "
        << (COLOR_BUFF[row][col]).b << ")"
        << '\n';
}

//green


/*
 * Input -> vertex v in NDC coordinates
 * Output -> a new vertex in screen coordinates
*/
Vertex screen(Vertex v, int W, int H){
  Vertex v_screen = {
      .x = (v.x + 1) * W/2,
      .y = H - ((v.y + 1) * H/2),
      .z = v.z, // preserve z for potential depth testing
      .w = v.w,

      .r = v.r,
      .g = v.g,
      .b = v.b
  };
  return v_screen;    
}

/*
 * Performs perspective projection on coordinates in camera space
 * Input -> vertex v in camera space coordinates
 * Output -> vertex v in clip space coordinates
 * 
 * FOV ranges from ]0, 360[ (degrees)
 *
 * Assumption:
 * - w = 1 (i.e., no perspective division built in yet)
 */
Vertex project(Vertex v, int FOV){
    float alpha = FOV / 2;
    float rad = alpha / 180 * PI;
    Vertex v_pr = {
        .x = v.x / (v.z * std::tan(rad)),
        .y = v.y / (v.z * std::tan(rad)),
        .z = v.z,
        .w = v.w,

        .r = v.r,
        .g = v.g,
        .b = v.b
    };
    return v_pr;
}

/*
 * Input -> vertex v in screen coordinates
 * Output -> a new vertex in NDC coordinates
*/
Vertex toNDC(Vertex v, int W, int H){
  Vertex v_NDC = {
      .x = (v.x / W) * 2 - 1,
      .y = -1 * ((v.y / H) * 2 - 1), 
      .z = v.z,
      .w = v.w,

      .r = v.r,
      .g = v.g,
      .b = v.b
  };
  return v_NDC;    
}


//
//
//
//
//
//
//
//
struct Face {
    public:
        std::vector<Vertex> vts {};
        Face(std::vector<Vertex> vts_in){
            for(auto& v : vts_in){
                vts.push_back(Vertex {.x = v.x, .y = v.y, .z = v.z, .w = v.w, .r = v.r, .g = v.g, .b = v.b});
            }
        };
        
        //
        Face translate_face(Vertex offset){
            std::vector<Vertex> vts_cp{};
            for(int i = 0; i < vts.size() ; i++){
                Vertex v_cp{};
                v_cp.x = vts.at(i).x + offset.x;
                v_cp.y = vts.at(i).y + offset.y;
                v_cp.z = vts.at(i).z + offset.z;
                v_cp.w  = 1;

                v_cp.r = vts.at(i).r;
                v_cp.g = vts.at(i).g;
                v_cp.b = vts.at(i).b;

                vts_cp.push_back(v_cp);
            }
            return Face(vts_cp);
        }
   protected:
        //
   private:
        //
};

struct Geometry {
    //virtual void render(SDL_Renderer* renderer){
    //     std::cout << "ERR: NO RENDER() FUNCTION DEFINED!" << '\n';
    //};
    //
    public:
        std::vector<Face> faces{};
        //
        Geometry(std::vector<Face> faces_in){
        for(auto& f : faces_in)
            {
                faces.push_back(f);
            };
        };
        /*
         * n = 4 -> cube
         *
         */
       Geometry(std::vector<Vertex> vts_in, int n){
            if(vts_in.size() % n != 0) {
                std::cout << "ERR: Amount of supplied vertices must be a multiple of n!" << '\n';
                throw;
            }
            std::vector<Vertex> tmp{};
            for(Vertex v : vts_in){
                tmp.push_back(v);
                if(tmp.size() % n == 0){
                    faces.push_back(Face(tmp));
                    //
                    tmp = {};
                }
            }
        }
        //
        //
        //
        std::vector<Vertex> extractVts(){
            std::vector<Vertex> vts{};
            for(Face f : faces){
                for(Vertex v : f.vts){
                    vts.push_back(v);
                }
            }
            return vts;
        }

        void printProperties(){
            std::vector<Vertex> vts = extractVts();
            std::cout << "------WORLD SPACE COORDS.--------" << '\n';
            for(int i = 0; i < vts.size(); i++){
                 Vertex v = vts.at(i);
                 std::cout << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')';
                 std::cout << '\n';
                 if((i+1) % 4 == 0){
                   std::cout << '\n';
                 }
            }
            std::cout << "--------------------------------" << '\n';
        };

        // Angle in degrees
        void rotate_geom(float angle, int axis){        
                for(int i = 0; i < faces.size() ; i++){
                    for(int j = 0; j < faces.at(0).vts.size(); j++){
                        faces.at(i).vts.at(j) = rot(faces.at(i).vts.at(j), angle, axis);
                    }
                }
       };
      
       //
       //Vertex v_t = translate(v, offset_vector);
       void translate_geom(Vertex offset_vertex){
         for(int i = 0; i < faces.size(); i++){
            //f.translate_face(offset_vertex);
            faces.at(i) = faces.at(i).translate_face(offset_vertex);
         }  
       }


    /*
     * 0 for wireframe -> no depth testing
     * 1 for interpolation of lines, but no fill -> depth testing, but no fill (i.e., black fill)
     * 2 for interpolation of lines, and fill
     */
    void render(SDL_Renderer* renderer, int mode)
    {
      
        
        // Assumes all faces have the same amount of vts associated with them for now
        for(int i = 0 ; i < faces.size(); i++)
        {
            //
            int maxY = -1;
            int minY = H+1;
            const float z_fight_corr = 0; // doesn't do anything
            
            //
            std::vector<std::tuple<int,int,float, Color>> face_coords{}; 
            for(int j = 0 ; j < faces.at(i).vts.size(); j++)
            {
                
                // Pixels on line(v1,v2)
                Vertex v1 = screen(project(faces.at(i).vts.at(j), FOV), W, H);
                Vertex v2 = screen(project(faces.at(i).vts.at((j+1) % faces.at(i).vts.size()), FOV), W, H);
                std::vector<std::pair<int, int>> pixels = bresenham(renderer, v1, v2);

                //  z-value and Color used for buffer updates
                //  minY & maxY used for scanline
                std::vector<std::tuple<int, int, float, Color>> coords{};
                for(const std::pair<int,int>& p : pixels){
                    coords.push_back(std::tuple<int, int, float, Color>{p.first, p.second, 0.0f, Color{}}); // 0 is temporary, to be overwritten 
                    minY = std::min(minY, p.second);
                    maxY = std::max(maxY, p.second); 
                }

                for(float i = 0; i < coords.size(); i++)
                {
                    /*
                     *
                     * [3/3 0/3] [2/3 1/3] [1/3 2/3] [ 0/3 3/3]
                     *
                     */
                    const float line_factor = i / (coords.size() - 1);
                    const auto p = coords.at(i);   
                    


                    if(trunc(v1.x) == std::get<0>(coords.at(0))) // v1 is on the left
                    {
                        const float z = (1-line_factor) * v1.z + line_factor * v2.z; 
                        const Color c = {
                                .r = round((1-line_factor) * v1.r + line_factor * v2.r), 
                                .g = round((1-line_factor) * v1.g + line_factor * v2.g), 
                                .b = round((1-line_factor) * v1.b + line_factor * v2.b) 
                        };
                        std::get<2>(coords.at(i)) = z; 
                        std::get<3>(coords.at(i)) = c; 
                        

                        // Here for if you want wireframe/line mode 
                            
                        if(inScreen(std::get<0>(p), std::get<1>(p))){
                            if(z - z_fight_corr <= Z_BUFF[std::get<0>(coords.at(i))][std::get<1>(coords.at(i))]){
                                Z_BUFF[std::get<0>(coords.at(i))][std::get<1>(coords.at(i))] = z;        
                                COLOR_BUFF[std::get<0>(coords.at(i))][std::get<1>(coords.at(i))] = Color{c.r, c.g, c.b};
                            }
                        }
                    }
                    
                    else if(trunc(v2.x) == std::get<0>(coords.at(0))) // v2 is on the left
                    {
                        
                        const float z = (1-line_factor) * v2.z + line_factor * v1.z; 
                        const Color c = {
                                .r = round((1-line_factor) * v2.r + line_factor * v1.r), 
                                .g = round((1-line_factor) * v2.g + line_factor * v1.g), 
                                .b = round((1-line_factor) * v2.b + line_factor * v1.b) 
                        };
                        std::get<2>(coords.at(i)) = z; 
                        std::get<3>(coords.at(i)) = c; 
                        
                       
                        if(inScreen(std::get<0>(p), std::get<1>(p))){
                            if(z - z_fight_corr <= Z_BUFF[std::get<0>(coords.at(i))][std::get<1>(coords.at(i))]){
                                Z_BUFF[std::get<0>(coords.at(i))][std::get<1>(coords.at(i))] = z;
                                COLOR_BUFF[std::get<0>(coords.at(i))][std::get<1>(coords.at(i))] = Color{c.r, c.g, c.b};
                            }
                        }
                   }

                  // Will be used for filling the face
                  face_coords.push_back(coords.at(i));
                  //std::cout << std::get<0>(coords.at(i)) << '\n';  
                  //std::cout << v1.x << '\n';  
                } 
            }
            //return;

            
            if (mode == FILL_MODE)
            {
               // 1 bucket for each y-coordinate that is crossed by some line of the face
               // buckets contain <x, depth, color> of pixels crossed at that y
               std::vector<std::vector<std::tuple<int, float, Color>>> buckets{}; // <x,z, Color>
               for(int i = 0; i < maxY - minY + 1; i++){
                    buckets.push_back({});
               }

               for(const std::tuple<int, int, float, Color> p : face_coords){
                    buckets.at(std::get<1>(p) - minY).push_back(std::tuple<int, float, Color>{std::get<0>(p), std::get<2>(p), std::get<3>(p)});
               }
               
               

               // Unwanted situations
               if(buckets.size() == 0){
                 std::cout << "RASTERIZATION ERR: no buckets made!\n"; 
               }
               if(buckets.size() == 1){
                 std::cout << "RASTERIZATION INFO: all pixels have the same y!\n"; 
               }
               for(int bi = 0; bi < buckets.size() ; bi++){
                  auto b = buckets.at(bi);
                   if(b.size() == 0)
                  {
                      std::cout << "RASTERIZATION ERROR: bucket is empty (may cause black horizontal flashes)!\n";
                      const int empty_y = bi + minY;
                      std::cout << "(minY, maxY)=" << "(" << minY << ", " << maxY << ") -> empty bucket:" << empty_y << '\n';
                      for(auto f : face_coords){
                          if(std::get<1>(f) == empty_y){
                            std::cout << std::get<1>(f) << " exists in face coordinates though" << '\n';
                          }
                      }
                  }
                  if(b.size() == 1)
                  {
                      std::cout << "RASTERIZATION INFO: bucket only has 1 element!\n";
                      b.push_back(b.at(0));
                  }
               }

               //
               for(int j = 0; j < buckets.size(); j++){
                    std::sort(
                            buckets.at(j).begin(),
                            buckets.at(j).end(),
                            [](std::tuple<int, float, Color> x, std::tuple<int, float, Color> y){return std::get<0>(x) < std::get<0>(y);}
                        ); 
               }
            

               // Interpolate using leftmost and rightmost pxl values
               for(int j = 0 ; j < buckets.size(); j++)
               {

                    const auto bucket = buckets.at(j);
                    
                    if(bucket.size() == 0){
                        continue;
                    }

                    int x_left = std::get<0>(bucket.at(0));
                    int x_right = std::get<0>(bucket.at(bucket.size() - 1));
                    const float dx = x_right - x_left;
                 
                //    

                    const float z_left = std::get<1>(bucket.at(0));
                    const float z_right = std::get<1>(bucket.at(bucket.size() - 1));
                        
                    const Color c_left  = std::get<2>(bucket.at(0));
                    const Color c_right = std::get<2>(bucket.at(bucket.size() - 1)); // has no color if gone off-screen?
                    
                    std::cout << "(minY, maxY)=" << "(" << minY << ", " << maxY << ")" << '\n';
                    std::cout << "(minX, maxX)=" << "(" << x_left << ", " << x_right << ")" << '\n';
                    //std::cout << (c_left.r + c_left.b + c_left.g) << '\n';
                    //std::cout << (c_right.r + c_right.b + c_right.g) << '\n';
                    std::cout << minY + j << '\n';

                    for(int k = 0; k < bucket.size(); k++)
                    {
                        for(int m = 0; m < dx+1; m++)
                        {
                            //
                            // same factor used for both color and z
                            float factor = m / dx;
                            if(dx == 0){
                                continue;
                            }

                            if(!inScreen(x_left+m, minY + j)){
                                continue;
                            }

                             
                            // INTERPOLATE HERE! (requires the z-values along the wireframe of the face)
                            //const float z = //(1-factor) * v2.z + line_factor * v1.z; 
                            
                            const float z = ((1-factor) * z_right) + (factor * z_left); 
                            if(z < Z_BUFF[std::get<0>(bucket.at(0)) + m][minY + j]){
                                Z_BUFF[std::get<0>(bucket.at(0)) + m][minY + j] = z;
                            
                                //std::cout << c_left.r << ", " << c_left.g << ", " << c_left.b << '\n'; 
                            
                                COLOR_BUFF[x_left + m][minY + j] = (Color{                               
                                    .r = round((1-factor) * c_left.r  + factor * c_right.r),
                                    .g = round((1-factor) * c_left.g  + factor * c_right.g),
                                    .b = round((1-factor) * c_left.b  + factor * c_right.b)}
                                 );
                            }
                        } 
                    } 
                }
               //
           } // fill mode if
         
      } 
    };

    /**
     *
     * No clipping
     * No visibility/depth testing
     * No interpolation of color or location
     * Wireframe only
     *
     * Assumes CCW coordinates
     */
    void safe_render(SDL_Renderer* renderer)
    {
        //
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE); // Black
        //
        for(Face f: faces){
           for(int i = 0; i < f.vts.size(); i++){
               Vertex v1 = screen(project(f.vts.at(i), FOV), W, H);
               Vertex v2 = screen(project(f.vts.at((i+1) % f.vts.size()), FOV), W, H);
               // SDL_RenderPoint(renderer, v1.x, v1.y);
               SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
           }
        };
    }
};


// very inefficient, but useful to see what is actually in the buffer
void render_COLOR_BUFF(SDL_Renderer* renderer){

            Color c{};
            for(int row = 0; row < H; row ++){
                for(int col = 0; col < W; col++)
                
                {
                    c.r =  (COLOR_BUFF[row][col]).r;
                    c.g =  (COLOR_BUFF[row][col]).g;
                    c.b =  (COLOR_BUFF[row][col]).b;
                    
                    SDL_SetRenderDrawColor(renderer, c.r,c.g,c.b, SDL_ALPHA_OPAQUE); 
                    SDL_RenderPoint(renderer, row, col);
                }
            } 
}

// render a depth map
void render_Z_BUFF(SDL_Renderer* renderer){

            for(int row = 0; row < H; row ++){
                for(int col = 0; col < W; col++)
                {
                    SDL_SetRenderDrawColor(
                            renderer, 
                                std::clamp((int) round(Z_BUFF[row][col]), 0, 255),
                                std::clamp((int) round(Z_BUFF[row][col]), 0, 255),
                                std::clamp((int) round(Z_BUFF[row][col]), 0, 255),
                                SDL_ALPHA_OPAQUE
                    );
                    SDL_RenderPoint(renderer,row,col);
                    //Z_BUFF[row][col] = 100; // just a big value. Anything bigger than 1 should suffice
                }
            } 
}

// input in screen coordinates
// assumes both vertices are on the screen (i.e., not OOB)
std::vector<std::pair<int, int>> bresenham(SDL_Renderer* renderer, Vertex p, Vertex q){
  //
  std::vector<std::pair<int, int>> filled{};
  //
  float dx = p.x - q.x;
  float dy = p.y - q.y;

  if(abs(dy) < abs(dx)){
  //
  int dir = 1;
  if(q.x < p.x){
      int tempx = p.x;
      int tempy = p.y;
      p.x = q.x;
      p.y = q.y;
      q.x = tempx;
      q.y = tempy;
      dir = -dir;
  }
      
  //
  if(dy < 0){
      dy = -dy;
      dir = -dir;
  }

  //
  if(dx != 0)
  {
      dx = abs(dx);
      float y = p.y;
      float D = 2*dy - dx;
      for(int i = 0; i < dx; i++)
      {
          //SDL_RenderPoint(renderer, p.x + i, y);
          // off-screen
          filled.push_back(std::pair<int,int>{p.x+i, y});
          if(D >= 0){
              y = y - dir;
              D = D - 2*dx;
          }
          D = D + 2*dy; 

         // std::cout << (p.x + i) << "-" << y << '\n';
      }
  }
  }

  else{
  //
  int dir = 1;
  if(q.y < p.y){
      int tempx = p.x;
      int tempy = p.y;
      p.x = q.x;
      p.y = q.y;
      q.x = tempx;
      q.y = tempy;
      dir = -dir;
  }
      
  //
  if(dx < 0){
      dx = -dx;
      dir = -dir;
  }

  //
  if(dy != 0)
  {
      dy = abs(dy);
      float x = p.x;
      float D = 2*dx - dy;
      for(int i = 0; i < dy; i++)
      {
          //SDL_RenderPoint(renderer, x, p.y + i);

          filled.push_back(std::pair<int,int>{x, p.y + i});
          if(D >= 0){
              x = x - dir;
              D = D - 2*dy;
          }
          D = D + 2*dx; 

      }
  }
  }
  //
  //std::cout << "Bresenham -> " << filled.size() << " coordinates!" << '\n';
  return filled;
}

//
//
//
//
//
int main(void){


    // SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
    SDL_Init(SDL_INIT_VIDEO);
    //   
    SDL_Window* window = SDL_CreateWindow("Clippy", W, H, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);

    /*Initialize buffers*/
    for(int row = 0; row < H; row ++){
        for(int col = 0; col < W; col++){
            Z_BUFF[row][col] = 100; // just a big value. Anything bigger than 1 should suffice
        }
    } 

    /* 
     * Objects in scene (World coordinates)
     *
    */
    std::vector<Geometry> objects{};

    Geometry t1 = Geometry(std::vector<Vertex>{
        {.x =    1, .y =     0, .z =  3.5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =    0, .y =    -1, .z =  3.5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =   -1, .y =     0, .z =  3.5, .w = 1, .r = 255, .g = 255, .b = 255}
    }, 3); 
    objects.push_back(t1);
    
    Geometry t2 = Geometry(std::vector<Vertex>{
        {.x =    1, .y =     0, .z =  3, .w = 1, .r = 255},
        {.x =    0, .y =    -1, .z =  3, .w = 1, .g = 255},
        {.x =   -1, .y =     0, .z =  3, .w = 1, .b = 255}
    }, 3); 
    objects.push_back(t2);
    
    Geometry t3 = Geometry(std::vector<Vertex>{
        {.x =    1, .y =     0, .z =  3, .w = 1, .r = 255},
        {.x =    0, .y =    -1, .z =  3, .w = 1, .g = 255},
        {.x =   -1, .y =     0, .z =  3, .w = 1, .b = 255}
    }, 3); 
    
    Geometry earth = Geometry(std::vector<Vertex>{
        {.x =    .5, .y =     .5, .z =  3, .w = 1, .r = 255},
        {.x =    .5, .y =    -.5, .z =  3, .w = 1, .g = 255},
        {.x =   -.5, .y =    -.5, .z =  3, .w = 1, .b = 255},
        {.x =   -.5, .y =     .5, .z =  3, .w = 1, .b = 255}
    }, 4); 
    
    Geometry moon = Geometry(std::vector<Vertex>{
        {.x =    .1, .y =     .1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =    .1, .y =    -.1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =   -.1, .y =    -.1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =   -.1, .y =     .1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255}
    }, 4); 
    
    /*
     * Render loop
     */
    SDL_Event event;
    bool running = true;
    const float FPS = 60;
    while(running) {
            // User input
            while(SDL_PollEvent(&event)){
                if(event.type == SDL_EVENT_QUIT){
                    running = false;
                }
            }
   
            // Reset screen to all black
            SDL_SetRenderDrawColor(renderer, 0, 0, 0,SDL_ALPHA_OPAQUE); // Black
            SDL_RenderClear(renderer);
            
            // Render scene
            //for(Geometry &g: objects)
            //{  
                 /*
                * 1. Translate from world space to camera space
                * 2. Apply transformations in camera space
                * 3. Translate back to world space
                */
                
            /*
                t1.translate_geom(Vertex {.x = 0, .y = 0, .z = -6});                
                t2.translate_geom(Vertex {.x = 0, .y = 0, .z = -4});                
                
                t1.rotate_geom( 1, Z_AXIS);                
                t2.rotate_geom(-1, Z_AXIS);                
                
                t1.translate_geom(Vertex {.x = 0, .y = 0, .z = 6});                
                t2.translate_geom(Vertex {.x = 0, .y = 0, .z = 4});                
              */  

                //
                //
                earth.translate_geom(Vertex {.x = 0.0, .y = -0.1, .z = 0});                
                
                /*  
                * All transformations/mutations are done, simply render the vertices
                */
                
                // g.printProperties();
                /*
                 *
                 * - Expected behavior if you render the geometry from furthest away to closest
                 * - Unexpected behavior otherwise 
                 *
                 */
                // blue (t2) should be closer
                //t2.render(renderer, FILL_MODE);
                //t1.render(renderer, FILL_MODE);
                //c1.safe_render(renderer);
                //t3.render(renderer, LINE_MODE);
                
                earth.render(renderer, FILL_MODE);
                //moon.render(renderer, FILL_MODE);
                
                // use safe_render to test these buffer functions for performance
                render_COLOR_BUFF(renderer); // Slowest function we have, but it is constant in the amount of geometry.
                // render_Z_BUFF(renderer);
                reset_Z_BUFF();
                reset_COLOR_BUFF();
                
            SDL_RenderPresent(renderer);
            SDL_Delay(1000 / FPS);
    }


    // cleanup
    SDL_Quit();
    return 0;
}



