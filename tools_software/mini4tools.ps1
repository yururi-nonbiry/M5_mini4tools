# python 仮想環境構築スクリプト
# このテキストファイルをコピーして[build_venv.ps1]等の名前で保存する
# 環境を構築したいフォルダに移動する
# 右クリックをして[PowerShellで実行]を押す

#仮想環境を構築して、仮想環境に入るためのコマンド
python -m venv mini4tools
.\mini4tools\Scripts\activate

# プロキシを通さないと外部にアクセスできない場合は
# 下記のコメントアウトを外してプロキシのアドレスとポート番号を入れる
#$env:http_proxy="192.168.0.0:0000"
#$env:https_proxy="192.168.0.0:0000"

# pipをアップグレードする
python -m pip install --upgrade pip

# 下記に入れたいライブラリを記載していく
python -m pip install numpy
python -m pip install pyinstaller
python -m pip install pyserial
python -m pip install matplotlib

#ウインドウが勝手に閉じないようにする
cmd /k