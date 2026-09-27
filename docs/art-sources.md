# 表示素材と出典

実地形は USGS 3DEP の公開標高データです。取得日時・範囲・座標系・高さ基準は `data/terrain/horseshoe_bend.json` と `CanyonRace/Content/Canyon/Data/provenance.json` に保存しています。

- [USGS 3DEP ImageServer](https://elevation.nationalmap.gov/arcgis/rest/services/3DEPElevation/ImageServer)
- [Poly Haven Rock Face](https://polyhaven.com/a/rock_face)：4K岩肌写真、DirectX法線、AO／roughness／metallic画像。
- [Poly Haven Rocky Terrain 02](https://polyhaven.com/a/rocky_terrain_02)：2K地面写真、法線、マスク画像。
- [Poly Haven Kloppenheim 06 Pure Sky](https://polyhaven.com/a/kloppenheim_06_puresky)：4K HDRの空。

Poly Havenの素材は [CC0](https://polyhaven.com/license)。ダウンロードURL・サイズ・チェックサムは `data/art/manifest.json`。岩肌写真や空はHorseshoe Bend現地の撮影ではなく、実標高に別の公開素材を組み合わせた景観です。航空写真を精密に再現したものではありません。

機体、装飾用の岩3種、エンジン音・衝突音・ゲート音は、このプロジェクト用のオリジナル生成素材です。生成ソースは `scripts/generate-models-audio.py`。Fatal Inertiaの画像、音声、機体モデルやゲームデータは使用していません。

水面は標高・傾斜から判定した領域へ割り当てた不透明PBR素材です。見た目の波は法線のアニメーションであり、地形衝突や浮力に波浪シミュレーションは加えていません。装飾用の岩は当たり判定を持たず、元の地形メッシュが衝突面です。
