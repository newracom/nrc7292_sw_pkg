import subprocess, sys, getopt

# Generate ping packets with different length varied from 42 to 1500
# and check the response

SIZE_DEFAULT_PING   = 42
SIZE_MAX_MPDU       = 1500 - SIZE_DEFAULT_PING

def test_ping(host_name, ping_size):
    try:
        output = subprocess.check_output("ping -W 1 -c 1 -s " + str(ping_size) + " " + host_name, shell=True)
    except Exception, e:
        return False
    return True

def print_help():
	print ("Usage:	test_ping <destination> [start size]")
	print ("	generate ICMP packets with different length varied from [start size] or ")
	print ("	42 bytes to 1500 bytes and check the reponse packets arrive")
			

if len(sys.argv) < 2:
    print_help()
    sys.exit(0)

host_name = str(sys.argv[1])
start_size = 0

if len(sys.argv) is 3:
	start_size = int(sys.argv[2])

success = 0
failed = 0
for i in range(start_size, SIZE_MAX_MPDU + 1):
    if test_ping(host_name, i + SIZE_DEFAULT_PING):
        print "Size:" + str(i + SIZE_DEFAULT_PING) + " - Success"
        success += 1
    else:
        print "Size:" + str(i + SIZE_DEFAULT_PING) + " - failed"
        failed += 1

print "Finish - success(" + str(success) + "), failed(" + str(failed) + ")"
