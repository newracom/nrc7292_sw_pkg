import sys
import subprocess
import socket
import unittest

host = "192.168.101.1"

def send_udp(dest):
	sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
	sock.sendto("t", (dest, 2000))

def make_ping(host, size):
    try:
        output = subprocess.check_output("ping -W 1 -c 1 -s " + str(size) + " " + host, shell=True)
    except Exception, e:
        return False
    return True

class TestBA(unittest.TestCase):
    def test_random_sized_pings(self):
	for i in range(2, 5):
		self.assertEqual(make_ping(host, 1458 * i), True)
    def test_small_bulk_udps(self):
	for i in range(2, 500):
		send_udp(host)
	
if __name__ == '__main__':
    unittest.main()
