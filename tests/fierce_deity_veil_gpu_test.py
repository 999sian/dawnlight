"""Execute the shipped veil shader; requires wgpu==0.32.0 (software GPU works)."""
from pathlib import Path
import struct
import wgpu

source=(Path(__file__).resolve().parents[1]/'src/fierce_deity_veil_shader.inc').read_text()
shader=source.split('R"wgsl(')[1].split(')wgsl"')[0]
device=wgpu.gpu.request_adapter_sync(power_preference='low-power').request_device_sync()
module=device.create_shader_module(code=shader)
size=(64,64,1)
usage=wgpu.TextureUsage
blend={'color':{'src_factor':'src-alpha','dst_factor':'one-minus-src-alpha','operation':'add'},
       'alpha':{'src_factor':'zero','dst_factor':'one','operation':'add'}}

def run(phase=0, opacity=1, hidden=False, reversed_z=False, samples=1, tint=(0,0,0)):
    color=device.create_texture(size=size,format='rgba8unorm',usage=usage.RENDER_ATTACHMENT|usage.COPY_SRC)
    normal=device.create_texture(size=size,format='rgba8unorm',usage=usage.RENDER_ATTACHMENT|usage.COPY_SRC)
    depth=device.create_texture(size=size,format='depth32float',sample_count=samples,usage=usage.RENDER_ATTACHMENT)
    pipeline=device.create_render_pipeline(layout='auto', vertex={'module':module,'entry_point':'vs','buffers':[
        {'array_stride':44,'attributes':[{'format':'float32x4','offset':0,'shader_location':0},
            {'format':'float32x3','offset':16,'shader_location':1},
            {'format':'float32','offset':28,'shader_location':2},
            {'format':'float32x3','offset':32,'shader_location':3}]}]},
        primitive={'topology':'triangle-list','cull_mode':'none'},
        depth_stencil={'format':'depth32float','depth_write_enabled':False,
            'depth_compare':'greater-equal' if reversed_z else 'less-equal'},
        multisample={'count':samples},fragment={'module':module,'entry_point':'fs','targets':[
            {'format':'rgba8unorm','blend':blend,'write_mask':7},
            {'format':'rgba8unorm','write_mask':0}]})
    values=[]
    for x,y in [(-1,-1),(3,-1),(-1,3)]:
        values.extend([x,y,0.5,1,x*3,y*3-phase,0.75,opacity,*tint])
    vertices=device.create_buffer_with_data(data=struct.pack('<33f',*values),usage=wgpu.BufferUsage.VERTEX)
    encoder=device.create_command_encoder()
    attachments=[]
    for target,clear in [(color,(0.8,0.7,0.6,0.375)),(normal,(0.2,0.4,0.6,0.8))]:
        view=target.create_view()
        if samples>1:
            msaa=device.create_texture(size=size,format='rgba8unorm',sample_count=samples,usage=usage.RENDER_ATTACHMENT)
            entry={'view':msaa.create_view(),'resolve_target':view}
        else:
            entry={'view':view}
        entry.update(load_op='clear',store_op='store',clear_value=clear)
        attachments.append(entry)
    z=0.25 if hidden else 0.75
    if reversed_z: z=1-z
    render=encoder.begin_render_pass(color_attachments=attachments,depth_stencil_attachment={
        'view':depth.create_view(),'depth_clear_value':z,'depth_load_op':'clear','depth_store_op':'store'})
    render.set_pipeline(pipeline);render.set_vertex_buffer(0,vertices);render.draw(3);render.end()
    device.queue.submit([encoder.finish()])
    def read(texture):
        return bytes(device.queue.read_texture({'texture':texture},{'bytes_per_row':256,'rows_per_image':64},size))
    result=read(color)
    assert set(result[3::4])=={96},'veil changed scene alpha'
    assert read(normal)==bytes([51,102,153,204])*4096,'veil changed normal target'
    return result

clear=run(opacity=0)
visible=run()
assert visible!=clear and max(visible[0::4])<=204,'veil did not darken the background'
assert max(visible[0::4])-min(visible[0::4])>20,'strand core has no contrast'
for y in range(64):
    assert visible[(y*64)*4:(y*64+16)*4]==clear[(y*64)*4:(y*64+16)*4], 'solid fill outside the strand'
    assert visible[(y*64+48)*4:(y*64+64)*4]==clear[(y*64+48)*4:(y*64+64)*4], 'solid fill outside the strand'
assert run(phase=0.8)!=visible,'density did not flow'
assert run(hidden=True)==clear,'veil drew through an occluder'
assert run(reversed_z=True)==visible
assert run(hidden=True,reversed_z=True)==clear
assert run(samples=4)!=clear
white=run(tint=(1,1,1));gold=run(tint=(1,0.68,0.09))
i=(32*64+32)*4
assert all(white[i+c]>clear[i+c] for c in range(3)), 'white aura did not brighten all channels'
assert gold[i]>clear[i] and gold[i+2]<clear[i+2], 'gold aura has the wrong color'
assert run(tint=(1,1,1),hidden=True)==clear
print('Dark veil GPU: palette colors, animated density, transparent fade, depth occlusion, reversed Z, MSAA, scene alpha and normal preservation passed')
