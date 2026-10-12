# GPU performance measurements

Made with AI assistance. These measurements retain the existing visual settings.

The mod samples its GPU pass once every 60 executions using a four-slot timestamp-query ring. Results are polled with `D3D11_ASYNC_GETDATA_DONOTFLUSH`; unfinished samples are skipped without waiting. Disjoint timestamps are rejected. Logs report averages of ten completed samples in milliseconds.

MoonandShadow writes `Mods/SteamShadows.log`: first is shadow-map generation; second is receiver application. TemporalAA writes `Mods/TemporalAA.log`: first is input copy and setup; second is resolve and output-to-history copy.

These measure GPU execution of the named passes, not total frame time, CPU capture overhead, or FPS. Sparse timestamps still have a small instrumentation cost. Compare the same location, camera, resolution, and settings after loading has settled. Test both enabled, shadows only (F9 off), TAA only (F7 off), and both disabled. Preserve ReShade settings throughout.

## Receiver local lights

Receiver parameters compact active native point lights in their original order. The vertex shader iterates only active slots instead of testing ten slots on every vertex. Position, radius, color, attenuation, and accumulation order are preserved; lantern protection is unchanged. No shadow quality setting changes. GPU tests compare compacted and original pixels across six normals, with sparse local lights and lantern combinations. Expected savings depend on mesh complexity and active-light count; in-game timing is still needed.

## Cascaded-shadow experiment

`test/cascaded-shadows` retains the 4096 map at radius 120 and adds a 2048 near map at radius 32. Both are independently texel-snapped. A blended overlap covers the outer fifth of the near region. The receiver geometry is replayed once; only the overlap samples both maps. Loaded terrain is conservatively culled in light space for the near pass, including tall chunks and low-angle light. Existing replay binding/constant reuse remains active. This is an independent camera-centered two-map implementation of the cascade principle; it does not copy Omega code or reproduce its complete renderer.

The near map adds approximately 16 MiB of depth texture memory and another caster pass. Timings include both maps. No actual FPS gain or visual parity is established by the automated tests. Tests cover original single-map behavior and the active two-map path with voxel normals, point lights, lanterns, bias and cloud casting.

The matching WorldMotion test branch provides one animation timestamp per scene depth boundary, shared by native leaves/clouds and shadow sampling. This prevents a 33 ms animation tick from changing geometry midway through a scene. Offscreen animation and live movement still require verification. Stable TAA is retained; the rejected SMAA/depth-reprojection DLL is not part of this test.

Model receivers consistently use the model-specific normal footprint and a 0.1-block minimum self-shadow depth tolerance, independent of their native matrix scale. Terrain bias is unchanged. This also removes a per-draw square-root classification. Real occluder shadows, lantern protection and both cascade paths remain covered by rendering tests; the reported face bands require a live comparison before calling this resolved.

Model receiver shaders are compiled independently from the exact accepted main-branch shader source. Terrain keeps the cascade shaders. Shader bindings change only at terrain/model boundaries, with no additional receiver geometry or texture samples. This isolates model arithmetic and depth alignment from cascade compilation. GPU checks cover the model shaders with lights across all normals; live face-angle verification is still required.

## Native model vertex path

The native Steam shader exposes view-space position (TEXCOORD3) and view-space normal (TEXCOORD1), while forming world/view/projection and depth offset internally. Model draws retain their original vertex shader with draw-local projection, view, world and depth-offset snapshots. The shadow pixel adapter transforms the native view-space outputs back to world space with the rigid camera inverse, so raster position/depth come from the original shader. This is not a captured CPU combined matrix: Steam does not expose one in this pipeline. Proprietary shader bytecode is not bundled or published.

Matrix uploads are cached by content; native resource/shader bindings are reused across consecutive model draws. Models still have one receiver draw; terrain cascade behavior is unchanged. The adapter evaluates local-light protection per model pixel rather than adding a geometry pass. Added CPU/GPU cost and the reported face bands require live verification. A dedicated independent native-like shader fixture exercises the actual receiver path alongside the original fallback and active cascades.

Native-lit models no longer receive the terrain-style away-facing darkening term a second time. Their real shadow-map occlusion, local-light protection and sun warmth remain active. Terrain's directional side shading is unchanged. GPU checks require all six unoccluded model normals to preserve native color and an occluded model to remain shadowed. The reported whole-body darkening still needs live confirmation.
