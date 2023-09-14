import nrcnetlink 
import sys


def usage():
    print("[Toggle automatic BA session] Usage")
    print("             auto_ba_toggle.py 0(off)|1(on)")
    sys.exit()

if __name__ == '__main__':
    nl = nrcnetlink.NrcNetlink()
    if len(sys.argv) < 2:
        usage()

    if sys.argv[1] == '0' or sys.argv[1] == 'off':
        on = 0
    elif sys.argv[1] == '1' or sys.argv[1] == 'on':
        on = 1
    else:
        usage()
    nl.nrc_auto_ba_toggle(on)
