#pragma once
// Original world-only grading, made with AI assistance. No ReShade integration.
namespace palette {
template<class T>void drop(T*&p){if(p){p->Release();p=nullptr;}}
static bool enabled=true,internal=false,finished=false;
static float saturation=.86f,cool=.025f;
static ID3D11Device*device=nullptr;static ID3D11DeviceContext*context=nullptr;static ID3D11DeviceContext1*context1=nullptr;
static ID3DDeviceContextState*state=nullptr;
static ID3D11Texture2D*copy=nullptr;static ID3D11ShaderResourceView*view=nullptr;
static ID3D11RenderTargetView*target=nullptr;static D3D11_VIEWPORT viewport{};
static ID3D11VertexShader*vs=nullptr;static ID3D11PixelShader*ps=nullptr;static ID3D11Buffer*params=nullptr;static ID3D11RasterizerState*raster=nullptr;
static shadows::DI oldDI=nullptr;static shadows::D oldD=nullptr;
static UINT width=0,height=0;static DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
static const char* code=R"(
cbuffer Grade:register(b0){float saturation,cool,pad0,pad1;};
Texture2D<float4> scene:register(t0);
float4 vs(uint id:SV_VertexID):SV_POSITION{float2 p=float2((id<<1)&2,id&2);return float4(p*float2(2,-2)+float2(-1,1),0,1);}
float4 ps(float4 p:SV_POSITION):SV_TARGET{
 float4 original=scene.Load(int3(p.xy,0));float l=dot(original.rgb,float3(.2126,.7152,.0722));
 float3 c=lerp(l.xxx,original.rgb,saturation);
 // Modest cooler palette, stronger in shade; no exposure/gamma lift at night.
 float shade=1-smoothstep(.15,.8,l);c*=float3(1-cool*.6,1,1+cool*(.6+.4*shade));
 return float4(saturate(c),original.a);
})";
static void reset(){drop(target);finished=false;}
static bool initialize(ID3D11DeviceContext*c){
 if(device)return context==c&&vs&&ps&&params&&state&&raster;
 context=c;context->AddRef();c->GetDevice(&device);ID3D11Device1*d1=nullptr;
 bool ok=SUCCEEDED(device->QueryInterface(__uuidof(ID3D11Device1),(void**)&d1))&&SUCCEEDED(c->QueryInterface(__uuidof(ID3D11DeviceContext1),(void**)&context1));
 auto level=device->GetFeatureLevel();if(ok)ok=SUCCEEDED(d1->CreateDeviceContextState(0,&level,1,D3D11_SDK_VERSION,__uuidof(ID3D11Device),nullptr,&state));drop(d1);
 for(int i=0;i<2&&ok;i++){ID3DBlob*b=nullptr,*error=nullptr;ok=SUCCEEDED(D3DCompile(code,strlen(code),"NativePalette",nullptr,nullptr,i?"ps":"vs",i?"ps_4_0":"vs_4_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&b,&error));if(error)shadows::log((char*)error->GetBufferPointer());if(ok)ok=SUCCEEDED(i?device->CreatePixelShader(b->GetBufferPointer(),b->GetBufferSize(),nullptr,&ps):device->CreateVertexShader(b->GetBufferPointer(),b->GetBufferSize(),nullptr,&vs));drop(b);drop(error);}
 D3D11_BUFFER_DESC cb{};cb.ByteWidth=16;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(ok)ok=SUCCEEDED(device->CreateBuffer(&cb,nullptr,&params));
 D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;if(ok)ok=SUCCEEDED(device->CreateRasterizerState(&rd,&raster));return ok;
}
static bool texture(ID3D11Texture2D*source,DXGI_FORMAT typed){
 D3D11_TEXTURE2D_DESC d{};source->GetDesc(&d);
 if(copy&&width==d.Width&&height==d.Height&&format==typed)return true;
 drop(view);drop(copy);width=d.Width;height=d.Height;format=typed;
 d.Format=typed;d.MipLevels=1;d.ArraySize=1;d.SampleDesc={1,0};d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.CPUAccessFlags=0;d.MiscFlags=0;
 return SUCCEEDED(device->CreateTexture2D(&d,nullptr,&copy))&&SUCCEEDED(device->CreateShaderResourceView(copy,nullptr,&view));
}
static bool apply(){
 if(internal||finished||!target||!initialize(context))return false;
 ID3D11Resource*resource=nullptr;target->GetResource(&resource);ID3D11Texture2D*source=nullptr;bool ok=SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D),(void**)&source));drop(resource);
 D3D11_RENDER_TARGET_VIEW_DESC rt{};target->GetDesc(&rt);D3D11_TEXTURE2D_DESC td{};if(source)source->GetDesc(&td);
 // Only ordinary full-size 2D scene targets; do not affect arrays/mips/offscreen previews.
 ok=ok&&(rt.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2D||rt.ViewDimension==D3D11_RTV_DIMENSION_TEXTURE2DMS)&&td.ArraySize==1&&td.MipLevels==1&&viewport.TopLeftX==0&&viewport.TopLeftY==0&&viewport.Width==td.Width&&viewport.Height==td.Height;
 if(ok)ok=texture(source,rt.Format);if(!ok){drop(source);return false;}
 internal=true;ID3DDeviceContextState*saved=nullptr;context1->SwapDeviceContextState(state,&saved);
 if(td.SampleDesc.Count>1)context->ResolveSubresource(copy,0,source,0,rt.Format);else context->CopyResource(copy,source);
 context->OMSetRenderTargets(1,&target,nullptr);context->OMSetBlendState(nullptr,nullptr,~0u);context->OMSetDepthStencilState(nullptr,0);context->RSSetState(raster);context->RSSetViewports(1,&viewport);
 context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->IASetInputLayout(nullptr);context->VSSetShader(vs,nullptr,0);context->PSSetShader(ps,nullptr,0);
 float values[4]={saturation,cool,0,0};context->UpdateSubresource(params,0,nullptr,values,0,0);context->PSSetConstantBuffers(0,1,&params);context->PSSetShaderResources(0,1,&view);
 oldD(context,3,0);
 ID3D11ShaderResourceView*empty=nullptr;context->PSSetShaderResources(0,1,&empty);context->OMSetRenderTargets(0,nullptr,nullptr);
 context1->SwapDeviceContextState(saved,nullptr);drop(saved);drop(source);drop(target);internal=false;finished=true;return true;
}
static void before(ID3D11DeviceContext*c){
 if(!enabled||internal||shadows::internal||moonlight::internal||finished)return;
 ID3D11Buffer*b=nullptr;c->VSGetConstantBuffers(0,1,&b);float projection[16]{};bool known=shadows::readBuffer(b,projection,64);drop(b);if(!known)return;
 if(fabsf(projection[15])>.5f){
  if(target){ID3D11RenderTargetView*current=nullptr;c->OMGetRenderTargets(1,&current,nullptr);bool same=current==target;drop(current);if(same){moonlight::renderSky();shadows::render();apply();}}
  return;
 }
 ID3D11Buffer*worldBuffers[6]{};c->VSGetConstantBuffers(0,6,worldBuffers);bool native=true;const UINT sizes[]={64,64,64,16,320,48};for(int i=0;i<6;i++){D3D11_BUFFER_DESC d{};if(worldBuffers[i])worldBuffers[i]->GetDesc(&d);if(d.ByteWidth!=sizes[i])native=false;drop(worldBuffers[i]);}if(!native)return;
 UINT n=1;D3D11_VIEWPORT vp{};c->RSGetViewports(&n,&vp);if(!n||vp.Width<320||vp.Height<180||vp.TopLeftX||vp.TopLeftY)return;
 if(!target){context=c;viewport=vp;c->OMGetRenderTargets(1,&target,nullptr);}
}
static void STDMETHODCALLTYPE di(ID3D11DeviceContext*c,UINT n,UINT s,INT b){before(c);oldDI(c,n,s,b);}
static void STDMETHODCALLTYPE draw(ID3D11DeviceContext*c,UINT n,UINT s){before(c);oldD(c,n,s);}
static void attach(){
 enabled=GetPrivateProfileIntW(L"Palette",L"Enabled",1,L".\\Mods\\NativePalette.ini")!=0;
 saturation=std::clamp((int)GetPrivateProfileIntW(L"Palette",L"SaturationPercent",86,L".\\Mods\\NativePalette.ini"),0,100)*.01f;
 cool=std::clamp((int)GetPrivateProfileIntW(L"Palette",L"CoolTintPercent",3,L".\\Mods\\NativePalette.ini"),0,10)*.01f;
 if(!enabled||!shadows::table)return;auto v=shadows::table;oldDI=(shadows::DI)v[12];oldD=(shadows::D)v[13];shadows::patch(v+12,(void*)di);shadows::patch(v+13,(void*)draw);
}
}
