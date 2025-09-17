#version 430

layout(std430,binding=0) restrict readonly buffer structure_buffer {
	octtree_node structure[];
};

layout(std430,binding=1) restrict readonly buffer node_data_buffer {
	node_data nodes[];
};

uniform vec2 u_resolution;

struct octtree_node {
	uint parent_idx; // traverse with constant memory
	uint child_idx;
};

struct node_data {
	vec3 centre;
	float half_length;
	int contains_volume
};


float sdf(vec3 ray){
	vec3 w = p;

	float m = dot(w,w);

	vec4 trap = vec4(abs(w),m);
	float dz = 1.0;

	for( int i=0; i<4; i++ ){
		dz = 8.0*pow(m,3.5)*dz + 1.0;

		float r = length(w);
		float b = 8.0*acos(w.y/r);
		float a = 8.0*atan(w.x, w.z);

		w = p + pow(r,8.0) * vec3( sin(b)*sin(a), cos(b), sin(b)*cos(a) );

		trap = min( trap, vec4(abs(w),m) );

		m = dot(w,w);
		if(m > 256.0 ) break;
    
	}
	resColor = vec4(m,trap.yzw);

    return 0.25*log(m)*sqrt(m)/dz;
}

vec3 central_difference(const vec3& p) {;
	const float eps = 1e-4;
	vec3 diff;
	diff.x = sdf(p + vec3(eps,0,0)) - sdf(p - vec3(eps,0,0));
	diff.y = sdf(p + vec3(0,eps,0)) - sdf(p - vec3(0,eps,0));
	diff.z = sdf(p + vec3(0,0,eps)) - sdf(p - vec3(0,0,eps));
	return normalize(diff);
}
vec3 traverse_octtree(vec3 ray_origin,vec3 ray_dir,out bool hit){
	uint tree_idx = 0;
	const float eps = 1e-4

	vec3 ray = ray_origin;

	while (tree_idx != -1){
		node_data node = nodes[tree_idx];

		vec3 rpos = ray - node.centre;
		vec3 arpos = abs(rpos);

		if (arpos.x>node.half_length) ||
		   (arpos.y>node.half_length) || 
		   (arpos.z>node.half_length){ // node doesnt contain ray

			tree_idx = structure[tree_idx].parent_idx;
		
		} 
		
		else if (structure[tree_idx].child_idx != 0){ // node contains ray, but its not a leaf node
			// this works because of the order that i generated the children nodes
			int child_offset = (rpos.z>0.0f)*4 + (rpos.y>0.0f)*2 + (rpos.x>0.0f);
			tree_idx = structure[tree_idx].child_idx + child_offset;
		
		} 
		
		else if (node.half_length < THRESHOLD){ // node contains ray and contains volume
			// do 1 raymarching step
			hit = true
			ray += ray_dir*sdf(ray);
			//ray += ray_dir*sdf(ray); // can try a mix of octtree resolution and raymarching steps
			return ray;
		} 
		
		else { // node contains ray and not volume
			
			vec3 box_min = node.centre - vec3(node.half_length);
			vec3 box_max = node.centre + vec3(node.half_length);
			
			vec3 inv_dir = 1.0 / ray_dir;
		
			vec3 t1 = (box_min - ray) * inv_dir;
			vec3 t2 = (box_max - ray) * inv_dir;

			vec3 tmin = min(t1, t2);
			vec3 tmax = max(t1, t2);

			float t_exit = min(min(tmax.x, tmax.y), tmax.z) + eps;
			
			ray = ray + ray_dir * t_exit;

		}
	}
	return ray;
}

void main(in vec2 fragCoord,out vec4 col){
	vec2 uv = (fragCoord*2.0f-u_resolution)/u_resolution.y;

	vec3 ray_dir = normalize(vec3(uv,1));
	vec3 ray_origin = vec3(0,0,-6);

	vec3 col = (0.5,0.5,0.2);
	vec3 sky_col = (0.2,0.9,0.3);

	vec3 light_pos = vec3(2,2,3);

	bool hit = false;

	vec3 ray = traverse_octtree(ray_origin,ray_dir,hit);

	if (hit){
		vec3 normal = central_difference(ray);
		line_to_light = normalize(ray-lightpos);
		float light = dot(line_to_light,normal);
		col.rgb = col*light
	} else
	{
		col.rgb = sky_col;
	}

}