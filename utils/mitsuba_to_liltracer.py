import xml.etree.ElementTree as ET
import json  
import cv2
import numpy as np 

filename = 'scene_v3.xml'
# filename = 'scene.xml'
dir = 'data/kitchen/'
#dir = 'data/bathroom2/'
tree = ET.parse(dir+filename)
root = tree.getroot()

for child in root:
    print(child.tag, child.attrib)

bsdfs_xml = root.findall('bsdf')
shapes_xml = root.findall('shape')

brdf = []
geom = []

light_count = 0
def next_light_id():
    global light_count
    light_id = "area_light_" + str(light_count)
    light_count = light_count + 1
    return light_id


brdf_count = 0
def next_brdf_id():
    global brdf_count
    brdf_id = "brdf_" + str(brdf_count)
    brdf_count = brdf_count + 1
    return brdf_id


def add_roughconductor(id,rough_x,rough_y,eta,kappa):
    brdf.append({
        "type": "RoughGGX",
        "name": id,
        "rough_x": rough_x,
        "rough_y": rough_y,
        "eta": eta,
        "kappa": kappa,
        "sample_visible_distribution" : True}
    )

def mean_texture(texture):
    path = texture.find("string").get("value")
    im = cv2.cvtColor(cv2.imread(dir+path), cv2.COLOR_BGR2RGB)/ 255.
    im = im ** 2.2
    return list(np.mean(im, axis=(0, 1)))

def process_eta(xml):
    eta = xml.find('rgb[@name="eta"]')
    eta_value = [1.34560,0.96521,0.61722]
    if eta is not None:
        eta_value = list(map(float, eta.get("value").split(',')))
    else:
        print("eta not found")
    return eta_value

def process_kappa(xml):
    kappa = xml.find('rgb[@name="k"]')
    kappa_value = [7.47460,6.39950,5.30310]
    if kappa is not None:
        kappa_value = list(map(float, kappa.get("value").split(',')))
    else:
        print("kappa not found")
    return kappa_value

add_dust = True

def process_bsdf(xml, id = None):

    bsdf_type = xml.get('type')
    if id is None:
        id = xml.get('id')
    
    match bsdf_type:
        case "twosided":
            process_bsdf(xml.find("bsdf"), id)
        case "bumpmap":
            process_bsdf(xml.find("bsdf"), id)
        case "mask":
            process_bsdf(xml.find("bsdf"), id)
        case "diffuse":
            rgb = xml.find("rgb")
            texture = xml.find("texture")
            albedo = [-1.0,-1.0,-1.0]
            if rgb is not None:
                albedo = list(map(float, rgb.get("value").split(',')))
            elif texture is not None:
                albedo = texture.find("string").get("value")
                #albedo = mean_texture(texture)
            else:
                print("rgb nor texture found")
            brdf.append({
                "type": "Diffuse",
                "name": id,
                "albedo": albedo
                })
        case "roughconductor":
            alpha = xml.find('float')
            if alpha is None:
                print("alpha not found")
            
            eta_value = process_eta(xml)
            kappa_value = process_kappa(xml)

            add_roughconductor(
                id
                , float(alpha.get("value"))
                , float(alpha.get("value"))
                , eta_value
                , kappa_value
            )

        case "conductor":

            eta_value = process_eta(xml)
            kappa_value = process_kappa(xml)
            
            add_roughconductor(
                id
                , 0.0001
                , 0.0001
                , eta_value
                , kappa_value
            )

        case "dielectric":  
            print("dielectric not implemented")
            add_roughconductor(
                id
                , 0.0001
                , 0.0001
                , [100.,100.,100.]
                , [100.,100.,100.]
            )
        
        case "thindielectric":
            print("thindielectric not implemented")
            add_roughconductor(
                id
                , 0.0001
                , 0.0001
                , [100.,100.,100.]
                , [100.,100.,100.]
            )

        case "roughdielectric":
            print("roughdielectric not implemented")
            add_roughconductor(
                id
                , 0.0001
                , 0.0001
                , [100.,100.,100.]
                , [100.,100.,100.]
            )

        case "roughplastic":
            alpha = float(xml.find('float').get("value"))
            rgb = xml.find("rgb")
            texture = xml.find("texture")
            albedo = [-1.0,-1.0,-1.0]
            if rgb is not None:
                albedo = list(map(float, rgb.get("value").split(',')))
            elif texture is not None:
                albedo = texture.find("string").get("value")
                #albedo = mean_texture(texture)
            else:
                print("rgb not texture found")
            brdf.append({
                "type": "Diffuse",
                "name": id+"base",
                "albedo": albedo
                })
            
            brdf.append(
            {
                "type": "RoughMicrograin",
                "name": id,
                "base": id+"base",
                "tau": 0.2,
                "rough_x": alpha,
                "rough_y": alpha,
                "eta": [1.0,1.0,1.0],
                "kappa": [100.,100.,100.],
                "sample_visible_distribution" : True
            })
            
            # <float name="alpha" value="0.1" />
			# <string name="distribution" value="ggx" />
			# <float name="int_ior" value="1.5" />
			# <float name="ext_ior" value="1" />
			# <boolean name="nonlinear" value="true" />
			# <texture name="diffuse_reflectance" type="bitmap">
			# 	<string name="filename" value="textures/Tabletop-light.jpg" />
			# 	<string name="filter_type" value="bilinear" />
			# </texture>

        case "plastic":
            rgb = xml.find("rgb")
            texture = xml.find("texture")
            albedo = [-1.0,-1.0,-1.0]
            if rgb is not None:
                albedo = list(map(float, rgb.get("value").split(',')))
            elif texture is not None:
                albedo = texture.find("string").get("value")
                #albedo = mean_texture(texture)
            else:
                print("rgb not texture found")
            brdf.append({
                "type": "Diffuse",
                "name": id+"base",
                "albedo": albedo
                })
            
            brdf.append(
            {
                "type": "RoughMicrograin",
                "name": id,
                "base": id+"base",
                "tau": 0.2,
                "rough_x": 0.3,
                "rough_y": 0.3,
                "eta": [1.0,1.0,1.0],
                "kappa": [100.,100.,100.],
                "sample_visible_distribution" : True
            }    
            )            
            
        case _:
            print(f"Unknow tag: {xml.tag}, type: {bsdf_type}, id: {id}")
        
    if add_dust and (id is not None):
        brdf.append(
        {
            "type": "DiffuseMicrograin",
            "name": id+"dust",
            "base": id,
            "tau": "DustTex",
            "rough_x": 1.0,
            "rough_y": 1.0,
            "albedo": [0.05,0.05,0.05],
            "sample_visible_distribution" : True
        }
        )

def process_shape(xml, id = None):
    shape_type = xml.get('type')
    match shape_type:
        case "obj":
            # <string name="filename" value="models/Mesh148.obj" />
            # <transform name="to_world">
            # 	<matrix value="1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1" />
            # </transform>
            # <boolean name="face_normals" value="true" />
            # <ref id="CupboardUnitsBSDF" />
            
            path = xml.find("string").get("value")
            to_world = xml.find("transform").find("matrix").get("value")
            bsdf_id = xml.find("ref").get("id")
            
            if add_dust:
                bsdf_id = bsdf_id + "dust"
            
            ltw = np.array(list(map(float, to_world.split(' '))))
            
            geom.append({
                "type": "Mesh",
                "brdf": bsdf_id,
                "local_to_world": list(ltw),
                "filename": path
            })

        case "rectangle":
            to_world = xml.find("transform").find("matrix").get("value")
            ltw = np.array(list(map(float, to_world.split(' '))))

            if xml.find("emitter") is not None:
                emit_id = next_light_id()
                emit_intensity = xml.find("emitter").find("rgb").get("value") 
                brdf.append({
                    "type": "Emissive",
                    "name": emit_id,
                    "intensity": list(map(float, emit_intensity.split(',')))
                })

                
                geom.append({
                    "type": "Rectangle",
                    "local_to_world": list(ltw),
                    "brdf": emit_id
                })
            
            elif xml.find("bsdf") is not None:
                brdf_id = next_brdf_id()
                
                process_bsdf(xml.find("bsdf"), brdf_id)
                geom.append({
                    "type": "Rectangle",
                    "local_to_world": list(ltw),
                    "brdf": brdf_id
                })
                

            # <transform name="to_world">
			#     <matrix value="0.811764 1.25471e-014 5.48452e-022 0.060314 -9.254e-015 1.10064 6.63724e-009 1.97379 0 5.90839e-008 0.549328 -3.1173 0 0 0 1" />
            # </transform>
            # <bsdf type="twosided">
            #     <bsdf type="diffuse">
            #         <rgb name="reflectance" value="0, 0, 0" />
            #     </bsdf>
            # </bsdf>
            # <emitter type="area">
            #     <rgb name="radiance" value="16.032, 16.032, 16.032" />
            # </emitter>
        case _:
            print(f"Unknow tag: {xml.tag}, type: {shape_type}, id: {id}")


for bsdf_xml in bsdfs_xml:
    process_bsdf(bsdf_xml)

for shape_xml in shapes_xml:
    process_shape(shape_xml)



integrator = {
    "type": "PathIntegrator",
    "sample_all_lights": False,
    "max_depth" : 8
  }

max_sample = 128


background =  {
    "type": "EnvironmentLight",
    "texture": "../envmaps/hallstatt4_hd.exr",
    "intensity": 0.5
  }

light = [
    {
      "type": "DirectionnalLight",
      "intensity": 10.0,
      "dir": [ 0.0, -1.0, 0.3 ]
    },
    {
      "type": "DirectionnalLight",
      "intensity": 10.0,
      "dir": [ 0.3, 1.0, 0.0 ]
    }
  ]

sensor = {
    "type": "Sensor",
    "width": 1280,
    "height": 720
} 

# camera = {
#     "type": "PerspectiveCamera",
#     "fov": 40,
#     "aspect": 1.7777777,
#     "center": [ -2.0308, 1.3732, 0.0 ],
#     "pos": [ -0.001688 , 1.94518, 2.07912 ]
# }

camera = {
    "type": "PerspectiveCamera",
    "fov": 60,
    "aspect": 1.7777777,
    "center": [ -0.657316, 1.6423, 0.0 ],
    "pos": [ 1.211 , 1.80475, 3.85239 ]
}


# camera = {
#     "type": "PerspectiveCamera",
#     "fov": 60,
#     "aspect": 1.7777777,
#     "center": [ -2.37845, 1.1532 , -1.02753 ],
#     "pos": [ -1.11273 , 1.40543, -0.129452  ]
# }

#### Bathroom
# camera = {
#     "type": "PerspectiveCamera",
#     "fov": 60,
#     "aspect": 1.7777777,
#     "center": [ -1.52598, 11.2103 , 0. ],
#     "pos": [ 4.44315 , 16.9344 , 49.9102  ]
# }

lil_scn = { 
  "integrator":  integrator
, "max_sample": max_sample
, "brdf": brdf
#, "background": background
# , "light": light 
, "geometries": geom
, "camera": camera
, "sensor": sensor
}




# Convert Python to JSON  
json_object = json.dumps(lil_scn, indent = 4) 

# Print JSON object
#print(json_object) 

with open(dir + filename.split(".")[0] + ".json", "w") as f:
    f.write(json_object)
















# - BRDF eval, sample, pdf -> ajout const SurfaceInteraction& si en parametre

