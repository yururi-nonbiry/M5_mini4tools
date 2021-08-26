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

config_pass = "./enami_config.ini"

class root():

    def __init__(self):

        # 変数定義
        self.serial_valid = 0 #シリアル通信継続用
        self.serial_task_mode = 0 # シリアルのタスクモード

        self.calibration_value = None # キャリブレーション用の値

        self.root = tkinter.Tk()
        self.root.title("enami03")
        self.root.geometry("800x800")

        self.moter_temp1_limit = 0
        self.moter_temp2_limit = 0
        self.moter_current1_limit = 0
        self.moter_current2_limit = 0

        self.config_read()


        # テキスト表示(静的)
        self.moter_temp1_info = tkinter.Label(self.root, text=u'モーター温度(北側)', font=('','16'))
        self.moter_temp1_info.place(x=10, y=20)

        self.moter_temp2_info = tkinter.Label(self.root, text=u'モーター温度(南側)', font=('','16'))
        self.moter_temp2_info.place(x=10, y=50)

        self.moter_current1_info = tkinter.Label(self.root, text=u'モーター電流(北側)', font=('','16'))
        self.moter_current1_info.place(x=10, y=80)

        self.moter_current2_info = tkinter.Label(self.root, text=u'モーター電流(南側)', font=('','16'))
        self.moter_current2_info.place(x=10, y=110)

   
        # 表示
        self.moter_temp1_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_temp1_entry.place(x=200, y=20)
        self.moter_temp1_entry.configure(state='normal')
        self.moter_temp1_entry.delete(0,tkinter.END)
        self.moter_temp1_entry.insert('end','-')
        self.moter_temp1_entry.configure(state='readonly')

        self.moter_temp2_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_temp2_entry.place(x=200, y=50)
        self.moter_temp2_entry.configure(state='normal')
        self.moter_temp2_entry.delete(0,tkinter.END)
        self.moter_temp2_entry.insert('end','-')
        self.moter_temp2_entry.configure(state='readonly')

        self.moter_current1_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_current1_entry.place(x=200, y=80)
        self.moter_current1_entry.configure(state='normal')
        self.moter_current1_entry.delete(0,tkinter.END)
        self.moter_current1_entry.insert('end','-')
        self.moter_current1_entry.configure(state='readonly')

        self.moter_current2_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_current2_entry.place(x=200, y=110)
        self.moter_current2_entry.configure(state='normal')
        self.moter_current2_entry.delete(0,tkinter.END)
        self.moter_current2_entry.insert('end','-')
        self.moter_current2_entry.configure(state='readonly')

        # リミット
        self.moter_temp1_limit_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_temp1_limit_entry.place(x=350, y=20)
        self.moter_temp1_limit_entry.configure(state='normal')
        self.moter_temp1_limit_entry.delete(0,tkinter.END)
        self.moter_temp1_limit_entry.insert('end',self.moter_temp1_limit)
        #self.moter_temp1_entry.configure(state='readonly')

        self.moter_temp2_limit_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_temp2_limit_entry.place(x=350, y=50)
        self.moter_temp2_limit_entry.configure(state='normal')
        self.moter_temp2_limit_entry.delete(0,tkinter.END)
        self.moter_temp2_limit_entry.insert('end',self.moter_temp2_limit)
        #self.moter_temp2_entry.configure(state='readonly')

        self.moter_current1_limit_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_current1_limit_entry.place(x=350, y=80)
        self.moter_current1_limit_entry.configure(state='normal')
        self.moter_current1_limit_entry.delete(0,tkinter.END)
        self.moter_current1_limit_entry.insert('end',self.moter_current1_limit)
        #self.moter_current1_entry.configure(state='readonly')

        self.moter_current2_limit_entry = tkinter.Entry(self.root,text="",width=10, font=('','16'),justify=tkinter.RIGHT)
        self.moter_current2_limit_entry.place(x=350, y=110)
        self.moter_current2_limit_entry.configure(state='normal')
        self.moter_current2_limit_entry.delete(0,tkinter.END)
        self.moter_current2_limit_entry.insert('end',self.moter_current2_limit)
        #self.moter_current2_entry.configure(state='readonly')
        
        #コンフィグ読取
        self.config_read()

        # ウインドウを閉じたときの処理
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)

        # Gui起動
        self.root.mainloop()

    # ウインドウが閉じたらプログラムも終了する
    def on_closing(self):

        # iniファイル書き込み
        config = configparser.ConfigParser()
        # config_ini.read(config_pass, encoding='utf-8')
        section_enami03 = 'enami03'
        config.add_section(section_enami03)
        config.set(section_enami03, 'moter_temp1_limit', self.moter_temp1_limit_entry.get())
        config.set(section_enami03, 'moter_temp2_limit', self.moter_temp2_limit_entry.get())
        config.set(section_enami03, 'moter_current1_limit', self.moter_current1_limit_entry.get())
        config.set(section_enami03, 'moter_current2_limit', self.moter_current2_limit_entry.get())
     

        with open(config_pass, 'w') as configfile:
            # 指定したconfigファイルを書き込み
            config.write(configfile)

        self.root.destroy() # 画面を閉じる
        sys.exit(0) # プログラム自体を終了する

    def serial_start(self):
        pass

    def calibration(self):
        pass

    # コンフィグ読込
    def config_read(self):

        config_ini = configparser.ConfigParser()
        config_ini.read(config_pass, encoding='utf-8')

        try:
            serial_config = config_ini['enami03']
            self.moter_temp1_limit = serial_config.get('moter_temp1_limit')
            self.moter_temp2_limit = serial_config.get('moter_temp2_limit')
            self.moter_current1_limit = serial_config.get('moter_current1_limit')
            self.moter_current2_limit = serial_config.get('moter_current2_limit')

        except KeyError:
            self.moter_temp1_limit = None
            self.moter_temp2_limit = None
            self.moter_current1_limit = None
            self.moter_current2_limit = None

        if self.moter_temp1_limit == None:
            self.moter_temp1_limit = 0
        if self.moter_temp2_limit == None:
            self.moter_temp2_limit = 0
        if self.moter_current1_limit == None:
            self.moter_current1_limit = 0
        if self.moter_current2_limit == None:
            self.moter_current2_limit = 0


if __name__ == '__main__':

    app = root()
