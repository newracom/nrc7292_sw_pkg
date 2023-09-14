import sys
import os

def usage():
    print("====USAGE=================================================================================")
    print("python3 test_remotecmd.py {countdown} {-b|MACADDR} {-sim|-dist} '{cli command for STA}'")
    print("e.g. python3 test_remotecmd.py 10 -b -sim 'iperf -u -c 192.168.200.1 -b 10M -t 10'")
    print("    options")
    print("    - countdown: 1~20 (beacons)")
    print("    - target type: -b (broadcast) / {MAC ADDRESS OF TARGET} (single target)")
    print("    - trigger type: -sim (simultaneous) / -dist (distributed)")
    print("==========================================================================================")
    sys.exit()

if __name__ == "__main__":
    # check argument count (5)
    param_cnt = len(sys.argv)
    print('param_cnt =', param_cnt)
    if param_cnt < 5:
        usage()

    oui_str = ' 0xfcffaa'
    subcmd_str = ' 0x5'

    # PARAM_1: countdown number (limit to 20 to reduce FW overhead)
    cntdwn = int(sys.argv[1])
    print('countdown =', cntdwn)
    if cntdwn < 1 or cntdwn > 20:
        print('Invalid countdown number.')
        usage()
    cntdwn_str = ' ' + hex(cntdwn)

    # PARAM_2: target type (address)
    address = sys.argv[2]
    print('address =', address)
    address_str = ''
    if address == '-b':
        address_str = ' 0xff 0xff 0xff 0xff 0xff 0xff'
    elif len(address.split(':')) == 6:
        for i in address.split(':'):
            if len(i) != 2:
                print('Invalid target type (address) format.')
                usage()
            address_str += ' 0x' + i
    else:
        print('Invalid target type (address) format.')
        usage()

    # PARAM_3: trigger type
    trigger_type = sys.argv[3]
    print('trigger type =', trigger_type)
    if trigger_type == '-sim':
        trigger_type_str = ' 0x0'
    elif trigger_type == '-dist':
        trigger_type_str = ' 0x1'
    else:
        print('Invalid trigger type.')
        usage()

    # PARAM_4: cli command string to request to peer sta
    cli_cmd = sys.argv[4]
    print('cli command =', cli_cmd)
    cli_cmd_str = ''
    for i in cli_cmd:
        cli_cmd_str += ' 0x' + i.encode('utf-8').hex()

    # all parameters
    params_str = oui_str + subcmd_str + cntdwn_str + address_str + trigger_type_str + cli_cmd_str
    print('all parameters (string) =', params_str)

    os.system('sudo iw dev wlan0 vendor recv %s'%(params_str))