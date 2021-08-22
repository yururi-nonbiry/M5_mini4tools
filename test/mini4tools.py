from os import read
import serial
import configparser
import time
import matplotlib.pyplot as plt
import tkinter
import threading

config_pass = "./config.ini"

class root():

    def __init__(self):

        # 変数定義
        self.serial_valid = 0 #シリアル通信継続用
        self.serial_task_mode = 0 # シリアルのタスクモード

        self.calibration_value = None # キャリブレーション用の値

        self.root = tkinter.Tk()
        self.root.title("mini4tools")
        self.root.geometry("800x800")

        #メニューバー
        self.men = tkinter.Menu(self.root)
        self.root.config(menu=self.men) 

        #メニューに親メニュー（ファイル）を作成する 
        self.menu_file = tkinter.Menu(self.root) 
        self.men.add_cascade(label='ファイル', menu=self.menu_file) 

        #親メニューに子メニュー（開く・閉じる）を追加する 
        self.menu_file.add_command(label='開く', command=self.open_file) 
        self.menu_file.add_separator() 
        self.menu_file.add_command(label='閉じる', command=self.close_disp)

        #ボタン関係
        self.connect_bt_text = tkinter.StringVar()
        self.connect_bt_text.set("接続")
        self.connect_bt = tkinter.Button(self.root, textvariable=self.connect_bt_text, width=15)
        self.connect_bt.place(x=105, y=60)
        self.connect_bt.bind("<Button-1>",self.serial_start) 

        self.calibration_bt = tkinter.Button(self.root,text="キャリブレーション", width=15)
        self.calibration_bt.bind("<Button-1>", self.calibration)
        self.calibration_bt.place(x=300, y=60)
        #self.buttonB.pack(side=tkinter.RIGHT)

        # テキスト表示
        self.calibration_label_text = tkinter.StringVar()
        self.calibration_label_text.set("未完了")
        self.calibration_label = tkinter.Label(self.root, textvariable=self.calibration_label_text)
        self.calibration_label.place(x=100, y=300)
        self.calibration_label.pack()

        #コンフィグ読取
        self.config_read()

        # Gui起動
        self.root.mainloop()

    def changeText(self):
        self.text.set("Updated Text")

    def open_file(self):
        pass
    def close_disp(self):
        pass

    # シリアルの接続と切断
    def serial_start(self,event):

        if self.serial_valid == 0 :

            self.serial_valid = 1 # シリアル通信継続用
            self.connect_bt_text.set("切断")
            self.serial_thread = threading.Thread(target=self.serial_read)
            self.serial_thread.start()

        else:
            self.serial_valid = 0 # シリアル通信継続用
            self.connect_bt_text.set("接続")

    def culc(self,buf):
        pass
        #fig, ax = plt.subplots(1, 1)
        #plt.plot(buf)
        #line, = ax.plot( buf, color='blue')
        #plt.pause(0.01)
        #line.remove()

    def calibration(self,event):
        self.serial_task_mode = 1

    def calibration_task(self,buf):
        self.calibration_value = sum(buf)/len(buf)
        self.calibration_label_text.set(str(self.calibration_value))
        

    def config_read(self):
        global serial_port
        global serial_bps
        global raw_sample
        global raw_frequency

        config_ini = configparser.ConfigParser()
        config_ini.read(config_pass, encoding='utf-8')
        serial_config = config_ini['serial']
        serial_port = serial_config.get('port')
        serial_bps = serial_config.get('bps')
        raw_config = config_ini['raw']
        raw_sample = raw_config.get('sample')
        raw_frequency = raw_config.get('sampling_frequency')

    def serial_read(self):

        global serial_port
        global serial_bps
        global raw_sample
        global raw_frequency

        read_trig = 0 #sirial読み込み用トリガ
        buf_read = []

        with serial.Serial(serial_port, serial_bps) as ser:  # timeoutを秒で設定（default:None)ボーレートはデフォルトで9600
            while(self.serial_valid):
                #buf_read.append(ser.read())
                while(ser.in_waiting > 0): # シリアルの受信が無くなるまで繰り返し
                    #buf_one = int.from_bytes(ser.read(1),'big')

                    if read_trig == 0: 
                        #ヘッダー検出
                        if int.from_bytes(ser.read(1),'big') == 0xff:
                            if int.from_bytes(ser.read(1),'big') == 0xff:
                                buf_read = [] # 読み込み用bufを空にする
                                read_trig = 1 #シリアル読み込みスタート
                    
                    if read_trig == 1:
                        # シリアル読み込み
                        buf_read.append(int.from_bytes(ser.read(1),'big'))
                        if len(buf_read) >= int(raw_sample)*2:
                            read_trig = 2
                    
                    if read_trig == 2:
                        # 計算処理
                        buf_culc = []
                        for i in range(int(len(buf_read)/2)):
                            buf_culc.append((buf_read[i*2] << 8)  + buf_read[i*2+1])

                        print('start')
                        if self.serial_task_mode == 0: # パスモード
                            pass
                        elif self.serial_task_mode == 1: # キャリブレーション
                            print('calibration')
                            self.calibration_task(buf_culc)
                            self.serial_task_mode = 0

                        self.culc(buf_read)
                        read_trig = 0

                time.sleep(0.1)


"""
def ticker_task():



    #メニューバー
    men = tkinter.Menu(root)
    root.config(menu=men) 

    #メニューに親メニュー（ファイル）を作成する 
    menu_file = tkinter.Menu(root) 
    men.add_cascade(label='ファイル', menu=menu_file) 

    #親メニューに子メニュー（開く・閉じる）を追加する 
    menu_file.add_command(label='開く', command=open_file) 
    menu_file.add_separator() 
    menu_file.add_command(label='閉じる', command=close_disp)

    #ボタン
    Button = tkinter.Button(text=u'接続', width=15)
    Button.place(x=105, y=60)
    Button.bind("<Button-1>",serial_read) 
    #Button.pack()

    # ラベルの作成
    label = tkinter.Label(root, text="This is the Label.")

    #ラベルの表示
    label.grid()

    root.mainloop()

"""

if __name__ == '__main__':
    #config_read()
    #serial_read()
    #ticker_task()
    app = root()