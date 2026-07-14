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
};

struct Cube : Geometry {
    // Overload constructor 
    Cube(std::vector<Face> faces_in) : Geometry(faces_in){};
    Cube(std::vector<Vertex> vts_in) : Geometry(vts_in, 4){};    

    //
    void render(SDL_Renderer* renderer){
        SDL_SetRenderDrawColor(renderer, 10, 245, 37,SDL_ALPHA_OPAQUE); // Green
        
        /*
        // project to near plane, then map to screen
        for(int i = 0 ; i < faces_cp.size() ; i++){
            for(int j = 0 ; j < faces_cp.at(i).vts.size() ; j++){
                c.faces.at(i).vts.at(j) = project(c.faces.at(i).vts.at(j), FOV);
                c.faces.at(i).vts.at(j) =  screen(c.faces.at(i).vts.at(j), W, H);
                //vts_cp.at(j) = screen(vts_cp.at(j), W, H);
            };
        };
        */

        // render the 2 cube faces separately
        for(int i = 0 ; i < faces.size(); i++){
            for(int j = 0 ; j < faces.at(i).vts.size(); j++){
                Vertex v1 = screen(project(faces.at(i).vts.at(j), FOV), W, H);
                Vertex v2 = screen(project(faces.at(i).vts.at((j+1) % faces.at(i).vts.size()), FOV), W, H);
                SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
            }
        }

        // connect the 2 cube faces
        for(int i = 0; i < faces.at(0).vts.size(); i++){
            Vertex v1 = screen(project(faces.at(0).vts.at(i), FOV), W, H);
            Vertex v2 = screen(project(faces.at(1).vts.at(i), FOV), W, H);
            SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);    
        }  
       // 
       //printProperties();
    };
};

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
     *
     * World coordinates
     *
    */
    const float offset = 0.5;
    const float base   = 0.5;
    const Vertex cube_center = Vertex {.x = 0.7, .y = 0.85, .z = 3, .w = 1};
    // const Vertex cube_center = Vertex {.x = 0, .y = 0, .z = 0, .w = 1};
    Vertex v1 = {.x =  cube_center.x + base, .y =  cube_center.y + base, .z =  cube_center.z - offset, .w = 1};
    Vertex v2 = {.x =  cube_center.x - base, .y =  cube_center.y + base, .z =  cube_center.z - offset, .w = 1};
    Vertex v3 = {.x =  cube_center.x - base, .y =  cube_center.y - base, .z =  cube_center.z - offset, .w = 1};
    Vertex v4 = {.x =  cube_center.x + base, .y =  cube_center.y - base, .z =  cube_center.z - offset, .w = 1};
    Vertex v5 = {.x =  cube_center.x + base, .y =  cube_center.y + base, .z =  cube_center.z + offset, .w = 1};
    Vertex v6 = {.x =  cube_center.x - base, .y =  cube_center.y + base, .z =  cube_center.z + offset, .w = 1};
    Vertex v7 = {.x =  cube_center.x - base, .y =  cube_center.y - base, .z =  cube_center.z + offset, .w = 1};
    Vertex v8 = {.x =  cube_center.x + base, .y =  cube_center.y - base, .z =  cube_center.z + offset, .w = 1};
    const Vertex CAMERA_POS = Vertex {.x = 0, .y = 0, .z = 0, .w = 1}; // camera placed at origin of world coord system for convenience

    /*
    * vts -> transformations in camera space are applied here
    * vts_cp -> will be used to store each iterations projected + screen-mapped coordinates for rendering
    *
    */
    Face f1    = std::vector<Vertex> {v1,v2,v3,v4}; 
    Face f2    = std::vector<Vertex> {v5,v6,v7,v8}; 
    std::vector<Face> faces = {f1, f2};
    
    Face f1_cp    = std::vector<Vertex> {v1,v2,v3,v4}; 
    Face f2_cp    = std::vector<Vertex> {v5,v6,v7,v8}; 
    std::vector<Face> faces_cp = {f1_cp, f2_cp};
    Cube c = Cube(faces_cp); 


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
        
        /*
         * 1. Translate from world space to camera space
         * 2. Apply transformations in camera space
         * 3. Translate back to world space
        */
        c.translate_geom(Vertex {.x = -cube_center.x, .y = -cube_center.y, .z = -cube_center.z}); 
        c.rotate_geom(1, X_AXIS);
        c.rotate_geom(1, Y_AXIS);
        c.translate_geom(Vertex {.x = cube_center.x, .y = cube_center.y, .z = cube_center.z});
        
        /*
         * All transformations/mutations are done, simply render the vertices
         */
        c.printProperties();
        c.render(renderer);
        
        //
        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / FPS);
    }


    // cleanup
    SDL_Quit();
    return 0;

}
