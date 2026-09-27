# 手動リリース

初版は手動リリース。自動runnerや定期ジョブは設置していません。

1. 検証済みの変更をmainへマージする。`VERSION` と `DefaultGame.ini` のProjectVersionを更新する。
2. mainが未変更の状態で `scripts/Build-Release.ps1` を実行する。
3. スクリプトはmainの新しいクローンでエディタビルド、ルールテスト、ワールド検証、Win64パッケージ、両コースと新操作の走行検証を行う。
4. `release/` のZIP、SHA-256、build-info.jsonを確認する。Saved、個人ログ、PDB、ビルドmanifestは配布しない。
5. build-info.jsonのcommitとmainが一致することを確認し、そのcommitを指す `v<version>` のGitHub Releaseへ3ファイルをアップロードする。リリースノートは `docs/releases/<version>.md`。

```powershell
.\scripts\Build-Release.ps1 -EngineRoot 'E:\Epic Games\UE_5.8'
```

Git、UE 5.8.3、Visual Studio 2022のC++ビルドツール、Windows SDKが必要。
公開リポジトリにはゲーム固有のContent/Canyon、ソース、設定、生成スクリプトを含める。
Unreal本体、未使用テンプレート、認証情報、実機の入力ログを含めない。
原本のCC0画像やGeoTIFFの再取得は通常のビルドには不要。

将来自動化する場合は、main更新から同じ検証手順を起動し、成功したcommitだけにバージョンとReleaseを付ける。
このPCを使うrunnerは非公開のビルド用リポジトリへ分離し、公開forkのPRを実行しない構成を検討する。
公開repoに永続self-hosted runnerを直結する構成は採用していない。
