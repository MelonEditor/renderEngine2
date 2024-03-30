
#include "renderer.h"
#include <fstream>
#include <strstream>
#include <vector>
#include <list>
#include <future>
#include <chrono>
#include <Windows.h>

int fbo_width = 256;
int fbo_height = 256;

int frame_rate = 0;
int frame_rate_counter = 0;
int target_fps = 60;

std::chrono::steady_clock::time_point fps_counter_start; 

GLuint fbo, color;

using namespace RenderEngine;

vec3 light = { 0.5, 0.5, 0 };
vec3 camera = {0, 0, 0};
vec3 look_dir;
vec3 background_color = { 0.1, 0.1, 0.1 };

mesh default_mesh;
std::future<void> world;

float fyaw = 0;

bool draw_wireframe_bool = false;
bool viewport_focused = false;
bool orthographic = false;
bool move_forward = false;
bool move_backward = false;


void RenderEngine::init(){
	glGenFramebuffers(1, &fbo);
	glGenTextures(1, &color);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glBindTexture(GL_TEXTURE_2D, color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbo_width, fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	world = std::async(std::launch::async, [](){ default_mesh.load_mesh_from_file("resources\\terrain3.obj"); } );
	fps_counter_start = std::chrono::steady_clock::now();
}
void RenderEngine::prepare(){
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_TEXTURE_2D);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glClearColor(background_color.x, background_color.y, background_color.z, 1);
	glClear(GL_COLOR_BUFFER_BIT);
}
void RenderEngine::rescale_framebuffer(float width, float height){
	fbo_width  = width;
	fbo_height = height;

	glBindTexture(GL_TEXTURE_2D, color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbo_width, fbo_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);

}
std::mutex mtx;
std::vector<triangle> RenderEngine::thread_lighting(mat4 mat_view, mat4 mat_proj){
	std::vector<triangle> queue;
	if(!default_mesh.loaded){ return queue; }
	for(triangle tri : default_mesh.tris){

		//tri.point[0] = world * tri.point[0];
		//tri.point[1] = world * tri.point[1];
		//tri.point[2] = world * tri.point[2];

		tri.calculate_normal();
		tri.normal.normalize();

		// Get Ray from triangle to camera
		vec3 vCameraRay = tri.point[0] - camera;

		// If ray is aligned with normal, then triangle is visible
		if(tri.normal.dot_product(vCameraRay) < 0.0f && look_dir.dot_product(vCameraRay) > 0.0f){

			tri.colour = vec3(tri.normal.dot_product(light)) * 1.0f;

			tri.point[0] = mat_view * tri.point[0];
			tri.point[1] = mat_view * tri.point[1];
			tri.point[2] = mat_view * tri.point[2];

			// Clip Viewed Triangle against near plane, this could form two additional triangles. 
			triangle clipped[2];
			int nClippedTriangles = triangle_clip_against_plane({0.0f, 0.0f, 0.1f}, {0.0f, 0.0f, 1.0f}, tri, clipped[0], clipped[1]);

			// We may end up with multiple triangles form the clip, so project as required
			for(int n = 0; n < nClippedTriangles; n++){
				// Project triangles from 3D --> 2D
				if(!orthographic){ 
					tri.point[0] = mat_proj * clipped[n].point[0];
					tri.point[1] = mat_proj * clipped[n].point[1];
					tri.point[2] = mat_proj * clipped[n].point[2];
				}

				tri.calculate_normal();
				tri.normal.normalize();
				tri.colour = clipped[n].colour;

				// Scale into view
				tri.point[0].x -= 1.0f;
				tri.point[1].x -= 1.0f;
				tri.point[2].x -= 1.0f;

				tri.point[0].x *= (float)fbo_height / (float)fbo_width;
				tri.point[1].x *= (float)fbo_height / (float)fbo_width;
				tri.point[2].x *= (float)fbo_height / (float)fbo_width;

				tri.point[0].x *= 720.0f / 1280.0f;
				tri.point[1].x *= 720.0f / 1280.0f;
				tri.point[2].x *= 720.0f / 1280.0f;

				// Store triangle for sorting
				queue.push_back(tri);
			}	
		}
	}
	return queue;
}

std::vector<triangle> RenderEngine::thread_clip(std::vector<triangle> queue){
	if(queue.size() == 0){ return queue; }
	std::vector<triangle> draw_order;
	// Loop through all transformed, viewed, projected, and sorted triangles
	for(triangle &tri : queue) {


		// Clip triangles against all four screen edges, this could yield
		// a bunch of triangles, so create a queue that we traverse to 
		//  ensure we only test new triangles generated against planes
		triangle clipped[2];
		std::list<triangle> listTriangles;

		// Add initial triangle
		listTriangles.push_back(tri);
		int nNewTriangles = 1;

		for(int p = 0; p < 4; p++){
			int nTrisToAdd = 0;
			while(nNewTriangles > 0){
				// Take triangle from front of queue
				triangle test = listTriangles.front();
				listTriangles.pop_front();
				nNewTriangles--;

				switch(p){
				case 0:	nTrisToAdd = triangle_clip_against_plane({ 0.0f, -1.0f, 0.0f },	{ 0.0f, 1.0f, 0.0f },  test, clipped[0], clipped[1]); break;
				case 1:	nTrisToAdd = triangle_clip_against_plane({ 0.0f, 1.0f, 0.0f },	{ 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
				case 2:	nTrisToAdd = triangle_clip_against_plane({ -1.0f, 0.0f, 0.0f },	{ 1.0f, 0.0f, 0.0f },  test, clipped[0], clipped[1]); break;
				case 3:	nTrisToAdd = triangle_clip_against_plane({ 1.0f, 0.0f, 0.0f },	{ -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
				}

				// Clipping may yield a variable number of triangles, so
				// add these new ones to the back of the queue for subsequent
				// clipping against next planes
				for(int w = 0; w < nTrisToAdd; w++){ listTriangles.push_back(clipped[w]); }
			}
			nNewTriangles = listTriangles.size();
		}
		// Draw the transformed, viewed, clipped, projected, sorted, clipped triangles
		for(triangle &t : listTriangles){
			draw_order.push_back(t);
		}
	}
	return draw_order;
}
std::vector<triangle> RenderEngine::thread_sort(std::vector<triangle> queue){
	if(queue.size() == 0){ return queue; }
	sort(queue.begin(), queue.end(), [](triangle& t1, triangle& t2){
		float z1 = t1.point[0].z + t1.point[1].z + t1.point[2].z;
		float z2 = t2.point[0].z + t2.point[1].z + t2.point[2].z;
		return z1 > z2;
	});
	return queue;
}
CycleContext cc;

GLuint RenderEngine::update(float width, float height){
	std::chrono::steady_clock::time_point mspf_counter_start = std::chrono::steady_clock::now(); 

	//calculate fps
	int duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - fps_counter_start).count();
	if(duration >= 1000){ 
		frame_rate = frame_rate_counter; 
		frame_rate_counter = 0; 
		fps_counter_start = std::chrono::steady_clock::now();
	}
	io();
	rescale_framebuffer(width, height);
	prepare();
	
	std::vector<triangle> draw_order;

	//light.normalize();

	//mat4 world;
	//world = make_rotation_matX(default_mesh.rotation.x);
	//world *= make_rotation_matY(default_mesh.rotation.y);
	//world *= make_rotation_matZ(default_mesh.rotation.z);
	//world *= make_translation_mat(default_mesh.position);
	
	vec3 up_vec = { 0, 1, 0 };
	vec3 target_vec = { 0, 0, 1 };

	if(move_forward ){ camera = camera + look_dir; move_forward  = false; }
	if(move_backward){ camera = camera - look_dir; move_backward = false; }
	
	look_dir = make_rotation_matY(fyaw) * target_vec;
	target_vec = camera + look_dir;

	mat4 matCamera = matrix_point_at(camera, target_vec, up_vec);
	mat4 mat_view = matrix_quick_inverse(matCamera);
	mat4 mat_proj = make_projection_mat();

	std::future<std::vector<triangle>> t1 = std::async(std::launch::async, thread_lighting, mat_view, mat_proj);
	//std::future<std::vector<triangle>> t2 = std::async(std::launch::async, thread_clip, cc.buffer1);
	std::future<std::vector<triangle>> t2 = std::async(std::launch::async, thread_sort, cc.buffer1);

	draw_order = t2.get();
	cc.buffer1 = t1.get();

	
	//actually drawing the triangles on the buffer
	for(triangle &t : draw_order){
		draw_triangle(t);
		if(draw_wireframe_bool){ draw_wireframe(t); };
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	++frame_rate_counter;

	//cap fps to target fps
	int frame_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - mspf_counter_start).count();
	if(frame_duration < 1000 / target_fps){ Sleep((1000 / target_fps) - frame_duration); }
	
	return color;
}

vec3 RenderEngine::vector_intersect_plane(vec3 &plane_p, vec3 &plane_n, vec3 &lineStart, vec3 &lineEnd){
	plane_n.normalize();
	float plane_d = - plane_n.dot_product(plane_p);
	float ad = lineStart.dot_product(plane_n);
	float bd = lineEnd.dot_product(plane_n);
	float t = (-plane_d - ad) / (bd - ad);
	vec3 lineStartToEnd = lineEnd - lineStart;
	vec3 lineToIntersect = lineStartToEnd * t;
	return lineStart + lineToIntersect;
}
int RenderEngine::triangle_clip_against_plane(vec3 plane_p, vec3 plane_n, triangle &in_tri, triangle &out_tri1, triangle &out_tri2){
	plane_n.normalize();

	// Return signed shortest distance from point to plane, plane normal must be normalised
	auto dist = [&](vec3 &p){
		vec3 n = p;
		n.normalize();
		return (plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - plane_n.dot_product(plane_p));
	};

	// Create two temporary storage arrays to classify points either side of plane
	// If distance sign is positive, point lies on "inside" of plane
	vec3* inside_points[3];  int nInsidePointCount = 0;
	vec3* outside_points[3]; int nOutsidePointCount = 0;

	// Get signed distance of each point in triangle to plane
	float d0 = dist(in_tri.point[0]);
	float d1 = dist(in_tri.point[1]);
	float d2 = dist(in_tri.point[2]);

	if(d0 >= 0) { inside_points[nInsidePointCount++]	= &in_tri.point[0]; }
	else		{ outside_points[nOutsidePointCount++]	= &in_tri.point[0]; }
	if(d1 >= 0) { inside_points[nInsidePointCount++]	= &in_tri.point[1]; }
	else		{ outside_points[nOutsidePointCount++]	= &in_tri.point[1]; }
	if(d2 >= 0) { inside_points[nInsidePointCount++]	= &in_tri.point[2]; }
	else		{ outside_points[nOutsidePointCount++]	= &in_tri.point[2]; }

	// Now classify triangle points, and break the input triangle into 
	// smaller output triangles if required. There are four possible
	// outcomes...

	// All points lie on the outside of plane, so clip whole triangle
	if(nInsidePointCount == 0){ return 0; }

	// All points lie on the inside of plane, so do nothing and allow the triangle to simply pass through
	if(nInsidePointCount == 3){
		out_tri1 = in_tri;
		return 1;
	}

	// Triangle should be clipped. As two points lie outside the plane, the triangle simply becomes a smaller triangle
	if(nInsidePointCount == 1 && nOutsidePointCount == 2){
		out_tri1.colour = in_tri.colour;

		out_tri1.point[0] = *inside_points[0];

		out_tri1.point[1] = vector_intersect_plane(plane_p, plane_n, *inside_points[0], *outside_points[0]);
		out_tri1.point[2] = vector_intersect_plane(plane_p, plane_n, *inside_points[0], *outside_points[1]);

		return 1;
	}

	// Triangle should be clipped. As two points lie inside the plane, the clipped triangle splits into 2 new triangles. 
	if(nInsidePointCount == 2 && nOutsidePointCount == 1){
		out_tri1.colour =  in_tri.colour;
		out_tri2.colour =  in_tri.colour;

		// The first triangle consists of the two inside points and a new
		// point determined by the location where one side of the triangle
		// intersects with the plane
		out_tri1.point[0] = *inside_points[0];
		out_tri1.point[1] = *inside_points[1];
		out_tri1.point[2] = vector_intersect_plane(plane_p, plane_n, *inside_points[0], *outside_points[0]);

		// The second triangle is composed of one of he inside points, a
		// new point determined by the intersection of the other side of the 
		// triangle and the plane, and the newly created point above
		out_tri2.point[0] = *inside_points[1];
		out_tri2.point[1] = out_tri1.point[2];
		out_tri2.point[2] = vector_intersect_plane(plane_p, plane_n, *inside_points[1], *outside_points[0]);

		return 2;
	}
	return -1;
}

mat4 RenderEngine::make_rotation_matX(float fAngleRad){
	mat4 matrix;
	matrix.m[0][0] = 1.0f;
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[1][2] = sinf(fAngleRad);
	matrix.m[2][1] = -sinf(fAngleRad);
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}
mat4 RenderEngine::make_rotation_matY(float fAngleRad){
	mat4 matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][2] = sinf(fAngleRad);
	matrix.m[2][0] = -sinf(fAngleRad);
	matrix.m[1][1] = 1.0f;
	matrix.m[2][2] = cosf(fAngleRad);
	matrix.m[3][3] = 1.0f;
	return matrix;
}
mat4 RenderEngine::make_rotation_matZ(float fAngleRad){
	mat4 matrix;
	matrix.m[0][0] = cosf(fAngleRad);
	matrix.m[0][1] = sinf(fAngleRad);
	matrix.m[1][0] = -sinf(fAngleRad);
	matrix.m[1][1] = cosf(fAngleRad);
	matrix.m[2][2] = 1.0f;
	matrix.m[3][3] = 1.0f;
	return matrix;
}
mat4 RenderEngine::make_translation_mat(vec3 pos){
	mat4 matrix;
	matrix.m[0][0] = 1;
	matrix.m[1][1] = 1;
	matrix.m[2][2] = 1;
	matrix.m[3][3] = 1;
	matrix.m[3][0] = pos.x;
	matrix.m[3][1] = pos.y;
	matrix.m[3][2] = pos.z;
	return matrix;
}
mat4 RenderEngine::make_projection_mat(){
	mat4 projection_mat;
	float fNear = 0.1f;
	float fFar = 1000.0f;
	float fFov = 90.0f;
	float fAspectRatio = (float)fbo_width / (float)fbo_height;
	float fFovRad = 1.0f / tanf(fFov * 0.5f / 180.0f * 3.14159f);

	projection_mat.m[0][0] = fAspectRatio * fFovRad;
	projection_mat.m[1][1] = fFovRad;
	projection_mat.m[2][2] = fFar / (fFar - fNear);
	projection_mat.m[3][2] = (-fFar * fNear) / (fFar - fNear);
	projection_mat.m[2][3] = 1.0f;
	projection_mat.m[3][3] = 0.0f;
	return projection_mat;
}
mat4 RenderEngine::matrix_point_at(vec3& pos, vec3& target, vec3& up){
	vec3 newForward = target - pos;
	newForward.normalize();

	vec3 a = newForward * (up.dot_product(newForward));
	vec3 newUp = up - a;
	newUp.normalize();

	vec3 newRight = newUp.cross_product(newForward);

	mat4 matrix;
	matrix.m[0][0] = newRight.x;	matrix.m[0][1] = newRight.y;	matrix.m[0][2] = newRight.z;	matrix.m[0][3] = 0.0f;
	matrix.m[1][0] = newUp.x;		matrix.m[1][1] = newUp.y;		matrix.m[1][2] = newUp.z;		matrix.m[1][3] = 0.0f;
	matrix.m[2][0] = newForward.x;	matrix.m[2][1] = newForward.y;	matrix.m[2][2] = newForward.z;	matrix.m[2][3] = 0.0f;
	matrix.m[3][0] = pos.x;			matrix.m[3][1] = pos.y;			matrix.m[3][2] = pos.z;			matrix.m[3][3] = 1.0f;
	return matrix;

}
mat4 RenderEngine::matrix_quick_inverse(mat4& m){
	mat4 matrix;
	matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
	matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
	matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
	matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
	matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
	matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
	matrix.m[3][3] = 1.0f;
	return matrix;
}
void RenderEngine::draw_triangle(triangle tri){
	glLineWidth(3.0f);
	glColor4f(tri.colour.x, tri.colour.y, tri.colour.z, 1.0f);
	glBegin(GL_TRIANGLES);
	glVertex2d(tri.point[0].x, tri.point[0].y);
	glVertex2d(tri.point[1].x, tri.point[1].y);
	glVertex2d(tri.point[2].x, tri.point[2].y);
	glEnd();
}
void RenderEngine::draw_wireframe(triangle tri) {
	glLineWidth(0.01f);
	glColor4f(0.7f, 0.7f, 0.7f, 1.0f);
	glBegin(GL_LINE_LOOP);
	glVertex2d(tri.point[0].x, tri.point[0].y);
	glVertex2d(tri.point[1].x, tri.point[1].y);
	glVertex2d(tri.point[2].x, tri.point[2].y);
	glEnd();
}

bool mesh::load_mesh_from_file(std::string sFilename){
	std::ifstream f;
	f.open(sFilename);
	if(!f){ return false; }

	std::vector<vec3> verts;

	while(!f.eof()){
		char line[128];
		f.getline(line, 128);

		std::strstream s;
		s << line;

		char junk;
		if(line[0] == 'v'){
			vec3 v;
			s >> junk >> v.x >> v.y >> v.z;
			verts.push_back(v);
		}
		if(line[0] == 'f'){
			int f[3];
			s >> junk >> f[0] >> f[1] >> f[2];
			tris.push_back({ verts[f[0] - 1], verts[f[1] - 1], verts[f[2] - 1] });
		}
	}
	f.close();
	triangle_count = tris.size();
	loaded = true;
	return true;
}

mat4 mat4::operator *(mat4 mat){
	mat4 matrix;
	for(int c = 0; c < 4; c++){
		for(int r = 0; r < 4; r++){
			matrix.m[r][c] = this->m[r][0] * mat.m[0][c] + this->m[r][1] * mat.m[1][c] + this->m[r][2] * mat.m[2][c] + this->m[r][3] * mat.m[3][c];
		}
	}
	return matrix;
}
void mat4::operator *=(mat4 mat){
	mat4 matrix;
	for(int c = 0; c < 4; c++){
		for(int r = 0; r < 4; r++){
			matrix.m[r][c] = this->m[r][0] * mat.m[0][c] + this->m[r][1] * mat.m[1][c] + this->m[r][2] * mat.m[2][c] + this->m[r][3] * mat.m[3][c];
		}
	}
	*this = matrix;
}
vec3 mat4::operator *(vec3 v){
	vec3 o;
	o.x = v.x * this->m[0][0] + v.y * this->m[1][0] + v.z * this->m[2][0] + this->m[3][0];
	o.y = v.x * this->m[0][1] + v.y * this->m[1][1] + v.z * this->m[2][1] + this->m[3][1];
	o.z = v.x * this->m[0][2] + v.y * this->m[1][2] + v.z * this->m[2][2] + this->m[3][2];
	float w = v.x * this->m[0][3] + v.y * this->m[1][3] + v.z * this->m[2][3] + this->m[3][3];
	
	if(w != 0.0f){
		o.x /= w; o.y /= w; o.z /= w;
	}
	return o;
}


void mesh::calculate_normals(){
	for(triangle i : tris){
		i.calculate_normal();
		i.normal.normalize();
	}
}
void triangle::calculate_normal(){
	vec3 v1 = point[0] - point[1];
	vec3 v2 = point[0] - point[2];
	this->normal = v1.cross_product(v2);
}

float vec3::dot_product(vec3 v2){
	return this->x * v2.x + this->y * v2.y + this->z * v2.z;
}
vec3 vec3::cross_product(vec3 v2){
	vec3 res;
	res.x = this->y * v2.z - this->z * v2.y;
	res.y = this->z * v2.x - this->x * v2.z;
	res.z = this->x * v2.y - this->y * v2.x;
	return res;
}
void vec3::normalize(){
	float l = sqrtf(this->x * this->x + this->y * this->y + this->z * this->z);
	this->x /= l; this->y /= l; this->z /= l;
}
vec3 vec3::operator-(vec3 i){
	return vec3(this->x - i.x, this->y - i.y, this->z - i.z);
}
void vec3::operator-=(vec3 i){
	this->x -= i.x;
	this->y -= i.y;
	this->z -= i.z;
}
vec3 vec3::operator+(vec3 i){
	return vec3(this->x + i.x, this->y + i.y, this->z + i.z);
}
void vec3::operator+=(vec3 i){
	this->x += i.x;
	this->y += i.y;
	this->z += i.z;
}
vec3 vec3::operator*(double k){
	return { (double)this->x * k, (double)this->y * k, (double)this->z * k };
}
void vec3::operator*=(double k){
	 this->x *= k;
	 this->y *= k;
	 this->z *= k;
}
vec3 vec3::operator/(float k){
	return { this->x / k, this->y / k, this->z / k };
}
void vec3::operator/=(float k){
	 this->x /= k;
	 this->y /= k;
	 this->z /= k;
}
vec3 vec3::operator*(mat4 mat){
	vec3 o;
	o.x = this->x * mat.m[0][0] + this->y * mat.m[1][0] + this->z * mat.m[2][0] + mat.m[3][0];
	o.y = this->x * mat.m[0][1] + this->y * mat.m[1][1] + this->z * mat.m[2][1] + mat.m[3][1];
	o.z = this->x * mat.m[0][2] + this->y * mat.m[1][2] + this->z * mat.m[2][2] + mat.m[3][2];
	float w = this->x * mat.m[0][3] + this->y * mat.m[1][3] + this->z * mat.m[2][3] + mat.m[3][3];

	if(w != 0.0f){ o.x /= w; o.y /= w; o.z /= w; }
	return o;
}
void vec3::operator*=(mat4 mat){
	vec3 o;
	o.x = this->x * mat.m[0][0] + this->y * mat.m[1][0] + this->z * mat.m[2][0] + mat.m[3][0];
	o.y = this->x * mat.m[0][1] + this->y * mat.m[1][1] + this->z * mat.m[2][1] + mat.m[3][1];
	o.z = this->x * mat.m[0][2] + this->y * mat.m[1][2] + this->z * mat.m[2][2] + mat.m[3][2];
	float w = this->x * mat.m[0][3] + this->y * mat.m[1][3] + this->z * mat.m[2][3] + mat.m[3][3];

	if(w != 0.0f){ o.x /= w; o.y /= w; o.z /= w; }
	*this = o;
}
