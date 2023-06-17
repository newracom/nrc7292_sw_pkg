import binascii
import os
import time
import struct
import nrcnetlink
import time

def build_sae_commit(bssid, addr, group=21, token=None):
    """
        SSID="test-sae", psk="12345678"
    """
    if group == 19:
		scalar = binascii.unhexlify("7332d3ebff24804005ccd8c56141e3ed8d84f40638aa31cd2fac11d4d2e89e7b")
		element = binascii.unhexlify("954d0f4457066bff3168376a1d7174f4e66620d1792406f613055b98513a7f03a538c13dfbaf2029e2adc6aa96aa0ddcf08ac44887b02f004b7f29b9dbf4b7d9")
    elif group == 21:
		scalar = binascii.unhexlify("001eec673111b902f5c8a61c8cb4c1c4793031aeea8c8c319410903bc64bcbaea134ab01c4e016d51436f5b5426f7e2af635759a3033fb4031ea79f89a62a3e2f828")
		element = binascii.unhexlify("00580eb4b448ea600ea277d5e66e4ed37db82bb04ac90442e9c3727489f366ba4b82f0a472d02caf4cdd142e96baea5915d71374660ee23acbaca38cf3fe8c5fb94b01abbc5278121635d7c06911c5dad8f18d516e1fbe296c179b7c87a1dddfab393337d3d215ed333dd396da6d8f20f798c60d054f1093c24d9c2d98e15c030cc375f0")
		pass
    frame = binascii.unhexlify("b0003a01")
    frame += bssid + addr + bssid
    frame += binascii.unhexlify("1000")
    auth_alg = 3
    transact = 1
    status = 0
    frame += struct.pack("<HHHH", auth_alg, transact, status, group)
    if token:
        frame += token
    frame += scalar + element
    return frame

def a2b(addr):
    """ Convert addr in string to hex
        e.g. ) addr = "00:11:22:33:44:55"
    """
    return binascii.unhexlify(addr.replace(':', ''))

def change_addr(addr, intf="wlan0"):
    os.system("ifconfig " + intf + " down")
    os.system("ifconfig " + intf + " hw ether " + addr)
    os.system("ifconfig " + intf + " up")

def get_addr(intf="wlan0"):
  try:
    mac = open('/sys/class/net/'+interface+'/address').readline()
  except:
    mac = "00:00:00:00:00:00"
  return mac[0:17]

def inject_sae_commit(bssid, srcs, password):
    nl = nrcnetlink.NrcNetlink()
    cur = get_addr()
    for src in srcs:
        frame = build_sae_commit(a2b(bssid), a2b(src), group=19)
        change_addr(src)
        nl.nrc_frame_injection(frame)
        time.sleep(1)

    if get_addr() != cur:
        change_addr(cur)

    return None

def test_sae_4_2_3_step_2():
    """ [SAE-4.2.3]
        Step2: Trigger the packet injector to transmit SAE Commit messages
                 from different MAC addresses.
    """
    srcs = {"00:11:22:33:41:01",
            "00:11:22:33:41:02",
            "00:11:22:33:41:03"}
    bssid = "02:00:eb:09:57:71"
    password = "12345678"

    return inject_sae_commit(bssid, srcs, "12345678")

if __name__ == "__main__":
    test_sae_4_2_3_step_2()
