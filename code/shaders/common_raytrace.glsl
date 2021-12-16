const float VERY_FAR = 1e20f;

struct ray
{
	vec4 origin;
	vec4 direction;
	vec4 throughput;
	vec4 normal;
	float distance;
	int identifier;
	int bounces;
	uint pixel_index;
};

struct shadow_ray
{
	vec4 origin;
	vec4 direction;
	vec4 color;
	uint pixel_index;
	uint pad0;
	uint pad1;
	uint pad2;
};

struct ray_hit
{
    vec3 normal;
    float distance;
    bool hit;
};
