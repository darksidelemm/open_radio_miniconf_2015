# OpenRadio v1.1 Quisk Configuration File
# 
# IMPORTANT: To be able to control the OpenRadio board from within Quisk,
# you will need to compile and upload the 'openradio_quisk' firmware, which
# is available from: https://github.com/darksidelemm/open_radio_miniconf_2015
#
# You will also need to install the pyserial package for python.
#


# SOUND CARD SETTINGS
#
# Uncomment these if you wish to use PortAudio directly
#name_of_sound_capt = "portaudio:(hw:2,0)"
#name_of_sound_play = "portaudio:(hw:1,0)"

# Uncomment these lines if you wish to use Pulseaudio
name_of_sound_capt = "pulse"
name_of_sound_play = "pulse"

# SERIAL PORT SETTINGS
# Set this as appropriate for your OS.
openradio_serial_port = "/dev/ttyUSB0"
openradio_serial_rate = 57600


# OpenRadio Frequency limits.
# These are just within the limits set in the openradio_quisk firmware.
openradio_lower = 100001
openradio_upper = 29999999

# OpenRadio Hardware Control Class
#
import serial,time
#from quisk_hardware_model import Hardware as BaseHardware

class Hardware:
  def __init__(self, app, conf):
    # We need to define a few of these default variables here. 
    self.application = app      # Application instance (to provide attributes)
    self.conf = conf        # Config file module
    self.rf_gain_labels = ()    # Do not add the Rf Gain button
    self.correct_smeter = conf.correct_smeter   # Default correction for S-meter
    self.use_sidetone = 0     # Don't show the sidetone volume control
    self.transverter_offset = 0

  def open(self):
    # Called once to open the Hardware
    # Open the serial port.
    self.or_serial = serial.Serial(openradio_serial_port,openradio_serial_rate,timeout=3)
    print("Opened Serial Port.")
    # Wait for the Arduino Nano to restart and boot.
    time.sleep(2)
    # Poll for version. Should probably confirm the response on this.
    version = str(self.get_parameter("VER"))
    print(version)
    # Return an informative message for the config screen
    t = version + ". Capture from sound card %s." % self.conf.name_of_sound_capt
    return t

  def close(self):      
    # Called once to close the Hardware
    self.or_serial.close()

  def ChangeFrequency(self, tune, vfo, source='', band='', event=None):
    # Called whenever quisk requests a frequency change.
    # This sends the FREQ command to set the centre frequency of the OpenRadio,
    # and will also move the 'tune' frequency (the section within the RX passband
    # which is to be demodulated) if it falls outside the passband (+/- sample_rate/2).
    print("Setting VFO to %d." % vfo)
    if(vfo<openradio_lower):
      vfo = openradio_lower
      print("Outside range! Setting to %d" % openradio_lower)

    if(vfo>openradio_upper):
      vfo = openradio_upper
      print("Outside range! Setting to %d" % openradio_upper)

    success = self.set_parameter("FREQ",str(vfo))

    # If the tune frequency is outside the RX bandwidth, set it to somewhere within that bandwidth.
    if(tune>(vfo + sample_rate/2) or tune<(vfo - sample_rate/2)):
      tune = vfo + 10000
      print("Bringing tune frequency back into the RX bandwidth.")

    if success:
      print("Frequency change succeeded!")
      return tune, vfo
    else:
      print("Frequency change failed.")
      return None, None

  # Boilerplate functions, most of which return nothing.
  def ReturnFrequency(self):
    # Return the current tuning and VFO frequency.  If neither have changed,
    # you can return (None, None).  This is called at about 10 Hz by the main.
    # return (tune, vfo)  # return changed frequencies
    return None, None   # frequencies have not changed
  def ReturnVfoFloat(self):
    # Return the accurate VFO frequency as a floating point number.
    # You can return None to indicate that the integer VFO frequency is valid.
    return None
  def ChangeMode(self, mode):   # Change the tx/rx mode
    # mode is a string: "USB", "AM", etc.
    pass
  def ChangeBand(self, band):
    # band is a string: "60", "40", "WWV", etc.
    try:
      self.transverter_offset = self.conf.bandTransverterOffset[band]
    except:
      self.transverter_offset = 0
  def OnBtnFDX(self, is_fdx):   # Status of FDX button, 0 or 1
    pass
  def HeartBeat(self):  # Called at about 10 Hz by the main
    pass
  # The "VarDecim" methods are used to change the hardware decimation rate.
  # If VarDecimGetChoices() returns any False value, no other methods are called.
  def VarDecimGetChoices(self): # Return a list/tuple of strings for the decimation control.
    return False  # Return a False value for no decimation changes possible.
  def VarDecimGetLabel(self): # Return a text label for the decimation control.
    return ''
  def VarDecimGetIndex(self): # Return the index 0, 1, ... of the current decimation.
    return 0    # This is called before open() to initialize the control.
  def VarDecimSet(self, index=None):  # Called when the control is operated.
    # Change the decimation here, and return the sample rate.  The index is 0, 1, 2, ....
    # Called with index == None before open() to set the initial sample rate.
    # Note:  The last used value is available as self.application.vardecim_set if
    #        the persistent state option is True.  If the value is unavailable for
    #        any reason, self.application.vardecim_set is None.
    return 48000
  def VarDecimRange(self):  # Return the lowest and highest sample rate.
    return (48000, 960000)


#
# Serial comms functions, to communicate with the OpenRadio board
#

  def get_parameter(self,string):
    self.or_serial.write(string+"\n")
    return self.get_argument()
        
  def set_parameter(self,string,arg):
    self.or_serial.write(string+","+arg+"\n")
    if self.get_argument() == arg:
      return True
    else:
       return False
    
  def get_argument(self):
    data1 = self.or_serial.readline()
    # Do a couple of quick checks to see if there is useful data here
    if len(data1) == 0:
       return -1
        
    # Maybe we didn't catch an OK line?
    if data1.startswith('OK'):
       data1 = self.or_serial.readline()
        
    # Check to see if we have a comma in the string. If not, there is no argument.
    if data1.find(',') == -1:
       return -1
    
    data1 = data1.split(',')[1].rstrip('\r\n')
    
    # Check for the OK string
    data2 = self.or_serial.readline()
    if data2.startswith('OK'):
       return data1