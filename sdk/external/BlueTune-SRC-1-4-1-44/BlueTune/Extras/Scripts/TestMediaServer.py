### Simple test media server

__version__ = "0.1"

__all__ = ["MediaRequestHandler"]

import os
import posixpath
import BaseHTTPServer
import urllib
import urlparse
import cgi
import shutil
import mimetypes
import time
import sys

VERBOSE=True
CHUNK_SIZE=4096

class MediaRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    server_version = "MediaTest/" + __version__

    def do_GET(self):
        if VERBOSE: print "GET", self.path
        f = self.send_head()
        if f:
            self.copyfile(f, self.wfile)
            f.close()

    def do_HEAD(self):
        f = self.send_head()
        if f:
            f.close()

    def send_head(self):
        path = self.translate_path(self.path)
        f = None
        if os.path.isdir(path): return None
        ctype = self.guess_type(path)
        if ctype.startswith('text/'):
            mode = 'r'
        else:
            mode = 'rb'
        try:
            f = open(path, mode)
        except IOError:
            self.send_error(404, "File not found")
            return None
        self.send_response(200)
        self.send_header("Content-type", ctype)
        fs = os.fstat(f.fileno())
        self.send_header("Content-Length", str(fs[6]))
        self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
        self.end_headers()
        return f

    def translate_path(self, path):
        path = urlparse.urlparse(path)[2]
        path = posixpath.normpath(urllib.unquote(path))
        words = path.split('/')
        words = filter(None, words)
        path = os.getcwd()
        for word in words:
            drive, word = os.path.splitdrive(word)
            head, word = os.path.split(word)
            if word in (os.curdir, os.pardir): continue
            path = os.path.join(path, word)
        return path

    def copyfile(self, source, outputfile):
        start = time.time()
        print "START", start
        
        total=0
        elapsed = 0
        while True:
            chunk = source.read(CHUNK_SIZE)
            print "READ", len(chunk)
            if len(chunk) == 0: break
            try:
                outputfile.write(chunk)
            except:
                break
            total += len(chunk)
            now = time.time()
            elapsed = now-start
            rate = total/elapsed
            
            print "STAT", total, elapsed, rate
            if rate > 200000:
                time.sleep(1)
                
        print "END", time.time()
        print "ELAPSED", elapsed
        print "BPS =", total/elapsed
        
    def guess_type(self, path):
        base, ext = posixpath.splitext(path)
        if ext in self.extensions_map:
            return self.extensions_map[ext]
        ext = ext.lower()
        if ext in self.extensions_map:
            return self.extensions_map[ext]
        else:
            return self.extensions_map['']

    if not mimetypes.inited:
        mimetypes.init() # try to read system mime.types
    extensions_map = mimetypes.types_map.copy()
    extensions_map.update({
        '': 'application/octet-stream', # Default
        '.py': 'text/plain',
        '.c': 'text/plain',
        '.h': 'text/plain',
        '.mp4': 'video/mp4',
        '.mp3': 'audio/mpeg'
        })


def test(HandlerClass = MediaRequestHandler,
         ServerClass = BaseHTTPServer.HTTPServer):
    BaseHTTPServer.test(HandlerClass, ServerClass)


if __name__ == '__main__':
    test()
