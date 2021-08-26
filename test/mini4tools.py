from os import read
import numpy as np
import serial
import configparser
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import tkinter
import tkinter.ttk as ttk
import threading
import sys


#from matplotlib.figure import Figure

config_pass = "./config.ini"

class root():

    def __init__(self):

        #コンフィグ読取
        self.config_read()

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
        self.serial_info = tkinter.Label(self.root, text=u'シリアル接続')
        self.serial_info.place(x=10, y=0)

        self.calibration_info = tkinter.Label(self.root, text=u'キャリブレーション')
        self.calibration_info.place(x=10, y=30)

        self.current_info = tkinter.Label(self.root, text=u'電流(A)', font=('','16'))
        self.current_info.place(x=10, y=80)

        self.rpm_info = tkinter.Label(self.root, text=u'回転数(RPM)', font=('','16'))
        self.rpm_info.place(x=10, y=110)

        self.trig_info = tkinter.Label(self.root, text=u'トリガ', font=('','16'))
        self.trig_info.place(x=400, y=80)

        self.trig_mode_info = tkinter.Label(self.root, text=u'種類', font=('','16'))
        self.trig_mode_info.place(x=400, y=110)

        # テキスト表示(動的)
        self.calibration_label_text = tkinter.StringVar()
        self.calibration_label_text.set("未完了")
        self.calibration_label = tkinter.Label(self.root, textvariable=self.calibration_label_text)
        self.calibration_label.place(x=300, y=30)

        """
                self.calibration_label_text = tkinter.StringVar()
                self.calibration_label_text.set("-")
                self.calibration_label = tkinter.Label(self.root, textvariable=self.calibration_label_text)
                self.calibration_label.place(x=300, y=30)

                self.calibration_label_text = tkinter.StringVar()
                self.calibration_label_text.set("-")
                self.calibration_label = tkinter.Label(self.root, textvariable=self.calibration_label_text)
                self.calibration_label.place(x=300, y=30)
        """
        # 入力欄
        self.current_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.current_entry.place(x=200, y=80)
        self.current_entry.configure(state='normal')
        self.current_entry.delete(0,tkinter.END)
        self.current_entry.insert('end','-')
        self.current_entry.configure(state='readonly')

        self.rpm_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.rpm_entry.place(x=200, y=110)
        self.rpm_entry.configure(state='normal')
        self.rpm_entry.delete(0,tkinter.END)
        self.rpm_entry.insert('end','-')
        self.rpm_entry.configure(state='readonly')

        self.trig_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.trig_entry.place(x=500, y=80)
        self.trig_entry.configure(state='normal')
        self.trig_entry.delete(0,tkinter.END)
        self.trig_entry.insert('end',self.oscilloscope_trig)

        #コンボボックス
        self.v = tkinter.StringVar()
        #self.v = "テスト"
        self.trig_mode_list = ('両方','立上り','立下り')
        self.trig_mode_combo = ttk.Combobox(self.root, height=3,width=10, font=('','16'),state="readonly", textvariable= self.v,values=self.trig_mode_list)
        self.trig_mode_combo.set(self.oscilloscope_trig_mode)
        #self.rpm_entry.insert()
        #self.trig_mode_combo.bind("<<ComboboxSelected>>",self.get_trig_mode) # コンボボックスが変化したら値を取得する
        self.trig_mode_combo.place(x=500, y=110)
 
        
        # グラフ表示
        x1 = np.linspace(0.0, 5.0)
        y1 = np.cos(2 * np.pi * x1) * np.exp(-x1)
        x2 = np.linspace(0.0, 3.0)
        y2 = np.cos(2 * np.pi * x2) * np.exp(-x1)

        self.fig = plt.figure(figsize=(8,6),dpi=100)
        # ax1
        ax1 = self.fig.add_subplot(2,2,1)
        #ax1.plot(x1, y1)
        ax1.set_title('line plot')
        ax1.set_ylabel('Damped oscillation')

        # ax2
        ax2 = self.fig.add_subplot(2,2,2)
        #ax2.scatter(x1, y1, marker='o')
        ax2.set_title('Scatter plot')

        # ax3
        ax3 = self.fig.add_subplot(2,2,3)
        #ax3.plot(x2, y2)
        ax3.set_ylabel('Damped oscillation')
        ax3.set_xlabel('time (s)')

        # ax4
        ax4 = self.fig.add_subplot(2,2,4)
        #ax4.scatter(x2, y2, marker='o')
        ax4.set_xlabel('time (s)')
        #self.fig, ax_current = plt.subplots(1, 1)
        #plt.plot(buf)
        #line, = ax.plot( buf, color='blue')
        #plt.pause(0.01)
        #line.remove()
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.root)  # Generate canvas instance, Embedding fig in root
        self.canvas.draw()
        self.canvas.get_tk_widget().place(x=0, y=150)

        #ツールバーを表示
        self.toolbar=NavigationToolbar2Tk(self.canvas, self.root)
        self.toolbar.place(x=10, y=750)

        # ウインドウを閉じたときの処理
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

        # Gui起動
        self.root.mainloop()

    # コンボボックスの値取得用
    #def get_trig_mode(self):
    #    print(self.trig_mode_combo.get())

    #    self.oscilloscope_trig_mode = self.v.get()

    # ウインドウが閉じたらプログラムも終了する
    def on_closing(self):
        self.config_write() # コンフィグを書き込む
        self.serial_valid = 0 # シリアル通信 切断用
        self.root.destroy() # 画面を閉じる
        time.sleep(3) # 終了まで3秒待つ(シリアル通信が切れるまで)
        sys.exit(0) # プログラム自体を終了する

    def changeText(self):
        self.text.set("Updated Text")

    def open_file(self):
        pass
    def close_disp(self):
        pass

    # シリアルの接続と切断
    def serial_start(self,event):

        # 別スレッドを立ててシリアル通信を実行する
        if self.serial_valid == 0 :

            self.serial_valid = 1 # シリアル通信 継続用
            self.connect_bt_text.set("切断")
            self.serial_thread = threading.Thread(target=self.serial_read)
            self.serial_thread.start()

        # 別スレッドのシリアル通信を終了する
        else:
            self.serial_valid = 0 # シリアル通信 切断用
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
        """
        global serial_port
        global serial_bps
        global raw_sample
        global raw_frequency
        global current_gain
        global current_offset
        """

        config_ini = configparser.ConfigParser()
        config_ini.read(config_pass, encoding='utf-8')

        try:
            serial_config = config_ini['serial']
            self.serial_port = serial_config.get('port')
            self.serial_bps = serial_config.get('bps')
        except KeyError:
            self.serial_port = None
            self.serial_bps = None
        
        if self.serial_port == None:
            self.serial_port = 'com1'
        if self.serial_bps == None:
            self.serial_bps = '115200'

        try:
            raw_config = config_ini['raw']
            self.raw_sample = raw_config.get('sample')
            self.raw_frequency = raw_config.get('sampling_frequency')
        except KeyError:
            self.raw_sample = None
            self.raw_frequency = None

        if self.raw_sample == None:
            self.raw_sample = '2048'
        if self.raw_frequency == None:
            self.raw_frequency = '10000'

        try:
            current_config = config_ini['current']
            self.current_gain = current_config.get('gain')
            self.current_offset = current_config.get('offset')
        except KeyError:
            self.current_gain = None
            self.current_offset = None

        if self.current_gain == None:
            self.current_gain = '100'
        if self.current_offset == None:
            self.current_offset = '0'

        try:
            oscilloscope_config = config_ini['oscilloscope']
            self.oscilloscope_trig = current_config.get('trig')
            self.oscilloscope_trig_mode = current_config.get('trig_mode')
        except KeyError:
            self.oscilloscope_trig = None
            self.oscilloscope_trig_mode = None

        if self.oscilloscope_trig == None:
            self.oscilloscope_trig = '0'
        if self.oscilloscope_trig_mode == None:
            self.oscilloscope_trig_mode = '両方'
        
    def config_write(self):
        # iniファイル書き込み
        config = configparser.ConfigParser()
        section_serial = 'serial'
        config.add_section(section_serial)
        config.set(section_serial, 'port', self.serial_port)
        config.set(section_serial, 'bps', self.serial_bps)

        section_raw = 'raw'
        config.add_section(section_raw)
        config.set(section_raw, 'sample', self.raw_sample)
        config.set(section_raw, 'sampling_frequency', self.raw_frequency)

        section_current = 'current'
        config.add_section(section_current)
        config.set(section_current, 'gain', self.current_gain)
        config.set(section_current, 'offset', self.current_offset)

        section_oscilloscope = 'oscilloscope'
        config.add_section(section_oscilloscope)
        config.set(section_oscilloscope, 'trig', self.trig_entry.get())
        config.set(section_oscilloscope, 'trig_mode', self.v.get())

     

        with open(config_pass, 'w',encoding='utf-8') as configfile:
            # 指定したconfigファイルを書き込み
            config.write(configfile)


    def serial_read(self):

        """ 
        global serial_port
        global serial_bps
        global raw_sample
        global raw_frequency
        global current_gain
        global current_offset
        """

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
