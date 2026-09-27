"""Plot the actual runtime terrain and route elevation profiles; no external data download."""
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
data = root / 'CanyonRace/Content/Canyon/Data'
h = np.fromfile(data / 'terrain.f32', dtype='<f4').reshape(505, 505) / 100
gy, gx = np.gradient(h, 8)
shade = np.clip((1 - gx * .5 - gy * .65) / np.sqrt(1 + gx*gx + gy*gy), 0, 1)
rgb = np.stack([shade*.45+.24, shade*.35+.18, shade*.24+.12], axis=-1)
rgb[h < 19] = [.08, .25, .26]
canvas = Image.new('RGB', (1100, 720), '#101c27')
canvas.paste(Image.fromarray((rgb*255).astype('uint8')).resize((620, 620)), (20, 70))
d = ImageDraw.Draw(canvas)
try:
    font = ImageFont.truetype('arial.ttf', 18)
    title = ImageFont.truetype('arial.ttf', 26)
except OSError:
    font = title = ImageFont.load_default()
d.text((20, 20), 'CANYON RACE / HORSESHOE BEND', font=title, fill='white')
d.text((670, 90), 'Elevation above terrain datum', font=font, fill='white')
for meters in [0, 100, 200, 300, 400]:
    y = 560 - meters
    d.line((680, y, 1070, y), fill='#344552')
    d.text((680, y-21), f'{meters} m', font=font, fill='#9aabb9')
for filename, label, color, line in [('route.csv', 'River Run', '#51d9ed', 0), ('route-summit.csv', 'Summit Drop', '#ffae57', 1)]:
    p = np.loadtxt(data / filename, delimiter=',') / 100
    points = (p + 2016) / 8
    d.line([(20+x*620/504, 70+y*620/504) for x, y in points], fill=color, width=4)
    x,y=points[0];d.ellipse((20+x*620/504-5,70+y*620/504-5,20+x*620/504+5,70+y*620/504+5),fill=color)
    distance, xs, zs = 0, [], []
    for a,b in zip(p,p[1:]):
        length=np.linalg.norm(b-a)
        for fraction in np.linspace(0,1,max(2,int(length/4))):
            x,y=(a+(b-a)*fraction+2016)/8
            ix,iy=int(x),int(y);fx,fy=x-ix,y-iy
            z=(h[iy,ix]*(1-fx)+h[iy,ix+1]*fx)*(1-fy)+(h[iy+1,ix]*(1-fx)+h[iy+1,ix+1]*fx)*fy
            xs.append(distance+length*fraction);zs.append(z)
        distance+=length
    d.line([(680+x/4500*390,560-z) for x,z in zip(xs,zs)],fill=color,width=3)
    d.text((675,610+line*32),f'{label}: {distance/1000:.2f} km / {len(p)-1} gates',font=font,fill=color)
    print(f'{label}: {distance:.1f}m length, {min(zs):.1f}..{max(zs):.1f}m elevation, {zs[0]-zs[-1]:.1f}m start-to-finish drop')
d.text((680,570),'0                    2                    4 km',font=font,fill='#9aabb9')
d.text((20,695),'USGS 3DEP / 8 m runtime grid / +X east, +Y south',font=font,fill='#9aabb9')
canvas.save(root/'docs/course-overview.png')
