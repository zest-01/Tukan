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

const Vertex CAMERA_POS = Vertex {.x = 0, .y = 0, .z = 0, .w = 1}; // camera placed at origin of world coord system for convenience
const int W = 800;
const int H = 800;
const float FOV = 90; // in degrees

void bresenham(SDL_Renderer* renderer, Vertex p, Vertex q);

/*
 * Input -> vertex v in NDC coordinates
 * Output -> a new vertex in screen coordinates
*/
Vertex screen(Vertex v, int W, int H){
  Vertex v_screen = {
      .x = (v.x + 1) * W/2,
      .y = H - ((v.y + 1) * H/2),
      .z = v.z,
      .w = v.w
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
        .w = v.w
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
      .w = v.w
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
                vts.push_back(Vertex {.x = v.x, .y = v.y, .z = v.z, .w = v.w});
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
    protected:
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
     public:
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
       Vertex center = {.w = 1};
       void setCenter(Vertex v){
           center.x = v.x;
           center.y = v.y;
           center.z = v.z;
           center.w = v.w;
       }
};

struct Cube : Geometry {
    // Overload constructor 
    Cube(std::vector<Face> faces_in) : Geometry(faces_in){};
    Cube(std::vector<Vertex> vts_in) : Geometry(vts_in, 4){};    
    //
    void render(SDL_Renderer* renderer, int color){
        switch(color){
           case 1:
               SDL_SetRenderDrawColor(renderer, 10, 245, 37,SDL_ALPHA_OPAQUE); // Green
               break;
           case 2:
               SDL_SetRenderDrawColor(renderer, 51, 213, 222,SDL_ALPHA_OPAQUE); // Blue
               break;
           default:         
               SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // White
        }
        
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
       // 
       //printProperties();
    };
};




// input in screen coordinates
void bresenham(SDL_Renderer* renderer, Vertex p, Vertex q){
  //
  SDL_SetRenderDrawColor(renderer, 51, 213, 222,SDL_ALPHA_OPAQUE); // Blue
  SDL_RenderPoint(renderer, p.x, p.y);
  SDL_RenderPoint(renderer, q.x, q.y);
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
          SDL_RenderPoint(renderer, p.x + i, y);
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
          SDL_RenderPoint(renderer, x, p.y + i);
          if(D >= 0){
              x = x - dir;
              D = D - 2*dy;
          }
          D = D + 2*dx; 

      }
  }
  }
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
    std::vector<Cube> objects{};
    float base   = 0.5;
    float offset = 0.5;
    
    
    //
    Vertex cube_center = Vertex {.x = 0.7, .y = 0.85, .z = 3, .w = 1};
    Cube c1 = Cube(std::vector<Vertex> {
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y + base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y + base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y - base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y - base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y + base, .z =  cube_center.z + offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y + base, .z =  cube_center.z + offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y - base, .z =  cube_center.z + offset, .w = 1},
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y - base, .z =  cube_center.z + offset, .w = 1}
    });
    c1.setCenter(cube_center);
    objects.push_back(c1);
    
    
    cube_center = Vertex {.x = 0.7, .y = 0.85, .z = 3, .w = 1};
    Cube c2 = Cube(std::vector<Vertex> {
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y + base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y + base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y - base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y - base, .z =  cube_center.z - offset, .w = 1},
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y + base, .z =  cube_center.z + offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y + base, .z =  cube_center.z + offset, .w = 1},
        Vertex {.x =  cube_center.x - base, .y =  cube_center.y - base, .z =  cube_center.z + offset, .w = 1},
        Vertex {.x =  cube_center.x + base, .y =  cube_center.y - base, .z =  cube_center.z + offset, .w = 1}
    });
    c2.setCenter(cube_center);
    objects.push_back(c2);
 

    //
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
            //
     
            // Render scene
            int i = 0;
            /*
             * default white
             * 0       green
             * 1       blue
             *
             *
             */
            int color = -1; // -1 for white (default) 
            for(Cube &c: objects){  
                 /*
                * 1. Translate from world space to camera space
                * 2. Apply transformations in camera space
                * 3. Translate back to world space
                */
                
                
                c.translate_geom(Vertex {.x = -c.center.x, .y = -c.center.y, .z = -c.center.z}); 
                if(i++ % 2 == 0){
                  c.rotate_geom(1, X_AXIS);
                  c.rotate_geom(1,  Z_AXIS);
                  color = 1;
                } else{
                  c.rotate_geom(-1, X_AXIS);
                  c.rotate_geom(-1, Z_AXIS);
                  color =2;   
                }
                c.translate_geom(Vertex {.x = c.center.x, .y = c.center.y, .z = c.center.z});
                
                /*  
                * All transformations/mutations are done, simply render the vertices
                */
                //c.printProperties();
                c.render(renderer, color);
                

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
