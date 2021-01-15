import socket
import sys
from flask import Flask, render_template, url_for

app = Flask(__name__)
global s
s = 0
sock_addr = ('192.168.1.193', 12000)

@app.route('/')
def login():
    global s
    if s == 0:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if s < 0:
            print("Error creating socket")
            sys.exit(1)

        try:
            s.connect(sock_addr)
            print("Connected to %s" % repr(sock_addr))
        except:
            print("Connection to %s refused" % repr(sock_addr))
            sys.exit(1)

        s.send("Start measurement")
        s.recv(100)

    s.send("Fetch humidity")
    buf = s.recv(100)
    hum = buf.split(': ')[1]
    s.send("Fetch temperature")
    buf = s.recv(100)
    temp = str((float(buf.split(': ')[1]) / 100))

    return render_template('index.html', temp = temp, hum = hum)
    

if __name__ == "__main__":

    app.run(host='0.0.0.0', port=80, debug=True)
