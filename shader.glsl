#version 430 core

struct octtree_node {
	uint parent_idx; // traverse with constant memory
	uint child_idx;
};

struct node_data {
	vec3 centre;
	float pad0;
	float half_length;
	float pad1[3];
};

layout(std430,binding=0) buffer structure_buffer {
	octtree_node structure[];
};

layout(std430,binding=1) buffer node_data_buffer {
	node_data nodes[];
};

uniform vec2 resolution;
uniform float min_leaf_size;
uniform vec3 camera_pos;

out vec4 FragColor;



float sdf(vec3 p){
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

    return 0.25*log(m)*sqrt(m)/dz;
}

vec3 central_difference(vec3 p) {;
	const float eps = 1e-4;
	vec3 diff;
	diff.x = sdf(p + vec3(eps,0,0)) - sdf(p - vec3(eps,0,0));
	diff.y = sdf(p + vec3(0,eps,0)) - sdf(p - vec3(0,eps,0));
	diff.z = sdf(p + vec3(0,0,eps)) - sdf(p - vec3(0,0,eps));
	return normalize(diff);
}

vec3 traverse_octtree(vec3 ray_origin,vec3 ray_dir,out bool hit){
	uint tree_idx = 0;
	const float eps = 1e-3;

	const vec3 inv_dir = 1.0/ray_dir;

	vec3 ray = ray_origin;

	for(int i = 0;i<100;i++){
		node_data node = nodes[tree_idx];

		vec3 rpos = ray - node.centre;
		vec3 arpos = abs(rpos);
		float max_delta = max(max(arpos.x,arpos.y),arpos.z);
		if (max_delta>node.half_length){ // node doesnt contain ray
			if (tree_idx == 0)break;
			tree_idx = structure[tree_idx].parent_idx;		
		} 
		
		else if (structure[tree_idx].child_idx != 0){ // node contains ray, but its not a leaf node
			// this works because of the order that i generated the children nodes
			int child_offset = int(rpos.z>0.0f)*4 + int(rpos.y>0.0f)*2 + int(rpos.x>0.0f);
			tree_idx = structure[tree_idx].child_idx + child_offset;
		
		} 
		
		else if (node.half_length <= min_leaf_size){ // node contains ray and contains volume
			// do a few raymarching steps
			float tr = 0.;
			for(int j = 0;j<20;j++){
				float dist = sdf(ray);
				tr += dist;
				ray += ray_dir*sdf(ray);
				if (dist<1e-3){hit=true;return ray;} // if actually hit volume, return
				if (tr>min_leaf_size)break; // otherwise, go back to traversing
			}

		} 
		
		else { // node contains ray and not volume
			
			vec3 box_min = node.centre - vec3(node.half_length);
			vec3 box_max = node.centre + vec3(node.half_length);
			
		
			vec3 t1 = (box_min - ray) * inv_dir;
			vec3 t2 = (box_max - ray) * inv_dir;

			vec3 tmin = min(t1, t2);
			vec3 tmax = max(t1, t2);

			float t_exit = min(min(tmax.x, tmax.y), tmax.z) + eps;
			
			ray = ray + ray_dir * t_exit;

			// not in the same node, so go back 1 level of depth
			tree_idx = structure[tree_idx].parent_idx;

		}
	}
	hit = false;
	return ray;
}

vec3 raymarch(vec3 ray_origin, vec3 ray_dir, out bool hit){
	float dist,tr;
	vec3 ray = ray_origin;
	for(int i = 0;i<200;i++){
		dist = sdf(ray);
		ray += ray_dir * dist;
		tr += dist;
		if (dist<1e-4){hit=true;return ray;}
		if (tr>1e2){hit=false;return ray;}
	}
}

void main(){
	vec2 uv = (gl_FragCoord.xy*2.0f-resolution)/resolution.y;

	vec3 ray_dir = normalize(vec3(uv,1));
	vec3 ray_origin = camera_pos;

	vec3 col = vec3(0);
	vec3 obj_col = vec3(0.5,0.5,0.5);
	vec3 sky_col = vec3(0.2,0.3,0.9);

	vec3 light_pos = vec3(2,2,3);
	
	bool hit = false;


	vec3 ray = traverse_octtree(ray_origin,ray_dir,hit);
	
//	vec3 ray = raymarch(ray_origin,ray_dir,hit);

	if (hit){

		vec3 normal = central_difference(ray);
		vec3 line_to_light = normalize(ray-light_pos);
		float light = clamp(dot(line_to_light,normal),0.,1.);
		col = obj_col*light;

	} else {

		col = sky_col;

	}
	FragColor = vec4(col,1.0);
}