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


//
const Vertex CAMERA_POS = Vertex {.x = 0, .y = 0, .z = 0, .w = 1}; // camera placed at origin of world coord system for convenience
const int W = 800;
const int H = 800;
const float FOV = 90; // in degrees
std::tuple<float, float, float> COLOR_BUFF[H][W]{};
float Z_BUFF[H][W]{};


std::vector<std::pair<int, int>> bresenham(SDL_Renderer* renderer, Vertex p, Vertex q);

// only works if the elements are printable

void printVector(std::vector<int> vctr){
    std::cout << "<";
    for(const auto& v : vctr){
        std::cout << v <<  ",";
    }
    std::cout << ">" << '\n';
};

void printColorBuff(){
    for(int i = 0; i < H; i++){
        std::cout << "[";
        for(int j = 0; j < W; j++){
           std::cout 
               << "{" 
               << (std::get<0>(COLOR_BUFF[i][j])) 
               << ','
               << (std::get<1>(COLOR_BUFF[i][j])) 
               << ','
               << (std::get<2>(COLOR_BUFF[i][j])) 
               << "}"
               << ',';
        }
        std::cout << "]" << '\n';
    }
};

// prints color from color buffer
void printColor(int row, int col){
    std::cout << "(" << row << ", " << col << ") = ("
        << std::get<0>(COLOR_BUFF[row][col]) << ", "
        << std::get<1>(COLOR_BUFF[row][col]) << ", "
        << std::get<2>(COLOR_BUFF[row][col]) << ")"
        << '\n';
}


/*
 * Input -> vertex v in NDC coordinates
 * Output -> a new vertex in screen coordinates
*/
Vertex screen(Vertex v, int W, int H){
  Vertex v_screen = {
      .x = (v.x + 1) * W/2,
      .y = H - ((v.y + 1) * H/2),
      .z = v.z,
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
       //
       static Geometry createCube(float center, float W){
           /**
            Vertex v1 {.x =  center + W, .y =  center + W, .z =  cube_center - W, .w = 1};
            Vertex v2 {.x =  center + W, .y =  center - W, .z =  cube_center - W, .w = 1};
            Vertex v3 {.x =  center - W, .y =  center - W, .z =  cube_center - W, .w = 1};
            Vertex v4 {.x =  center - W, .y =  center + W, .z =  cube_center - W, .w = 1};
            Face f1 = Face(std::vector<Vertex> {v1,v2,v3,v4});
            Vertex v5 {.x =  center + W, .y =  center + W, .z =  cube_center + W, .w = 1};
            Vertex v6 {.x =  center + W, .y =  center - W, .z =  cube_center + W, .w = 1};
            Vertex v7 {.x =  center - W, .y =  center - W, .z =  cube_center + W, .w = 1};
            Vertex v8 {.x =  center - W, .y =  center + W, .z =  cube_center + W, .w = 1};
            Face f2 = Face(std::vector<Vertex> {v5,v6,v7,v8});
*/

            return Geometry(std::vector<Vertex>{}, 4);
            
       }
    void render(SDL_Renderer* renderer){
        
        
        // assumes all faces have the same amount of vts associated with them for now
        for(int i = 0 ; i < faces.size(); i++){
            int bucketCount = 0;
            int maxY = -1;
            int minY = H+1;  
            std::vector<std::pair<int,int>> face_coords{}; 
            for(int j = 0 ; j < faces.at(i).vts.size(); j++){
                
                Vertex v1 = screen(project(faces.at(i).vts.at(j), FOV), W, H);
                SDL_SetRenderDrawColor(renderer, v1.r, v1.g, v1.b, SDL_ALPHA_OPAQUE);
                SDL_RenderPoint(renderer, v1.x, v1.y);
                
                Vertex v2 = screen(project(faces.at(i).vts.at((j+1) % faces.at(i).vts.size()), FOV), W, H);
                SDL_SetRenderDrawColor(renderer, v2.r, v2.g, v2.b, SDL_ALPHA_OPAQUE);
                SDL_RenderPoint(renderer, v2.x, v2.y);
                
                //
                //minY = std::min(minY, std::min(v1.y, v2.y));
                int curr_min = std::min(v1.y, v2.y); 
                int curr_max = std::min(v1.y, v2.y); 
                minY = std::min(minY, curr_min); 
                maxY = std::max(maxY, curr_max); 

                std::vector<std::pair<int, int>> coords = bresenham(renderer, v1, v2);
                //
                //
                for(const auto& p : coords){
                    face_coords.push_back(p);
                
                }
                /*
                if(v1.x <= v2.x){
                    face_coords.push_back(std::pair<int, int>{v2.x, v2.y});
                } else{
                    face_coords.push_back(std::pair<int, int>{v1.x, v1.y});
                }
                */
               

                //
                for(float i = 0; i < coords.size(); i++){
                    const float factor = i / (coords.size() - 1); 
                   //  std::cout << factor << "~~" << (1 - factor) << '\n';
                        
                        // Bresenham could have swapped the role of the vertices
                        if(v1.x == coords.at(0).first){
                       /* 
                       SDL_SetRenderDrawColor(renderer, 
                            (1-factor) * v1.r + factor * v2.r, 
                            (1-factor) * v1.g + factor * v2.g, 
                            (1-factor) * v1.b + factor * v2.b,
                           SDL_ALPHA_OPAQUE
                           );
                         */

                        const auto p = coords.at(i);  
                        COLOR_BUFF[p.second][p.first] = std::tuple(
                                (1-factor) * v1.r + factor * v2.r, 
                                (1-factor) * v1.g + factor * v2.g, 
                                (1-factor) * v1.b + factor * v2.b 
                        );
                        SDL_SetRenderDrawColor(renderer,
                                std::get<0>(COLOR_BUFF[p.second][p.first]),
                                std::get<1>(COLOR_BUFF[p.second][p.first]),
                                std::get<2>(COLOR_BUFF[p.second][p.first]),
                                SDL_ALPHA_OPAQUE
                                );

                        SDL_RenderPoint(renderer, p.first, p.second);

                        }
                        
                        if(v2.x == coords.at(0).first){
                         
                        const auto p = coords.at(i);  
                        COLOR_BUFF[p.second][p.first] = std::tuple(
                                (1-factor) * v2.r + factor * v1.r, 
                                (1-factor) * v2.g + factor * v1.g, 
                                (1-factor) * v2.b + factor * v1.b 
                        );
                        SDL_SetRenderDrawColor(renderer,
                                std::get<0>(COLOR_BUFF[p.second][p.first]),
                                std::get<1>(COLOR_BUFF[p.second][p.first]),
                                std::get<2>(COLOR_BUFF[p.second][p.first]),
                                SDL_ALPHA_OPAQUE
                                );
                        SDL_RenderPoint(renderer, p.first, p.second);
                        }
                        
                }
            }
                    /* scanline algorithm here!
                    *
                    *  - create dy+1 buckets
                    *  - bucket sort those buckets based on x-coordinate
                    *  - interpolate rows
                   * */
                
               
                 std::vector<std::vector<int>> buckets{};
                 for(int i = 0; i < maxY - minY+1; i++){
                     buckets.push_back({});
                 }
                 for(const std::pair<int, int> p : face_coords){
                     buckets.at(p.second - minY).push_back(p.first);
                 }

                 //
                 for(int i = 0; i < buckets.size(); i++){
                    
                    std::sort(
                            buckets.at(i).begin(),
                            buckets.at(i).end(),
                            [](int x, int y){return x < y;}
                            );
                   // printVector(buckets.at(i)); 
                 }
                 
                 //printColorBuff();
                 // interpolate using leftmost and rightmost
                 for(int j = 0 ; j  < buckets.size(); j++){
                    const auto bucket = buckets.at(j);
                    for(int i = 0; i < bucket.size(); i++){
                       
                        const float dx = bucket.at(bucket.size() - 1) - bucket.at(0);
                        for(int k = 0; k < dx+1; k++){
                            
                            //std::cout << "k=" << k << '\n';
                            // interpolate color
                           
                           //printVector(bucket); 
                           float factor = k / dx; 
                           
                            
                            

                            
                            
                            //printColor(minY + j, bucket.at(0));
                            //printColor(minY + j, bucket.at(bucket.size()-1));
                            COLOR_BUFF[minY + j][bucket.at(0)+k] =
                               std::tuple<float, float, float>{
                                ((1-factor) * std::get<0>(COLOR_BUFF[minY + j][bucket.at(0)])) + (factor * std::get<0>(COLOR_BUFF[minY +j][bucket.at(bucket.size()-1)])), 
                               
                                ((1-factor) * std::get<1>(COLOR_BUFF[minY + j][bucket.at(0)])) + (factor * std::get<1>(COLOR_BUFF[minY +j][bucket.at(bucket.size()-1)])), 
                              
                                ((1-factor) * std::get<2>(COLOR_BUFF[minY + j][bucket.at(0)])) + (factor * std::get<2>(COLOR_BUFF[minY +j][bucket.at(bucket.size()-1)])), 
                               } 
                                ;
                            //
                            std::cout << factor << '\n'; 
                            //printColor(minY + j, bucket.at(0));
                            //printColor(minY + j, bucket.at(bucket.size()-1));
                             
                            
                            
                            
                            //std::cout << "dy=" << buckets.size() << '\n';
                            //std::cout << "(row, col)=" <<  "(" << (minY + j) << "," << (bucket.at(0))<< ")"<< '\n';
                            
                            // printVector(bucket);
                            SDL_SetRenderDrawColor(renderer,
                                //255,255,255, 
                                std::get<0>(COLOR_BUFF[minY + j][bucket.at(0)+k]), 
                                std::get<1>(COLOR_BUFF[minY + j][bucket.at(0)+k]), 
                                std::get<2>(COLOR_BUFF[minY + j][bucket.at(0)+k]), 
                                SDL_ALPHA_OPAQUE
                            );
                            
                            SDL_RenderPoint(renderer, bucket.at(0)+k, minY + j);
                        }
                        
                    }
                 }
        }
        /*
        // render the 2 cube faces separately
        for(int i = 0 ; i < faces.size(); i++){
            for(int j = 0 ; j < faces.at(i).vts.size(); j++){
                Vertex v1 = screen(project(faces.at(i).vts.at(j), FOV), W, H);
                Vertex v2 = screen(project(faces.at(i).vts.at((j+1) % faces.at(i).vts.size()), FOV), W, H);
                // SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
                bresenham(renderer, Vertex{v1.x, v1.y} ,Vertex{v2.x, v2.y});
            }
        }

        // connect the 2 cube faces
        for(int i = 0; i < faces.at(0).vts.size(); i++){
            Vertex v1 = screen(project(faces.at(0).vts.at(i), FOV), W, H);
            Vertex v2 = screen(project(faces.at(1).vts.at(i), FOV), W, H);
            //SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);    
            bresenham(renderer, Vertex{v1.x, v1.y} ,Vertex{v2.x, v2.y});
        } 
       */ 
       // 
       //printProperties();
    };
};


/*
struct Cube : Geometry {
    // Overload constructor 
    Cube(std::vector<Face> faces_in) : Geometry(faces_in){};
    Cube(std::vector<Vertex> vts_in) : Geometry(vts_in, 4){};    
    //
};
*/



// input in screen coordinates
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
          filled.push_back(std::pair<int,int>{p.x+i, y});
          if(D >= 0){
              y = y - dir;
              D = D - 2*dx;
          }
          D = D + 2*dy; 

         // std::cout << (p.x + i) << "-" << y << '\n';
      }
      //filled.push_back(std::pair<int, int>{q.x, q.y});
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
      //filled.push_back(std::pair<int, int>{q.x, q.y});
  }
  }
  //
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
    SDL_Window* window = SDL_CreateWindow("spinny cube", W, H, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
 

    /* 
     * Objects in scene (World coordinates)
     *
    */
    std::vector<Geometry> objects{};

    Geometry t1 = Geometry(std::vector<Vertex>{
        {.x =   .5, .y =    0, .z =  1, .w = 1, .g = 255},
        {.x =    0, .y =   .5, .z =  1, .w = 1, .r = 255},
        {.x =  -.5, .y =    0, .z =  1, .w = 1, .b = 255}
    }, 3); 
    objects.push_back(t1);
    
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
            for(Geometry &g: objects)
            {  
                 /*
                * 1. Translate from world space to camera space
                * 2. Apply transformations in camera space
                * 3. Translate back to world space
                */
                
                // [...]

                /*  
                * All transformations/mutations are done, simply render the vertices
                */
                
                // g.printProperties();
                g.render(renderer);
                
                      
                 
                

                // Bresenham test lines
                /*
                // Q2 
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 600, .y = 380, .z=1, .w=1}
                );
                // Q3
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 600, .y = 420, .z=1, .w=1}
                );
                // Q6
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 200, .y = 420, .z=1, .w=1}
                );
                // Q7
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 200, .y = 380, .z=1, .w=1}
                );
                // Q1
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 420, .y = 200, .z=1, .w=1}
                );
                // Q4
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 420, .y = 600, .z=1, .w=1}
                );
                // Q5
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 380, .y = 600, .z=1, .w=1}
                );             
                // Q8
                bresenham(renderer, 
                        Vertex {.x = 400, .y = 400, .z=1, .w=1},
                        Vertex {.x = 380, .y = 200, .z=1, .w=1}
                );
                */
          }
          //
          SDL_RenderPresent(renderer);
          SDL_Delay(1000 / FPS);
    }


    // cleanup
    SDL_Quit();
    return 0;


}
