from http.server import HTTPServer
from http.server import BaseHTTPRequestHandler

class handler(BaseHTTPRequestHandler):
    def do_POST(self):
        # Set http header.
        self.send_response(200)
        self.end_headers()

        # Read data sent by POST request.
        content_length = int(self.headers['Content-Length'])
        data = self.rfile.read(content_length)
        print('\033[31m' + data.decode('utf-8') + '\033[0m')

        # Response to client.
        self.wfile.write("Response to POST request\n".encode())

def main():
    localhost = '127.0.0.1'
    port      = 9999
    HTTPServer((localhost, port), handler).serve_forever()

if __name__ == '__main__':
    main()
