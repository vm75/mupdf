#!/usr/bin/env python3
"""Run the C integration tests against temporary folder and comic fixtures."""
from pathlib import Path
import subprocess
import sys
import tempfile
from xml.sax.saxutils import quoteattr
import zipfile


def index(paths, count="99999999999999999999"):
    return '<container count="' + count + '">' + ''.join(
        '<item path=' + quoteattr(str(path)) + '/>' for path in paths) + '</container>'


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: test-document-options.py <test-document-options binary>")
    binary = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="mupdf folder & ") as directory:
        root = Path(directory)
        green = b'P6\n4 2\n255\n' + bytes([0, 255, 0]) * 8
        red = b'P6\n2 1\n255\n' + bytes([255, 0, 0]) * 2
        (root / 'large & green.ppm').write_bytes(green)
        (root / 'small.ppm').write_bytes(red)
        (root / 'relative.ldir').write_text(index(['large & green.ppm', 'small.ppm']))
        (root / 'relative.imagedir').write_text(index(['large & green.ppm', 'small.ppm']))
        (root / 'absolute.ldir').write_text(index([root / 'large & green.ppm', root / 'small.ppm']))
        (root / 'uri.ldir').write_text(index([(root / 'large & green.ppm').as_uri()]))
        (root / 'utf16.ldir').write_bytes(index(['large & green.ppm']).encode('utf-16'))
        (root / 'empty.ldir').write_text(index([], count='0'))
        (root / 'bad-root.ldir').write_text('<other/>')
        (root / 'missing-path.ldir').write_text('<container><item/></container>')
        (root / 'bad-xml.ldir').write_text('<container><item')
        (root / 'image.html').write_text('<html><body><img src="small.ppm" style="width:100px;height:50px"></body></html>')
        for name, content in {
            'heading.xhtml': '<h1>Hello</h1>',
            'underline.xhtml': '<p style="text-decoration:underline">Hello</p>',
            'missing.xhtml': '<img src="missing.ppm"/>',
            'extended.xhtml': '<image href="small.ppm"/>',
        }.items():
            (root / name).write_text('<html xmlns="http://www.w3.org/1999/xhtml"><body>' + content + '</body></html>')
        (root / 'publisher.fb2').write_text(
            '<FictionBook xmlns="http://www.gribuser.ru/xml/fictionbook/2.0">'
            '<stylesheet type="text/css">p {font-size:30pt}</stylesheet>'
            '<body><section><p>Hello</p></section></body></FictionBook>')
        for name, navigation, version in [('broken-ncx.epub', 'ncx', '2.0'),
                                           ('broken-nav.epub', 'nav', '3.0'),
                                           ('deep-nav.epub', 'nav', '3.0')]:
            with zipfile.ZipFile(root / name, 'w') as archive:
                archive.writestr('mimetype', 'application/epub+zip')
                archive.writestr('META-INF/container.xml',
                    '<container><rootfiles><rootfile full-path="content.opf"/></rootfiles></container>')
                archive.writestr('content.opf',
                    '<package version="' + version + '"><metadata/><manifest>'
                    '<item id="page" href="page.xhtml" media-type="application/xhtml+xml"/>'
                    '<item id="toc" href="toc.xml" media-type="' +
                    ('application/x-dtbncx+xml' if navigation == 'ncx' else 'application/xhtml+xml') + '"' +
                    (' properties="nav"' if navigation == 'nav' else '') + '/>'
                    '</manifest><spine' + (' toc="toc"' if navigation == 'ncx' else '') +
                    '><itemref idref="page"/></spine></package>')
                archive.writestr('toc.xml', '<a>' * 5000 + '</a>' * 5000 if name == 'deep-nav.epub' else '<ncx><navPoint bad="')
                archive.writestr('page.xhtml',
                    '<html xmlns="http://www.w3.org/1999/xhtml"><body><img src="green.ppm"/></body></html>')
                archive.writestr('green.ppm', green)
        with zipfile.ZipFile(root / 'hidden.cbz', 'w') as archive:
            for name in ['.hidden.ppm', 'sub/.hidden.ppm', '__MACOSX/image.ppm']:
                archive.writestr(name, red)
            archive.writestr('page10.ppm', red)
            archive.writestr('page2.ppm', green)
        subprocess.run([str(binary), directory], check=True)


if __name__ == '__main__':
    main()
