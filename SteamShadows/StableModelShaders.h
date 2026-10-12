#pragma once
// Model shader path based on main; native-lit models receive cast occlusion without extra directional face darkening.
inline constexpr char stableModelShaders[] = R"MODEL(
cbuffer Params:register(b0){row_major float4x4 world,view,proj,light;float4 config,eye,sun;float4 pointPositions[10],pointColors[10],lantern,sunTone,depthAdjust;};
struct V{float4 p:SV_POSITION;float3 w:TEXCOORD0;float3 n:TEXCOORD1;float local:TEXCOORD2;};
float3 normal(float a){int n=(int)round(a*255);return n==0?float3(1,0,0):n==1?float3(-1,0,0):n==2?float3(0,1,0):n==3?float3(0,-1,0):n==4?float3(0,0,1):float3(0,0,-1);}
V caster(float4 p:POSITION){V o;float4 w=mul(world,float4(p.xyz*255,1));o.p=mul(light,w);o.w=w.xyz;o.n=0;o.local=0;return o;}
// Keep the native y-multiply then x/z/w MAD order. Without precise, FXC
// swaps the first two terms, producing angle-dependent coplanar triangles.
float4 combine(row_major float4x4 m,float4 r){precise float4 a=r.y*m[1];a=mad(m[0],r.x,a);a=mad(m[2],r.z,a);return mad(m[3],r.w,a);}
V receiver(float4 p:POSITION){V o;float4 v=float4(p.xyz*255,1);float4 w=mul(world,v);row_major float4x4 vw=float4x4(combine(world,view[0]),combine(world,view[1]),combine(world,view[2]),combine(world,view[3]));o.p=float4(dot(combine(vw,proj[0]),v),dot(combine(vw,proj[1]),v),dot(combine(vw,proj[2])-float4(0,0,0,depthAdjust.x),v),dot(combine(vw,proj[3]),v));o.w=w.xyz;o.n=normalize(mul((float3x3)world,normal(p.w)));o.local=0;
 // Match native point-light attenuation at vertices; no extra light shadow maps.
 int lightCount=depthAdjust.w>0?(int)depthAdjust.z:10;
 [loop]for(int j=0;j<lightCount;j++)if(pointPositions[j].w>0){
  float3 delta=o.w-pointPositions[j].xyz;float d2=dot(delta,delta);
  float falloff=saturate(1-d2/(pointPositions[j].w*pointPositions[j].w));
  float facing=clamp(-4*dot(o.n,delta*rsqrt(max(d2,.0001))),.3,1);
  o.local+=falloff*falloff*facing*max(pointColors[j].r,max(pointColors[j].g,pointColors[j].b));
 }o.local=saturate(o.local);return o;}
Texture2D<float> shadowMap:register(t0);SamplerComparisonState shadowSampler:register(s0);
float4 visibility(V i):SV_TARGET{float3 p=mul(light,float4(i.w,1)).xyz;float2 uv=float2(p.x,-p.y)*.5+.5;
 float facing=dot(normalize(i.n),sun.xyz);
 // Receiver-plane depth change across the comparison sampler footprint.
 float2 ux=ddx(uv),uy=ddy(uv);float zx=ddx(p.z),zy=ddy(p.z);
 float det=ux.x*uy.y-ux.y*uy.x;
 float slope=sqrt(saturate(1-facing*facing))/max(abs(facing),.1);
 float footprint=config.x*config.w*eye.w*min(slope,8);
 if(abs(det)>1e-12){float2 gradient=float2(zx*uy.y-zy*ux.y,zy*ux.x-zx*uy.x)/det;footprint=config.x*(abs(gradient.x)+abs(gradient.y));}
 if(any(uv<config.x)||any(uv>1-config.x)||p.z<0||p.z>1)return 0;
 // Small sprite faces need a stable normal-derived footprint. Screen
 // derivatives can mix adjacent faces across a pixel quad on these models.
 if(depthAdjust.y>0){
  float3 n=normalize(i.n);float denom=max(abs(dot(n,light[2].xyz)),1e-7);
  footprint=config.x*(abs(dot(n,light[0].xyz))+abs(dot(n,light[1].xyz)))/denom*config.w*config.w*eye.w*eye.w*.5;
 }
 float baseBias=depthAdjust.y>0?max(config.z,.1*eye.w*.5):config.z;
 float bias=baseBias+min(footprint,config.x*config.w*eye.w*16)*(sun.w>.5?2.1:1.1);float lit=0;
 [branch]if(sun.w>.5){
  [unroll]for(int y=-1;y<=1;y++)[unroll]for(int x=-1;x<=1;x++)lit+=shadowMap.SampleCmpLevelZero(shadowSampler,uv+float2(x,y)*config.x,p.z-bias);
  lit/=9;
 }else lit=shadowMap.SampleCmpLevelZero(shadowSampler,uv,p.z-bias);
 // A face turned away from the directional light has no direct contribution.
 // Do not let its own depth samples produce alternating bright edge teeth.
 lit*=depthAdjust.y>0?1:smoothstep(0,.08,facing);
 float fade=1-smoothstep(config.w*sunTone.y,config.w,length(i.w.xy-eye.xy));float opacity=(1-lit)*config.y*fade;
 float local=saturate(i.local);float lanternProtection=0;
 if(lantern.w>0){float3 delta=i.w-lantern.xyz;float d2=dot(delta,delta);
  float falloff=saturate(1-d2/(lantern.w*lantern.w));
  lanternProtection=saturate(4*falloff*falloff*clamp(-4*dot(normalize(i.n),delta*rsqrt(max(d2,.0001))),.3,1));
 }
 // Preserve the native lantern color. Synthetic fill tied to shadow visibility
 // stamped a second colored silhouette into the illuminated ground.
 float protection=max(local,lanternProtection);float alpha=opacity*(1-protection);
 float3 sunlight=float3(1,.65,.2)*sunTone.x*lit*fade*saturate(facing)*(1-local);
 return float4(sunlight,alpha);}
)MODEL";
