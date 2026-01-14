# -*- coding: utf-8 -*-
"""
Extrusion force test bench

Code for GUI
"""

import tkinter as tk
from tkinter import ttk
import tkinter.scrolledtext as st
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2Tk)
import numpy as np
import serial
import serial.tools.list_ports


# %%

"""
globale Variablen
"""

baud_rate=115200

schrift_klein=12
schrift_gross=15


min_temp=180 #Temperatur in Grad Celsius, die nicht unterschritten werden darf
max_temp=300 #Temperatur in Grad Celsius, die nicht überschritten werden darf

diameter=1.75 #Durchmesser Filament

start=0 #1, falls Messung beginnt

#esp32s3
VENDOR_ID = 0x1A86
PRODUCT_ID = 0x55D3


#Listen zum Speichern der empfangenen Messdaten
time_list=[]
temp_list=[]
force_list=[]
slip_list=[]

init_time=-1. # Zeitwert von esp wenn Messung startet (in Millisekunden)

gesamtzeit=100. #in Sekunden

plot_cnt=0 #zählt bis wieder geplottet werden soll

# %%

"""
Funktionen
"""

def find_esp32_port(vendor_id, product_id):
    """
    Sucht den Port, an dem der ESP angeschlossen ist

    Parameters
    ----------
    vendor_id : int
        ID des Herstellers
    product_id : int
        ID des Produkts

    Returns
    -------
    TYPE
        Port des ESP oder none

    """
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if port.vid == vendor_id and port.pid == product_id:
            return port.device
    return None


def check_for_float(num):
    """
    Überprüft, ob string als float interpretierbar ist

    Parameters
    ----------
    num : string
        zu überprüfenden String

    Returns
    -------
    bool
        true, falls string als float interpretierbar ist
        false, sonst

    """
    if num==None:
        return False
    try:
        float(num)
        return True
    except ValueError:
        return False


def check_data():
    """
    Überprüft, ob die Eingaben korrekt sind und ruft send_data auf, falls das
    der Fall ist

    Returns
    -------
    None.

    """
    all_right=True
    
    
    temp_string=temp_entry.get()
    if not check_for_float(temp_string):
        all_right=False
    elif float(temp_string)>max_temp or float(temp_string)<min_temp:
        all_right=False
        
    feedrate_string=feedrate_entry.get()
    if not check_for_float(feedrate_string):
        all_right=False
    elif float(feedrate_string)<=0:
        all_right=False
    
    feedlength_string=feedlength_entry.get()
    if not check_for_float(feedlength_string):
        all_right=False
    elif float(feedlength_string)<=0:
        all_right=False
    
    if all_right:
        #falls kein Eingabefehler
        instruction_label.configure(text="Extrusion test started!",foreground="green")
        #calculate_flow_rate aufrufen
        calculate_flow_rate()
        #send_data aufrufen
        send_data()
    else:
        #falls Eingabefehler:
        instruction_label.configure(text="Some entries are invalid!",foreground="red")
        
        
        
        
def send_data():
    """
    Sendet die eingegebenen Parameter an den ESP

    Returns
    -------
    None.

    """
    data = temp_entry.get() + ' '
    
    data += feedrate_entry.get() + ' '

    data += feedlength_entry.get() + ' '
    
    if check_var.get():
        data+="1 "
    else:
        data+="0 "
    
    data+="\n"
    
    try:
        ser.write(data.encode('utf-8'))
        print("Sent:",data)
    except serial.SerialException as e:
        print(f"Serial error bei Senden: {e}")


def send_tare():
    """
    Sendet einen tare-Befehl an den ESP

    Returns
    -------
    None.

    """
    data="tare\n"
    
    try:
        ser.write(data.encode('utf-8'))
        print("Sent:",data)
    except serial.SerialException as e:
        print(f"Serial error bei tare: {e}")
        
def calculate_flow_rate():
    """
    Berechnet die flow rate und lässt sie auf der GUI anzeigen
    Berechnet die voraussichtliche Dauer der Messung

    Returns
    -------
    None.

    """
    feed_rate=float(feedrate_entry.get())
    flow_rate=feed_rate*np.pi*diameter**2*(1/4)
    flow_rate=round(flow_rate,2)
    flowrate_display.delete(0,tk.END)
    flowrate_display.insert(tk.END, str(flow_rate))
    
    #Gesamtzeit berechnen
    feed_length=float(feedlength_entry.get())
    global gesamtzeit
    gesamtzeit=feed_length/feed_rate    
        
def check_serial():
    """
    Empfängt und verarbeitet Daten vom ESP

    Returns
    -------
    None.

    """
    try:
        if ser and ser.in_waiting > 0: #falls Daten vom ESP im Buffer
            msg = ser.readline().decode('utf-8')
            
            #splitter den Input auf
            parts=msg.split(" ")
            
            #sortiere die Daten
            for part in parts:
                flag=part[0] #flag markiert die Daten
                global start
                global temp_list
                global time_list
                global slip_list
                global force_list
                global init_time
                global plot_cnt
                                
                if flag=="t": #time
                    if start:
                        if init_time<0: #init_time wurde noch nicht gesetzt
                            init_time=float(part[1:])
                        
                        time_list.append((float(part[1:])-init_time)/1000)
                        update_progress((float(part[1:])-init_time)/1000)
                        plot_cnt+=1
                        
                elif flag=="c": #temperature
                    if not(part[1]=="n"): #Wert ist nicht nan
                        if start:
                            temp_list.append(float(part[1:]))
                        current_temp.delete(0,tk.END)
                        current_temp.insert(tk.END, part[1:])
                    
                elif flag=="f": #force
                    if not(part[1]=="n"): #nicht nan
                        if start:
                            force_list.append(float(part[1:]))
                            current_load.delete(0,tk.END)
                            current_load.insert(tk.END, part[1:])
                    
                elif flag=="s": #slip
                    if not(part[1]=="n"): #nicht nan
                        if start:
                            slip_list.append(float(part[1:]))
                            current_slip.delete(0,tk.END)
                            current_slip.insert(tk.END, part[1:])
                            
                elif flag=="b": #Messung beginnt
                    start=1
                    print("Messung startet")
                    
                elif flag=="e": #Messung ist zu Ende
                    start=0
                    #schreibe Daten von Listen in csv-File
                    write_to_csv()
                    #leere die Listen
                    temp_list=[]
                    time_list=[]
                    force_list=[]
                    slip_list=[]
                    init_time=-1.
                    
                    #resette die Labels und Textfelder der GUI
                    current_load.delete(0,tk.END)
                    current_load.insert(tk.END,"0")
                    current_slip.delete(0,tk.END)
                    current_slip.insert(tk.END,"0")
                    current_temp.delete(0,tk.END)
                    current_temp.insert(tk.END,"0")
                    
                    update_progress(0)
                    
                    instruction_label.configure(text="Please fill out and press \"START TEST\" or Enter",foreground="black")
                    
                    print("Ende der Messung") 
                
                    
            
            # Daten auf Debug-Monitor schreiben
            monitor_text.insert(tk.END, msg)
            monitor_text.see(tk.END) # Auto-scroll to bottom
            
            if len(time_list)>1 and plot_cnt==5: #live-plotten, mit 5 neuen Datensätzen
                plot()
                plot_cnt=0
        root.after(5, check_serial) #ruft sich selbst nach 5ms wieder auf
    except:
        print("No ser found")
    
def update_progress(current_time):
    """
    berechnet die prozentualen Fortschritt des Messung und gibt das auf dem
    Progressbar aus

    Parameters
    ----------
    current_time : float
        vergangene Zeit seit Messbeginn in Sekunden

    Returns
    -------
    None.

    """
    #setze Balkenlänge
    progress_bar["value"]=round(100*current_time/gesamtzeit)
    if progress_bar["value"]>=100:
        progress_bar["value"]=99
    #setze Zahl auf Balken
    style_progress.configure("text.Horizontal.TProgressbar", 
                     text=str(progress_bar["value"])+"%", anchor="center")


def plot():
    """
    Live-Plotting auf GUI

    Returns
    -------
    None.

    """
    min_len=min(len(time_list),len(force_list),len(slip_list)) #minimale Länge der Listen, arrays müssen gleich lang sein
    
    #konvertiere in numpy-arrays
    t_new=np.array(time_list[0:min_len])
    force_new=np.array(force_list[0:min_len])
    slip_new=np.array(slip_list[0:min_len])
    
    #Graphen updaten
    line_force.set_data(t_new,force_new)
    line_slip.set_data(t_new, slip_new)
    
    #Achsen updaten
    ax_force.set_xlim(min(t_new), max(t_new)+1)
    ax_force.set_ylim(min(force_new), max(force_new)+1)
    ax_slip.set_ylim(min(slip_new), max(slip_new)+1)
    
    canvas.draw()


def write_to_csv():
    """
    Speichert die empfangenen Messdaten in csv-Datei ab

    Returns
    -------
    None.

    """
    min_len=min(len(time_list),len(force_list),len(slip_list), len(temp_list)) #minimale Länge der Listen, arrays müssen gleich lang sein
    
    #konvertiere in numpy-arrays
    t_new=np.array(time_list[0:min_len])
    force_new=np.array(force_list[0:min_len])
    slip_new=np.array(slip_list[0:min_len])
    temp_new=np.array(temp_list[0:min_len])
    
    data=np.stack((t_new, temp_new, force_new, slip_new))
    data=data.transpose()
    
    filename="testfile1.csv"
    
    newname=csv_entry.get()
    if newname!="": #falls Nutzer individuellen Dateinamen eingegeben hat
        filename=newname+".csv"
        
    turn_off=""
        
    if check_var.get():
        turn_off+="True"
    else:
        turn_off+="False"
        
    title="Parameters: temperature="+temp_entry.get()+", feed rate="+feedrate_entry.get()+", feed length="+feedlength_entry.get()+", turn off hot end="+turn_off+"\n"
    
    with open(filename, 'w') as csv_file:
        
        csv_file.write(title) #parameters
        csv_file.write("time;temperature;force;slip\n")  #headers
        np.savetxt(csv_file, data, delimiter=';') #measured data
    
    
   
# %%
"""
Verbindung mit esp aufbauen
"""    

esp32_port = find_esp32_port(VENDOR_ID, PRODUCT_ID)

try:
    ser = serial.Serial(esp32_port, baud_rate, timeout=1)
    print(f"Connected to ESP32 on port: {esp32_port}")
except serial.SerialException as e:
    print("Serial Error", f"Could not open port {esp32_port}: {e}")
    
# %%
"""
Set up GUI
"""
   
root=tk.Tk()

root.title("Extrusion Test Bench")
root.configure(background="white")

#GUI soll sich bei resize anpassen
root.columnconfigure(0,weight=1)
root.columnconfigure(1,weight=2)
root.rowconfigure(0,weight=1)
root.rowconfigure(1,weight=1)

# %%
"""
Frame links oben
"""

frame_entry=tk.Frame(root)
frame_entry.grid(row=0, column=0, sticky="nsew",padx=20, pady=5) 
frame_entry.configure(background="white")

entry_label=tk.Label(frame_entry, text="Settings", font=("Arial", schrift_gross, "bold"), background="white")
entry_label.grid(row=0, column=0, sticky="w", pady=10)

instruction_label=tk.Label(frame_entry, text="Please fill out and press \"START TEST\" or Enter", font=("Arial", schrift_klein, "italic"), background="white")
instruction_label.grid(row=1, column=0, sticky="w", columnspan=2, pady=10)

temp_entry=tk.Entry(frame_entry, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
temp_entry.grid(row=2, column=0)
temp_entry_label=tk.Label(frame_entry, text="°C temperature", font=("Arial", schrift_klein), background="white")
temp_entry_label.grid(row=2, column=1, sticky="w", pady=10)

feedrate_entry=tk.Entry(frame_entry, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
feedrate_entry.grid(row=3, column=0)
feedrate_entry_label=tk.Label(frame_entry, text="mm/s feed rate", font=("Arial", schrift_klein), background="white")
feedrate_entry_label.grid(row=3, column=1, sticky="w", pady=10)

feedlength_entry=tk.Entry(frame_entry, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
feedlength_entry.grid(row=4, column=0)
feedlength_entry_label=tk.Label(frame_entry, text="mm feed length", font=("Arial", schrift_klein), background="white")
feedlength_entry_label.grid(row=4, column=1, sticky="w", pady=10)

flowrate_display=tk.Listbox(frame_entry,height=1, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
flowrate_display.grid(row=5, column=0)
flowrate_display_label=tk.Label(frame_entry, text="mm\u00b3/s flow rate (calculated)", font=("Arial", schrift_klein), background="white")
flowrate_display_label.grid(row=5, column=1, sticky="w", pady=10)


check_var=tk.BooleanVar() #True, falls Häkchen gesetzt 

Hotend_abschalten_check=tk.Checkbutton(frame_entry, variable=check_var, 
                    text="Switch off Hot-End?", font=("Arial", schrift_klein), background="white")
Hotend_abschalten_check.grid(row=6, column=0, pady=10)


"""
Frame links mittig
"""

frame_buttons=tk.Frame(root)
frame_buttons.grid(row=1, column=0, sticky="nsew",padx=20, pady=5, rowspan=1) 
frame_buttons.configure(background="white")

tare_button=tk.Button(frame_buttons, text="TARE LOAD CELL", font=("Arial", schrift_klein), background="light green", 
                      height=2, width=30, command=send_tare)
tare_button.grid(row=0, column=0, columnspan=2, sticky="we", pady=10)


start_button=tk.Button(frame_buttons, text="START TEST", font=("Arial", schrift_klein), background="orange", 

                       height=2, command=check_data)
start_button.grid(row=1, column=0, columnspan=2, sticky="we")



"""
Frame links unten
"""

frame_space=tk.Frame(root)
frame_space.grid(row=2, column=0, sticky="nsew",padx=5, pady=5) 
frame_space.configure(background="white")


csv_entry=tk.Entry(frame_space, background="green", foreground="white", font=("Arial", schrift_klein, "bold"))
csv_entry.grid(row=0, column=1)
csv_entry_label=tk.Label(frame_space, text="Enter filename:", font=("Arial", schrift_klein), background="white")
csv_entry_label.grid(row=0, column=0, sticky="w", pady=10)
csv_label=tk.Label(frame_space, text=".csv", font=("Arial", schrift_klein), background="white")
csv_label.grid(row=0, column=2, sticky="w", pady=10)

#Monitor zum debuggen
monitor_text = st.ScrolledText(frame_space, height=10)
#monitor_text.grid(row=1, column=0,sticky="sn", columnspan=3)

# %%

"""
Frame rechts oben
"""


frame_current_data=tk.Frame(root)
frame_current_data.grid(row=0, column=1, sticky="nsew",padx=5, pady=5) 
frame_current_data.configure(background="white")

current_label=tk.Label(frame_current_data, text="Live Data", font=("Arial", schrift_gross, "bold"), background="white")
current_label.grid(row=0, column=0, sticky="w", pady=10)

current_temp=tk.Listbox(frame_current_data,height=1, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
current_temp.grid(row=1, column=0)
temp_current_label=tk.Label(frame_current_data, text="°C temperature", font=("Arial", schrift_klein), background="white")
temp_current_label.grid(row=1, column=1, sticky="w", pady=10)
current_temp.insert(tk.END, "0")

current_load=tk.Listbox(frame_current_data,height=1, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
current_load.grid(row=2, column=0)
load_current_label=tk.Label(frame_current_data, text="N load", font=("Arial", schrift_klein), background="white")
load_current_label.grid(row=2, column=1, sticky="w", pady=10)
current_load.insert(tk.END, "0")

current_slip=tk.Listbox(frame_current_data,height=1, background="blue", foreground="white", font=("Arial", schrift_klein, "bold"))
current_slip.grid(row=3, column=0)
slip_current_label=tk.Label(frame_current_data, text="% slip", font=("Arial", schrift_klein), background="white")
slip_current_label.grid(row=3, column=1, sticky="w", pady=10)
current_slip.insert(tk.END, "0")

# %%

"""
Frame rechts mittig
"""

frame_current_display=tk.Frame(root)
frame_current_display.grid(row=1, column=1, sticky="nsew",padx=5, pady=5) 
frame_current_display.configure(background="white")

progress_label=tk.Label(frame_current_display, text="Progress:", font=("Arial", schrift_klein), background="white")
progress_label.grid(row=0, column=0, sticky="w")

progress_bar=ttk.Progressbar(frame_current_display,mode="determinate", 
                             style="text.Horizontal.TProgressbar", value=10, length=200)
progress_bar.grid(row=0, column=1,sticky="we")
progress_bar["value"]=0

#Style des Progressbars (Prozentzahl mittig auf dem Bar)
style_progress=ttk.Style(frame_current_display)
style_progress.layout("text.Horizontal.TProgressbar",[("Horizontal.Progressbar.trough",
                {'children': [('Horizontal.Progressbar.pbar',
                {'side': 'left', 'sticky': 'ns'})],'sticky': 'nswe'}), 
                ('Horizontal.Progressbar.label', {'sticky': 'nswe'})])

#style_progress.theme_use("clam")
style_progress.configure("text.Horizontal.TProgressbar", anchor="center")
style_progress.configure("text.Horizontal.TProgressbar", 
                 text=str(progress_bar["value"])+"%", anchor="center")

# %%
"""
Frame rechts unten
"""

frame_plot=tk.Frame(root)
frame_plot.grid(row=2, column=1,padx=5, pady=5, columnspan=2) 
frame_plot.configure(background="white")


"""
initialer Plot
"""

fig = Figure(figsize=(7, 2), dpi=100)
t = np.arange(0, 1, .01)
ax_force = fig.add_subplot()
fig.subplots_adjust(bottom=0.3, left=0.2, right=0.7)

ax_slip=ax_force.twinx()
line_force, = ax_force.plot(t, 0*t, "g-")
line_slip, = ax_slip.plot(t, 0*t, "b--")

ax_force.set_xlabel("Time in s", fontsize=14)
ax_force.set_ylabel("Force in N", color="green", fontsize=14)
ax_slip.set_ylabel("Slip in %", color="blue", fontsize=14)

canvas = FigureCanvasTkAgg(fig, master=frame_plot)
canvas.draw()
canvas.get_tk_widget().pack()

toolbar = NavigationToolbar2Tk(canvas, frame_plot, pack_toolbar=True)
toolbar.update()
canvas.get_tk_widget().pack()

# %%
check_serial() #initiales Aufrufen, ruft sich danach selber auf
root.mainloop() #erzeugt die GUI

ser.close() #schliesst Port 