#include <SDL3/SDL_init.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include "matrix.h"
#include <iostream>
#include <vector>
#include <math.h>


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

int main(void){

    // SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
    SDL_Init(SDL_INIT_VIDEO);
    //   
    int W = 800;
    int H = 800;
    SDL_Window* window = SDL_CreateWindow("spinny cube", W, H, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
 
    /* Vertex data for a unit cube 
    * - vertex data relative to camera (i.e., camera space -> camera is at (0,0,0))
    */ 
    // offset == base gives a cube
    const float offset = 0.5; // controls depth of cuboid (0 for flat rectangle)
    const float base = 0.5; // controls width + height of cuboid (smaller base -> smaller width & height)
    Vertex v1 = {.x =  base, .y =  base, .z =  - offset, .w = 1};
    Vertex v2 = {.x = -base, .y =  base, .z =  - offset, .w = 1};
    Vertex v3 = {.x = -base, .y = -base, .z =  - offset, .w = 1};
    Vertex v4 = {.x =  base, .y = -base, .z =  - offset, .w = 1};
    Vertex v5 = {.x =  base, .y =  base, .z =    offset, .w = 1};
    Vertex v6 = {.x = -base, .y =  base, .z =    offset, .w = 1};
    Vertex v7 = {.x = -base, .y = -base, .z =    offset, .w = 1};
    Vertex v8 = {.x =  base, .y = -base, .z =    offset, .w = 1};
   
    /*
    * vts -> transformations in camera space are applied here
    * vts_cp -> will be used to store each iterations projected + screen-mapped coordinates for rendering
    *
    */ 
    std::vector<Vertex> vts = {
        v1,v2,v3,v4, // 2 faces 
        v5,v6,v7,v8
    };
    std::vector<Vertex> vts_cp = {
        v1,v2,v3,v4, // 2 faces 
        v5,v6,v7,v8
    };

    //
    SDL_Event event;
    bool running = true;
    const float FPS = 60;
    const float FOV = 90;
    while(running) {
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){
               running = false;
            }
        }
        //
        SDL_SetRenderDrawColor(renderer, 0, 0, 0,SDL_ALPHA_OPAQUE); // Black
        SDL_RenderClear(renderer);


        /*
        * Apply transformations in camera space to vertices in camera space coords
        */
        for(int i = 0 ; i < vts.size(); i++){
	    vts.at(i) = rot(vts.at(i), 1, X_AXIS); // rotation relative to camera (vts coordinates are relative to the camera, cam at origin)
            vts.at(i) = rot(vts.at(i), 1, Y_AXIS); // rotation relative to camera (vts coordinates are relative to the camera, cam at origin)

            // draw point that is being rotates around
            SDL_SetRenderDrawColor(renderer, 245, 66, 197, SDL_ALPHA_OPAQUE); // Pink
            
            float x_off = -0.7;
            float y_off = -0.85;
            float z_off = -3;  // camera translation along the axes
            Vertex rotPoint = screen(project({.x = -x_off, .y = -y_off, .z = -z_off, .w = 1}, FOV), W, H);
            SDL_RenderPoint(renderer, rotPoint.x, rotPoint.y);

            // move camera away (here, we move all objects in the scene in the *opposite* direction of the camera to achieve this)
            
            for(int i =0 ; i < vts.size() ; i++){
               Vertex v = vts.at(i);
               Vertex offset_vector = Vertex {.x = -x_off, .y = -y_off, .z = -z_off, .w = 0};
               Vertex v_t = translate(v, offset_vector);
               vts_cp.at(i) = v_t;
            };

        }
        
        /*
        * Transition to screen coordinates for rendering
        */
        SDL_SetRenderDrawColor(renderer, 10, 245, 37,SDL_ALPHA_OPAQUE); // Green
        for(int i =0 ; i < vts.size() ; i++){
           vts_cp.at(i) = project(vts_cp.at(i), FOV);
           vts_cp.at(i) = screen(vts_cp.at(i), W, H);
        };
                
        const int half = vts.size() / 2; 
        for(int i = 0 ; i < half ; i++){
            // face 1
            SDL_RenderLine(renderer, vts_cp.at(i).x,      vts_cp.at(i).y,      vts_cp.at((i+1) % half).x,        vts_cp.at((i+1) % half).y);
      
            // face 2
            SDL_RenderLine(renderer, vts_cp.at(i+half).x, vts_cp.at(i+half).y, vts_cp.at((i+1) % half + half).x, vts_cp.at((i+1) % half + half).y);
            
            // connect the 2 faces
            SDL_RenderLine(renderer, vts_cp.at(i).x, vts_cp.at(i).y, vts_cp.at((i+ half)).x, vts_cp.at((i + half)).y);
        }
      
        SDL_RenderPresent(renderer);
        // printVertices(vts_cp); 
        SDL_Delay(1000 / FPS);
    }


    // cleanup
    SDL_Quit();
    return 0;

}
