from os import read
import numpy as np
import serial
import configparser
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
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
        self.connect_bt = tkinter.Button(self.root, textvariable=self.connect_bt_text, width=5)
        self.connect_bt.place(x=200, y=0)
        self.connect_bt.bind("<Button-1>",self.serial_start) 

        self.calibration_bt = tkinter.Button(self.root,text="実行", width=5)
        self.calibration_bt.bind("<Button-1>", self.calibration)
        self.calibration_bt.place(x=200, y=30)
        #self.buttonB.pack(side=tkinter.RIGHT)

        # テキスト表示(静的)
        self.calibration_info = tkinter.Label(self.root, text=u'シリアル接続')
        self.calibration_info.place(x=10, y=0)
        self.calibration_info = tkinter.Label(self.root, text=u'キャリブレーション')
        self.calibration_info.place(x=10, y=30)

        # テキスト表示(動的)
        self.calibration_label_text = tkinter.StringVar()
        self.calibration_label_text.set("未完了")
        self.calibration_label = tkinter.Label(self.root, textvariable=self.calibration_label_text)
        self.calibration_label.place(x=300, y=30)

        # グラフ表示

        x1 = np.linspace(0.0, 5.0)
        y1 = np.cos(2 * np.pi * x1) * np.exp(-x1)
        x2 = np.linspace(0.0, 3.0)
        y2 = np.cos(2 * np.pi * x2) * np.exp(-x1)

        self.fig = plt.figure()
        # ax1
        ax1 = self.fig.add_subplot(221)
        ax1.plot(x1, y1)
        ax1.set_title('line plot')
        ax1.set_ylabel('Damped oscillation')

        # ax2
        ax2 = self.fig.add_subplot(222)
        ax2.scatter(x1, y1, marker='o')
        ax2.set_title('Scatter plot')

        # ax3
        ax3 = self.fig.add_subplot(223)
        ax3.plot(x2, y2)
        ax3.set_ylabel('Damped oscillation')
        ax3.set_xlabel('time (s)')

        # ax4
        ax4 = self.fig.add_subplot(224)
        ax4.scatter(x2, y2, marker='o')
        ax4.set_xlabel('time (s)')
        #self.fig, ax_current = plt.subplots(1, 1)
        #plt.plot(buf)
        #line, = ax.plot( buf, color='blue')
        #plt.pause(0.01)
        #line.remove()
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)  # Generate canvas instance, Embedding fig in root
        self.canvas.draw()
        self.canvas.get_tk_widget().pack()

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
        
    # コンフィグ読込
    def config_read(self):
        global serial_port
        global serial_bps
        global raw_sample
        global raw_frequency
        global current_gain
        global current_offset

        config_ini = configparser.ConfigParser()
        config_ini.read(config_pass, encoding='utf-8')
        serial_config = config_ini['serial']
        serial_port = serial_config.get('port')
        serial_bps = serial_config.get('bps')
        raw_config = config_ini['raw']
        raw_sample = raw_config.get('sample')
        raw_frequency = raw_config.get('sampling_frequency')
        current_config = config_ini['current']
        current_gain = current_config.get('gain')
        current_offset = current_config.get('offset')

    def serial_read(self):

        global serial_port
        global serial_bps
        global raw_sample
        global raw_frequency
        global current_gain
        global current_offset

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

                        elif self.serial_task_mode == 2: # 各種表示
                            cur_list = (buf_culc - self.calibration_value) * current_gain / 200 + current_offset # 電流計算

                        self.culc(buf_read)
                        read_trig = 0

                time.sleep(0.1)


if __name__ == '__main__':
    #config_read()
    #serial_read()
    #ticker_task()
    app = root()
