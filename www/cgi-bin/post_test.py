#!/usr/bin/env python3
import sys
import os
from urllib.parse import unquote_plus

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

params = {}
if body:
    for pair in body.split('&'):
        if '=' in pair:
            key, value = pair.split('=', 1)
            params[unquote_plus(key)] = unquote_plus(value)

name    = params.get('name', 'stranger')
email   = params.get('email', '')
age     = params.get('age', '')
message = params.get('message', '')

sys.stdout.write("Content-Type: text/html\r\n\r\n")

print("""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>POST Response · webserv</title>
  <link href="https://fonts.googleapis.com/css2?family=Syne:wght@700;800&family=DM+Sans:wght@300;400;500&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
  <style>
    *,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
    body{background:#04050f;color:#e2e8f0;font-family:'DM Sans',sans-serif;min-height:100vh;overflow-x:hidden;padding-bottom:60px}
    .bg{position:fixed;inset:0;z-index:0;overflow:hidden}
    .bg::before{content:'';position:absolute;width:600px;height:600px;background:radial-gradient(circle,rgba(0,240,212,.18),transparent);filter:blur(100px);top:-150px;left:-100px}
    .bg::after{content:'';position:absolute;width:500px;height:500px;background:radial-gradient(circle,rgba(167,139,250,.15),transparent);filter:blur(100px);bottom:-100px;right:-50px}
    body::after{content:'';position:fixed;inset:0;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.85' numOctaves='4'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)' opacity='0.035'/%3E%3C/svg%3E");pointer-events:none;z-index:0}

    nav{position:fixed;top:18px;left:50%;transform:translateX(-50%);width:calc(100% - 64px);max-width:1000px;z-index:200;display:flex;align-items:center;justify-content:space-between;padding:12px 24px;border-radius:18px;background:rgba(4,5,15,.65);backdrop-filter:blur(28px);border:1px solid rgba(255,255,255,.09)}
    .nav-logo{font-family:'Syne',sans-serif;font-weight:800;font-size:18px;letter-spacing:-.03em;text-decoration:none;background:linear-gradient(130deg,#00f0d4,#a78bfa);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
    .nav-links{display:flex;gap:4px;list-style:none}
    .nav-links a{color:#94a3b8;text-decoration:none;font-size:14px;font-weight:500;padding:7px 14px;border-radius:10px;transition:all .2s}
    .nav-links a:hover{color:#fff;background:rgba(255,255,255,.05)}
    .nav-pill{font-size:12px;font-weight:600;color:#00f0d4;background:rgba(0,240,212,.1);border:1px solid rgba(0,240,212,.2);padding:6px 16px;border-radius:30px;display:flex;align-items:center;gap:6px}
    .pulse{width:7px;height:7px;background:#00f0d4;border-radius:50%;box-shadow:0 0 10px #00f0d4;animation:blink 2s ease infinite}
    @keyframes blink{0%,100%{opacity:1}50%{opacity:.35}}

    .page{position:relative;z-index:1;max-width:560px;margin:0 auto;padding:120px 40px 0;animation:fadeU .5s ease both}
    @keyframes fadeU{from{opacity:0;transform:translateY(14px)}to{opacity:1;transform:translateY(0)}}

    .label{font-size:11px;font-weight:700;letter-spacing:.2em;text-transform:uppercase;color:#00f0d4;margin-bottom:10px;display:flex;align-items:center;gap:8px}
    .label::before{content:'';width:18px;height:2px;background:#00f0d4;border-radius:1px}

    h1{font-family:'Syne',sans-serif;font-size:clamp(28px,5vw,46px);font-weight:800;letter-spacing:-.03em;margin-bottom:6px;background:linear-gradient(135deg,#00f0d4,#a78bfa);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
    .sub{font-size:14px;color:#475569;margin-bottom:32px;font-weight:300}

    /* success ring */
    .success-ring{width:64px;height:64px;border-radius:18px;margin-bottom:24px;display:flex;align-items:center;justify-content:center;background:rgba(0,240,212,.08);border:1px solid rgba(0,240,212,.25);box-shadow:0 0 40px rgba(0,240,212,.15);position:relative}
    .success-ring::before{content:'';position:absolute;inset:-10px;border-radius:24px;border:1px solid rgba(0,240,212,.15);animation:ping 2.4s ease-out infinite}
    @keyframes ping{0%{opacity:.6;transform:scale(1)}100%{opacity:0;transform:scale(1.18)}}
    .success-ring svg{width:28px;height:28px;stroke:#00f0d4;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}

    /* fields */
    .fields{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.09);border-radius:16px;overflow:hidden;margin-bottom:20px}
    .field{display:flex;align-items:flex-start;gap:16px;padding:14px 20px;border-bottom:1px solid rgba(255,255,255,.06)}
    .field:last-child{border-bottom:none}
    .field-icon{width:32px;height:32px;border-radius:8px;display:flex;align-items:center;justify-content:center;flex-shrink:0;margin-top:1px}
    .field-icon svg{width:14px;height:14px;fill:none;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}
    .field-key{font-size:11px;font-weight:700;letter-spacing:.12em;text-transform:uppercase;color:#475569;margin-bottom:4px}
    .field-val{font-family:'JetBrains Mono',monospace;font-size:13px;color:#e2e8f0;word-break:break-all;line-height:1.6}

    /* raw body card */
    .raw-card{background:rgba(255,255,255,.03);border:1px solid rgba(255,255,255,.07);border-radius:12px;padding:14px 18px;margin-bottom:20px}
    .raw-label{font-size:10px;font-weight:700;letter-spacing:.18em;text-transform:uppercase;color:#334155;margin-bottom:8px}
    .raw-body{font-family:'JetBrains Mono',monospace;font-size:11px;color:#475569;word-break:break-all;line-height:1.7}

    /* back button */
    .btn-ghost{font-family:'Syne',sans-serif;font-weight:600;font-size:14px;padding:13px 28px;border-radius:12px;cursor:pointer;text-decoration:none;display:inline-flex;align-items:center;justify-content:center;gap:8px;background:rgba(255,255,255,.05);color:#e2e8f0;border:1px solid rgba(255,255,255,.09);backdrop-filter:blur(12px);transition:all .25s}
    .btn-ghost:hover{background:rgba(255,255,255,.09);border-color:rgba(255,255,255,.18);transform:translateY(-3px)}
    .btn-ghost svg{width:15px;height:15px;stroke:currentColor;fill:none;stroke-width:2;stroke-linecap:round;stroke-linejoin:round}

    footer{position:relative;z-index:1;border-top:1px solid rgba(255,255,255,.09);padding:32px 40px;margin-top:48px}
    .foot-inner{max-width:1000px;margin:0 auto;display:flex;justify-content:space-between;align-items:center;gap:20px}
    .foot-logo{font-family:'Syne',sans-serif;font-weight:800;font-size:16px;background:linear-gradient(130deg,#00f0d4,#a78bfa);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
    .foot-copy{font-size:13px;color:#475569}
    .foot-links{display:flex;gap:20px;list-style:none}
    .foot-links a{font-size:13px;color:#94a3b8;text-decoration:none;transition:color .2s}
    .foot-links a:hover{color:#fff}
  </style>
</head>
<body>
  <div class="bg"></div>

  <nav>
    <a class="nav-logo" href="/">webserv</a>
    <ul class="nav-links">
      <li><a href="/">Home</a></li>
      <li><a href="/upload.html">Upload</a></li>
      <li><a href="/index2.html">Playground</a></li>
    </ul>
    <div class="nav-pill"><div class="pulse"></div>CGI · POST</div>
  </nav>

  <div class="page">
    <div class="success-ring">
      <svg viewBox="0 0 24 24"><polyline points="20 6 9 17 4 12"/></svg>
    </div>

    <div class="label">POST · CGI Response</div>
    <h1>Form received.</h1>
    <p class="sub">Your submission was parsed by the Python CGI script and is displayed below.</p>""")

# ── field definitions: (key, label, value, icon_svg, accent_color)
fields = [
    ('name',    'Name',    name,    '<path d="M20 21v-2a4 4 0 0 0-4-4H8a4 4 0 0 0-4 4v2"/><circle cx="12" cy="7" r="4"/>',                        '#00f0d4'),
    ('email',   'Email',   email,   '<path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/><polyline points="22,6 12,13 2,6"/>', '#60a5fa'),
    ('age',     'Age',     age,     '<rect x="3" y="4" width="18" height="18" rx="2" ry="2"/><line x1="16" y1="2" x2="16" y2="6"/><line x1="8" y1="2" x2="8" y2="6"/><line x1="3" y1="10" x2="21" y2="10"/>', '#a78bfa'),
    ('message', 'Message', message, '<path d="M21 15a2 2 0 0 1-2 2H7l-4 4V5a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2z"/>',                                  '#fb7185'),
]

has_fields = any(v for _, _, v, _, _ in fields)

if has_fields:
    print('<div class="fields">')
    for key, label, value, icon, color in fields:
        if not value:
            continue
        bg  = color.replace('#', '')
        print(f"""  <div class="field">
    <div class="field-icon" style="background:rgba(0,0,0,.25);border:1px solid {color}33">
      <svg viewBox="0 0 24 24" style="stroke:{color}">{icon}</svg>
    </div>
    <div>
      <div class="field-key">{label}</div>
      <div class="field-val">{value}</div>
    </div>
  </div>""")
    print('</div>')

if body:
    print(f"""<div class="raw-card">
  <div class="raw-label">Raw body</div>
  <div class="raw-body">{body}</div>
</div>""")

print("""
    <a href="javascript:history.back()" class="btn-ghost">
      <svg viewBox="0 0 24 24"><polyline points="15 18 9 12 15 6"/></svg>
      Go Back
    </a>
  </div>

  <footer>
    <div class="foot-inner">
      <div class="foot-logo">webserv</div>
      <span class="foot-copy">© 2025 REDA, ELAMRANI, SIMO · 1337 School</span>
      <ul class="foot-links">
        <li><a href="/">Home</a></li>
        <li><a href="/pages/upload.html">Upload</a></li>
        <li><a href="/errors/404.html">404</a></li>
        <li><a href="/errors/500.html">500</a></li>
      </ul>
    </div>
  </footer>
</body>
</html>""")