// A flowing density field on continuous body-attached surfaces, not sprites.
R"wgsl(
struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) field: vec3f,
    @location(1) opacity: f32,
}
@vertex fn vs(@location(0) position: vec4f, @location(1) field: vec3f,
              @location(2) opacity: f32) -> VertexOutput {
    var out: VertexOutput;
    out.position=position; out.field=field; out.opacity=opacity;
    return out;
}
fn hash(p: vec3f) -> f32 {
    var q=fract(p*0.1031);
    q+=dot(q,q.yzx+33.33);
    return fract((q.x+q.y)*q.z);
}
fn noise(p: vec3f) -> f32 {
    let cell=floor(p);
    let f=fract(p);
    let u=f*f*(3.0-2.0*f);
    return mix(mix(mix(hash(cell),hash(cell+vec3f(1,0,0)),u.x),
                   mix(hash(cell+vec3f(0,1,0)),hash(cell+vec3f(1,1,0)),u.x),u.y),
               mix(mix(hash(cell+vec3f(0,0,1)),hash(cell+vec3f(1,0,1)),u.x),
                   mix(hash(cell+vec3f(0,1,1)),hash(cell+vec3f(1,1,1)),u.x),u.y),u.z);
}
@fragment fn fs(input: VertexOutput) -> @location(0) vec4f {
    // Elongate the structure vertically, and distort it with another smooth
    // scale so the sheet changes shape instead of repeating one flame image.
    let p=input.field*vec3f(1.0,0.48,1.0);
    let warp=noise(p*0.65+vec3f(7.1,3.8,1.7));
    let density=0.65*noise(p+vec3f(warp*1.1,0.0,warp*0.7))+
                0.35*noise(p*2.1+vec3f(2.4,9.1,5.3));
    let alpha=0.48*input.opacity*smoothstep(0.18,0.72,density);
    return vec4f(0.0,0.0,0.0,alpha);
}
)wgsl"
