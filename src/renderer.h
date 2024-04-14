#pragma once

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <string>

namespace RenderEngine {

	struct mat4;
	struct vec3{
		double x, y, z;
		vec3(double n = 0){ x = y = z = n; }
		vec3(double x, double y, double z){
			this->x = x;
			this->y = y;
			this->z = z;
		}
		vec3 normalize();
		vec3 cross_product(vec3 v2);
		float dot_product(vec3 v2);
		float magnitude();
		vec3 operator-(vec3 i);
		void operator-=(vec3 i);
		vec3 operator+(vec3 i);
		void operator+=(vec3 i);
		vec3 operator*(double k);
		void operator*=(double k);
		vec3 operator/(float k);
		void operator/=(float k);
		vec3 operator*(mat4 mat);
		void operator*=(mat4 mat);
	};
	struct quaternion{
		double w = 0;
		double x = 0;
		double y = 0;
		double z = 0;
		quaternion(){ }
		quaternion(double angle, vec3 axis){
			this->w = cos(angle / 2);
			this->x = axis.x * sin(angle / 2);
			this->y = axis.y * sin(angle / 2);
			this->z = axis.z * sin(angle / 2);
		}
		quaternion operator *(quaternion q2);
		void operator *=(quaternion q2);
		mat4 quaternion_to_rotation_matrix();
	};
	struct triangle{
		vec3 point[3];
		vec3 colour;
		vec3 normal;
		void calculate_normal();
	};
	struct mesh{
		std::vector<triangle> tris;
		int triangle_count = 0;
		bool loaded = 0;
		vec3 position = { 0, 0, 0 };
		vec3 rotation = { 0, 0, 0 };
		vec3 scale = { 1, 1, 1 };
		void calculate_normals();
		bool load_mesh_from_file(std::string sFilename);
	};
	struct mat4{
		mat4(){  }
		mat4(float m[4][4]){
			this->m[0][0] = m[0][0];
			this->m[0][1] = m[0][1];
			this->m[0][2] = m[0][2];
			this->m[0][3] = m[0][3];
			this->m[1][0] = m[1][0];
			this->m[1][1] = m[1][1];
			this->m[1][2] = m[1][2];
			this->m[1][3] = m[1][3];
			this->m[2][0] = m[2][0];
			this->m[2][1] = m[2][1];
			this->m[2][2] = m[2][2];
			this->m[2][3] = m[2][3];
			this->m[3][0] = m[3][0];
			this->m[3][1] = m[3][1];
			this->m[3][2] = m[3][2];
			this->m[3][3] = m[3][3];
		}
		float m[4][4] = { 0 };
		mat4 operator *(mat4 mat);
		void operator *=(mat4 mat);
		vec3 operator *(vec3 v);
	};
	struct CycleContext{
		std::vector<triangle> buffer1;
		std::vector<triangle> buffer2;
	};

	void init();
	GLuint update(float width, float height);

	void draw_triangle(triangle tri);
	void draw_wireframe(triangle tri);
	mat4 make_projection_mat();
	vec3 vector_intersect_plane(vec3 &plane_p, vec3 &plane_n, vec3 &lineStart, vec3 &lineEnd);
	int triangle_clip_against_plane(vec3 plane_p, vec3 plane_n, triangle &in_tri, triangle &out_tri1, triangle &out_tri2);
	mat4 make_rotation_matX(float fAngleRad);
	mat4 make_rotation_matY(float fAngleRad);
	mat4 make_rotation_matZ(float fAngleRad);
	mat4 make_translation_mat(vec3 pos);
	mat4 matrix_point_at(vec3 pos, vec3 target, vec3 up);
	mat4 matrix_quick_inverse(mat4& m);
	void prepare();
	void rescale_framebuffer(float width, float height);
	std::vector<triangle> thread_lighting(mat4 mat_view, mat4 mat_proj);
	std::vector<triangle> thread_clip(std::vector<triangle> queue);
	std::vector<triangle> thread_sort(std::vector<triangle> queue);
	void io();
};




extern RenderEngine::mesh default_mesh;
extern RenderEngine::vec3 light;
extern RenderEngine::vec3 camera;
extern RenderEngine::vec3 look_dir;
extern RenderEngine::vec3 background_color;
extern RenderEngine::vec3 delta_mouse;
extern RenderEngine::vec3 new_look_dir;

extern int fbo_width;
extern int fbo_height;
extern int frame_rate;
extern float mouse_sens;

extern bool draw_wireframe_bool;
extern bool viewport_focused;
extern bool orthographic;
extern bool move_forward;
extern bool move_backward;