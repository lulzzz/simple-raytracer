#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>

const double PI = 3.14159265358979323846;
const double INF = 1e20;
const double EPS = 1e-6;
const double MaxDepth = 5;

// *** ���̑��̊֐� ***
inline double rand01() { return (double)rand()/RAND_MAX; }

// *** �f�[�^�\�� ***
struct Vec {
	double x, y, z;
	Vec(const double x_ = 0, const double y_ = 0, const double z_ = 0) : x(x_), y(y_), z(z_) {}
	inline Vec operator+(const Vec &b) const {return Vec(x + b.x, y + b.y, z + b.z);}
	inline Vec operator-(const Vec &b) const {return Vec(x - b.x, y - b.y, z - b.z);}
	inline Vec operator*(const double b) const {return Vec(x * b, y * b, z * b);}
	inline Vec operator/(const double b) const {return Vec(x / b, y / b, z / b);}
	inline const double LengthSquared() const { return x*x + y*y + z*z; }
	inline const double Length() const { return sqrt(LengthSquared()); }
};
inline Vec operator*(double f, const Vec &v) { return v * f; }
inline Vec Normalize(const Vec &v) { return v / v.Length(); }
// �v�f���Ƃ̐ς��Ƃ�
inline const Vec Multiply(const Vec &v1, const Vec &v2) {
	return Vec(v1.x * v2.x, v1.y * v2.y, v1.z * v2.z);
}
inline const double Dot(const Vec &v1, const Vec &v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}
inline const Vec Cross(const Vec &v1, const Vec &v2) {
	return Vec((v1.y * v2.z) - (v1.z * v2.y), (v1.z * v2.x) - (v1.x * v2.z), (v1.x * v2.y) - (v1.y * v2.x));
}
typedef Vec Color;
const Color BackgroundColor(0.0, 0.0, 0.0);

struct Ray {
	Vec org, dir;
	Ray(const Vec org_, const Vec &dir_) : org(org_), dir(dir_) {}
};

enum ReflectionType {
	DIFFUSE,    // ���S�g�U�ʁB������Lambertian�ʁB
	SPECULAR,   // ���z�I�ȋ��ʁB
	REFRACTION, // ���z�I�ȃK���X�I�����B
};

struct Sphere {
	double radius;
	Vec position;
	Color emission, color;
	ReflectionType ref_type;

	Sphere(const double radius_, const Vec &position_, const Color &emission_, const Color &color_, const ReflectionType ref_type_) :
	  radius(radius_), position(position_), emission(emission_), color(color_), ref_type(ref_type_) {}
	// ���͂�ray�ɑ΂�������_�܂ł̋�����Ԃ��B�������Ȃ�������0��Ԃ��B
	const double intersect(const Ray &ray) {
		Vec o_p = position - ray.org;
		const double b = Dot(o_p, ray.dir), det = b * b - Dot(o_p, o_p) + radius * radius;
		if (det >= 0.0) {
			const double sqrt_det = sqrt(det);
			const double t1 = b - sqrt_det, t2 = b + sqrt_det;
			if (t1 > EPS)		return t1;
			else if(t2 > EPS)	return t2;
		}
		return 0.0;
	}
};

// *** �����_�����O����V�[���f�[�^ ****
// from small ppt
Sphere spheres[] = {
	Sphere(1e5, Vec( 1e5+1,40.8,81.6), Color(), Color(0.75, 0.25, 0.25),DIFFUSE),// ��
	Sphere(1e5, Vec(-1e5+99,40.8,81.6),Color(), Color(0.25, 0.25, 0.75),DIFFUSE),// �E
	Sphere(1e5, Vec(50,40.8, 1e5),     Color(), Color(0.75, 0.75, 0.75),DIFFUSE),// ��
	Sphere(1e5, Vec(50,40.8,-1e5+170), Color(), Color(), DIFFUSE),// ��O
	Sphere(1e5, Vec(50, 1e5, 81.6),    Color(), Color(0.75, 0.75, 0.75),DIFFUSE),// ��
	Sphere(1e5, Vec(50,-1e5+81.6,81.6),Color(), Color(0.75, 0.75, 0.75),DIFFUSE),// �V��
	Sphere(16.5,Vec(27,16.5,47),       Color(), Color(1,1,1)*.99, SPECULAR),// ��
	Sphere(16.5,Vec(73,16.5,78),       Color(), Color(1,1,1)*.99, REFRACTION),//�K���X
};

// �_����
const Vec light_pos(50.0, 75.0, 81.6);
const Color light_color(256,256,256);

// *** �����_�����O�p�֐� ***
// �V�[���Ƃ̌�������֐�
inline bool intersect_scene(const Ray &ray, double *t, int *id) {
	const double n = sizeof(spheres) / sizeof(Sphere);
	*t  = INF;
	*id = -1;
	for (int i = 0; i < int(n); i ++) {
		double d = spheres[i].intersect(ray);
		if (d > 0.0 && d < *t) {
			*t  = d;
			*id = i;
		}
	}
	return *t < INF;
}

// ray��������̕��ˋP�x�����߂�
Color radiance(const Ray &ray, const int depth) {
	double t; // ���C����V�[���̌����ʒu�܂ł̋���
	int id;   // ���������V�[�����I�u�W�F�N�g��ID
	if (!intersect_scene(ray, &t, &id))
		return BackgroundColor;

	const Sphere &obj = spheres[id];
	const Vec hitpoint = ray.org + t * ray.dir; // �����ʒu
	const Vec normal  = Normalize(hitpoint - obj.position); // �����ʒu�̖@��
	const Vec orienting_normal = Dot(normal, ray.dir) < 0.0 ? normal : (-1.0 * normal); // �����ʒu�̖@���i���̂���̃��C�̓��o���l���j

	if (depth > MaxDepth)
		return Color();

	switch (obj.ref_type) {
	case DIFFUSE: {
		// ������
		double t_;
		int id_;
		const Vec ldir = light_pos - hitpoint;
		const double len = ldir.Length();
		intersect_scene(Ray(hitpoint, ldir / len), &t_, &id_);
		if (t_ >= len) {
			// Lambert��
			return obj.emission + std::max(0.0, Dot(orienting_normal, ldir / len)) * Multiply(light_color, obj.color) / (len * len);
		} else {
			return Color();
		}
	} break;
	case SPECULAR: {
		// ���S���ʂȂ̂Ń��C�̔��˕����͌���I�B
		// ���V�A�����[���b�g�̊m���ŏ��Z����̂͏�Ɠ����B
		return obj.emission + Multiply(obj.color,
			radiance(Ray(hitpoint, ray.dir - normal * 2.0 * Dot(normal, ray.dir)), depth+1));
	} break;
	case REFRACTION: {
		Ray reflection_ray = Ray(hitpoint, ray.dir - normal * 2.0 * Dot(normal, ray.dir));
		bool into = Dot(normal, orienting_normal) > 0.0; // ���C���I�u�W�F�N�g����o��̂��A����̂�

		// Snell�̖@��
		const double nc = 1.0; // �^��̋��ܗ�
		const double nt = 1.5; // �I�u�W�F�N�g�̋��ܗ�
		const double nnt = into ? nc / nt : nt / nc;
		const double ddn = Dot(ray.dir, orienting_normal);
		const double cos2t = 1.0 - nnt * nnt * (1.0 - ddn * ddn);
		
		if (cos2t < 0.0) { // �S���˂���
			return obj.emission + Multiply(obj.color, (radiance(reflection_ray, depth+1)));
		}
		// ���܂��Ă�������
		Vec tdir = Normalize(ray.dir * nnt - normal * (into ? 1.0 : -1.0) * (ddn * nnt + sqrt(cos2t)));

		// Schlick�ɂ��Fresnel�̔��ˌW���̋ߎ�
		const double a = nt - nc, b = nt + nc;
		const double R0 = (a * a) / (b * b);
		const double c = 1.0 - (into ? -ddn : Dot(tdir, normal));
		const double Re = R0 + (1.0 - R0) * pow(c, 5.0);
		const double Tr = 1.0 - Re; // ���܌��̉^�Ԍ��̗�
		const double probability  = 0.25 + 0.5 * Re;

		return obj.emission + 
			Multiply(obj.color, radiance(reflection_ray, depth+1) * Re + radiance(Ray(hitpoint, tdir), depth+1) * Tr);
	} break;
	}
}


// *** .hdr�t�H�[�}�b�g�ŏo�͂��邽�߂̊֐� ***
struct HDRPixel {
	unsigned char r, g, b, e;
	HDRPixel(const unsigned char r_ = 0, const unsigned char g_ = 0, const unsigned char b_ = 0, const unsigned char e_ = 0) :
	r(r_), g(g_), b(b_), e(e_) {};
	unsigned char get(int idx) {
		switch (idx) {
		case 0: return r;
		case 1: return g;
		case 2: return b;
		case 3: return e;
		} return 0;
	}

};

// double��RGB�v�f��.hdr�t�H�[�}�b�g�p�ɕϊ�
HDRPixel get_hdr_pixel(const Color &color) {
	double d = std::max(color.x, std::max(color.y, color.z));
	if (d <= 1e-32)
		return HDRPixel();
	int e;
	double m = frexp(d, &e); // d = m * 2^e
	d = m * 256.0 / d;
	return HDRPixel(color.x * d, color.y * d, color.z * d, e + 128);
}

// �����o���p�֐�
void save_hdr_file(const std::string &filename, const Color* image, const int width, const int height) {
	FILE *fp = fopen(filename.c_str(), "wb");
	if (fp == NULL) {
		std::cerr << "Error: " << filename << std::endl;
		return;
	}
	// .hdr�t�H�[�}�b�g�ɏ]���ăf�[�^����������
	// �w�b�_
	unsigned char ret = 0x0a;
	fprintf(fp, "#?RADIANCE%c", (unsigned char)ret);
	fprintf(fp, "# Made with 100%% pure HDR Shop%c", ret);
	fprintf(fp, "FORMAT=32-bit_rle_rgbe%c", ret);
	fprintf(fp, "EXPOSURE=1.0000000000000%c%c", ret, ret);

	// �P�x�l�����o��
	fprintf(fp, "-Y %d +X %d%c", height, width, ret);
	for (int i = height - 1; i >= 0; i --) {
		std::vector<HDRPixel> line;
		for (int j = 0; j < width; j ++) {
			HDRPixel p = get_hdr_pixel(image[j + i * width]);
			line.push_back(p);
		}
		fprintf(fp, "%c%c", 0x02, 0x02);
		fprintf(fp, "%c%c", (width >> 8) & 0xFF, width & 0xFF);
		for (int i = 0; i < 4; i ++) {
			for (int cursor = 0; cursor < width;) {
				const int cursor_move = std::min(127, width - cursor);
				fprintf(fp, "%c", cursor_move);
				for (int j = cursor;  j < cursor + cursor_move; j ++)
					fprintf(fp, "%c", line[j].get(i));
				cursor += cursor_move;
			}
		}
	}

	fclose(fp);
}

int main(int argc, char **argv) {
	int width = 640;
	int height = 480;
	// �J�����ʒu
	Ray camera(Vec(50.0, 52.0, 295.6), Normalize(Vec(0.0, -0.042612, -1.0)));
	// �V�[�����ł̃X�N���[����x,y�����̃x�N�g��
	Vec cx = Vec(width * 0.5135 / height);
	Vec cy = Normalize(Cross(cx, camera.dir)) * 0.5135;
	Color *image = new Color[width * height];

	for (int y = 0; y < height; y ++) {
		std::cerr << "Rendering " << (100.0 * y / (height - 1)) << "%" << std::endl;
		srand(y * y * y);
		for (int x = 0; x < width; x ++) {
			int image_index = y * width + x;	
			image[image_index] = Color();
			// 2x2�̃T�u�s�N�Z���T���v�����O
			for (int sy = 0; sy < 2; sy ++) {
				for (int sx = 0; sx < 2; sx ++) {
					const double dx = sx / 2.0;
					const double dy = sy / 2.0;
					Vec dir = cx * (((sx + 0.5 + dx) / 2.0 + x) / width - 0.5) +
								cy * (((sy + 0.5 + dy) / 2.0 + y) / height- 0.5) + camera.dir;
					image[image_index] = image[image_index] + radiance(Ray(camera.org + dir * 130.0, Normalize(dir)), 0);
				}
			}
		}
	}
	
	// .hdr�t�H�[�}�b�g�ŏo��
	save_hdr_file(std::string("image.hdr"), image, width, height);
}