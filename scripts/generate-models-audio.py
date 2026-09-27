"""Original low-cost cockpit/rock meshes and synthesized sounds; no external model dependencies."""
import math, wave
from pathlib import Path
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'data/art/generated'
OUT.mkdir(parents=True,exist_ok=True)

class Mesh:
    def __init__(self): self.v=[];self.f=[];self.mat=[]
    def box(self,center,size,material='Hull',yaw=0):
        c=np.array(center); s=np.array(size)/2; off=len(self.v)
        a=math.radians(yaw); rot=np.array([[math.cos(a),-math.sin(a),0],[math.sin(a),math.cos(a),0],[0,0,1]])
        self.v.extend((rot@np.array([x*s[0],y*s[1],z*s[2]])+c).tolist() for x,y,z in
            [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)])
        for face in [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)]:
            self.f.extend([[off+face[0],off+face[1],off+face[2]],[off+face[0],off+face[2],off+face[3]]]); self.mat.extend([material]*2)
    def hull(self,sections,material='Hull'):
        start=len(self.v)
        # x, half width, lower z, upper z; chamfered cross sections.
        for x,w,z0,z1 in sections:
            self.v.extend([[x,-w*.7,z0],[x,w*.7,z0],[x,w,z0+8],[x,w,z1-8],
                           [x,w*.7,z1],[x,-w*.7,z1],[x,-w,z1-8],[x,-w,z0+8]])
        faces=[]
        for j in range(len(sections)-1):
            for k in range(8):
                a=start+j*8+k;b=start+j*8+(k+1)%8;c=b+8;d=a+8
                faces.extend([[a,b,c],[a,c,d]])
        for k in range(1,7):
            faces.append([start,start+k+1,start+k])
            end=start+(len(sections)-1)*8;faces.append([end,end+k,end+k+1])
        self.f.extend(faces);self.mat.extend([material]*len(faces))
    def save(self,name):
        # UE's legacy OBJ importer preserves Z-up and flips Y on import.
        lines=['# Original CanyonRace geometry; centimetres; Z-up','mtllib cockpit.mtl']
        lines += [f'v {x:.6f} {-y:.6f} {z:.6f}' for x,y,z in self.v]
        for face,mat in zip(self.f,self.mat):
            a,b,c=[np.array(self.v[i],dtype=float) for i in face]; n=np.cross(b-a,c-a); n/=max(np.linalg.norm(n),1e-8)
            lines.append(f'vn {n[0]:.6f} {-n[1]:.6f} {n[2]:.6f}')
        last=None
        for i,(f,mat) in enumerate(zip(self.f,self.mat)):
            if mat!=last: lines.append('usemtl '+mat);last=mat
            lines.append('f '+' '.join(f'{v+1}//{i+1}' for v in reversed(f)))
        (OUT/(name+'.obj')).write_text('\n'.join(lines))

m=Mesh()
for side in [-1,1]:
    rail=Mesh();rail.hull([(-130,12,-40,22),(80,14,-30,28),(245,11,-12,36),(350,6,-20,5)])
    rail.v=[[x,y+side*110,z] for x,y,z in rail.v]
    off=len(m.v);m.v+=rail.v;m.f += [[v+off for v in f] for f in rail.f];m.mat+=rail.mat
    m.box((150,side*111,35),(95,5,3),'Light')
    m.box((0,side*98,25),(95,15,7),'Trim')
    m.box((185,side*95,37),(65,20,8),'Trim',side*12)
    for x in [170,185,200]: m.box((x,side*95,43),(7,9,3),'Accent')
    nacelle=Mesh();nacelle.hull([(-220,33,-65,10),(20,45,-68,18),(230,25,-60,5),(340,3,-30,-5)],'Hull')
    off=len(m.v);m.v += [[x,y+side*177,z] for x,y,z in nacelle.v];m.f += [[v+off for v in f] for f in nacelle.f];m.mat+=nacelle.mat
    m.box((95,side*176,16),(130,22,2),'Accent')
    for x in [-115,-85,-55,-25]: m.box((x,side*177,19),(10,50,3),'Trim')
m.hull([(225,65,-35,10),(280,75,-38,18),(380,55,-40,8),(470,10,-30,-10)])
m.box((260,0,94),(12,125,30),'Trim')
m.box((253,0,94),(3,115,24),'Screen')
m.box((270,0,45),(14,15,90),'Trim')
m.box((362,0,9),(120,12,3),'Accent')
m.box((-110,0,-47),(180,190,10),'Trim')
m.save('SM_Cockpit')
(OUT/'cockpit.mtl').write_text('\n'.join(f'newmtl {n}\nKd {r} {g} {b}\n' for n,r,g,b in
    [('Hull',.12,.16,.18),('Trim',.025,.035,.04),('Accent',.8,.22,.035),('Light',.02,.7,1),('Screen',.005,.01,.018),('Rock',.4,.3,.2)]))

# UV-free triplanar rocks: staggered rings, smoothly varied radius, flat angular facets.
for variant in range(3):
    rock=Mesh();rng=np.random.default_rng(50+variant);rings=18;sides=32
    for j in range(rings+1):
        phi=math.pi*j/rings
        for i in range(sides):
            theta=2*math.pi*i/sides
            radius=1+.09*math.sin(theta*3+phi*4+variant)+.035*math.sin(theta*9-phi*7)
            p=np.array([math.sin(phi)*math.cos(theta),math.sin(phi)*math.sin(theta),math.cos(phi)])*radius
            p[2]*=.68+.1*variant; p[0]*=1.15
            rock.v.append((p*100).tolist())
    for j in range(rings):
        for i in range(sides):
            a=j*sides+i;b=j*sides+(i+1)%sides;c=b+sides;d=a+sides
            rock.f.extend([[a,c,b],[a,d,c]]);rock.mat.extend(['Rock','Rock'])
    rock.save('SM_Rock'+str(variant))

SR=48000
def wav(name,x):
    x=np.clip(x,-1,1)
    with wave.open(str(OUT/(name+'.wav')),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(SR);f.writeframes((x*32767).astype('<i2').tobytes())
t=np.arange(SR*4)/SR;rng=np.random.default_rng(31)
wind=rng.standard_normal(len(t)); wind=np.convolve(wind,np.ones(15)/15,mode='same')
engine=.2*np.sin(2*np.pi*70*t)+.12*np.sin(2*np.pi*140*t)+.045*np.sin(2*np.pi*420*t)
wav('Engine',engine+wind*.2)
t=np.arange(int(SR*.45))/SR
wav('Impact',(rng.standard_normal(len(t))*.32+np.sin(2*np.pi*65*t)*.4)*np.exp(-t*12))
wav('Gate',np.sin(2*np.pi*(750*t+400*t*t))*.22*np.sin(np.pi*t/.45)**2)
print('Generated cockpit, 3 rock meshes, 3 original sounds')
