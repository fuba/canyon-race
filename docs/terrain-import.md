# Horseshoe Bend の地形取り込み

取得・変換：`scripts/fetch_terrain.py`。初回実行済み。再現する場合はワークスペースで以下を実行する。

```powershell
python -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r scripts\requirements-terrain.txt
.\.venv\Scripts\python.exe scripts\fetch_terrain.py
```

USGSの配信モザイクは更新され得る。再取得時には現在の地形ファイルを上書きするため、同一入力の再現が必要なら元のGeoTIFFとJSONを保管する。

Unreal Editorで新しい地形検証用レベルを作り、LandscapeモードのImport from Fileから次の設定を使う。

| 設定 | 値 |
|---|---|
| Heightmap | `data/terrain/horseshoe_bend_height16.png` |
| 解像度 | 1009 × 1009 |
| Section Size | 63 × 63 quads |
| Sections Per Component | 1 × 1 |
| Components | 16 × 16 |
| Scale X / Y | 400 / 400 cm |
| Scale Z | 84.2007014382 |
| Location Z（絶対標高を保つ場合） | 116761.672292779 cm |
| Location Z（標高952.06 mをゲーム内0 mにする場合） | 21555.37956817 cm |

ローカル X は東、Y は南。PNGの先頭行が北。X/YのLandscape原点位置は任意だが、将来GIS素材と重ねる際はJSONの座標と半画素オフセットを使用する。

Unrealの高さ式は `(uint16 - 32768) / 128 * ZScale` cm。端点まで一致するよう65535で正規化した。元の標高値と逆変換を比較して約3.3 mm以内の量子化誤差を確認済み。

これは標高の取り込み素材。岩肌、植生、水面、コース、衝突調整はまだ含まれない。`horseshoe_bend_hillshade.png` は欠損や形状を目視するための画像で、地形材質に貼る写真ではない。

出典表記：USGS National Map 3D Elevation Program (3DEP)。取得URLとSHA256は `horseshoe_bend.json` を参照。
