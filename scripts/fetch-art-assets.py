"""Fetch CC0 Poly Haven source maps with API-provided checksums and provenance."""
import concurrent.futures, hashlib, json, urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'data/art'
OUT.mkdir(parents=True,exist_ok=True)
HEADERS={'User-Agent':'CanyonRaceDevelopment/0.1'}
def request(url):
    return urllib.request.urlopen(urllib.request.Request(url,headers=HEADERS),timeout=120)
manifest=[]
for asset,res,maps in [('rock_face','4k',['Diffuse','nor_dx','arm']),
                       ('rocky_terrain_02','2k',['Diffuse','nor_dx','arm']),
                       ('kloppenheim_06_puresky','4k',['hdri'])]:
    with request('https://api.polyhaven.com/files/'+asset) as response:
        api=json.load(response)
    (OUT/(asset+'-api.json')).write_text(json.dumps(api,indent=2))
    for kind in maps:
        fmt='hdr' if kind=='hdri' else 'jpg'
        entry=api[kind][res][fmt]
        suffix={'Diffuse':'color','nor_dx':'normal','arm':'arm','hdri':'sky'}[kind]
        manifest.append(dict(asset=asset,kind=kind,file=f'{asset}_{suffix}.{fmt}',
            url=entry['url'],md5=entry['md5'],page='https://polyhaven.com/a/'+asset,
            license='CC0-1.0',license_url='https://polyhaven.com/license'))
def fetch(entry):
    path=OUT/entry['file']
    if not path.exists() or hashlib.md5(path.read_bytes()).hexdigest()!=entry['md5']:
        with request(entry['url']) as response:
            data=response.read()
        assert hashlib.md5(data).hexdigest()==entry['md5'],entry['file']
        path.write_bytes(data)
    entry['sha256']=hashlib.sha256(path.read_bytes()).hexdigest()
    entry['bytes']=path.stat().st_size
    print(entry['file'],entry['bytes'],flush=True)
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
    list(pool.map(fetch,manifest))
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
