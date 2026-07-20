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

//
std::vector<std::pair<int, int>> bresenham(SDL_Renderer* renderer, Vertex p, Vertex q);
void printColor(int row, int col);
std::vector<float> interpolate_line_z(Vertex v1, Vertex v2);
std::vector<Color> interpolate_line_color(Vertex v1, Vertex v2);
//

bool inScreen(int x, int y){
    return 0 <= x && x < W && 0 <= y && y < H;
}

/**
 *
 * Resets the buffer by setting Z to some arbitrary cut off point
 * 
 * */
void reset_Z_BUFF(){
    int arbitrary_cutoff = 255;
    for(int row = 0 ; row < H ; row++){
        for(int col = 0; col < H ; col++){
            Z_BUFF[row][col] = arbitrary_cutoff;
        }
    }
}

/**
 *
 *
 * Checks whether the given vertex is visible at the given pixel
 *
 * NOTE: input vertex must be in screen coordinates
 */
bool isVisible(Vertex v){
    //Vertex v = project(v);
    
    // NOTE: for 2 objects at the same depth, the one rendered last will end up being rendered.
    const float z_fight_corr = 0.000f; // idk if this even does anything ngl
    return inScreen(v.x, v.y) && v.z > CAMERA_POS.z && v.z <= Z_BUFF[(int) v.x][(int) v.y] + z_fight_corr;

}













/**
 *
 * Resets the buffer by setting all colors to 0
 * 
 * */
void reset_COLOR_BUFF(){
    for(int row = 0 ; row < H ; row++){
        for(int col = 0; col < H ; col++){
            COLOR_BUFF[row][col] = Color{};
        }
    }
}



/* 
 * Prints color from color buffer at (x,y)
*/
void printColor(int x, int y){
    std::cout << "(" << x << ", " << y << ") = ("
        << (COLOR_BUFF[x][y]).r << ", "
        << (COLOR_BUFF[x][y]).g << ", "
        << (COLOR_BUFF[x][y]).b << ")"
        << '\n';
}


/*
 * @params
 * v -> vertex in NDC coordinates
 * W -> width  of viewport
 * H -> height of viewport
 *
 *@out
 * a NEW vertex in screen coordinates
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
 *
 * Perspective projection on coordinates in camera space.
 * 
 * @params 
 * v   -> vertex in camera space coordinates
 * FOV -> field of view \in ]0, 360[
 *
 * @out
 * a NEW vertex projected onto the near plane -> clip space coordinates
 * 
 * Assumption:
 * - w = 1 (i.e., no perspective division built in yet)
 */
Vertex project(Vertex v, int FOV){
    float alpha = FOV / 2;
    float rad = alpha / 180 * PI;
    
    // Note that z could become really smalll (and even pactically 0) when rotating
    // To avoid issues with that, we have the abs(z) > 0.1
    Vertex v_pr = {
        .x = v.x,
        .y = v.y,
        .z = v.z,
        .w = v.w,

        .r = v.r,
        .g = v.g,
        .b = v.b
    };
    if(v.z > CAMERA_POS.z && abs(v.z) > 0.10){
        v_pr.x = v.x / (v.z * std::tan(rad));
        v_pr.y = v.y / (v.z * std::tan(rad));
    }
    
    // Projections from behind the camera become inverted among y (pinhole camera effect)
    if(v_pr.z < CAMERA_POS.z){
        v_pr.y = -v_pr.y;
    }

    return v_pr;
}

/*
 * @params
 * v ->  vertex in screen coordinates
 * W ->  viewport width
 * H ->  viewport height
 *
 * @out
 * a NEW vertex in NDC coordinates
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


/*
 *
 * Represents a face of a particular geometric object / mesh.
 * 
 * NOTE: all vertex coordinates are in NDC relative to camera!
*/
struct Face {
    public:
        std::vector<Vertex> vts {};
        Face(std::vector<Vertex> vts_in){
            for(auto& v : vts_in){
                vts.push_back(Vertex {.x = v.x, .y = v.y, .z = v.z, .w = v.w, .r = v.r, .g = v.g, .b = v.b});
            }
        };

        /* Renders the vertices */
        void renderVts(SDL_Renderer* renderer){
                
            for(int i = 0; i < vts.size(); i++){
                Vertex v = screen(project(vts.at(i), FOV), W, H);
                SDL_SetRenderDrawColor(renderer, 
                        v.r,
                        v.g,
                        v.b,
                        SDL_ALPHA_OPAQUE); // Black
                SDL_RenderPoint(renderer, v.x, v.y);
            }
        }    
   protected:
        // N/A
   private:
        // N/A
};

/**
 *
 * Represents a piece of geometry by storing its associated faces.
 *
 * NOTE: all vertex coordinates are in NDC relative to camera! 
 */
struct Geometry {
    public:
        std::vector<Face> faces{};
        Vertex origin{};

        Geometry(std::vector<Face> faces_in){
        for(auto& f : faces_in)
            {
                faces.push_back(f);
            };
        };
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
            //origin = vts_in.at(0);
        }



        /* Give CCW order. The first point given will be considered the reference origin of the triangle */
        static Geometry createTriangle(std::vector<Vertex> vts_in){ 
            Geometry triangle = Geometry(vts_in, 3);
            triangle.origin = vts_in.at(0);
            return triangle;
        }
        
        /* 
         * Returns all the vertices of the geometry.
         *
         * NOTE: some vertices may be duplicated. (e.g. a cube will have 6*4=24 vertices, not 8)
         * 
        */
        std::vector<Vertex> getVts(){
            std::vector<Vertex> vts{};
            for(Face f : faces){
                for(Vertex v : f.vts){
                    vts.push_back(v);
                }
            }
            return vts;
        }



        //
        void printPos(){
            Vertex v = project(origin, FOV);
            std::cout << "origin at (x,y,z): " << origin.x << "~" << origin.y << "~" << origin.z << '\n'; 
            std::cout << "perspective projected origin at (x,y,z): " << v.x << "~" << v.y << "~" << v.z << '\n'; 
        }

        /* 
         * 
         * Rotates this geometry around the global x/y/z axes 
         *
         * @params
         * angle -> specified in degrees
         * axis  -> X_AXIS, Y_AXIS, Z_AXIS
         * 
         */
        void rotate_geom(float angle, int axis){        
                for(int i = 0; i < faces.size() ; i++){
                    for(int j = 0; j < faces.at(0).vts.size(); j++){
                        faces.at(i).vts.at(j) = rot(faces.at(i).vts.at(j), angle, axis);
                    }
                }
                origin = rot(origin, angle, axis);                
                //origin.x = 777;// rot(origin, angle, axis);
                //origin = normalize(origin);

               // std::cout << norm(origin) << "dogoooo\n";
                
       };
      
       
       /* 
        *
        * Translates this geometry along the global x/y/z axes
        *
        * @params
        * offset_vertex -> vertex containing offsets to take
        *
        */
       void translate_geom(Vertex offset_vertex){
         for(int i = 0; i < faces.size(); i++){
            for(int j = 0 ; j < faces.at(i).vts.size() ; j++){
                Vertex v = faces.at(i).vts.at(j);
                v.x = v.x + offset_vertex.x;
                v.y = v.y + offset_vertex.y;
                v.z = v.z + offset_vertex.z;
                faces.at(i).vts.at(j) = v; 
            };
         }
         origin = Vertex{
                .x = origin.x + offset_vertex.x,
                .y = origin.y + offset_vertex.y,
                .z = origin.z + offset_vertex.z
         };
       }





    protected:
        // N/A
    private:
        // N/A
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
void safe_render(SDL_Renderer* renderer, Geometry g)
    {
        //
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE); // Black
        //
        for(Face f: g.faces){
           for(int i = 0; i < f.vts.size(); i++){
               Vertex v1 = screen(project(f.vts.at(i), FOV), W, H);
               Vertex v2 = screen(project(f.vts.at((i+1) % f.vts.size()), FOV), W, H);
               // SDL_RenderPoint(renderer, v1.x, v1.y);
               SDL_RenderLine(renderer, v1.x, v1.y, v2.x, v2.y);
           }
        };
}

/*
 * 0 for wireframe -> no depth testing
 * 1 for interpolation of lines, but no fill -> depth testing, but no fill (i.e., black fill)
 * 2 for interpolation of lines, and fill
*/
void render(SDL_Renderer* renderer, Geometry g, int mode)
    {
        // Assumes all faces have the same amount of vts associated with them for now
        for(int i = 0 ; i < g.faces.size(); i++)
        {
            //
            int maxY = -1;
            int minY = H+1;
            //
            std::vector<std::tuple<int,int,float, Color>> face_coords{}; 
            int on_screen_line_count = 0;
            for(int j = 0 ; j < g.faces.at(i).vts.size(); j++)
            {
                // Pixels on line(v1,v2)
                Vertex v1 = screen(project(g.faces.at(i).vts.at(j), FOV), W, H);
                Vertex v2 = screen(project(g.faces.at(i).vts.at((j+1) % g.faces.at(i).vts.size()), FOV), W, H);

                // 1.75 is magic number. The higher the number, the stricter the visibility, to prevent lines 
                //bool lineOffScreen = false && (!inScreen(v1.x,v1.y) && !inScreen(v2.x,v2.y));
                bool lineOffScreen = !isVisible(v1) && !isVisible(v2);
                
                    
                if(!lineOffScreen){
                    //std::cout << "line off screen!\n";
                    on_screen_line_count += 1;
                } 
               

                // Calls Bresenham 3 times total...
                std::vector<float>interpolated_z       = interpolate_line_z(v1,v2);
                std::vector<Color>interpolated_color   = interpolate_line_color(v1,v2);
                std::vector<std::pair<int, int>> pixels = bresenham(nullptr, v1, v2);

                //  minY & maxY used for scanline
                std::vector<std::tuple<int, int, float, Color>> coords{};
                
                for(int k = 0; k < pixels.size() ; k++){
                    const auto pxl = pixels.at(k);
                    coords.push_back(std::tuple<int, int, float, Color>
                                {
                                    pxl.first, 
                                    pxl.second, 
                                    interpolated_z.at(k), 
                                    interpolated_color.at(k)
                                }
                             );  
                    face_coords.push_back(coords.at(k));
                    //
                    minY = std::min(minY, pxl.second);
                    maxY = std::max(maxY, pxl.second); 
                    //
                }
            }
           // 
            g.printPos();
            bool faceOffScreen = (on_screen_line_count == 0);
            if(!faceOffScreen){
                std::cout << "face on screen!\n";
                
            
               // 1 bucket for each y-coordinate that is crossed by some line of the face
               // buckets contain <x, depth, color> of pixels crossed at that y
               std::vector<std::vector<std::tuple<int, float, Color>>> buckets{}; // <x,z, Color>
               for(int i = 0; i < maxY - minY + 1; i++){
                    buckets.push_back({});
               }
               std::cout << face_coords.size() << '\n';

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
               int count = 0;
               for(int bi = 0; bi < buckets.size() ; bi++){
                   auto b = buckets.at(bi);
                   if(b.size() == 0)
                  {
                      std::cout << "RASTERIZATION ERROR: bucket is empty (may cause black horizontal flashes)!\n";
                      const int empty_y = bi + minY;
                      //std::cout << "(minY, maxY)=" << "(" << minY << ", " << maxY << ") -> empty bucket:" << empty_y << '\n';
                      for(auto f : face_coords){
                          if(std::get<1>(f) == empty_y){
                            std::cout << std::get<1>(f) << " exists in face coordinates though" << '\n';
                          }
                      }
                  }
                  if(b.size() == 1)
                  {
                      //std::cout << "RASTERIZATION INFO: bucket only has 1 element (duplicating it...)!\n";
                      b.push_back(b.at(0));
                      count -= 1; // compensate for double count
                  }
                  count += b.size();
               }
               if(count != face_coords.size()){
                  std::cout << "RASTERIZATION ERROR: diff face coords & buckets counts (should be equal...)! -> face~buckets-> " << face_coords.size() << "~" << count << '\n';
               }

               //
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
                    const float dy = maxY - minY;
                    const float z_left = std::get<1>(bucket.at(0));
                    const float z_right = std::get<1>(bucket.at(bucket.size() - 1));
                        
                    const Color c_left  = std::get<2>(bucket.at(0));
                    const Color c_right = std::get<2>(bucket.at(bucket.size() - 1)); // has no color if gone off-screen?
                    
                    //std::cout << "(minY, maxY)=" << "(" << minY << ", " << maxY << ")" << '\n';
                    //std::cout << "(minX, maxX)=" << "(" << x_left << ", " << x_right << ")" << '\n';
                    //std::cout << minY + j << '\n';
                    
                    // For each row/bucket (a row is at a certain y)
                    for(int k = 0; k < bucket.size(); k++)
                    {
                        // for each cell in that row (at that y)
                        for(int m = 0; m < dx+1; m++)
                        {
                            
                            // same factor used for both color and z
                            float factor = m / dx;
                            
                            //
                            if(dx == 0){
                                continue;
                            }
                            if(!inScreen(x_left+m, minY + j)){
                                continue;
                            }
                           
                            // 
                            const float z = ((1-factor) * z_right) + (factor * z_left); 
                            if(isVisible(Vertex{.x = std::get<0>(bucket.at(0)) + m, .y = minY + j, .z = z}))
                            {
                                Z_BUFF[std::get<0>(bucket.at(0)) + m][minY + j] = z;
                                if(mode == FILL_MODE){
                                COLOR_BUFF[x_left + m][minY + j] = (Color{                               
                                    .r = round((1-factor) * c_left.r  + factor * c_right.r),
                                    .g = round((1-factor) * c_left.g  + factor * c_right.g),
                                    .b = round((1-factor) * c_left.b  + factor * c_right.b)}
                                 );
                                }
                                if(mode == LINE_MODE)
                                {              
                                  
                                    COLOR_BUFF[x_left + m][minY + j] = (Color{                               
                                        .r = 0,
                                        .g = 0,
                                        .b = 0
                                    });
                                    //
                                
                                }
                            }
                         }
                        
                        }          
               //
                
            
           
                 // Outline with depth testing
                 if(mode == LINE_MODE)
                 {
                        
                                         
                    for(const auto& coord : face_coords){ 
                        const int x   = std::get<0>(coord); 
                        const int y   = std::get<1>(coord);

                        if(!inScreen(x, y)){
                                continue;
                        }

                        const float z = std::get<2>(coord); 
                        const Color c = std::get<3>(coord); 
                        
                        if(isVisible(Vertex {.x = x, .y = y, .z = z}))
                        {
                            Z_BUFF[x][y] = z;
                            COLOR_BUFF[x][y] = c;
                        }

                      
                    }
                 
                    
                 }
                 // No depth testing for wireframe
                 if(mode == WIREFRAME_MODE && false)
                 {
                    const Color WIREFRAME_COLOR = Color{.g = 255};
                    // TODO: this unconditionally draws the outline, no depth testing. 
                    
                    for(const auto& coord : face_coords){ 
                    COLOR_BUFF[std::get<0>(coord)][std::get<1>(coord)] = (Color{                               
                        .r = WIREFRAME_COLOR.r,
                        .g = WIREFRAME_COLOR.g,
                        .b = WIREFRAME_COLOR.b,
                      });
                    }
                 }
           
           
            
         
        }
            } 
           
    }
        }
        ;


/**
 *
 * Interpolates the depth z between v1 and v2 using lerp
 *
 * @params
 * v1 -> one end of the line
 * v2 -> the other end of the line
 *
 */

std::vector<float> interpolate_line_z(Vertex v1, Vertex v2){
            
        //
        std::vector<std::pair<int, int>> pixels = bresenham(nullptr, v1, v2);
        std::vector<float> interpolated_z{};

        for(float i = 0; i < pixels.size(); i++)
        {
            {
                /*
                *
                * [3/3 0/3] [2/3 1/3] [1/3 2/3] [ 0/3 3/3]
                *
                */
                const float line_factor = i / (pixels.size() - 1);
                const auto p = pixels.at(i);

                float z = (1-line_factor) * v1.z + line_factor * v2.z; 
                const float dx  = abs(v1.x - v2.x);
                const float dy  = abs(v1.y - v2.y);
                
                if(v1.x > v2.x && dx > dy ||  v1.y > v2.y && dy >= dx){
                    //
                    z = (1-line_factor) * v2.z + line_factor * v1.z; 
                }
                //
                interpolated_z.push_back(z); 
                
            } 
        }
        return interpolated_z;
}

/**
 *
 * Interpolates the color between v1 and v2 using lerp
 *
 * @params
 * v1 -> one end of the line
 * v2 -> the other end of the line
 *
 */
std::vector<Color> interpolate_line_color(Vertex v1, Vertex v2){
            
        //
        std::vector<std::pair<int, int>> pixels = bresenham(nullptr, v1, v2);
        std::vector<Color> interpolated_color{};

        for(float i = 0; i < pixels.size(); i++)
        {
            {
                /*
                *
                * [3/3 0/3] [2/3 1/3] [1/3 2/3] [ 0/3 3/3]
                *
                */
                const float line_factor = i / (pixels.size() - 1);
                const auto p = pixels.at(i);

                // Quadrants 6,7
                Color c = {
                        .r = round((1-line_factor) * v1.r + line_factor * v2.r), 
                        .g = round((1-line_factor) * v1.g + line_factor * v2.g), 
                        .b = round((1-line_factor) * v1.b + line_factor * v2.b), 
                    };
                
                
                const float dx  = abs(v1.x - v2.x);
                const float dy  = abs(v1.y - v2.y);
                
                // If condition found through trial and error using Bresenham test lines
                if(v1.x > v2.x && dx > dy ||  v1.y > v2.y && dy >= dx){
                    //
                    c.r = round((1-line_factor) * v2.r + line_factor * v1.r); 
                    c.g = round((1-line_factor) * v2.g + line_factor * v1.g); 
                    c.b = round((1-line_factor) * v2.b + line_factor * v1.b);
                    
                    //c.r = 255;
                    //c.g = 255;
                    //c.b = 255;
                }
                //
                interpolated_color.push_back(c); 
                
            } 
        }
        return interpolated_color;
}






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

/* 
 *
 * Render the current state of the Z_BUFFER as a depth map.
 * -      0 is full black
 * - >= 255 is full white
 *
 */
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

/*
 *
 * Calculcates the coordinates of all pixels that lie on the line connecting 2 vertices
 *
 * @params
 * p -> the  first vertex, screen coordinates
 * q -> the second vertex, screen coordinates
 * renderer -> just here for debugging, to be removed
 *
 * Does not take into account when pixels would be off screen (i.e., when p || q is off screen, the line will be partially off screen).
 */
std::vector<std::pair<int, int>> bresenham(SDL_Renderer* renderer, Vertex p, Vertex q){
  //
  std::vector<std::pair<int, int>> filled{};
  //
  float dx = p.x - q.x;
  float dy = p.y - q.y;

  // Quadrants 2,3,6,7
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
          
          //SDL_SetRenderDrawColor(renderer, q.r, q.g, q.b,SDL_ALPHA_OPAQUE); // Black
          //SDL_RenderPoint(renderer, q.x, q.y);
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

  // Quadrants 1,4,5,8
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
          //SDL_SetRenderDrawColor(renderer, q.r, q.g, q.b,SDL_ALPHA_OPAQUE); // Black
          //SDL_RenderPoint(renderer, q.x, q.y);
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
    
    
    Geometry t3 = Geometry(std::vector<Vertex>{
        {.x =    1, .y =     0, .z =  2, .w = 1, .r = 255,.g = 255, .b = 255},
        {.x =    0, .y =    -1, .z =  2, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =   -1, .y =     0, .z =  2, .w = 1, .r = 255, .g = 255, .b = 255}
    }, 3); 
    
    Geometry earth = Geometry(std::vector<Vertex>{
        {.x =    .5, .y =     .5, .z =  3, .w = 1, .r = 255},
        {.x =   -.5, .y =     .5, .z =  3, .w = 1, .r=255, .g = 255, .b = 255},
        {.x =   -.5, .y =    -.5, .z =  3, .w = 1, .b = 255},
        {.x =    .5, .y =    -.5, .z =  3, .w = 1, .g = 255}
    }, 4); 
    
    Geometry moon = Geometry(std::vector<Vertex>{
        {.x =    .1, .y =     .1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =    .1, .y =    -.1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =   -.1, .y =    -.1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255},
        {.x =   -.1, .y =     .1, .z =  5, .w = 1, .r = 255, .g = 255, .b = 255}
    }, 4); 
    
                // Q2 
                Face Q2 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0.25, .y = 0.1, .z=1, .w=1, .b = 255}
                });
                
                Face Q3 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0.25, .y = -0.1, .z=1, .w=1, .b = 255}
                });
                
                Face Q6 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = -0.25, .y = -0.1, .z=1, .w=1, .b = 255}
                });
                
                Face Q7 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = -0.25, .y = +0.1, .z=1, .w=1, .b = 255}
                });
                

                Face Q1 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0.1, .y = +0.25, .z=1, .w=1, .g = 255}
                });
                
                Face Q4 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0.1, .y = -0.25, .z=1, .w=1, .g = 255}
                        });
                
                Face Q5 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = -0.1, .y = -0.25, .z=1, .w=1, .g = 255}
                });
                Face Q8 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = -0.1, .y = +0.25, .z=1, .w=1, .g = 255}
                });

                
                Face UP = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0,    .y = +0.25, .z=1, .w=1, .g = 255, .b = 255}
                });
                Face RIGHT = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0.25,    .y = 0, .z=1, .w=1, .g = 255, .b = 255}
                });
                Face DOWN = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0,    .y = -0.25, .z=1, .w=1, .g = 255, .b = 255}
                });
                Face LEFT = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = -0.25,    .y = 0, .z=1, .w=1, .g = 255, .b = 255}
                });
                
                Face EXTRA_1 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = 0.25,    .y = 0.25, .z=1, .w=1, .g = 255, .b = 255}
                });

                Face EXTRA_2 = Face(std::vector<Vertex>{ 
                        Vertex {.x = 0,    .y = 0,   .z=1, .w=1, .r = 255},
                        Vertex {.x = -0.24,    .y = -0.25, .z=1, .w=1, .g = 255, .b = 255}
                });
                Geometry breslines(std::vector<Face>{
                        UP, RIGHT, DOWN, UP,
                        Q1,Q2,Q3,Q4,Q5,Q6,Q7,Q8,
                        EXTRA_1, EXTRA_2
                        });
                breslines.translate_geom(Vertex {.x = 0.3, .y = 0.5, .z = 0});                



    Geometry t2 = Geometry::createTriangle(std::vector<Vertex>{
        {.x =    0.5, .y =     0, .z =  2, .w = 1, .r = 255},
        {.x =    0, .y =    -0.5, .z =  2, .w = 1, .g = 255},
        {.x =   -0.5, .y =     0, .z =  2, .w = 1, .b = 255}
    }); 
    //objects.push_back(t2);
    
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
                //earth.translate_geom(Vertex {.x = 0.0, .y = -0.1, .z = 0});                
                

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

                
                // look at origin print, the x-values randomly jump. Renormalize matrix.
                //  https://stackoverflow.com/questions/23080791/eigen-re-orthogonalization-of-rotation-matrix
                t2.rotate_geom(1, Z_AXIS);                
                //t2.translate_geom(Vertex {.x = 0.1, .y = 0.0, .z = 0});                
                 //t1.rotate_geom(5, Z_AXIS);                
                
                 //t3.translate_geom(Vertex {.x = 0.1, .y = 0.0, .z = 0});                
                 
                 render(renderer, t2, FILL_MODE);
                 render(renderer, t3, FILL_MODE);

                
                //breslines.rotate_geom(1, Z_AXIS);                
                //render(renderer, breslines, LINE_MODE);
                
            /* Buffer functions */
                render_COLOR_BUFF(renderer); // Slowest function we have, but it is constant in the amount of geometry.
                //render_Z_BUFF(renderer);
                reset_Z_BUFF();
                reset_COLOR_BUFF();
                
            SDL_RenderPresent(renderer);
            SDL_Delay(1000 / FPS);
    }


    // cleanup
    SDL_Quit();
    return 0;
}



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




