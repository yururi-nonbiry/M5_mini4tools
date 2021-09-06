from os import read
import numpy as np
import serial
import configparser
import time
import datetime
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import tkinter
import tkinter.ttk as ttk
import threading
import sys


config_pass = "./config.ini"
log_pass = "./log"

class root():

    def __init__(self):

        #コンフィグ読取
        self.config_read()

        # 変数定義
        self.serial_valid = 0 #シリアル通信継続用
        self.serial_task_mode = 0 # シリアルのタスクモード
        self.log_valid = 0 # ログの継続用

        self.calibration_value = None # キャリブレーション用の値

        self.root = tkinter.Tk()
        self.root.title("mini4tools")
        self.root.geometry("800x900")

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

        self.log_bt_text = tkinter.StringVar()
        self.log_bt_text.set('スタート')
        self.log_bt = tkinter.Button(self.root,textvariable=self.log_bt_text, width=8)
        self.log_bt.bind("<Button-1>", self.log_task)
        self.log_bt.place(x=500, y=50)


        # テキスト表示(静的)
        self.serial_info = tkinter.Label(self.root, text=u'シリアル接続')
        self.serial_info.place(x=10, y=0)

        self.calibration_info = tkinter.Label(self.root, text=u'キャリブレーション')
        self.calibration_info.place(x=10, y=30)

        self.current_info = tkinter.Label(self.root, text=u'電流(A)', font=('','16'))
        self.current_info.place(x=10, y=80)

        self.rpm_info = tkinter.Label(self.root, text=u'回転数(RPM)', font=('','16'))
        self.rpm_info.place(x=10, y=110)

        self.loadsell_info = tkinter.Label(self.root, text=u'トルク(g/cm)', font=('','16'))
        self.loadsell_info.place(x=10, y=140)

        self.trig_info = tkinter.Label(self.root, text=u'トリガ', font=('','16'))
        self.trig_info.place(x=400, y=80)

        self.trig_mode_info = tkinter.Label(self.root, text=u'種類', font=('','16'))
        self.trig_mode_info.place(x=400, y=110)

        self.trig_mode_info = tkinter.Label(self.root, text=u'ログ', font=('','16'))
        self.trig_mode_info.place(x=400, y=50)

        # テキスト表示(動的)
        self.calibration_label_text = tkinter.StringVar()
        self.calibration_label_text.set("未完了")
        self.calibration_label = tkinter.Label(self.root, textvariable=self.calibration_label_text)
        self.calibration_label.place(x=300, y=30)

        self.loadsell_calibration_label_text = tkinter.StringVar()
        self.loadsell_calibration_label_text.set("未完了")
        self.loadsell_calibration_label = tkinter.Label(self.root, textvariable=self.loadsell_calibration_label_text)
        self.loadsell_calibration_label.place(x=400, y=30)

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

        self.loadsell_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.loadsell_entry.place(x=200, y=140)
        self.loadsell_entry.configure(state='normal')
        self.loadsell_entry.delete(0,tkinter.END)
        self.loadsell_entry.insert('end','-')
        self.loadsell_entry.configure(state='readonly')

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
        self.ax1 = self.fig.add_subplot(2,2,1)
        #ax1.plot(x1, y1)
        self.line1, = self.ax1.plot(0, 0, color='blue')
        self.ax1.set_title('line plot')
        self.ax1.set_ylabel('Damped oscillation')

        # ax2
        self.ax2 = self.fig.add_subplot(2,2,2)
        self.line2, = self.ax2.plot(0, 0, color='blue')
        #ax2.scatter(x1, y1, marker='o')
        self.ax2.set_title('Scatter plot')

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
        self.canvas.get_tk_widget().place(x=0, y=200)

        #anim = FuncAnimation(fig, update, frames=range(8), interval=1000)

        #ツールバーを表示
        self.toolbar=NavigationToolbar2Tk(self.canvas, self.root)
        self.toolbar.place(x=10, y=800)

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

    # 電流と回転数の表示
    def culc(self,buf,loadsell):

        # 電流計算
        current_np = np.array(buf,dtype=float)
        current_np = ((current_np - self.calibration_value) * float(self.current_gain) /100 + float(self.current_offset)) /200 # 電流計算
        self.current_average = abs(np.average(current_np))

        # 電流表示
        self.current_entry.configure(state='normal')
        self.current_entry.delete(0,tkinter.END)
        self.current_entry.insert('end','{:.3f}'.format(self.current_average))
        self.current_entry.configure(state='readonly')

        # 回転数計算
        current_fft = np.fft.fft(current_np)
        current_fft_abs = np.abs(current_fft)
        current_fft_abs = current_fft_abs[1:int(len(current_fft_abs)/2)]
        fft_peak = np.argmax(current_fft_abs)
        fft_rpm = int(self.raw_frequency)/int(self.raw_sample) * (fft_peak + 1) * 10

        self.rpm_entry.configure(state='normal')
        self.rpm_entry.delete(0,tkinter.END)
        self.rpm_entry.insert('end','{:.3f}'.format(fft_rpm))
        self.rpm_entry.configure(state='readonly')

        # 電流波形計算
        current_graph = current_np[:200]
        self.line1.remove()
        self.line1, = self.ax1.plot(current_graph, color='blue')

        # FFTグラフ計算
        self.line2.remove()
        self.line2, = self.ax2.plot(current_fft_abs, color='blue')
        self.canvas.draw()

        # トルク計算
        loadsell_np = np.array(loadsell,dtype=float)
        loadsell_np = ((loadsell_np - self.loadsell_calibration_value) * float(self.loadsell_gain) /100 + float(self.loadsell_offset)) /200 
        self.loadsell_average = abs(np.average(loadsell_np))

        # トルク表示
        self.loadsell_entry.configure(state='normal')
        self.loadsell_entry.delete(0,tkinter.END)
        self.loadsell_entry.insert('end','{:.3f}'.format(self.loadsell_average))
        self.loadsell_entry.configure(state='readonly')



        # ログの保存
        if self.log_valid == 1:
            self.log_current.append(self.current_average) # 電流ログ
            self.log_rpm.append(fft_rpm) # 回転数ログ
            self.line3, = self.ax3.plot(self.log_current, self.log_rpm, color='blue') # グラフ用

            with open(log_pass + self.log_title + '.csv', 'a') as f:
                print(fft_rpm + ',' + self.current_average + ',' + self.loadsell_average, file=f)
        

    def calibration(self,event):
        self.serial_task_mode = 1

    def calibration_task(self,buf,loadsell):
        buf_np = np.array(buf,dtype=float)
        self.calibration_value = np.average( buf_np )
        self.calibration_label_text.set(str(self.calibration_value))

        loadsell_np = np.array(loadsell,dtype=float)
        self.loadsell_calibration_value = np.average( loadsell_np )
        self.loadsell_calibration_label_text.set(str(self.loadsell_calibration_value))

    # ログボタンの処理
    def log_task(self,event):

        if self.log_valid == 0:

            # self.log_list = [] #ログデータを空にする
            self.log_current = [] # 電流用
            self.log_rpm = [] # 回転数用

            tdatetime = datetime.datetime.now()
            self.log_title = tdatetime.strftime('%Y/%m/%d_%H:%M:%S')
            self.log_valid = 1
            self.log_bt_text.set("ストップ")

            with open(log_pass + self.log_title + '.csv', 'w') as f:
                print(self.log_title + ' log start', file=f)
            

        else:
            self.log_valid = 0
            self.log_bt_text.set("スタート")
            


        
    # コンフィグ読込
    def config_read(self):
    
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
            self.loadsell_sample = raw_config.get('loadsell_sample')
        except KeyError:
            self.raw_sample = None
            self.raw_frequency = None

        if self.raw_sample == None:
            self.raw_sample = '2048'
        if self.raw_frequency == None:
            self.raw_frequency = '10000'
        if self.loadsell_sample == None:
            self.loadsell_sample = '64'

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
            loadsell_config = config_ini['loadsell']
            self.loadsell_gain = loadsell_config.get('gain')
            self.loadsell_offset = loadsell_config.get('offset')
        except KeyError:
            self.loadsell_gain = None
            self.loadsell_offset = None

        if self.loadsell_gain == None:
            self.loadsell_gain = '100'
        if self.loadsell_offset == None:
            self.loadsell_offset = '0'

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
        
    # iniファイル書き込み
    def config_write(self):

        config = configparser.ConfigParser()
        section_serial = 'serial'
        config.add_section(section_serial)
        config.set(section_serial, 'port', self.serial_port)
        config.set(section_serial, 'bps', self.serial_bps)

        section_raw = 'raw'
        config.add_section(section_raw)
        config.set(section_raw, 'sample', self.raw_sample)
        config.set(section_raw, 'sampling_frequency', self.raw_frequency)
        config.set(section_raw, 'loadsell_sample', self.raw_sample)

        section_current = 'current'
        config.add_section(section_current)
        config.set(section_current, 'gain', self.current_gain)
        config.set(section_current, 'offset', self.current_offset)

        section_loadsell = 'loadsell'
        config.add_section(section_loadsell)
        config.set(section_loadsell, 'gain', self.loadsell_gain)
        config.set(section_loadsell, 'offset', self.loadsell_offset)

        section_oscilloscope = 'oscilloscope'
        config.add_section(section_oscilloscope)
        config.set(section_oscilloscope, 'trig', self.trig_entry.get())
        config.set(section_oscilloscope, 'trig_mode', self.v.get())

        with open(config_pass, 'w',encoding='utf-8') as configfile:
            # 指定したconfigファイルを書き込み
            config.write(configfile)


    def serial_read(self):

        read_trig = 0 #sirial読み込み用トリガ

        buf_read = []
        buf_read.clear()
        buf_loadsell = [] 
        buf_loadsell.clear() 

        with serial.Serial(self.serial_port, self.serial_bps) as ser:  # timeoutを秒で設定（default:None)ボーレートはデフォルトで9600
            while(self.serial_valid):
                #buf_read.append(ser.read())
                while(ser.in_waiting > 0): # シリアルの受信が無くなるまで繰り返し
                    #buf_one = int.from_bytes(ser.read(1),'big')

                    if read_trig == 0: 
                        #ヘッダー検出
                        if int.from_bytes(ser.read(1),'big') == 0xff:
                            read_byte = int.from_bytes(ser.read(1),'big')

                            # 送信完了
                            if read_byte == 0xff: 
                                read_trig = 1 # 計算スタート
                            
                             # 電流値送信
                            if read_byte == 0xfe:
                                buf_read.clear() # 読み込み用bufを空にする
                                read_trig = 2 #シリアル読み込みスタート
                            
                            # トルク送信
                            if read_byte == 0xfd: 
                                buf_loadsell.clear() # 読み込み用bufを空にする
                                read_trig = 3 #シリアル読み込みスタート
                                
                    # 電流値計算        
                    if read_trig == 2:
                        # シリアル読み込み
                        buf_read.append(int.from_bytes(ser.read(1),'big'))
                        if len(buf_read) >= int(self.raw_sample)*2:
                            read_trig = 0
                    
                                
                    # トルク値計算        
                    if read_trig == 2:
                        # シリアル読み込み
                        buf_loadsell.append(int.from_bytes(ser.read(1),'big'))
                        if len(buf_loadsell) >= int(self.loadsell_sample)*2:
                            read_trig = 0


                     # 計算処理
                    if read_trig == 1:
                       
                        buf_culc = []
                        buf_culc.clear()
                        for i in range(int(len(buf_read)/2)):
                            buf_culc.append((buf_read[i*2] << 8)  + buf_read[i*2+1])

                        loadsell_culc = []
                        loadsell_culc.clear()
                        for i in range(int(len(buf_loadsell)/4)):
                            loadsell_culc.append((loadsell_read[i*2] << 24) + (loadsell_read[i*2+1] << 16) + (loadsell_read[i*2+2] << 8) + loadsell_read[i*2+3])

                        print('start')

                        # 常時行う処理
                        if self.calibration_value != None:
                            self.culc(buf_culc,loadsell_culc) # 電流と回転数の表示


                        if self.serial_task_mode == 0: # パスモード
                            pass

                        elif self.serial_task_mode == 1: # キャリブレーション
                            print('calibration')
                            self.calibration_task(buf_culc,loadsell_culc)
                            self.serial_task_mode = 0

                        elif self.serial_task_mode == 2: # 各種表示
                            cur_list = (buf_culc - self.calibration_value) * self.current_gain / 200 + self.current_offset # 電流計算

                        read_trig = 0


                time.sleep(0.1)


if __name__ == '__main__':
    app = root()
