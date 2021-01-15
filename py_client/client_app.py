import socket
import time
import sys
import matplotlib.pyplot as plt

def main():
    temp_list = []
    hum_list = []

    plt.style.use('fivethirtyeight')

    sock_addr = ('192.168.1.193', 12000)
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    if s < 0:
        print("Error creating socket")

    try:
        s.connect(sock_addr)
        print("Connected to %s" % repr(sock_addr))
    except:
        print("Connection to %s refused" % repr(sock_addr))
        sys.exit(1)

    try:
        s.send("Start measurement")
        s.recv(100)

        while(1):
            s.send("Fetch humidity")
            buf = s.recv(100)
            hum_list.append(int(buf.split(': ')[1]))
            if len(hum_list) > 10:
                del hum_list[0]
            print(hum_list)
            s.send("Fetch temperature")
            buf = s.recv(100)
            temp_list.append(int(buf.split(': ')[1]))
            if len(temp_list) > 10:
                del temp_list[0]
            print(temp_list)
            time.sleep(0.2)

            plt.cla()
            plt.ylim(0, 100)
            plt.plot(hum_list)
            plt.tight_layout()
            plt.pause(0.1)

    finally:
        print "Closing socket"
        s.close()


if __name__ == "__main__":
    main()
