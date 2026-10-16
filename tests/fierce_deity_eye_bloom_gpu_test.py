"""Execute the shipped bloom shader (requires wgpu; software adapters are supported).

Run: python3 -m pip install wgpu==0.32.0
     python3 tests/fierce_deity_eye_bloom_gpu_test.py
"""
from pathlib import Path
import struct
import wgpu

source = (Path(__file__).resolve().parents[1] / 'src/fierce_deity_eye_bloom_shader.inc').read_text()
shader = source.split('R"wgsl(')[1].split(')wgsl"')[0]
device = wgpu.gpu.request_adapter_sync(power_preference='low-power').request_device_sync()
module = device.create_shader_module(code=shader)
usage = wgpu.TextureUsage
width, height = 65, 49  # odd dimensions exercise the last compute workgroup/texel
half = ((width+1)//2, (height+1)//2, 1)

def texture(format, size=(width, height, 1), extra=0):
    return device.create_texture(size=size, format=format,
        usage=usage.TEXTURE_BINDING | usage.COPY_DST | usage.COPY_SRC | extra)

def upload(tex, values, code, stride):
    device.queue.write_texture({'texture': tex}, struct.pack('<'+code*len(values), *values),
        {'bytes_per_row': width*stride, 'rows_per_image': height}, (width, height, 1))

color = texture('rgba8unorm')
eye_z = texture('r32float')
scene_z = texture('r32float')
a = texture('rgba16float', half, usage.STORAGE_BINDING)
b = texture('rgba16float', half, usage.STORAGE_BINDING)
output = texture('rgba16float', extra=usage.RENDER_ATTACHMENT)

horizontal = device.create_compute_pipeline(layout='auto', compute={'module': module, 'entry_point': 'horizontal'})
blend = {'color': {'src_factor': 'one', 'dst_factor': 'one', 'operation': 'add'},
         'alpha': {'src_factor': 'zero', 'dst_factor': 'one', 'operation': 'add'}}
render = device.create_render_pipeline(layout='auto', vertex={'module': module, 'entry_point': 'vs'},
    fragment={'module': module, 'entry_point': 'fs', 'targets': [
        {'format': 'rgba16float', 'blend': blend, 'write_mask': wgpu.ColorWrite.RED | wgpu.ColorWrite.GREEN | wgpu.ColorWrite.BLUE}]})

def group(pipeline, entries):
    return device.create_bind_group(layout=pipeline.get_bind_group_layout(0), entries=[
        {'binding': binding, 'resource': resource} for binding, resource in entries])

horizontal_group = group(horizontal, [(4, a.create_view()), (5, b.create_view())])
render_group = group(render, [(6, b.create_view()), (7, device.create_sampler(min_filter='linear', mag_filter='linear'))])

def run(reversed_z=False, alpha=255, hidden=False):
    pixels = [0]*(width*height*4)
    depth = [1.0]*(width*height)
    scene = [1.0]*(width*height)
    for y in range(21, 25):
        for x in list(range(12, 24)) + list(range(44, 56)):
            i = y*width+x
            # Native mask draw uses SRCALPHA/ZERO with alpha writes disabled.
            pixels[i*4:i*4+4] = [round(c*alpha/255) for c in (255, 56, 40)] + [0]
            depth[i] = 0.5
            # Right eye occluded; left eye visible unless the whole head is hidden.
            scene[i] = 0.2 if x >= 44 or hidden else 0.5
    if reversed_z:
        depth = [1-z for z in depth]
        scene = [1-z for z in scene]
    upload(color, pixels, 'B', 4)
    upload(eye_z, depth, 'f', 4)
    upload(scene_z, scene, 'f', 4)
    extract = device.create_compute_pipeline(layout='auto', compute={'module': module,
        'entry_point': 'extract', 'constants': {'reversedZ': reversed_z}})
    extract_group = group(extract, [(0, color.create_view()), (1, eye_z.create_view()),
        (2, scene_z.create_view()), (3, a.create_view())])
    encoder = device.create_command_encoder()
    for pipeline, binding in [(extract, extract_group), (horizontal, horizontal_group)]:
        compute = encoder.begin_compute_pass()
        compute.set_pipeline(pipeline)
        compute.set_bind_group(0, binding)
        compute.dispatch_workgroups((half[0]+7)//8, (half[1]+7)//8, 1)
        compute.end()
    render_pass = encoder.begin_render_pass(color_attachments=[{'view': output.create_view(),
        'load_op': 'clear', 'store_op': 'store', 'clear_value': (0, 0, 0, 0.375)}])
    render_pass.set_pipeline(render)
    render_pass.set_bind_group(0, render_group)
    render_pass.draw(3)
    render_pass.end()
    device.queue.submit([encoder.finish()])
    raw = device.queue.read_texture({'texture': output}, {'bytes_per_row': width*8,
        'rows_per_image': height}, (width, height, 1))
    result = struct.unpack('<'+'e'*(width*height*4), raw)
    assert all(result[i] == 0.375 for i in range(3, len(result), 4)), 'scene alpha changed'
    return result

def red(image, x, y):
    return image[(y*width+x)*4]

normal = run()
assert red(normal, 18, 22) > 0.1, 'visible eye did not emit'
assert red(normal, 18, 18) > 0.005, 'no soft spill beyond the eye boundary'
assert red(normal, 18, 18) > red(normal, 18, 16) > red(normal, 18, 13), 'blur is not a soft falloff'
assert red(normal, 48, 22) == 0, 'occluded eye emitted through the head'
assert all(abs(a-b) < 0.002 for a,b in zip(normal, run(True))), 'reversed Z disagrees'
for image in (run(alpha=0), run(hidden=True)):
    assert max(image[0::4]) == 0, 'transparent/hidden eye emitted'
partial = run(alpha=128)
assert 0.49 < red(partial, 18, 22)/red(normal, 18, 22) < 0.52, 'native alpha lost'
print('Eye bloom GPU: real soft spill, occlusion, alpha, reversed Z and odd dimensions passed')
